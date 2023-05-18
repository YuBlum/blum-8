#include <cpu.h>
#include <bus.h>
#include <stdio.h>
#include <assert.h>

#define C 0
#define Z 1
#define L 2
#define N 3
#define V 4

static u8  x;
static u8  y;
static u8  z;
static u8  f;
static u8  s = 0xff;
static u16 p = ROM_BEGIN;
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
	"nop", "inx", "dex", "iny", "dey", "zrx", "zry", "psh", "pop", "psf", "pof", "ret", "rti", "tzy", "tzx",
	"tyz", "tyx", "txz", "txy", "szy", "szx", "sxy", "sec", "sev", "clc", "clv", "shr", "shl", "ror", "rol",
	"and", "lor", "xor", "inc", "dec", "adc", "sbc", "ldz", "stz", "ldx", "stx", "ldy", "sty", "jts", "jmp",
	"jeq", "jle", "jgr", "jne", "jnl", "jng", "jpv", "jnv", "jpn", "jnn", "jpc", "jnc", "cmp", "cpx", "cpy"
};

/*
static char *addrmd_name[] = {
	"noa", "cst", "zpg", "zpx",
	"zpy", "adr", "adx", "ady"
};
*/

static struct instruction inst[0x100];
static u16 interrupts[INTERRUPT_COUNT] = {
	[RESET_VECTOR] = ROM_BEGIN,
	[VBLNK_VECTOR] = ROM_BEGIN + 2
};

#define FLAG_GET(FLAG) ((f & (1 << FLAG)) != 0)
#define FLAG_SET(FLAG, VAL) do {\
	if (VAL) f |=  (1 << (FLAG));\
	else     f &= ~(1 << (FLAG));\
} while(0)

void
cpu_interrupt(enum interrupt interrupt) {
	bus_write_byte(0x0800 + s--, p >> 8);
	bus_write_byte(0x0800 + s--, p & 0xff);
	bus_write_byte(0x0800 + s--, f);
	p = bus_read_word(interrupts[interrupt]);
}

