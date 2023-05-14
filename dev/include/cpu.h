#ifndef __CPU_H__
#define __CPU_H__

#include <types.h>

enum {
	NOP, INX, DEX, INY, DEY, ADD,
	SUB, LDZ, STZ, CPL, CTL, DRW,
	INTRUCTION_COUNT
};

#endif/*__CPU_H__*/
