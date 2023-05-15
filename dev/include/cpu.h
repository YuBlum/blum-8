#ifndef __CPU_H__
#define __CPU_H__

#include <types.h>

enum {
	NOP, INX, DEX, INY, DEY, SZR, SZL, SHR, SHL, ROR, ROL, RZR, RZL, AND, LOR, XOR, INC, DEC, ADC, SBC, LDZ, 
	STZ, LDX, STX, LDY, STY, ZRX, ZRY, PSH, POP, PSF, POF, JTS, JMP, JEQ, JLE, JGR, JNE, JNL, JNG, JPV, JNV, 
	JPN, JNN, JPC, JNC, RFS, RFI, TZY, TZX, TYZ, TYX, TXZ, TXY, SZY, SZX, SXY, SEC, SEV, CLC, CLV, CMP, CPX,
  CPY, ADN, ORN, XRN, ANC, SNC, LNZ, LNX, LNY, CPN, CNX, CNY,
	INSTRUCTION_COUNT
};

void cpu_startup(void);
void cpu_clock(void);

#endif/*__CPU_H__*/
