#include <bus.h>
#include <string.h>
#include <stdio.h>

static u8 memory[0xffff];
static u8 memory_access[0xffff];
static u8 invalid_access;

void
bus_initialize(void) {
	memset(memory + 0x0000, MEMORY_READ|MEMORY_WRITE, 0x0800);
	memset(memory + 0x0800, MEMORY_READ             , 0x0800);
	memset(memory + 0x1000, MEMORY_WRITE            , 0x2150);
	memset(memory + 0x3150, MEMORY_READ|MEMORY_WRITE, 0x0002);
	memset(memory + 0x3152, MEMORY_READ             , 0xcead);
}

u8
bus_invalid_access(void) {
	return invalid_access;
}

void
bus_write_byte(u16 addr, u8 val) {
	if (!(memory_access[addr] & MEMORY_WRITE)) invalid_access = MEMORY_WRITE;
	memory[addr] = val;
}

void
bus_write_word(u16 addr, u16 val) {
	if (!(memory_access[addr] & MEMORY_WRITE && memory_access[addr + 1] & MEMORY_WRITE)) invalid_access = MEMORY_WRITE;
	*(u16 *)(memory + addr) = val;
}

u8
bus_read_byte(u16 addr) {
	if (!(memory_access[addr] & MEMORY_READ)) invalid_access = MEMORY_READ;
	return memory[addr];
}

u16
bus_read_word(u16 addr) {
	if (!(memory_access[addr] & MEMORY_READ && memory_access[addr + 1] & MEMORY_READ)) invalid_access = MEMORY_READ;
	return *(u16 *)(memory + addr);
}
