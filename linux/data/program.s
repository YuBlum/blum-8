ldz %101
ldx $0a
stx player.y
jmp &.

entity: @strdef
	x: @byte,
	y: @byte,
	z: @byte,
; 
player: @str entity
