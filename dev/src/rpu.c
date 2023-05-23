#include <rpu.h>
#include <bus.h>
#include <cpu.h>
#include <crt.h>
#include <math.h>
#include <stdio.h>
#include <config.h>

static i8 scroll_x;
static i8 scroll_y;

#define TILE_HEIGHT_BYTES 8
#define TILE_WIDTH_BYTES  2
#define TILE_WIDTH_PIXELS 8

static u8 memory[0x4000];

static u32 colors[] = {
	0x1a1717, 0x8d8383, 0xd3c1c1, 0xede5e5, /*00:#1a1717 01:#8d8383 02:#d3c1c1 03:#ede5e5*/
	0x92afed, 0x7a84d3, 0x7958c1, 0x6c2c9e, /*04:#92afed 05:#7a84d3 06:#7958c1 07:#6c2c9e*/
	0x8d1e1e, 0xc15133, 0xe588a6, 0xe5c1d4, /*08:#8d1e1e 09:#c15133 10:#e588a6 11:#e5c1d4*/
	0x27953f, 0x4bb93e, 0xa2d34e, 0xfaff6b, /*12:#27953f 13:#4bb93e 14:#a2d34e 15:#faff6b*/
};

static void
rpu_bg_tile_draw(u8 x, u8 y) {
	u8 tile = bus_read_byte(SCREEN00 + y * 32 + x);
	for (u8 i = 0; i < TILE_HEIGHT_BYTES; i++) {
		for (u8 j = 0; j < TILE_WIDTH_BYTES; j++) {
			for (u8 k = 0; k < TILE_WIDTH_PIXELS; k += 2) {
				u8 attribs = bus_read_byte(BG_ATTR + (y % 0x80) * 16 + (x % 0x80));
				u8 inv_x   = attribs & 0x8;
				u8 pixel_x = x * 8 + (inv_x ? !j : j) * 4 + ((inv_x ? k : 6 - k) >> 1);
				u8 pixel_y = y * 8 + i;
				if (pixel_x < scroll_x || pixel_x >= (scroll_x + GAME_SIZE) % 0x100 || pixel_y < scroll_x || (pixel_y >= GAME_SIZE) % 0x100) continue;
				u8 pixel   = bus_read_byte(BG_TILES + (tile * 16) + i * 2 + j) >> k & 0x03;
				u8 trans   = attribs & 0x4;
				if (pixel == 0 && trans) continue;
				u8 palette = attribs & 0x3;
				memory[(pixel_y - scroll_y) * GAME_SIZE + (pixel_x - scroll_x)] = bus_read_word(BG_PALS + palette * 2) >> (pixel * 4) & 0x0f;
			}
		}
	}
}

void
rpu_tick(void) {
	scroll_x = bus_read_byte(SCROLL_X);
	scroll_y = bus_read_byte(SCROLL_Y);
	//printf("a %u\n", scroll_x);
	for (u8 i = 0; i < 32; i++) {
		for (u8 j = 0; j < 32; j++) {
			rpu_bg_tile_draw(j, i);
		}
	}
	for (u8 i = 0; i < GAME_SIZE; i++) {
		for (u8 j = 0; j < GAME_SIZE; j++) {
			crt_electron_gun_shoot(colors[memory[i * GAME_SIZE + j]]);
		}
	}
}
