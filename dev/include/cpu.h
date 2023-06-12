#ifndef __CPU_H__
#define __CPU_H__

#include <types.h>

enum opcode {
	NOP, INX, DEX, INY, DEY, ZRX, ZRY, PSH, POP,
	PSF, POF, RET, EXT, DLY, TZY, TZX, TYZ, TYX,
	TXZ, TXY, SZY, SZX, SXY, SEC, SEV, CLC, CLV,
	SHR, SHL, ROR, ROL, AND, LOR, XOR, INC, DEC,
	ADC, SBC, LDZ, STZ, LDX, STX, LDY, STY, JTS,
	JMP, JEQ, JLE, JGR, JNE, JNL, JNG, JPV, JNV,
	JPN, JNN, JPC, JNC, CMP, CPX, CPY,
	OPCODE_COUNT
};

enum addrmd {
	NOA, CST, ZPG, ZPX,
	ZPY, ADR, ADX, ADY,
};

struct cpuarg {
	b8 run;
	f64 (*time)(void);
};

void cpu_print_registers(void);
void cpu_disassemble(void);
void cpu_startup(void);
void cpu_tick(void);
void cpu_rsu_tick(void);
i8  *cpu_opcode_str(enum opcode opcode);
i32  cpu_opcode_get(const i8 *str, u32 str_size);
u8   cpu_instruction_get(enum opcode opcode, enum addrmd addrmd);

#endif/*__CPU_H__*/
