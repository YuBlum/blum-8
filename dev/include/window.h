#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <types.h>

b8   window_open(void);
b8   window_key_get(u32 key);
void window_loop(void);
void window_close(void);

#endif/*__WINDOW_H__*/
