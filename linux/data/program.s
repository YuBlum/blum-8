jmp start

render: ; x: tile index, y: tile attr pos, z: tile attr
  stx &1200,y ; store x into &1200 + y
  stz &1600,y ; store z into &1600 + y
  ret         ; return to caller subroutine

cls:
  txz psh tyz psh  ; save state
  zrx zry          ; zero out x and y
cls_loop:          ; clear screen loop
  sty &1600,x      ; store y into &1600 + x
  sty &1200,x      ; store y into &1200 + x
  inx jne cls_loop ; check if has done clearing the screen
  pop tzy pop tzx  ; reset state
  ret              ; return to the caller subroutine

start:
; set the 'palettes' label to be the palettes address
  ldx <palettes
  stx &17fa
  ldx >palettes
  stx &17fb
; set the 'tiles' label to be the tiles address
  ldx <tiles
  stx &17f6
  ldx >tiles
  stx &17f7

ldy $00 link_loop:   ; loop for drawing the link, sets y (counter variable) to 0
  jts cls            ; clear the screen
  ldx $1             ; load x with the tile index 1 
  stx &1270,y        ; store the index into the correct address
  stx &1271,y        ; store the index into the correct address
  ldx %00000001      ; load x with an attribute value
  stx &1670,y        ; store the attribute in the correct address
  stx &1680,y        ; store the attribute in the correct address
  stx &1681,y        ; store the attribute in the correct address
  ldx %00001001      ; load x with an attribute value
  stx &1671,y        ; store the attribute in the correct address
  ldx $2 stx &1280,y ; store the tile '2' into the address &1280+y
  ldx $3 stx &1281,y ; store the tile '3' into the address &1281+y
  ldx $cc tyz zry    ; prepare for delay
  dly dly tzy        ; delay for 408 clock cycles
  iny cpy $0f        ; incremeant y
  jne link_loop      ; checks if is the last position

  ldx $4                           ; arrow tile index
  ldz %00110001 ldy $56 jts render ; draw arrow in position (6,5) with flip_y and rot90
  ldz %00001001 ldy $46 jts render ; draw arrow in position (6,4) with flip_x
  ldz %00001001 ldy $45 jts render ; draw arrow in position (4,4) with flip_x
  ldz %00100001 ldy $44 jts render ; draw arrow in position (4,5) with rot90
  ldz %00100001 ldy $54 jts render ; draw arrow in position (4,5) with rot90
  ldz %00000001 ldy $64 jts render ; draw arrow in position (4,6)
  ldz %00000001 ldy $65 jts render ; draw arrow in position (6,6)
  ldz %00110001 ldy $66 jts render ; draw arrow in position (6,6) with flip_y and rot90

  jmp &. ; infinite loop


palettes:
$60 $00
$1f $8d

tiles:
%00000000 %00000000
%00000000 %00000000
%00000000 %00000000
%00000000 %00000000
%00000000 %00000000
%00000000 %00000000
%00000000 %00000000
%00000000 %00000000

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

%00000000 %11000000
%00000000 %11110000
%11111111 %11111100
%11111111 %11111111
%11111111 %11111100
%00000000 %11110000
%00000000 %11000000
%00000000 %00000000
