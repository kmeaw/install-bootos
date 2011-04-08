#include <psl1ght/lv2.h>
#include <errno.h>
#include <stdio.h>
#include <sysutil/video.h>
#include <rsx/gcm.h>
#include <rsx/reality.h>
#include <io/pad.h>
#include "sconsole.h"
#include <unistd.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "hvcall.h"
#include "peek_poke.h"
#include "mm.h"

#define CHUNK 65536
#define QUICK_SCAN 1

void
xputs(
    const char* msg);

VideoConfiguration vconfig;
VideoState state;

unsigned char*
read_file(
    FILE*   f,
    size_t* sz)
{
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char* userlandBuffer = malloc(*sz);
    if (!userlandBuffer) {
        return NULL;
    }
    fread(userlandBuffer, 1, *sz, f);
    fclose(f);
    if (*( (u32*)userlandBuffer ) == 0) {
        free(userlandBuffer);
        userlandBuffer = NULL;
    }
    return userlandBuffer;
}
PadInfo padinfo;
PadData paddata;

typedef struct {
    int height;
    int width;
    uint32_t* ptr;
    // Internal stuff
    uint32_t offset;
} buffer;

gcmContextData* context;
VideoResolution res;
int currentBuffer = 0;
buffer* buffers[2];

void
waitFlip()
{
    // Block the PPU thread untill the previous flip operation has finished.
    while (gcmGetFlipStatus() != 0) {
        usleep(200);
    }
    gcmResetFlipStatus();
}

void
flip(s32 buffer)
{
    assert(gcmSetFlip(context, buffer) == 0);
    realityFlushBuffer(context);
    gcmSetWaitFlip(context);
}

void
makeBuffer(int id, int size)
{
    buffer* buf = malloc( sizeof(buffer) );
    buf->ptr = rsxMemAlign(16, size);
    assert(buf->ptr != NULL);

    assert(realityAddressToOffset(buf->ptr, &buf->offset) == 0);
    assert(gcmSetDisplayBuffer
            (id, buf->offset, res.width * 4, res.width, res.height) == 0);

    buf->width = res.width;
    buf->height = res.height;
    buffers[id] = buf;
}

u64 mmap_lpar_addr;

int
map_lv1()
{
    char buf[256];
    int result = lv1_undocumented_function_114(0, 0xC, HV_SIZE, &mmap_lpar_addr);
    if (result != 0) {
        snprintf(buf, sizeof(buf), "Error code %d calling lv1_undocumented_function_114", result);
        xputs(buf);
        return 0;
    }
    result = mm_map_lpar_memory_region(mmap_lpar_addr, HV_BASE, HV_SIZE, 0xC, 0);
    if (result) {
        snprintf(buf, sizeof(buf), "Error code %d calling mm_map_lpar_memory_region", result);
        xputs(buf);
        return 0;
    }
    return 1;
}

void
unmap_lv1()
{
    if (mmap_lpar_addr != 0) {
        lv1_undocumented_function_115(mmap_lpar_addr);
    }
}


void
init_screen()
{
    void* host_addr = memalign(1024 * 1024, 1024 * 1024);
    assert(host_addr != NULL);

    context = realityInit(0x10000, 1024 * 1024, host_addr);
    assert(context != NULL);

    assert(videoGetState(0, 0, &state) == 0);
    assert(state.state == 0);

    assert(videoGetResolution(state.displayMode.resolution, &res) == 0);

    memset( &vconfig, 0, sizeof(VideoConfiguration) );
    vconfig.resolution = state.displayMode.resolution;
    vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
    vconfig.pitch = res.width * 4;

    assert(videoConfigure(0, &vconfig, NULL, 0) == 0);
    assert(videoGetState(0, 0, &state) == 0);

    s32 buffer_size = 4 * res.width * res.height;

    gcmSetFlipMode(GCM_FLIP_VSYNC);
    makeBuffer(0, buffer_size);
    makeBuffer(1, buffer_size);

    gcmResetFlipStatus();
    flip(1);
}

