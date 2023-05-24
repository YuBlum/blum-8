#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <types.h>

enum {
	TKN_CST, /* constant */
	TKN_ADR, /* address */
	TKN_ADS, /* address sufix */
	TKN_INS, /* instruction */
	TKN_LBL, /* label */
	TKN_MLB, /* most significant byte of label */
	TKN_LLB, /* least significant byte of label */
	TKN_STR, /* string */
};

enum lbltyp {
	LTP_NORMAL,
	LTP_MOST,
	LTP_LEAST,
};

b8 assemble(const i8 *name);
void assembler_lex(const i8 *name);
void assembler_parse(void);

#endif/*__ASSEMBLER_H__*/
