#include <bus.h>
#include <string.h>
#include <stdio.h>
#include <cpu.h>

static u8 memory[0x10000] = {
	[0xfffe] = 0x14,
	[0xffff] = 0x2f,
	[0x2f14] = LNX,
	[0x2f15] = 3,
	[0x2f16] = ANC,
	[0x2f17] = 5,
	[0x2f18] = DEX,
	[0x2f19] = JNE,
	[0x2f1a] = 0x16,
	[0x2f1b] = 0x2f,
};

b8
bus_write_byte(u16 addr, u8 val) {
	if (addr > ROM_BEGIN) return 0;
	memory[addr] = val;
	return 1;
}

b8
bus_write_word(u16 addr, u16 val) {
	if (addr > ROM_BEGIN) return 0;
	*(u16 *)(memory + addr) = val;
	return 1;
}

u8
bus_read_byte(u16 addr) {
	return memory[addr];
}

u16
bus_read_word(u16 addr) {
	return *(u16 *)(memory + addr);
}
