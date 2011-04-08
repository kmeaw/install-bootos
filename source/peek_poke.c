#include <psl1ght/lv2.h>

#include "peek_poke.h"
#include "hvcall.h"
#include "syscall_patch.h"

static int poke_syscall = SC_POKE;

uint64_t lv2_peek(uint64_t address)
{
	return Lv2Syscall1(SC_PEEK, address);
}

void lv2_poke(uint64_t address, uint64_t value)
{
	Lv2Syscall2(poke_syscall, address, value);
}

void lv2_poke32(uint64_t address, uint32_t value)
{
	uint32_t next = lv2_peek(address) & 0xffffffff;
	lv2_poke(address, (uint64_t) value << 32 | next);
}

uint64_t lv1_peek(uint64_t address)
{
	return Lv2Syscall1(SC_PEEK, HV_BASE + address);
}

void lv1_poke(uint64_t address, uint64_t value)
{
	Lv2Syscall2(SC_POKE, HV_BASE + address, value);
}

static struct syscall_patch_ctx poke_ctx;

void install_new_poke()
{

	/* Shell code is:
	 * std  r4,0(r3)
	 * icbi r0,r3
	 * isync
	 * blr */
	static const uint64_t shell_code[] = {
		0xF88300007C001FACULL, 0x4C00012C4E800020ULL
	};
	syscall_patch(&poke_ctx, SC_SLOT_1, 2, shell_code);
	poke_syscall = SC_SLOT_1;
}

void remove_new_poke()
{
	poke_syscall = SC_POKE;
	syscall_unpatch(&poke_ctx);
}
