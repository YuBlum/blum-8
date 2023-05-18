reset
vblnk

reset:
	ldx %ff
loop:
	inx
	ldz msg,x
	jne loop
	jmp $.
vblnk:
	rti

msg: "Hello, World!" %00
