#ifndef PEEK_POKE_H
#define PEEK_POKE_H

#include <stdint.h>

extern uint64_t
lv2_peek(
    uint64_t address);

extern void
lv2_poke(
    uint64_t address,
    uint64_t value);

extern void
lv2_poke32(
    uint64_t address,
    uint32_t value);

extern uint64_t
lv1_peek(
    uint64_t address);

extern void
lv1_poke(
    uint64_t address,
    uint64_t value);

extern void
install_new_poke();

extern void
remove_new_poke();

#endif
