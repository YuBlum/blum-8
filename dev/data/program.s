reset

div: ; x / y -> x: result, y: remainder
	ldz $60 psh  ; pushes previous stuff of $60 into the stack
	cpy %00      ; compare y(divisor) with zero
	jeq div_end  ; if the divisor is zero jump to the end
	txz          ; transfers x(dividend) to z
	sty $60      ; stores y(divisor) on $(60)
	zrx          ; zero out x
	cmp %00      ; compare z(dividend) with zero
div_loop:
	jeq div_end  ; if x is divisible by y jumps to div_end
	inx          ; increament x
	sbc $60      ; subtract z by $60
	jpv div_rmd  ; if x isn't divisible by y jumps to div_rmd
	jmp div_loop ; jumps to div_loop
div_rmd:
	adc $60      ; add $60 to z to get the remainder
	dex          ; decreament x to get the result
div_end:
	tzy          ; transfers z(remainder) to y
	pop stz $60  ; stores back the stuff of $60
	ret          ; returns to the main program

reset:
	ldx %0f
loop:
	txz psh
	ldy %03
	jts div
	sty $00
	pop tzx psh
	ldy %05
	jts div
	cpy %00
	jne div_03
div_35:
	ldy $00
	cpy %00
	jne div_05
	ldy %35
	jmp loop_end
div_03:
	ldy $00
	cpy %00
	jne div_00
	ldy %03
	jmp loop_end
div_05:
	ldy %05
	jmp loop_end
div_00:
	ldy %00
loop_end:
	pop tzx dex
	jne loop
; lock program
	jmp $.
