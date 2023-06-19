#include "glfw3.h"
#include <window.h>
#include <math.h>
#include <os.h>
#include <cpu.h>
#include <bus.h>
#include <renderer.h>
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
static struct {
  u8 val;
  u8 x      : 7;
  u8 y      : 7;
  u8 unused : 2;
} screen_pixel;

union attributes {
  u8 val;
  struct {
    u8 palette         : 2;
    u8 not_transparent : 1;
    u8 flip_x          : 1;
    u8 flip_y          : 1;
    u8 rot90           : 1;
    u8 below           : 1;
    u8 enabled         : 1;
  };
};

static u32 colors[] = {
  0x1a1717, 0x8d8383, 0xd3c1c1, 0xede5e5, /*00:#1a1717 01:#8d8383 02:#d3c1c1 03:#ede5e5*/
  0x92afed, 0x7a84d3, 0x7958c1, 0x6c2c9e, /*04:#92afed 05:#7a84d3 06:#7958c1 07:#6c2c9e*/
  0x8d1e1e, 0xc15133, 0xe588a6, 0xe5c1d4, /*08:#8d1e1e 09:#c15133 10:#e588a6 11:#e5c1d4*/
  0x27953f, 0x4bb93e, 0xa2d34e, 0xfaff6b, /*12:#27953f 13:#4bb93e 14:#a2d34e 15:#faff6b*/
};

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
  b8 valid;
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

static i8 *addrmd_name[] = {
  "noa", "cst", "zpg", "zpx",
  "zpy", "adr", "adx", "ady"
};

static struct instruction inst[0x100];

#define FLAG_GET(FLAG) ((f >> FLAG) & 1)
#define FLAG_SET(FLAG, VAL) do {\
  if (VAL) f |=  (1 << (FLAG));\
  else     f &= ~(1 << (FLAG));\
} while(0)

void
cpu_startup(void) {
  s = 0xff;
  b8 carry = 0;
  for (union byte i = { 0 }; i.b || !carry; carry = (u8)(++i.b) == 0) {
    if      (i.h == 0x0 || i.h == 0x1) inst[i.b].addrmd = NOA;
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
          inst[i.b].opcode = i.l + TYZ;
        }
        break;
      case CST:
        inst[i.b].opcode = i.l + AND;
        if      (i.l == 0x3) inst[i.b].opcode = ADC;
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
          if      (i.l == 0x0) inst[i.b].opcode = STY;
          else if (i.l == 0x1) inst[i.b].opcode = CMP;
          else if (i.l == 0x2) inst[i.b].opcode = CPX;
          else if (i.l == 0x3) inst[i.b].opcode = CPY;
        }
        break;
      case ADR:
        inst[i.b].opcode = (i.b - 0xd0 + SHR) % OPCODE_COUNT;
        break;
    }
    inst[i.b].valid = i.b != 0x1f && (i.b <= 0x2a || i.b >= 0x30)
                                  && (i.b <= 0x43 || i.b >= 0x50)
                                  && (i.b <= 0x63 || i.b >= 0x70)
                                  && (i.b <= 0x83 || i.b >= 0x90)
                                  && (i.b <= 0xa3 || i.b >= 0xb0)
                                  && (i.b <= 0xc3 || i.b >= 0xd0)
                                  && (i.b <= 0xc3 || i.b >= 0xd0)
                                  && i.b <= 0xf1;
    /*printf("%.2x %s %s\n", i.b, opcode_name[inst[i.b].opcode], addrmd_name[inst[i.b].addrmd]);*/
  }
}

void
cpu_print_registers(void) {
  printf("x: %.2x, y: %.2x, z: %.2x\n", x, y, z);
  printf("Flags:\n");
  printf("\tCarry: %u, Zero: %u, Less: %u, Negative: %u, Overflow: %u, Delay: %u\n",
    FLAG_GET(C), FLAG_GET(Z), FLAG_GET(L), FLAG_GET(N), FLAG_GET(V), FLAG_GET(D));
  printf("Addresses:\n");
  printf("\tStack: %.2x, Pointer: %.4x\n", s, p);
  u8 idx = bus_read_byte(p);
  printf("Instruction: %s ", opcode_name[inst[idx].opcode]);
  switch(inst[idx].addrmd) {
    case NOA: printf("\n"); break;
    case CST: printf("$%.2x\n", bus_read_byte(p+1)); break;
    case ZPG: printf("&%.2x\n", bus_read_byte(p+1)); break;
    case ADR: printf("&%.4x\n", bus_read_word(p+1)); break;
    case ZPX: printf("&%.2x\n", bus_read_byte(p+1)+x); break;
    case ZPY: printf("&%.2x\n", bus_read_byte(p+1)+y); break;
    case ADX: printf("&%.4x\n", bus_read_word(p+1)+x); break;
    case ADY: printf("&%.4x\n", bus_read_word(p+1)+y); break;
  }
  printf("Stack[0]: %.2x\n", bus_read_byte(STACK_END + 0xff));
  printf("Stack[1]: %.2x\n", bus_read_byte(STACK_END + 0xfe));
  printf("Stack[2]: %.2x\n", bus_read_byte(STACK_END + 0xfd));
  printf("Stack[3]: %.2x\n", bus_read_byte(STACK_END + 0xfc));
  printf("Stack[4]: %.2x\n", bus_read_byte(STACK_END + 0xfb));
}

