$380b
$3822

; multiply function
ldz %00
sty $69
adc $69
dex
jne $3804
ret

ldx %03
ldy %05
jts $3800
stz $0420
ldx %02
ldy %06
jts $3800
adc $0420
jmp $381f

rti
