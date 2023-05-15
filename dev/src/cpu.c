#include <cpu.h>
#include <bus.h>
#include <stdio.h>

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
static u16 p = 0xfffe;
static u8  clock;

static u8 cycles[INSTRUCTION_COUNT] = {
	2, /* nop */ 2, /* inx */ 2, /* dex */ 2, /* iny */ 2, /* dey */ 3, /* szr */ 3, /* szl */ 5, /* shr */ 5, /* shl */
	5, /* ror */ 5, /* rol */ 3, /* rzr */ 3, /* rzl */ 5, /* and */ 5, /* lor */ 5, /* xor */ 4, /* inc */ 4, /* dec */
	6, /* adc */ 6, /* sbc */ 4, /* ldz */ 4, /* stz */ 4, /* ldx */ 4, /* stx */ 4, /* ldy */ 4, /* sty */ 2, /* zrx */
	2, /* zry */ 4, /* psh */ 4, /* pop */ 4, /* psf */ 4, /* pof */ 6, /* jts */ 4, /* jmp */ 4, /* jeq */ 4, /* jle */
	4, /* jgr */ 4, /* jne */ 4, /* jnl */ 4, /* jng */ 4, /* jpv */ 4, /* jnv */ 4, /* jpn */ 4, /* jnn */ 4, /* jpc */
	4, /* jnc */ 5, /* rfs */ 5, /* rfi */ 2, /* tzy */ 2, /* tzx */ 2, /* tyz */ 2, /* tyx */ 2, /* txz */ 2, /* txy */
	2, /* szy */ 2, /* szx */ 2, /* sxy */ 2, /* sec */ 2, /* sev */ 2, /* clc */ 2, /* clv */ 3, /* cmp */ 3, /* cpx */
	3, /* cpy */ 4, /* adn */ 4, /* orn */ 4, /* xrn */ 4, /* anc */ 4, /* snc */ 2, /* lnz */ 2, /* lnx */ 2, /* lny */
	2, /* cpx */ 2, /* cnx */ 2, /* cny */
};

static char *opcode_name[] = {
	"NOP",
	"INX",
	"DEX",
	"INY",
	"DEY",
	"SZR",
	"SZL",
	"SHR",
	"SHL", 
	"ROR", 
	"ROL", 
	"RZR", 
	"RZL", 
	"AND", 
	"LOR", 
	"XOR", 
	"INC", 
	"DEC", 
	"ADC", 
	"SBC",
	"LDZ", 
	"STZ",
	"LDX", 
	"STX", 
	"LDY", 
	"STY", 
	"ZRX", 
	"ZRY", 
	"PSH", 
	"POP", 
	"PSF", 
	"POF", 
	"JTS",
	"JMP", 
	"JEQ", 
	"JLE", 
	"JGR", 
	"JNE", 
	"JNL",
	"JNG",
	"JPV",
	"JNV", 
	"JPN",
	"JNN", 
	"JPC", 
	"JNC", 
	"RFS", 
	"RFI", 
	"TZY", 
	"TZX", 
	"TYZ", 
	"TYX", 
	"TXZ", 
	"TXY", 
	"SZY", 
	"SZX", 
	"SXY", 
	"SEC", 
	"SEV", 
	"CLC", 
	"CLV", 
	"CMP", 
	"CPX",
  "CPY",
	"ADN",
	"ORN", 
	"XRN", 
	"ANC", 
	"SNC", 
	"LNZ", 
	"LNX", 
	"LNY", 
	"CPN", 
	"CNX", 
	"CNY",
};

#define FLAG_GET(FLAG) ((f & (1 << FLAG)) != 0)
#define FLAG_SET(FLAG, VAL) do {\
	if (VAL) f |=  (1 << (FLAG));\
	else     f &= ~(1 << (FLAG));\
} while(0)

void
cpu_startup(void) {
	p = bus_read_word(p);
}

/*
 * 0x12 0x0a 0xff
 *
 * */

