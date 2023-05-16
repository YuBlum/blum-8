$200a
$2021

; multiply function
ldz 00
sty $69
adc $69
dex
jne $2003
ret

ldx 03
ldy 05
jts $1fff
stz $0420
jmp $201e

rti