void
cpu_disassemble(void) {
  return;
  static b8 test = 0;
  test = test || p >= 0x1a59;
  if (!test) return;
	u8 idx = bus_read_byte(p);
  printf("clock: %u\n", clock);
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
  getchar();
}

void
cpu_start_interrupt(void) {
  p = bus_read_word(START_VEC);
}

void
cpu_frame_interrupt(void) {
  bus_write_byte(STACK_END + s--, p >> 8, 1);
  bus_write_byte(STACK_END + s--, p & 0x00ff, 1);
  bus_write_byte(STACK_END + s--, f, 1);
  bus_write_byte(STACK_END + s--, x, 1);
  bus_write_byte(STACK_END + s--, y, 1);
  p = bus_read_word(FRAME_VEC);
  f = 0;
}

void
cpu_tick(void) {
  bus_write_byte(IN_OUT,
    window_key_get(GLFW_KEY_A) << 7 |
    window_key_get(GLFW_KEY_D) << 6 |
    window_key_get(GLFW_KEY_W) << 5 |
    window_key_get(GLFW_KEY_S) << 4 |
    window_key_get(GLFW_KEY_J) << 3 |
    window_key_get(GLFW_KEY_K) << 2 |
    window_key_get(GLFW_KEY_U) << 1 |
    window_key_get(GLFW_KEY_I) << 0
  , 1);
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
      bus_write_byte(STACK_END + s--, z, 1);
      break;
    case POP:
      z = bus_read_byte(STACK_END + ++s);
      FLAG_SET(Z, !z);
      FLAG_SET(N, z & 0x80);
      clock++;
      break;
    case PSF: bus_write_byte(STACK_END + s--, f, 1); clock += 1; break;
    case POF: f = bus_read_byte(STACK_END + ++f); clock += 2; break;
    case RET:
      p  = bus_read_byte(STACK_END + ++s)  & 0x00ff;
      p |= bus_read_byte(STACK_END + ++s) << 8;
      clock += 3;
      break;
    case EXT:
      y  = bus_read_byte(STACK_END + ++s);
      x  = bus_read_byte(STACK_END + ++s);
      f  = bus_read_byte(STACK_END + ++s);
      p  = bus_read_byte(STACK_END + ++s)  & 0x00ff;
      p |= bus_read_byte(STACK_END + ++s) << 8;
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
      bus_write_byte(addr, byte, 1);
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
      bus_write_byte(addr, byte, 1);
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
      bus_write_byte(addr, byte, 1);
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
      bus_write_byte(addr, byte, 1);
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
      bus_write_byte(addr, byte, 1);
      break;
    case DEC:
      byte = bus_read_byte(addr) - 1;
      FLAG_SET(Z, !byte);
      FLAG_SET(N, byte & 0x80);
      bus_write_byte(addr, byte, 1);
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
      bus_write_byte(addr, z, 1);
      break;
    case LDX:
      x = bus_read_byte(addr);
      FLAG_SET(Z, !x);
      FLAG_SET(N, x & 0x80);
      break;
    case STX:
      bus_write_byte(addr, x, 1);
      break;
    case LDY:
      y = bus_read_byte(addr);
      FLAG_SET(Z, !y);
      FLAG_SET(N, y & 0x80);
      break;
    case STY:
      bus_write_byte(addr, y, 1);
      break;
    case JTS:
      bus_write_byte(STACK_END + s--, p >> 8, 1);
      bus_write_byte(STACK_END + s--, p & 0xff, 1);
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

static b8
sprite_render(b8 below) {
  b8 result = 0;
  for (i32 i = 63; i >= 0; i--) {
    union attributes attr = { bus_read_byte(SPR_ATTR + i * 4 + 3) };
    if (!attr.enabled || (below && !attr.below) || (!below && attr.below)) continue;
    i8 pos_x = bus_read_byte(SPR_ATTR + i * 4 + 0);
    i8 pos_y = bus_read_byte(SPR_ATTR + i * 4 + 1);
    if ((screen_pixel.x < pos_x || screen_pixel.x >= pos_x + 8)
     || (screen_pixel.y < pos_y || screen_pixel.y >= pos_y + 8)) continue;
    u16 tile = bus_read_word(SPR_TILES) + bus_read_byte(SPR_ATTR + i * 4 + 2) * 16;
    u8 pixel_x = (screen_pixel.x - pos_x);
    u8 pixel_y = (screen_pixel.y - pos_y);
    if (attr.flip_x) pixel_x = 7 - pixel_x;
    if (attr.flip_y) pixel_y = 7 - pixel_y;
    if (attr.rot90) {
      pixel_x ^= pixel_y;
      pixel_y ^= pixel_x;
      pixel_x ^= pixel_y;
    }
    u8 pixel = (bus_read_byte(tile + pixel_y * 2 + (pixel_x > 3)) >> ((3 - (pixel_x % 4)) * 2)) & 0b11;
    if (attr.not_transparent || pixel != 0) {
      u8 color = (bus_read_byte(bus_read_word(SPR_PALS) + (attr.palette * 2) + (pixel > 1)) >> ((1 - (pixel % 2)) * 4)) & 0b1111;
      crt_display_pixel(colors[color], screen_pixel.x, screen_pixel.y);
      result = 1;
    } else if (below) {
      u8 color = (bus_read_byte(bus_read_word(BG_PALS)) >> 4) & 0b1111;
      crt_display_pixel(colors[color], screen_pixel.x, screen_pixel.y);
    }
  }
  return result;
}

void
cpu_rsu_tick(void) {
  /* Screen Rendering Unit (SRU) */
  /* background tiles */
  i8 scroll_x = bus_read_byte(SCROLL_X);
  i8 scroll_y = bus_read_byte(SCROLL_Y);
  b8 have_pixel = sprite_render(1);
  u8 tile_x = (u8)(screen_pixel.x + scroll_x) / 8;
  u8 tile_y = (u8)(screen_pixel.y + scroll_y) / 8;
  union attributes attr = { bus_read_byte(BG_ATTR  + ((tile_y % 16) * 16 + (tile_x % 16))) };
  u16 screen = SCREEN00 + ((tile_y > 15) * 0x200 + (tile_x > 15) * 0x100);
  u16 tile  = bus_read_word(BG_TILES) + (bus_read_byte(screen + ((tile_y % 16) * 16 + (tile_x % 16))) * 16);
  // 0 7 
  // 32 bytes -> 8 bytes
  u8 pixel_x = (u8)(screen_pixel.x + scroll_x) % 8;
  u8 pixel_y = (u8)(screen_pixel.y + scroll_y) % 8;
  if (attr.flip_x) pixel_x = 7 - pixel_x;
  if (attr.flip_y) pixel_y = 7 - pixel_y;
  if (attr.rot90) {
    pixel_x ^= pixel_y;
    pixel_y ^= pixel_x;
    pixel_x ^= pixel_y;
  }
  u8  pixel = (bus_read_byte(tile + pixel_y * 2 + (pixel_x > 3)) >> ((3 - (pixel_x % 4)) * 2)) & 0b11;
  if (attr.not_transparent || pixel != 0) {
    u8 color = (bus_read_byte(bus_read_word(BG_PALS) + (attr.palette * 2) + (pixel > 1)) >> ((1 - (pixel % 2)) * 4)) & 0b1111;
    crt_display_pixel(colors[color], screen_pixel.x, screen_pixel.y);
  } else if (!have_pixel) {
    u8 color = (bus_read_byte(bus_read_word(BG_PALS)) >> 4) & 0b1111;
    crt_display_pixel(colors[color], screen_pixel.x, screen_pixel.y);
  }
  sprite_render(0);
  /* go to next pixel */
  screen_pixel.x++;
  if (screen_pixel.x == 0) {
    screen_pixel.y++;
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

i8 *
cpu_opcode_str(enum opcode opcode) {
  return opcode_name[opcode];
}

i8 *
cpu_addrmd_str(enum addrmd addrmd) {
  return addrmd_name[addrmd];
}

u8
cpu_instruction_get(enum opcode opcode, enum addrmd addrmd) {
  for (u32 i = 0; i < 0x100; i++) {
    if (!inst[i].valid) continue;
    if (inst[i].opcode == opcode && inst[i].addrmd == addrmd) return i;
  }
  return -1;
}
