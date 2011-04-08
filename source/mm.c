#include "mm.h"
#include "hvcall.h"
#include "debug.h"

extern int debug_sock;
extern char debug_buffer[1024];

// This is mainly adapted from graf's code

int
mm_insert_htab_entry(
    uint64_t va_addr,
    uint64_t lpar_addr,
    uint64_t prot,
    uint64_t* index)
{
    uint64_t hpte_group;
    uint64_t hpte_index = 0;
    uint64_t hpte_v;
    uint64_t hpte_r;
    uint64_t hpte_evicted_v;
    uint64_t hpte_evicted_r;

    hpte_group = (((va_addr >> 28) ^ ((va_addr & 0xFFFFFFFULL) >> 12 )) & 0x7FF)<<3;
    hpte_v = ((va_addr >> 23) << 7) | HPTE_V_VALID;
    hpte_r = lpar_addr | 0x38 | (prot & HPTE_R_PROT_MASK);

    int result = lv1_insert_htab_entry(0, hpte_group, hpte_v, hpte_r, HPTE_V_BOLTED,
                                       0,
                                       &hpte_index, &hpte_evicted_v,
                                       &hpte_evicted_r);
    if (result != 0) {
        PRINTF("lv1_insert_htab_entry failed, returned %d (va_addr=%016lx, lpar_addr=%016lx)\n",
               result, va_addr, lpar_addr);
    }
    if ((result == 0) && (index != 0)) {
        *index = hpte_index;
    }
    return result;
}

int
mm_map_lpar_memory_region(
    uint64_t lpar_start_addr,
    uint64_t ea_start_addr,
    uint64_t size,
    uint64_t page_shift,
    uint64_t prot)
{
    int i;
    int result;
    for (i = 0; i < (size>>page_shift); i++) {
        result = mm_insert_htab_entry(MM_EA2VA(ea_start_addr), lpar_start_addr, prot, 0);
        if (result != 0) {
            return result;
        }
        lpar_start_addr += (1 << page_shift);
        ea_start_addr += (1 << page_shift);
    }
    return 0;
}

void*
lv1_alloc(
    size_t sz,
    void* addr)
{
    uint64_t lpar, muid;
    int s = 1;
    int p = 12;
    int result;
    while ((1 << p) < sz ) {
        p++;
    }
    s = sz;
    s += (1 << p) - 1;
    s &= ~((1 << p) - 1);

    result = lv1_allocate_memory(s, p, 0, &lpar, &muid);
    if (result) {
        PRINTF("lv1_allocate_memory(%d,%d,...) has failed with %08X :(\n", s, p, result);
        return NULL;
    } else {
        PRINTF("lv1_allocate_memory(%d,%d,...) done!\n", s, p);
    }
    if (!addr) {
        addr = (void*)0x8000000013390000ULL;
    } else {
        addr = (void*)(((uint64_t) addr) | 0x8000000000000000ULL);
    }
    result = mm_map_lpar_memory_region(lpar, (uint64_t)addr, s, 0xC, 0);
    if (result) {
        PRINTF("mm_map_lpar_memory_region(%p,...) has failed with %08X :(\n", (void*)lpar, result);
        return NULL;
    }
    return addr;
}
