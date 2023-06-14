ldx <palettes stx &17fa
ldx >palettes stx &17fb
ldx <background_tiles stx &17f6
ldx >background_tiles stx &17f7

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

palettes:
  $80 $07

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