void
cpu_clock(void) {
	u16 addr;
	u16 word;
	u8  byte;
	u8  opcode = bus_read_byte(p);
	if (clock < cycles[opcode]) {
		printf("[%s] - x: %u, y: %u, z: %u, p: %x\n", opcode_name[opcode], x, y, z, p);
		clock = 0;
		p++;
		switch (opcode) {
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
			case SZR:
				FLAG_SET(C, z & 1);
				z >>= 1;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case SZL:
				FLAG_SET(C, z >> 7);
				z <<= 1;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case SHR:
				addr = bus_read_word(p);
				byte = bus_read_byte(addr);
				p += 2;
				FLAG_SET(C, byte & 1);
				byte >>= 1;
				FLAG_SET(Z, !byte);
				FLAG_SET(N, byte & 0x80);
				bus_write_byte(addr, byte);
				break;
			case SHL:
				addr = bus_read_word(p);
				byte = bus_read_byte(addr);
				p += 2;
				FLAG_SET(C, byte >> 7);
				byte <<= 1;
				FLAG_SET(Z, !byte);
				FLAG_SET(N, byte & 0x80);
				bus_write_byte(addr, byte);
				break;
			case ROR:
				addr = bus_read_word(p);
				byte = bus_read_byte(addr);
				byte = (byte >> 1) | ((byte & 1) << 7);
				p += 2;
				FLAG_SET(Z, !byte);
				FLAG_SET(N, byte & 0x80);
				bus_write_byte(addr, byte);
				break;
			case ROL:
				addr = bus_read_word(p);
				byte = bus_read_byte(addr);
				byte = (byte << 1) | (byte >> 7);
				p += 2;
				FLAG_SET(Z, !byte);
				FLAG_SET(N, byte & 0x80);
				bus_write_byte(addr, byte);
				break;
			case RZR:
				z = (z >> 1) | ((z & 1) << 7);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case RZL:
				z = (z << 1) | (z >> 7);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case AND:
				z &= bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case LOR:
				z |= bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case XOR:
				z ^= bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case INC:
				addr = bus_read_word(p);
				p += 2;
				byte = bus_read_byte(addr) + 1;
				FLAG_SET(Z, !byte);
				FLAG_SET(N, byte & 0x80);
				bus_write_byte(addr, byte);
				break;
			case DEC:
				addr = bus_read_word(p);
				p += 2;
				byte = bus_read_byte(addr) - 1;
				FLAG_SET(Z, !byte);
				FLAG_SET(N, byte & 0x80);
				bus_write_byte(addr, byte);
				break;
			case ADC:
				addr = bus_read_word(p);
				p += 2;
				byte = bus_read_byte(addr);
				word = z + byte + FLAG_GET(C);
				FLAG_SET(C, word & 0x100);
				FLAG_SET(Z, !(word & 0xff));
				FLAG_SET(N, (u8)(((z^(word & 0xff))&~(z^byte)) >> 7));
				z = (word & 0xff);
				break;
			case SBC:
				addr = bus_read_word(p);
				p += 2;
				byte = bus_read_byte(addr);
				word = z - byte - !FLAG_GET(C);
				FLAG_SET(C, word & 0x100);
				FLAG_SET(Z, !(word & 0xff));
				FLAG_SET(N, (u8)(((z^(word & 0xff))&~(z^byte)) >> 7));
				z = (word & 0xff);
				break;
			case LDZ: 
				z = bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case STZ:
				bus_write_byte(bus_read_word(p), z);
				p += 2;
				break;
			case LDX:
				x = bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, !x);
				FLAG_SET(N, x & 0x80);
				break;
			case STX:
				bus_write_byte(bus_read_word(p), x);
				p += 2;
				break;
			case LDY:
				y = bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, !y);
				FLAG_SET(N, y & 0x80);
				break;
			case STY:
				bus_write_byte(bus_read_word(p), y);
				p += 2;
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
				bus_write_byte(0x0800 + s--, z);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case POP:
				z = bus_read_byte(0x0800 + ++s);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case PSF:
				bus_write_byte(0x0800 + s--, f);
				break;
			case POF:
				f = bus_read_byte(0x0800 + ++f);
				break;
			case JTS:
				bus_write_byte(0x0800 + s--, (p + 2) >> 8);
				bus_write_byte(0x0800 + s--, (p + 2) & 0xff);
				p = bus_read_word(p);
				break;
			case JMP:
				p = bus_read_word(p);
				break;
			case JEQ:
				if (FLAG_GET(Z)) p = bus_read_word(p);
				else p += 2;
				break;
			case JLE:
				if (FLAG_GET(L)) p = bus_read_word(p);
				else p += 2;
				break;
			case JGR:
				if (!FLAG_GET(L) && !FLAG_GET(Z)) p = bus_read_word(p);
				else p += 2;
				break;
			case JNE:
				if (!FLAG_GET(Z)) p = bus_read_word(p);
				else p += 2;
				break;
			case JNL:
				if (!FLAG_GET(L)) p = bus_read_word(p);
				else p += 2;
				break;
			case JNG:
				if (FLAG_GET(L) || FLAG_GET(Z)) p = bus_read_word(p);
				else p += 2;
				break;
			case JPV:
				if (FLAG_GET(V)) p = bus_read_word(p);
				else p += 2;
				break;
			case JNV: 
				if (!FLAG_GET(V)) p = bus_read_word(p);
				else p += 2;
				break;
			case JPN:
				if (FLAG_GET(N)) p = bus_read_word(p);
				else p += 2;
				break;
			case JNN:
				if (!FLAG_GET(N)) p = bus_read_word(p);
				else p += 2;
				break;
			case JPC:
				if (FLAG_GET(C)) p = bus_read_word(p);
				else p += 2;
				break;
			case JNC:
				if (!FLAG_GET(C)) p = bus_read_word(p);
				else p += 2;
				break;
			case RFS:
				p  = bus_read_byte(0x0800 + ++s)  & 0x00ff;
				p |= bus_read_byte(0x0800 + ++s) << 8;
				break;
			case RFI:
				s  = bus_read_byte(0x0800 + ++s);
				p  = bus_read_byte(0x0800 + ++s)  & 0x00ff;
				p |= bus_read_byte(0x0800 + ++s) << 8;
				break;
			case TZY:
				y = z;
				break;
			case TZX:
				x = z;
				break;
			case TYZ:
				z = y;
				break;
			case TYX:
				x = y;
				break;
			case TXZ:
				z = x;
				break;
			case TXY:
				y = x;
				break;
			case SZY:
				z ^= y;
				y ^= z;
				z ^= y;
				break;
			case SZX:
				z ^= x;
				x ^= z;
				z ^= x;
				break;
			case SXY:
				x ^= y;
				y ^= x;
				x ^= y;
				break;
			case SEC:
				FLAG_SET(C, 1);
				break;
			case SEV:
				FLAG_SET(V, 1);
				break;
			case CLC:
				FLAG_SET(C, 0);
				break;
			case CLV:
				FLAG_SET(V, 0);
				break;
			case CMP:
				byte = z - bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, byte == 0);
				FLAG_SET(L, byte & 0x80);
				break;
			case CPX:
				byte = x - bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, byte == 0);
				FLAG_SET(L, byte & 0x80);
				break;
			case CPY:
				byte = y - bus_read_byte(bus_read_word(p));
				p += 2;
				FLAG_SET(Z, byte == 0);
				FLAG_SET(L, byte & 0x80);
				break;
			case ADN:
				z &= bus_read_byte(p++);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case ORN:
				z |= bus_read_byte(p++);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case XRN:
				z ^= bus_read_byte(p++);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case ANC:
				byte = bus_read_byte(p++);
				word = z + byte + FLAG_GET(C);
				FLAG_SET(C, word & 0x100);
				FLAG_SET(Z, !(word & 0xff));
				FLAG_SET(N, (u8)(((z^(word & 0xff))&~(z^byte)) >> 7));
				z = (word & 0xff);
				break;
			case SNC:
				byte = bus_read_byte(p++);
				word = z - byte - !FLAG_GET(C);
				FLAG_SET(C, word & 0x100);
				FLAG_SET(Z, !(word & 0xff));
				FLAG_SET(N, (u8)(((z^(word & 0xff))&~(z^byte)) >> 7));
				z = (word & 0xff);
				break;
			case LNZ: 
				z = bus_read_byte(p++);
				FLAG_SET(Z, !z);
				FLAG_SET(N, z & 0x80);
				break;
			case LNX: 
				x = bus_read_byte(p++);
				FLAG_SET(Z, !x);
				FLAG_SET(N, x & 0x80);
				break;
			case LNY: 
				y = bus_read_byte(p++);
				FLAG_SET(Z, !y);
				FLAG_SET(N, y & 0x80);
				break;
			case CPN:
				byte = z - bus_read_byte(p++);
				FLAG_SET(Z, byte == 0);
				FLAG_SET(L, byte & 0x80);
				break;
			case CNX:
				byte = x - bus_read_byte(p++);
				FLAG_SET(Z, byte == 0);
				FLAG_SET(L, byte & 0x80);
				break;
			case CNY:
				byte = y - bus_read_byte(p++);
				FLAG_SET(Z, byte == 0);
				FLAG_SET(L, byte & 0x80);
				break;
			default:
				break;
		}
	} else {
		clock++;
	}
}
