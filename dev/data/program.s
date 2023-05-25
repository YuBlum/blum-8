ldx $0a
stx player.z
jmp &.

entity: @strdef
	x: @byte,
	y: @byte,
	z: @byte

player: @str entity
