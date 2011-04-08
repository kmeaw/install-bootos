#ifndef SYSCALL_PLUG_H
#define SYSCALL_PLUG_H

/* ---------------------------------------------------------------------------
 * Defines
 * -------------------------------------------------------------------------*/

/** Maximum 64bit writes allowed by this framework */
#define MAXPATCH 4

/** Peek syscall */
#define SC_PEEK 6

/** Poke syscall */
#define SC_POKE 7

/** Syscall lv2FsMkdir
 *  Used for injecting code in lv2:
 *    - new poke with cache invalidation (peek_poke.c)
 *    - memcpy (main.c)
 *    - panic (main.c) */
#define SC_SLOT_1 811

/** Syscall lv2FsRmdir
 *  Used for injecting code in HV
 *    - hv redirections (hvcall.c) */
#define SC_SLOT_2 813

/** Syscall ?
 *  Syscall table entry used for overwriting a jumping address */
#define SC_SLOT_3 11

/* ---------------------------------------------------------------------------
 * Structures
 * -------------------------------------------------------------------------*/

/** Syscall context for (un)patching custom code */
struct syscall_patch_ctx
{
    int syscall;               /**< Syscall number */
    int nbpatch;               /**< Number of uint64_t writes/read to operate */
    uint64_t backup[MAXPATCH]; /**< Backup of the original code */
    uint64_t patch[MAXPATCH];  /**< New code to inject */
};

/* ---------------------------------------------------------------------------
 * Functions
 * -------------------------------------------------------------------------*/

extern int
syscall_patch(
    struct syscall_patch_ctx* ctx,
    int syscall,
    int nbpatch,
    const uint64_t* patch);

extern int
syscall_unpatch(
    struct syscall_patch_ctx* ctx);

extern void
syscall_jumpto(
    int syscall,
    void* addr);

#endif
