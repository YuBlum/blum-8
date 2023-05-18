	reset
	vblnk
	mul:
		ldz %00
		sty $69
	mul_loop:
		adc $69
		dex
		jne mul_loop
		ret
	reset:
		ldx %03
		ldy %05
		jts mul
		stz $0420
		ldx %02
		ldy %06
		jts mul
		adc $0420
		jmp $.
	vblnk:
		rti
