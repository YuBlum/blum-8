jmp start

render: ; x: tile index, y: tile attr pos, z: tile attr
  stx &1200,y ; store x into &1200 + y
  stz &1600,y ; store z into &1600 + y
  ret         ; return to caller subroutine

start:
; set the 'palettes' label to be the background palettes address
  ldx <palettes
  stx &17fa
  ldx >palettes
  stx &17fb
; set the 'palettes' label to be the sprite palettes address
  ldx <palettes
  stx &17fc
  ldx >palettes
  stx &17fd
; set the bgtiles to be the background tiles address
  ldx <bgtiles
  stx &17f6
  ldx >bgtiles
  stx &17f7
; set the sprtiles to be the sprite tiles address
  ldx <sprtiles
  stx &17f8
  ldx >sprtiles
  stx &17f9

  ldx $1                           ; arrow tile index
  ldz %00110001 ldy $56 jts render ; draw arrow in position (6,5) with flip_y and rot90
  ldz %00001001 ldy $46 jts render ; draw arrow in position (6,4) with flip_x
  ldz %00001001 ldy $45 jts render ; draw arrow in position (4,4) with flip_x
  ldz %00100001 ldy $44 jts render ; draw arrow in position (4,5) with rot90
  ldz %00100001 ldy $54 jts render ; draw arrow in position (4,5) with rot90
  ldz %00000001 ldy $64 jts render ; draw arrow in position (4,6)
  ldz %00000001 ldy $65 jts render ; draw arrow in position (6,6)
  ldz %00110001 ldy $66 jts render ; draw arrow in position (6,6) with flip_y and rot90

; top left
  ldx $2
  ldy $23
  stx &1100
  sty &1101
  ldz $0
  stz &1102
  ldz %11000000
  stz &1103
; top right
  ldx $0a
  ldy $23
  stx &1104
  sty &1105
  ldz $0
  stz &1106
  ldz %11001000
  stz &1107
; bot left
  ldx $2
  ldy $2b
  stx &1108
  sty &1109
  ldz $1
  stz &110a
  ldz %11000000
  stz &110b
; bot right
  ldx $0a
  ldy $2b
  stx &110c
  sty &110d
  ldz $2
  stz &110e
  ldz %11000000
  stz &110f

loop:
  ldx $ff ldy $ff
  dly dly
  ldx &1100 inx stx &1100
  ldx &1104 inx stx &1104
  ldx &1108 inx stx &1108
  ldx &110c inx stx &110c
  jmp loop

  jmp &. ; infinite loop


palettes:
  $0f $8d
  $0f $84

bgtiles:
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000

  %00000000 %11000000
  %00000000 %11110000
  %11111111 %11111100
  %11111111 %11111111
  %11111111 %11111100
  %00000000 %11110000
  %00000000 %11000000
  %00000000 %00000000

sprtiles:
  %00000000 %00111111
  %00000000 %11111111
  %00000100 %11101010
  %00000100 %10101010
  %00000101 %10011101
  %00000101 %10011001
  %00000001 %01010101
  %00000011 %11010110

  %00101010 %10100101
  %10100110 %10101011
  %10010101 %10100110
  %10100110 %10100111
  %10100110 %10100110
  %10101010 %10100111
  %00010101 %01011000
  %00000000 %10101000

  %01011111 %10101000
  %11111111 %01101000
  %10111101 %01011000
  %10101010 %01010100
  %10111111 %11010000
  %11111111 %00000000
  %00101010 %00000000
  %00000000 %00000000
