#ifndef __CRT_H__
#define __CRT_H__

#include <types.h>

void crt_begin(void);
void crt_electron_gun_shoot(u32 rgb);
void crt_update(void);
void crt_end(void);

#endif
