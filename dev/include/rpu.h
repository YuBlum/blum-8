#ifndef __RPU_H__
#define __RPU_H__

#include <types.h>

#define RPU_TILES_BACKGROUND 0x21ee
#define RPU_TILES_FOREGROUND 0x29ee
#define RPU_SPRITES          0x31ee
#define RPU_BACKGROUND_TL    0x32ee
#define RPU_BACKGROUND_TR    0x33ee
#define RPU_BACKGROUND_BL    0x34ee
#define RPU_BACKGROUND_BR    0x35ee
#define RPU_BACKGROUND_ATTR  0x36ee
#define RPU_BACKGROUND_PAL   0x37ee
#define RPU_SPRITES_PAL      0x37f6
#define RPU_SCROLL_X         0x37fe
#define RPU_SCROLL_Y         0x37ff

#define RPU_TILES_BACKGROUND_SIZE 2048
#define RPU_TILES_FOREGROUND_SIZE 2048
#define RPU_SPRITES_SIZE          256
#define RPU_BACKGROUND_TL_SIZE    256
#define RPU_BACKGROUND_TR_SIZE    256
#define RPU_BACKGROUND_BL_SIZE    256
#define RPU_BACKGROUND_BR_SIZE    256
#define RPU_BACKGROUND_ATTR_SIZE  256
#define RPU_BACKGROUND_PAL_SIZE   8
#define RPU_SPRITES_PAL_SIZE      8
#define RPU_SCROLL_X_SIZE         1
#define RPU_SCROLL_Y_SIZE         1

void rpu_startup(void);
void rpu_update(void);

#endif/*__RPU_H__*/
