reset
vblnk

reset:
	ldx %02
	ldz msg,xu
vblnk:
	rti

msg: "Hello, World!" %00
