#include <bus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u8 memory[0x10000];

void
bus_write_byte(u16 addr, u8 val, b8 read_only) {
	if (addr >= ROM_BEGIN && read_only) return;
	memory[addr] = val;
}

void
bus_write_word(u16 addr, u16 val, b8 read_only) {
	if (addr >= ROM_BEGIN && read_only) return;
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
