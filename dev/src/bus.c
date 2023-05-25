#include <bus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 memory[0x10000] = {
	[BG_PALS  +  0] = 0x10,0x23,
	[BG_PALS  +  2] = 0xf0,0x8c,
	[BG_TILES + 16] = 0b00000000,0b00111111,
										0b00000000,0b11111111,
										0b00000000,0b10101001,
										0b00000010,0b01100101,
										0b00000010,0b01101001,
										0b00000010,0b10010101,
										0b00000000,0b00010101,
										0b00000000,0b10101110,
	[BG_TILES + 32] = 0b11110000,0b00000000,
										0b11111111,0b11000000,
										0b01100100,0b00000000,
										0b01100101,0b01000000,
										0b01011001,0b01010000,
										0b01101010,0b10000000,
										0b01010101,0b00000000,
										0b10100000,0b00000000,
	[BG_TILES + 48] = 0b00000010,0b10101110,
										0b00001010,0b10101111,
										0b00000101,0b10110111,
										0b00000101,0b01111111,
										0b00000101,0b11111111,
										0b00000000,0b11111100,
										0b00000010,0b10100000,
										0b00001010,0b10100000,
	[BG_TILES + 64] = 0b11111111,0b11111111,
										0b11111111,0b11111111,
										0b11111111,0b11111111,
										0b11111111,0b11111111,
										0b11111111,0b11111111,
										0b11111111,0b11111111,
										0b11111111,0b11111111,
										0b11111111,0b11111111,
	[SCREEN00 + 7 * 32 + 7] = 1,
	[BG_ATTR  + 7 * 16 + 7] = 0b00000001,
	[SCREEN00 + 7 * 32 + 8] = 2,
	[BG_ATTR  + 7 * 16 + 8] = 0b00000001,
	[SCREEN00 + 8 * 32 + 7] = 3,
	[BG_ATTR  + 8 * 16 + 7] = 0b00000001,
	[SCREEN00 + 8 * 32 + 8] = 3,
	[BG_ATTR  + 8 * 16 + 8] = 0b00001001,
	[SCREEN00 + 0 * 32 + 16] = 4,
};

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
bus_save_state(void) {
	FILE *file = fopen("memory", "w");
	fprintf(file, "address | hex | bin\n");
	for (u32 i = 0; i < 0x10000; i++) {
		fprintf(file, "[%.4x]  | %.2x  | ", i, memory[i]);
		for (i8 j = 7; j >= 0; j--) {
			fprintf(file, "%u", (memory[i] >> j) & 0x1);
		}
		fputc('\n', file);
	}
	fclose(file);
}

void
bus_cartridge_load(u8 *rom, u32 rom_size) {
	if (rom_size > 0x10000 - ROM_BEGIN) {
		fprintf(stderr, "error: rom is too big\n");
		exit(1);
	}
	memcpy(memory + ROM_BEGIN, rom, rom_size);
	bus_save_state();
}
