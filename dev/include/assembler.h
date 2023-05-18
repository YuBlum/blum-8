#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <types.h>

enum {
	TKN_CST,
	TKN_ADR,
	TKN_ZPG,
	TKN_INS,
	TKN_LBL,
};

void assembler_lex(const i8 *name);
void assembler_parse(void);

#endif/*__ASSEMBLER_H__*/
