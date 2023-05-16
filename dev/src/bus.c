#include <bus.h>
#include <string.h>
#include <stdio.h>
#include <cpu.h>

static u8 memory[0x10000] = {
	/* sets start vector address */
	[0x1ffb] = 0x0a, 0x20,
	/* sets frame vector address */
	[0x1ffd] = 0x21, 0x20,
	/* function to multiply x by y and stores in z */
	[0x1fff] = 0x25, 0x00,       /* set z to 0 */
	[0x2001] = 0xc0, 0x69,       /* stores y in address 0x0069 */
	[0x2003] = 0xb9, 0x69,       /* add content of address 0x0069 into z*/
	[0x2005] = 0x02,             /* decremeant x by 1 */
	[0x2006] = 0xe6, 0x03, 0x20, /* jump to address 0x2f1a if x is not 0 */
	[0x2009] = 0x0b,             /* exit function */
	/* start vector */
	[0x200a] = 0x26, 3,          /* set x to 3 */
	[0x200c] = 0x27, 5,          /* set y to 5 */
	[0x200e] = 0xe1, 0xff, 0x1f, /* call multiply function */
	[0x2011] = 0xdc, 0x20, 0x04, /* stores result in address 0x0420 */
	[0x2014] = 0x26, 2,          /* set x to 2 */
	[0x2016] = 0x27, 6,          /* set y to 6 */
	[0x2018] = 0xe1, 0xff, 0x1f, /* call multiply function again */
	[0x201b] = 0xd9, 0x20, 0x04, /* add the value stored in 0x0420 to the result */
	[0x201e] = 0xe2, 0x1e, 0x20, /* infinite loop */
	/* frame vector */
	[0x2021] = 0x0c              /* exit frame vector */
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
