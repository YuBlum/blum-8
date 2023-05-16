#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <types.h>

enum {
	TKN_INST,
	TKN_NUM,
	TKN_ADDR,
};

void assembler_lex(const i8 *name);

#endif/*__ASSEMBLER_H__*/
