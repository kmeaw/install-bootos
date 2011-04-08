#include <stdint.h>
#include <psl1ght/lv2.h>

#include "syscall_patch.h"
#include "peek_poke.h"

/* ---------------------------------------------------------------------------
 * Defines
 * -------------------------------------------------------------------------*/

#ifndef FIRMWARE
#define FIRMWARE 355
#endif

#if FIRMWARE == 355
#define SYSCALL_TABLE_ADDRESS 0x8000000000346570ULL
#elif FIRMWARE == 341 || FIRMWARE == 340
#define SYSCALL_TABLE_ADDRESS 0x80000000002EB128ULL;
#elif FIRMWARE == 330
#define SYSCALL_TABLE_ADDRESS 0x80000000002EA728ULL;
#elif FIRMWARE == 321
#define SYSCALL_TABLE_ADDRESS 0x80000000002EA8A0ULL;
#elif FIRMWARE == 315
#define SYSCALL_TABLE_ADDRESS 0x80000000002EA820ULL
#else
#error Firmware not supported
#endif

/* ---------------------------------------------------------------------------
 * Structures
 * -------------------------------------------------------------------------*/

/** HV syscall table entry
 * Just informative so you know why a syscall plug always requires two
 * indirections */
struct syscall_table_entry {
	void *func;
		/**< Function pointer */
	void *toc;
		/**< TOC pointer */
};

/* ---------------------------------------------------------------------------
 * Module vars
 * -------------------------------------------------------------------------*/

/** Syscall table - Do not use directly, access requires privileges only
 * lv2_peek can circumvent. This variable serves as a simple mean to get
 * the address of an entry */
static struct syscall_table_entry **syscall_table =
    (struct syscall_table_entry **)SYSCALL_TABLE_ADDRESS;

/* ---------------------------------------------------------------------------
 * Functions
 * -------------------------------------------------------------------------*/

int
syscall_patch(struct syscall_patch_ctx *ctx,
	      int syscall, int nbpatch, const uint64_t * patch)
{
	int i;
	uint64_t entry;
	uint64_t func;

	if (syscall < 0 || nbpatch > MAXPATCH) {
		return -1;
	}

	ctx->syscall = syscall;
	ctx->nbpatch = nbpatch;

	// First find the adress of the code
	entry = lv2_peek((uint64_t) & syscall_table[syscall]);
	func = lv2_peek(entry);

	// Copy over the new code
	for (i = 0; i < ctx->nbpatch; i++) {
		ctx->backup[i] = lv2_peek(func);
		ctx->patch[i] = patch[i];
		lv2_poke(func, patch[i]);
		func += sizeof(uint64_t);
	}

	return 0;
}

int syscall_unpatch(struct syscall_patch_ctx *ctx)
{
	int i;
	uint64_t entry;
	uint64_t func;

	if (ctx->syscall < 0) {
		return -1;
	}
	// First get the adress of the code
	entry = lv2_peek((uint64_t) & syscall_table[ctx->syscall]);
	func = lv2_peek(entry);

	for (i = 0; i < ctx->nbpatch; i++) {
		uint64_t patched = lv2_peek(func);
		if (patched != ctx->patch[i]) {
			while (i--) {
				func -= sizeof(uint64_t);
				lv2_poke(func, ctx->patch[i]);
			}
			return -1;
		}
		lv2_poke(func, ctx->backup[i]);
		func += sizeof(uint64_t);
	}

	ctx->syscall = -1;
	ctx->nbpatch = 0;

	return 0;
}

void syscall_jumpto(int syscall, void *addr)
{
	uint64_t entry = lv2_peek((uint64_t) & syscall_table[syscall]);
	uint64_t func = lv2_peek(entry);

	// Make the entry function point to the specified address
	lv2_poke(entry, (uint64_t) addr);

	// Call it
	Lv2Syscall0(syscall);

	// Retore old func entry
	lv2_poke(entry, func);
}
