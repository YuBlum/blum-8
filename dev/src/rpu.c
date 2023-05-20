#include <rpu.h>
#include <bus.h>
#include <cpu.h>
#include <math.h>
#include <stdio.h>
#include <renderer.h>

static u8 scroll_x;
static u8 scroll_y;

#define TILE_HEIGHT_BYTES 8
#define TILE_WIDTH_BYTES  2
#define TILE_WIDTH_PIXELS 8

void
rpu_startup(void) {
}

static void
rpu_bg_tile_draw(u8 x, u8 y) {
	u8 idx  = y * 16 + x;
	u8 tile = bus_read_byte(RPU_BACKGROUND_TL + idx);
	for (u8 i = 0; i < TILE_HEIGHT_BYTES; i++) {
		for (u8 j = 0; j < TILE_WIDTH_BYTES; j++) {
			for (u8 k = 0; k < TILE_WIDTH_PIXELS; k += 2) {
				u8 attribs = bus_read_byte(RPU_BACKGROUND_ATTR + idx);
				u8 inv_x   = attribs & 0x8;
				u8 pixel_x = x * 8 + (inv_x ? !j : j) * 4 + ((inv_x ? k : 6 - k) >> 1);
				u8 pixel_y = y * 8 + i;
				u8 pixel   = bus_read_byte(RPU_TILES_BACKGROUND + (tile * 16) + i * 2 + j) >> k & 0x03;
				u8 palette = attribs & 0x3;
				u8 trans   = attribs & 0x4;
				u8 color   = bus_read_word(RPU_BACKGROUND_PAL + palette * 2) >> (pixel * 4) & 0x0f;
				if (pixel != 0 || !trans) renderer_pixel_set(pixel_x, pixel_y, color);
			}
		}
	}
}

void
rpu_update(void) {
	scroll_x = bus_read_byte(RPU_SCROLL_X);
	scroll_y = bus_read_byte(RPU_SCROLL_Y);
	for (u8 i = 0; i < 16; i++) {
		for (u8 j = 0; j < 16; j++) {
			rpu_bg_tile_draw(j, i);
		}
	}
	renderer_update_screen();
}
