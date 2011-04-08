#include "hvcall.h"
#include "peek_poke.h"
#include "syscall_patch.h"

#define SC_QUOTE_(x) # x
#define SYSCALL(num) "li %%r11, " SC_QUOTE_(num) "; sc;"

static int
install_hvsc_redirection(
    struct syscall_patch_ctx* ctx,
    int hvcall)
{
    uint64_t shell_code_hvsc_base[] = {
        0x7C0802A6F8010010ULL, 0x3960000044000022ULL,
        0xE80100107C0803A6ULL, 0x4e80002060000000ULL
    };
    shell_code_hvsc_base[1] |= (uint64_t)hvcall << 32;
    return syscall_patch(ctx, SC_SLOT_2, 4, shell_code_hvsc_base);
}

int
lv1_insert_htab_entry(
    uint64_t htab_id,
    uint64_t hpte_group,
    uint64_t hpte_v,
    uint64_t hpte_r,
    uint64_t bolted_flag,
    uint64_t flags,
    uint64_t* hpte_index,
    uint64_t* hpte_evicted_v,
    uint64_t* hpte_evicted_r)
{
    struct syscall_patch_ctx ctx;
    install_hvsc_redirection(&ctx, 0x9e);

    // call lv1_insert_htab_entry
    uint64_t ret = 0, ret_hpte_index = 0, ret_hpte_evicted_v =
        0, ret_hpte_evicted_r = 0;
    __asm__ __volatile__ ("mr %%r3, %4;" "mr %%r4, %5;" "mr %%r5, %6;"
                          "mr %%r6, %7;" "mr %%r7, %8;" "mr %%r8, %9;"
                          SYSCALL(SC_SLOT_2) "mr %0, %%r3;" "mr %1, %%r4;"
                          "mr %2, %%r5;" "mr %3, %%r6;" : "=r" (ret),
                          "=r" (ret_hpte_index), "=r" (ret_hpte_evicted_v),
                          "=r" (ret_hpte_evicted_r)
                          : "r" (htab_id), "r" (hpte_group), "r" (hpte_v),
                          "r" (hpte_r), "r" (bolted_flag), "r" (flags)
                          : "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
                          "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                          "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    syscall_unpatch(&ctx);

    *hpte_index = ret_hpte_index;
    *hpte_evicted_v = ret_hpte_evicted_v;
    *hpte_evicted_r = ret_hpte_evicted_r;
    return (int)ret;
}

int
lv1_allocate_memory(
    uint64_t size,
    uint64_t page_size_exp,
    uint64_t flags,
    uint64_t* addr,
    uint64_t* muid)
{
    struct syscall_patch_ctx ctx;
    install_hvsc_redirection(&ctx, 0x00);

    // call lv1_allocate_memory
    uint64_t ret = 0, ret_addr = 0, ret_muid = 0;
    __asm__ __volatile__ ("mr %%r3, %3;"
                          "mr %%r4, %4;"
                          "li %%r5, 0;"
                          "mr %%r6, %5;"
                          SYSCALL(SC_SLOT_2)
                          "mr %0, %%r3;"
                          "mr %1, %%r4;"
                          "mr %2, %%r5;" : "=r" (ret), "=r" (ret_addr),
                          "=r" (ret_muid)
                          : "r" (size), "r" (page_size_exp), "r" (flags)
                          : "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
                          "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                          "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    syscall_unpatch(&ctx);

    *addr = ret_addr;
    *muid = ret_muid;
    return (int)ret;
}

int
lv1_undocumented_function_114(
    uint64_t start,
    uint64_t page_size,
    uint64_t size,
    uint64_t* lpar_addr)
{
    struct syscall_patch_ctx ctx;
    install_hvsc_redirection(&ctx, 0x72);

    // call lv1_undocumented_function_114
    uint64_t ret = 0, ret_lpar_addr = 0;
    __asm__ __volatile__ ("mr %%r3, %2;"
                          "mr %%r4, %3;"
                          "mr %%r5, %4;"
                          SYSCALL(SC_SLOT_2)
                          "mr %0, %%r3;"
                          "mr %1, %%r4;" : "=r" (ret), "=r" (ret_lpar_addr)
                          : "r" (start), "r" (page_size), "r" (size)
                          : "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
                          "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                          "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    syscall_unpatch(&ctx);

    *lpar_addr = ret_lpar_addr;
    return (int)ret;
}

void
lv1_undocumented_function_115(
    uint64_t lpar_addr)
{
    struct syscall_patch_ctx ctx;
    install_hvsc_redirection(&ctx, 0x73);

    // call lv1_undocumented_function_115
    __asm__ __volatile__ ("mr %%r3, %0;" SYSCALL(SC_SLOT_2)
                              : // no return registers
                              : "r" (lpar_addr)
                              : "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
                              "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                              "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    syscall_unpatch(&ctx);
}

void*
lv2_alloc(
    uint64_t size,
    uint64_t pool)
{
    struct syscall_patch_ctx ctx;
    static const uint64_t shell_code[] = {
        0x7C0802A67C140378ULL, 0x4BECB6317E80A378ULL, 0x7C0803A64E800020ULL
    };
    syscall_patch(&ctx, SC_SLOT_2, 3, shell_code);

    void* ret = 0;
    __asm__ __volatile__ ("mr %%r3, %1;"
                          "mr %%r4, %2;"
                          SYSCALL(SC_SLOT_2) "mr %0, %%r3;" : "=r" (ret)
                          : "r" (size), "r" (pool)
                          : "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8",
                          "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
                          "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

    syscall_unpatch(&ctx);

    return ret;
}
