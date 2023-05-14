#include <cpu.h>
#include <bus.h>

struct instruction {
	u32 opcode;
	b8  zeropage;
	/*simulate cycles*/
};

static u8  x;
static u8  y;
static u8  z;
static u8  s;
static u8  f;
static u16 p;

static struct instruction lookup[0xff] = {
	[0x60] = { NOP, 0 }, [0x71] = { INX, 0 }, [0x72] = { DEX, 0 }, [0x73] = { INY, 0 }, [0x74] = { DEY, 0 },
	[0x79] = { ADD, 0 }, [0x7a] = { SUB, 0 }, [0x7d] = { ADD, 1 }, [0x7e] = { SUB, 1 }, [0xb9] = { LDZ, 0 },
	[0xbd] = { LDZ, 1 }, [0xf5] = { CPL, 0 }, [0xf1] = { CTL, 0 }, [0xf8] = { DRW, 0 },
};
