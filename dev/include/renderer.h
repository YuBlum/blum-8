#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <types.h>

void renderer_begin(void);
void renderer_pixel_set(u32 x, u32 y, u8 color);
void renderer_update_screen(void);
void renderer_update(void);
void renderer_end(void);

#endif
