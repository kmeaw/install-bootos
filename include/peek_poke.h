#ifndef PEEK_POKE_H
#define PEEK_POKE_H

u64 lv2_peek(u64 address);
void lv2_poke(u64 address, u64 value);
void lv2_poke32(u64 address, u32 value);
void install_new_poke();
void remove_new_poke();

#endif
