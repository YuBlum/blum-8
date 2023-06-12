#ifndef __CRT_H__
#define __CRT_H__

#include <types.h>
#include <os.h>

b8   crt_begin(const struct glfw *glfw);
void crt_display_pixel(u32 rgb, u32 x, u32 y);
void crt_update(void);
void crt_end(void);

#endif
