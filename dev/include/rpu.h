#ifndef __RPU_H__
#define __RPU_H__

#include <types.h>

#define RPU_TILES_BACKGROUND 0x21f2
#define RPU_TILES_FOREGROUND 0x29f2
#define RPU_SPRITES          0x31f2
#define RPU_BACKGROUND_TL    0x32f2
#define RPU_BACKGROUND_TR    0x33f2
#define RPU_BACKGROUND_BL    0x34f2
#define RPU_BACKGROUND_BR    0x35f2
#define RPU_BACKGROUND_ATTR  0x36f2
#define RPU_BACKGROUND_PAL   0x37f2
#define RPU_SPRITES_PAL      0x37f6
#define RPU_SCROLL_X         0x37fa
#define RPU_SCROLL_Y         0x37fb

#define RPU_TILES_BACKGROUND_SIZE 2048
#define RPU_TILES_FOREGROUND_SIZE 2048
#define RPU_SPRITES_SIZE          256
#define RPU_BACKGROUND_TL_SIZE    256
#define RPU_BACKGROUND_TR_SIZE    256
#define RPU_BACKGROUND_BL_SIZE    256
#define RPU_BACKGROUND_BR_SIZE    256
#define RPU_BACKGROUND_ATTR_SIZE  256
#define RPU_BACKGROUND_PAL_SIZE   4
#define RPU_SPRITES_PAL_SIZE      4
#define RPU_SCROLL_X_SIZE         1
#define RPU_SCROLL_Y_SIZE         1

void rpu_startup(void);

#endif/*__RPU_H__*/