void
install_bootos()
{
    char ts[400];

    u64 i;

    xputs("Mapping LV1...");
    install_new_poke();
    if ( !map_lv1() ) {
        xputs("Cannot map LV1!");
        return;
    }
    if (QUICK_SCAN) {
        xputs("Quickscanning LV1...");
        if (lv1_peek(0x1600C0ULL) == 0x2F666C682F6F732FULL) {
            i = 0x1600C0ULL;
        } else if (lv1_peek(0x980C0ULL) == 0x2F666C682F6F732FULL) {
            i = 0x980C0ULL;
        } else if (lv1_peek(0xA7E60ULL) == 0x2F666C682F6F732FULL) {
            i = 0xA7E60ULL;
        }
    } else {
        u64 q1 = 0;
        u64 q2 = 0;
        xputs("Scanning LV1...");
        for (i = 0; i < HV_SIZE; i += 8) {
            q2 = lv1_peek(i);
            if (q1 == 0x2F666C682F6F732FULL && q2 == 0x6C76325F6B65726E) {
                i -= 8;
                break;
            }
            q1 = q2;
        }
    }
    xputs("Unmapping LV1...");
    unmap_lv1();
    remove_new_poke();

    snprintf(ts, sizeof(ts), "LV1 PS3_LPAR kernel filename offset at %08llX.", i & 0xFFFFFFFFULL);
    xputs(ts);
    if (i == 0x1600C0ULL) {
        xputs("Patch: PS3 FAT 16M + FW3.55.");
    } else if (i == 0x980C0ULL) {
        xputs("Patch: PS3 FAT 256M + FW3.55.");
    } else if (i == 0xA7E60ULL) {
        xputs("Patch: PS3 Slim + FW3.55.");
    } else {
        xputs("Unknown LV1 offset. Please report!");
    }
    if (i >= HV_SIZE) {
        xputs("Cannot generate a patch :( Please report your PS3 model.");
        return;
    }
    // Lv2Patcher works on mapped memory
    i += HV_BASE;
    i &= 0xFFFFFFFFULL;
    if ( Lv2Syscall8(837, (u64)"CELL_FS_IOS:BUILTIN_FLSH1", (u64)"CELL_FS_FAT", (u64)"/dev_rwflash", 0, 0, 0, 0, 0) ) {
        xputs("Flash remap failed!");
    }
    xputs("Reading BootOS...");
    FILE* f = fopen("/dev_hdd0/game/LNX000001/USRDIR/bootos.bin", "r");
    if (!f) {
        xputs("Cannot open BootOS binary!");
        return;
    }
    size_t sz, sz1;
    u8* data = (u8*)read_file(f, &sz);
    if (!data) {
        xputs("Cannot read BootOS binary!");
        fclose(f);
        return;
    }
    unlink("/dev_rwflash/lv2_kernel.self");

    FILE* g = fopen("/dev_rwflash/lv2_kernel.self", "w");
    if (!g) {
        fclose(f);
        xputs("Cannot open flash!");
        return;
    }
    sz1 = sz;
    while (sz > 0) {
        sprintf( ts, "Writing BootOS: %02d%%", (int)( (sz1 - sz) * 100 / sz1 ) );
        xputs(ts);
        if (sz >= CHUNK) {
            fwrite(data + (sz1 - sz), CHUNK, 1, g);
            sz -= CHUNK;
        } else {
            fwrite(data + (sz1 - sz), sz, 1, g);
            sz = 0;
        }
    }
    fclose(f);
    fclose(g);

    xputs("Adding \"Linux\" entry...");
    f = fopen("/dev_hdd0/game/LV2000000/USRDIR/linux.txt", "w");
    if (!f) {
        xputs("Cannot add a new patchset to LV2 patcher!");
        return;
    }
    fputs("# Linux\nlv1en\n", f);
    fprintf(f, "%08lX: 2f6c6f63616c5f73\n", i);
    fprintf(f, "%08lX: 7973302f6c76325f\n", i + 8);
    fprintf(f, "%08lX: 6b65726e656c2e73\n", i + 16);
    fprintf(f, "%08lX: 656c6600\n", i + 24);
    fprintf(f, "%08lX: 000000000000001b\n", i + 0x120);
    fputs("lv1dis\n", f);
    fputs("panic\n", f);
    fclose(f);

    xputs("Creating kboot configuration file...");
    f = fopen("/dev_hdd0/kboot.conf", "w");
    fputs("Install Debian GNU/Linux=http://ftp.debian.org/debian/dists/squeeze/main/installer-powerpc/current/images/powerpc64/netboot/vmlinux initrd=http://ftp.debian.org/debian/dists/squeeze/main/installer-powerpc/current/images/powerpc64/netboot/initrd.gz preseed/url=http://boot.khore.org/mod/preseed.cfg auto=true interface=auto priority=critical\n", f);
    fclose(f);

    /*
       xputs("Creating 10G file...");
       f = fopen("/dev_hdd0/linux.img", "w");
       data = malloc(1 << 20);
       for(i = 0; i < 10240; i++)
       fwrite(data, 1 << 20, 1, f);
       fclose(f);
       free(data);
     */

    xputs("All done.");
    xputs("Please run the LV2 patcher.");
    xputs("");
}

void
xputs(const char* msg)
{
    static int y = 150;
    int i, j;

    waitFlip();
    if (y == 150) {
        for (i = 0; i < res.height; i++) {
            for (j = 0; j < res.width; j++) {
                buffers[currentBuffer]->ptr[i * res.width + j] =
                    FONT_COLOR_BLACK;
            }
        }
    } else {
        memcpy( buffers[currentBuffer]->ptr,
            buffers[!currentBuffer]->ptr,
            res.width * res.height * sizeof(u32) );
    }
    if (y >= 470) {
        memmove( buffers[currentBuffer]->ptr,
            buffers[currentBuffer]->ptr + res.width * 40,
            res.width * (res.height - 40) * sizeof(u32) );
        y -= 40;
    }
    for (i = y; i < y + 32; i++) {
        for (j = 0; j < res.width; j++) {
            buffers[currentBuffer]->ptr[i * res.width + j] =
                FONT_COLOR_BLACK;
        }
    }
    print(150, y, msg, buffers[currentBuffer]->ptr);
    if (msg[strlen(msg + 1)] != '%') {
        y += 40;
    }
    flip(currentBuffer);
    currentBuffer = !currentBuffer;
}

int
main()
{
    int i;

    init_screen();
    ioPadInit(7);
    sconsoleInit(FONT_COLOR_BLACK, FONT_COLOR_WHITE, res.width, res.height);

    xputs("== BootOS Installer ==");
    install_bootos();
    Lv2Syscall1(838, (u64)"/dev_rwflash");

    xputs("Press [X] to exit.");
    while (1) {
        ioPadGetInfo(&padinfo);
        for (i = 0; i < MAX_PADS; i++) {
            if (padinfo.status[i]) {
                ioPadGetData(i, &paddata);
                if (paddata.BTN_CROSS) {
                    return 0;
                }
            }
        }
        usleep(100000);
    }
    return 0;
}
