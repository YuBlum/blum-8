#ifndef __CRT_H__
#define __CRT_H__

#include <types.h>
#include <os.h>

void crt_display_pixel(u32 rgb, u32 x, u32 y);
b8   renderer_begin(const struct glfw *glfw);
void renderer_update(void);
void renderer_end(void);

#endif
