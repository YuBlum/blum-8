#include <bus.h>
#include <cpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 memory[0x10000];

void
bus_write_byte(u16 addr, u8 val) {
	if (addr >= ROM_BEGIN) return;
	memory[addr] = val;
}

void
bus_write_word(u16 addr, u16 val) {
	if (addr >= ROM_BEGIN) return;
	*(u16 *)(memory + addr) = val;
}

u8
bus_read_byte(u16 addr) {
	return memory[addr];
}

u16
bus_read_word(u16 addr) {
	return *(u16 *)(memory + addr);
}

void
bus_cartridge_load(u8 *rom, u32 rom_size) {
	if (rom_size > 0xffff - ROM_BEGIN) {
		fprintf(stderr, "error: rom is too big\n");
		exit(1);
	}
	memcpy(memory + ROM_BEGIN, rom, rom_size);
}
