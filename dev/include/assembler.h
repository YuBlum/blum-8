#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__

#include <types.h>

b8 assemble(const i8 *name);
void assembler_lex(const i8 *name);
void assembler_parse(void);

#endif/*__ASSEMBLER_H__*/
