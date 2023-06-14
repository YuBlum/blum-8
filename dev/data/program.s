ldx <background_tiles    stx &17f6
ldx >background_tiles    stx &17f7
ldx <sprite_tiles        stx &17f8
ldx >sprite_tiles        stx &17f9
ldx <background_palettes stx &17fa
ldx >background_palettes stx &17fb
ldx <sprite_palettes     stx &17fc
ldx >sprite_palettes     stx &17fd

zrx
bg_pattern:
  zry
  txz and $01
  jne odd_col
  txz and $10
  jne end_pattern
  jmp change_pattern
odd_col:
  txz and $10
  jne change_pattern
  jmp end_pattern
change_pattern:
  iny
end_pattern:
  sty &1200,x
  sty &1300,x
  sty &1400,x
  sty &1500,x
  inx jne bg_pattern

main_loop:
  zry ldx $a0 dly
  ldx scroll_x inx stx scroll_x
  ldy scroll_y iny sty scroll_y
  jmp main_loop

background_palettes:
  $80 $07
sprite_palettes:
  $0f $d9

background_tiles:
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000

  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111

; --------------------
; --------------------

sprite_tiles:
  %00000000 %00101010
  %00000000 %10101010
  %00000100 %10111111
  %00000100 %11111111
  %00000101 %11011001
  %00000101 %11011101
  %00000001 %01010101
  %00000010 %10010111

  %00111111 %11110101
  %11110111 %11111110
  %11010101 %11110111
  %11110111 %11110110
  %11110111 %11110111
  %11111111 %11110110
  %00010101 %01011100
  %00000000 %11111100

  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
  %00000000 %00000000
