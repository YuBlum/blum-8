#ifndef __BUS_H__
#define __BUS_H__

#include <types.h>

#define ROM_BEGIN 0x1ffb

void bus_write_byte(u16 addr, u8  val);
void bus_write_word(u16 addr, u16 val);
u8   bus_read_byte(u16 addr);
u16  bus_read_word(u16 addr);

#endif/*__BUS_H__*/
