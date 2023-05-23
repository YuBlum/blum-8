#ifndef __BUS_H__
#define __BUS_H__

#include <types.h>

#define STACK_BEGIN 0x1000
#define SPR_ATTR    0x1100
#define SCREEN00    0x1200
#define SCREEN01    0x1300
#define SCREEN10    0x1400
#define SCREEN11    0x1500
#define BG_ATTR     0x1600
#define IN_OUT      0x1700
#define BG_TILES    0x16f5
#define SPR_TILES   0x17f7
#define BG_PALS     0x17f9
#define SPR_PALS    0x17fb
#define SCROLL_X    0x17fd
#define SCROLL_Y    0x17fe
#define BG_TILE     0x17ff
#define ROM_BEGIN   0x1800

void bus_write_byte(u16 addr, u8  val);
void bus_write_word(u16 addr, u16 val);
u8   bus_read_byte(u16 addr);
u16  bus_read_word(u16 addr);
void bus_save_state(void);
void bus_cartridge_load(u8 *rom, u32 rom_size);

#endif/*__BUS_H__*/
