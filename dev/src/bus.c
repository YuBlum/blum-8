#include <bus.h>
#include <string.h>
#include <stdio.h>
#include <cpu.h>

static u8 memory[0x10000] = {
	/* sets main program address */
	[0xfffe] = 0x14, 0x2f,
	/* function to multiply x by y and stores in z */
	[0x5000] = 0x25, 0x00,       /* set z to 0 */
	[0x5002] = 0xc0, 0x69,       /* stores y in address 0x0069 */
	[0x5004] = 0xb9, 0x69,       /* add content of address 0x0069 into z*/
	[0x5006] = 0x02,             /* decremeant x by 1 */
	[0x5007] = 0xe6, 0x04, 0x50, /* jump to address 0x5004 if x is not 0 */
	[0x500a] = 0x0b,             /* exit function */
	/* main program */
	[0x2f14] = 0x26, 3,          /* set x to 3 */
	[0x2f16] = 0x27, 5,          /* set y to 5 */
	[0x2f18] = 0xe1, 0x00, 0x50, /* call multiply function */
	[0x2f1b] = 0xdc, 0x20, 0x04, /* stores result in address 0x0420 */
	[0x2f1e] = 0x26, 2,          /* set x to 2 */
	[0x2f20] = 0x27, 6,          /* set y to 6 */
	[0x2f22] = 0xe1, 0x00, 0x50, /* call multiply function again */
	[0x2f25] = 0xd9, 0x20, 0x04, /* add the value stored in 0x0420 to the result */
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
