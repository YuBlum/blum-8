#include <os.h>
#include <cpu.h>
#include <bus.h>
#include <stdio.h>
#include <assert.h>

#define C 0
#define Z 1
#define L 2
#define N 3
#define V 4
#define D 5

static u8  x; /* x       register */
static u8  y; /* y       register */
static u8  z; /* z       register */
static u8  f; /* flags   register */
static u8  s; /* stack   register */
static u16 t; /* timer   register */
static u16 p; /* pointer register */
static u8  clock;

union byte {
	u8 b;
	struct {
		u8 l : 4;
		u8 h : 4;
	};
};

struct instruction {
	enum opcode opcode;
	enum addrmd addrmd;
};

static i8 *opcode_name[] = {
	"nop", "inx", "dex", "iny", "dey", "zrx", "zry", "psh", "pop",
	"psf", "pof", "ret", "ext", "dly", "tzy", "tzx", "tyz", "tyx",
	"txz", "txy", "szy", "szx", "sxy", "sec", "sev", "clc", "clv",
	"shr", "shl", "ror", "rol", "and", "lor", "xor", "inc", "dec",
	"adc", "sbc", "ldz", "stz", "ldx", "stx", "ldy", "sty", "jts",
	"jmp", "jeq", "jle", "jgr", "jne", "jnl", "jng", "jpv", "jnv",
	"jpn", "jnn", "jpc", "jnc", "cmp", "cpx", "cpy"
};

/*
static char *addrmd_name[] = {
	"noa", "cst", "zpg", "zpx",
	"zpy", "adr", "adx", "ady"
};
*/

static struct instruction inst[0x100];

#define FLAG_GET(FLAG) ((f >> FLAG) & 1)
#define FLAG_SET(FLAG, VAL) do {\
	if (VAL) f |=  (1 << (FLAG));\
	else     f &= ~(1 << (FLAG));\
} while(0)

void
cpu_startup(void) {
	p = ROM_BEGIN;
	b8 carry = 0;
	for (union byte i = { 0 }; i.b || !carry; carry = (u8)(++i.b) == 0) {
		     if (i.h == 0x0 || i.h == 0x1) inst[i.b].addrmd = NOA;
		else if (i.h == 0x2              ) inst[i.b].addrmd = CST;
		else if (i.h == 0x3 || i.h == 0x4) inst[i.b].addrmd = ADX;
		else if (i.h == 0x5 || i.h == 0x6) inst[i.b].addrmd = ADY;
		else if (i.h == 0x7 || i.h == 0x8) inst[i.b].addrmd = ZPX;
		else if (i.h == 0x9 || i.h == 0xa) inst[i.b].addrmd = ZPY;
		else if (i.h == 0xb || i.h == 0xc) inst[i.b].addrmd = ZPG;
		else if (i.h >= 0xd              ) inst[i.b].addrmd = ADR;
		switch (inst[i.b].addrmd) {
			case NOA:
				if ((i.h & 1) == 0) {
					inst[i.b].opcode = i.l;
				} else {
					inst[i.b].opcode = i.l + TYX;
				}
				break;
			case CST:
				inst[i.b].opcode = i.l + AND;
				     if (i.l == 0x3) inst[i.b].opcode = ADC;
				else if (i.l == 0x4) inst[i.b].opcode = SBC;
				else if (i.l == 0x5) inst[i.b].opcode = LDZ;
				else if (i.l == 0x6) inst[i.b].opcode = LDX;
				else if (i.l == 0x7) inst[i.b].opcode = LDY;
				else if (i.l == 0x8) inst[i.b].opcode = CMP;
				else if (i.l == 0x9) inst[i.b].opcode = CPX;
				else if (i.l == 0xa) inst[i.b].opcode = CPY;
				break;
			case ADX:
			case ADY:
			case ZPX:
			case ZPY:
			case ZPG:
				inst[i.b].opcode = i.l + SHR;
				if ((i.h & 1) == 0) {
					     if (i.l == 0x0) inst[i.b].opcode = STY;
					else if (i.l == 0x1) inst[i.b].opcode = CMP;
					else if (i.l == 0x2) inst[i.b].opcode = CPX;
					else if (i.l == 0x3) inst[i.b].opcode = CPY;
				}
				break;
			case ADR:
				inst[i.b].opcode = (i.b - 0xd0 + SHR) % OPCODE_COUNT;
				break;
		}
		/*printf("%.2x %s %s\n", i.b, opcode_name[inst[i.b].opcode], addrmd_name[inst[i.b].addrmd]);*/
	}
}

