#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <types.h>

enum {
	TKN_CST, /* constant */
	TKN_ADR, /* address */
	TKN_ADS, /* address sufix */
	TKN_INS, /* instruction */
	TKN_LBL, /* label */
	TKN_STR, /* string */
};

b8 assemble(const i8 *name);
void assembler_lex(const i8 *name);
void assembler_parse(void);

#endif/*__ASSEMBLER_H__*/