void
cpu_startup(void) {
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
cpu_disassemble(void) {
	u8 idx = bus_read_byte(p);
	printf("[%.4x] %s", p, opcode_name[inst[idx].opcode]);
	switch (inst[idx].addrmd) {
		case NOA:                                                break;
		case CST: printf(" 0x%.2x",       bus_read_byte(p + 1)); break;
		case ZPG: printf(" [0x%.2x]",     bus_read_byte(p + 1)); break;
		case ZPX: printf(" [0x%.2x + x]", bus_read_byte(p + 1)); break;
		case ZPY: printf(" [0x%.2x + y]", bus_read_byte(p + 1)); break;
		case ADR: printf(" [0x%.4x]",     bus_read_word(p + 1)); break;
		case ADX: printf(" [0x%.4x + x]", bus_read_word(p + 1)); break;
		case ADY: printf(" [0x%.4x + y]", bus_read_word(p + 1)); break;
		default:  printf(" ????");                               break;
	}
	putchar('\n');
}

b8
cpu_clock(void) {
	if (clock > 0) {
		clock--;
		return 0;
	}
	cpu_print_registers();
	cpu_disassemble();
	u16 addr = 0;
	b8  usez = 0;
	u16 word;
	u8  byte;
	u8  idx = bus_read_byte(p);
	p++;
	switch (inst[idx].addrmd) {
		case NOA: usez = 1;                            clock = 2; break;
		case CST: addr = p++;                          clock = 2; break;
		case ZPG: addr = bus_read_byte(p++);           clock = 2; break;
		case ZPX: addr = bus_read_byte(p++) + x;       clock = 2; break;
		case ZPY: addr = bus_read_byte(p++) + y;       clock = 2; break;
		case ADR: addr = bus_read_word(p);     p += 2; clock = 3; break;
		case ADX: addr = bus_read_word(p) + x; p += 2; clock = 3; break;
		case ADY: addr = bus_read_word(p) + y; p += 2; clock = 3; break;
		default: assert(0);                                      break;
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
			FLAG_SET(N, 0);
			break;
		case ZRY:
			y ^= y;
			FLAG_SET(N, 0);
			break;
		case PSH:
			bus_write_byte(0x0800 + s--, z);
			clock++;
			break;
		case POP:
			z = bus_read_byte(0x0800 + ++s);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			clock += 2;
			break;
		case PSF: bus_write_byte(0x0800 + s--, f); clock += 1; break;
		case POF: f = bus_read_byte(0x0800 + ++f); clock += 2; break;
		case RET:
			p  = bus_read_byte(0x0800 + ++s)  & 0x00ff;
			p |= bus_read_byte(0x0800 + ++s) << 8;
			clock += 4;
			break;
		case RTI:
			s  = bus_read_byte(0x0800 + ++s);
			p  = bus_read_byte(0x0800 + ++s)  & 0x00ff;
			p |= bus_read_byte(0x0800 + ++s) << 8;
			clock += 4;
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
			clock += 3;
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
			clock += 3;
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
			clock += 3;
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
			clock += 3;
			break;
		case AND:
			z &= bus_read_byte(addr);
			clock++;
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case LOR:
			z |= bus_read_byte(addr);
			clock++;
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case XOR:
			z ^= bus_read_byte(addr);
			clock++;
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			break;
		case INC:
			byte = bus_read_byte(addr) + 1;
			clock++;
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case DEC:
			byte = bus_read_byte(addr) - 1;
			clock++;
			FLAG_SET(Z, !byte);
			FLAG_SET(N, byte & 0x80);
			bus_write_byte(addr, byte);
			break;
		case ADC:
			byte = bus_read_byte(addr);
			word = z + byte + FLAG_GET(C);
			FLAG_SET(C, word & 0x100);
			FLAG_SET(Z, !(word & 0xff));
			FLAG_SET(N, (u8)(((z^(word & 0xff))&~(z^byte)) >> 7));
			z = (word & 0xff);
			clock += 3;
			break;
		case SBC:
			byte = bus_read_byte(addr);
			word = z - byte - !FLAG_GET(C);
			FLAG_SET(C, word & 0x100);
			FLAG_SET(Z, !(word & 0xff));
			FLAG_SET(N, (u8)(((z^(word & 0xff))&~(z^byte)) >> 7));
			z = (word & 0xff);
			clock += 3;
			break;
		case LDZ: 
			z = bus_read_byte(addr);
			FLAG_SET(Z, !z);
			FLAG_SET(N, z & 0x80);
			clock++;
			break;
		case STZ:
			bus_write_byte(addr, z);
			clock++;
			break;
		case LDX:
			x = bus_read_byte(addr);
			FLAG_SET(Z, !x);
			FLAG_SET(N, x & 0x80);
			clock++;
			break;
		case STX:
			bus_write_byte(addr, x);
			clock++;
			break;
		case LDY:
			y = bus_read_byte(addr);
			FLAG_SET(Z, !y);
			FLAG_SET(N, y & 0x80);
			clock++;
			break;
		case STY:
			bus_write_byte(addr, y);
			clock++;
			break;
		case JTS:
			bus_write_byte(0x0800 + s--, p >> 8);
			bus_write_byte(0x0800 + s--, p & 0xff);
			p = addr;
			clock += 4;
			break;
		case JMP:                                   p = addr; clock += 3; break;
		case JEQ: if ( FLAG_GET(Z)                ) p = addr; clock += 3; break;
		case JLE: if ( FLAG_GET(L)                ) p = addr; clock += 3; break;
		case JGR: if (!FLAG_GET(L) && !FLAG_GET(Z)) p = addr; clock += 3; break;
		case JNE: if (!FLAG_GET(Z)                ) p = addr; clock += 3; break;
		case JNL: if (!FLAG_GET(L)                ) p = addr; clock += 3; break;
		case JNG: if ( FLAG_GET(L) ||  FLAG_GET(Z)) p = addr; clock += 3; break;
		case JPV: if ( FLAG_GET(V)                ) p = addr; clock += 3; break;
		case JNV: if (!FLAG_GET(V)                ) p = addr; clock += 3; break;
		case JPN: if ( FLAG_GET(N)                ) p = addr; clock += 3; break;
		case JNN: if (!FLAG_GET(N)                ) p = addr; clock += 3; break;
		case JPC: if ( FLAG_GET(C)                ) p = addr; clock += 3; break;
		case JNC: if (!FLAG_GET(C)                ) p = addr; clock += 3; break;
		case CMP:
			byte = z - bus_read_byte(addr);
			FLAG_SET(Z, byte == 0);
			FLAG_SET(L, byte & 0x80);
			clock++;
			break;
		case CPX:
			byte = x - bus_read_byte(addr);
			FLAG_SET(Z, byte == 0);
			FLAG_SET(L, byte & 0x80);
			clock++;
			break;
		case CPY:
			byte = y - bus_read_byte(addr);
			FLAG_SET(Z, byte == 0);
			FLAG_SET(L, byte & 0x80);
			clock++;
			break;
		default:
			assert(0);
			break;
	}
	return 1;
}

i32
cpu_opcode_get(const i8 *str, u32 str_size) {
	if (str_size != 3) return -1;
	for (i32 i = 0; i < OPCODE_COUNT; i++) {
		if (str[0] == opcode_name[i][0] && str[1] == opcode_name[i][1] && str[2] == opcode_name[i][2])
			return i;
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