void
cpu_print_registers(void) {
	printf("x: %.2x, y: %.2x, z: %.2x\n", x, y, z);
	printf("Flags:\n");
	printf("Carry: %u, Zero: %u, Less: %u, Negative: %u, Overflow: %u\n",
		FLAG_GET(C), FLAG_GET(Z), FLAG_GET(L), FLAG_GET(N), FLAG_GET(V));
	printf("Addresses:\n");
	printf("Stack: %.2x, Pointer: %.4x\n", s, p);
}

void
cpu_tick(void) {
	if (clock > 0) {
		clock--;
		return;
	}
	/*
	cpu_print_registers();
	cpu_disassemble();
		*/
	u16 addr = 0;
	b8  usez = 0;
	u16 word;
	u8  byte;
	u8  idx = bus_read_byte(p);
	if (FLAG_GET(D)) {
		//printf("%u\n", t);
		FLAG_SET(D, --t != 0);
		return;
	}
	p++;
	switch (inst[idx].addrmd) {
		case NOA: usez = 1;                    clock = 2;                                             break;
		case CST: addr = p++;                  clock = 2;                                             break;
		case ZPG: addr = bus_read_byte(p++);   clock = 3;                                             break;
		case ADR: addr = bus_read_word(p);     clock = 4;                                     p += 2; break;
		case ZPX: addr = bus_read_byte(p) + x; clock = 4 + ((0xff00 & addr) != (0xff00 & p)); p += 1; break;
		case ZPY: addr = bus_read_byte(p) + y; clock = 4 + ((0xff00 & addr) != (0xff00 & p)); p += 1; break;
		case ADX: addr = bus_read_word(p) + x; clock = 5 + ((0xff00 & addr) != (0xff00 & p)); p += 2; break;
		case ADY: addr = bus_read_word(p) + y; clock = 5 + ((0xff00 & addr) != (0xff00 & p)); p += 2; break;
		default: assert(0);                                                                           break;
	}
	switch (inst[idx].opcode) {
		case NOP:
			break;
		case INX:
			x++;
			FLAG_SET(Z, !x);
			FLAG_SET(N, x & 0x80);
			break;
		case DEX:
			x--;
			FLAG_SET(Z, !x);
			FLAG_SET(N, x & 0x80);
			break;
		case INY:
			y++;
			FLAG_SET(Z, !y);
			FLAG_SET(N, y & 0x80);
			break;
		case DEY:
			y--;
			FLAG_SET(Z, !y);
			FLAG_SET(N, y & 0x80);
			break;
		case ZRX:
			x ^= x;
			FLAG_SET(Z, 1);
			FLAG_SET(N, 0);
			break;
		case ZRY:
			y ^= y;
			FLAG_SET(Z, 1);
			FLAG_SET(N, 0);
			break;
		case PSH:
			bus_write_byte(STACK_BEGIN + s--, z);
			break;
		case POP:
			z = bus_read_byte(STACK_BEGIN + ++s);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			clock++;
			break;
		case PSF: bus_write_byte(STACK_BEGIN + s--, f); clock += 1; break;
		case POF: f = bus_read_byte(STACK_BEGIN + ++f); clock += 2; break;
		case RET:
			p  = bus_read_byte(STACK_BEGIN + ++s)  & 0x00ff;
			p |= bus_read_byte(STACK_BEGIN + ++s) << 8;
			clock += 3;
			break;
		case EXT:
			f  = bus_read_byte(STACK_BEGIN + ++s);
			p  = bus_read_byte(STACK_BEGIN + ++s)  & 0x00ff;
			p |= bus_read_byte(STACK_BEGIN + ++s) << 8;
			clock += 3;
			break;
		case DLY:
			t = x << 8 | y;
			FLAG_SET(D, t);
			break;
		case TZY: y = z;                  break;
		case TZX: x = z;                  break;
		case TYZ: z = y;                  break;
		case TYX: x = y;                  break;
		case TXZ: z = x;                  break;
		case TXY: y = x;                  break;
		case SZY: z ^= y; y ^= z; z ^= y; break;
		case SZX: z ^= x; x ^= z; z ^= x; break;
		case SXY: x ^= y; y ^= x; x ^= y; break;
		case SEC: FLAG_SET(C, 1);         break;
		case SEV: FLAG_SET(V, 1);         break;
		case CLC: FLAG_SET(C, 0);         break;
		case CLV: FLAG_SET(V, 0);         break;
		case SHR:
			if (usez) {
				FLAG_SET(C, z & 1);
				z >>= 1;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			}
			byte = bus_read_byte(addr);
			FLAG_SET(C, byte & 1);
			byte >>= 1;
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case SHL:
			if (usez) {
				FLAG_SET(C, z >> 7);
				z <<= 1;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			}
			byte = bus_read_byte(addr);
			FLAG_SET(C, byte >> 7);
			byte <<= 1;
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case ROR:
			if (usez) {
				z = (z >> 1) | ((z & 1) << 7);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			}
			byte = bus_read_byte(addr);
			byte = (byte >> 1) | ((byte & 1) << 7);
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case ROL:
			if (usez) {
				z = (z << 1) | (z >> 7);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			}
			byte = bus_read_byte(addr);
			byte = (byte << 1) | (byte >> 7);
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case AND:
			z &= bus_read_byte(addr);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case LOR:
			z |= bus_read_byte(addr);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case XOR:
			z ^= bus_read_byte(addr);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case INC:
			byte = bus_read_byte(addr) + 1;
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case DEC:
			byte = bus_read_byte(addr) - 1;
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case ADC:
			byte = bus_read_byte(addr);
			word = (u16)z + (u16)byte + (u16)FLAG_GET(C);
			FLAG_SET(C, word & 0xff00);
			FLAG_SET(Z, !(word & 0xff));
			FLAG_SET(N, word & 0x80);
			FLAG_SET(V, (((z^word))&~(z^byte)) & 0x80);
			z = word & 0xff;
			break;
		case SBC:
			byte = bus_read_byte(addr);
			word = (u16)z - (u16)byte + (u16)FLAG_GET(C);
			FLAG_SET(C, word & 0xff00);
			FLAG_SET(Z, !(word & 0xff));
			FLAG_SET(N, word & 0x80);
			FLAG_SET(V, (((z^word))&~(z^byte)) & 0x80);
			z = word & 0xff;
			break;
		case LDZ: 
			z = bus_read_byte(addr);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case STZ:
			bus_write_byte(addr, z);
			break;
		case LDX:
			x = bus_read_byte(addr);
			FLAG_SET(Z, !x);
			FLAG_SET(N, x & 0x80);
			break;
		case STX:
			bus_write_byte(addr, x);
			break;
		case LDY:
			y = bus_read_byte(addr);
			FLAG_SET(Z, !y);
			FLAG_SET(N, y & 0x80);
			break;
		case STY:
			bus_write_byte(addr, y);
			break;
		case JTS:
			bus_write_byte(STACK_BEGIN + s--, p >> 8);
			bus_write_byte(STACK_BEGIN + s--, p & 0xff);
			p = addr;
			clock += 3;
			break;
		case JMP:                                     p = addr; clock++;   break;
		case JEQ: if ( FLAG_GET(Z)                ) { p = addr; clock++; } break;
		case JLE: if ( FLAG_GET(L)                ) { p = addr; clock++; } break;
		case JGR: if (!FLAG_GET(L) && !FLAG_GET(Z)) { p = addr; clock++; } break;
		case JNE: if (!FLAG_GET(Z)                ) { p = addr; clock++; } break;
		case JNL: if (!FLAG_GET(L)                ) { p = addr; clock++; } break;
		case JNG: if ( FLAG_GET(L) ||  FLAG_GET(Z)) { p = addr; clock++; } break;
		case JPV: if ( FLAG_GET(V)                ) { p = addr; clock++; } break;
		case JNV: if (!FLAG_GET(V)                ) { p = addr; clock++; } break;
		case JPN: if ( FLAG_GET(N)                ) { p = addr; clock++; } break;
		case JNN: if (!FLAG_GET(N)                ) { p = addr; clock++; } break;
		case JPC: if ( FLAG_GET(C)                ) { p = addr; clock++; } break;
		case JNC: if (!FLAG_GET(C)                ) { p = addr; clock++; } break;
		case CMP:
			byte = z - bus_read_byte(addr);
			FLAG_SET(Z, byte == 0);
			FLAG_SET(L, byte & 0x80);
			break;
		case CPX:
			byte = x - bus_read_byte(addr);
			FLAG_SET(Z, byte == 0);
			FLAG_SET(L, byte & 0x80);
			break;
		case CPY:
			byte = y - bus_read_byte(addr);
			FLAG_SET(Z, byte == 0);
			FLAG_SET(L, byte & 0x80);
			break;
		default:
			assert(0);
			break;
	}
}

i32
cpu_opcode_get(const i8 *str, u32 str_size) {
	if (str_size != 3) return -1;
	for (i32 i = 0; i < OPCODE_COUNT; i++) {
		if (
			(str[0] == opcode_name[i][0]      && str[1] == opcode_name[i][1]      && str[2] == opcode_name[i][2]     ) ||
			(str[0] == opcode_name[i][0] + 32 && str[1] == opcode_name[i][1] + 32 && str[2] == opcode_name[i][2] + 32)
		) return i;
	}
	return -1;
}

u8
cpu_instruction_get(enum opcode opcode, enum addrmd addrmd) {
	for (u32 i = 0; i < 0x100; i++) {
		if (inst[i].opcode == opcode && inst[i].addrmd == addrmd) return i;
	}
	return 0;
}
