#ifndef HVCALL_H
#define HVCALL_H

#include <stdint.h>

#define HV_BASE 0x8000000014000000ULL   // where in lv2 to map lv1
#define HV_SIZE 0x800000

#define HPTE_V_BOLTED    0x0000000000000010ULL
#define HPTE_V_LARGE     0x0000000000000004ULL
#define HPTE_V_VALID     0x0000000000000001ULL
#define HPTE_R_PROT_MASK 0x0000000000000003ULL
#define MM_EA2VA(ea)     ( (ea) & ~0x8000000000000000ULL )

extern int
lv1_insert_htab_entry(
    uint64_t  htab_id,
    uint64_t  hpte_group,
    uint64_t  hpte_v,
    uint64_t  hpte_r,
    uint64_t  bolted_flag,
    uint64_t  flags,
    uint64_t* hpte_index,
    uint64_t* hpte_evicted_v,
    uint64_t* hpte_evicted_r);

extern int
lv1_allocate_memory(
    uint64_t  size,
    uint64_t  page_size_exp,
    uint64_t  flags,
    uint64_t* addr,
    uint64_t* muid);

extern int
lv1_undocumented_function_114(
    uint64_t  start,
    uint64_t  page_size,
    uint64_t  size,
    uint64_t* lpar_addr);

extern void
lv1_undocumented_function_115(
    uint64_t lpar_addr);

extern void*
lv2_alloc(
    uint64_t size,
    uint64_t pool);

#endif
