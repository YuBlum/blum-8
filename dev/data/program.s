sprite: @strdef
  x:    @byte,
  y:    @byte,
  tile: @byte,
  attr: @byte

@org vectors
start
render

@org zero_page
cam_x:        @byte
cam_y:        @byte
link_x:       @byte
link_y:       @byte
link_frame:   @byte
link_tl_tile: @byte
link_tr_tile: @byte
link_bl_tile: @byte
link_br_tile: @byte
link_tl_attr: @byte
link_tr_attr: @byte
link_bl_attr: @byte
link_br_attr: @byte

@org spr_attr
link_tl: @str sprite
link_tr: @str sprite
link_bl: @str sprite
link_br: @str sprite

@org rom_begin
start:
  ldx <background_tiles    stx bg_tiles  + $0
  ldx >background_tiles    stx bg_tiles  + $1
  ldx <sprite_tiles        stx spr_tiles + $0
  ldx >sprite_tiles        stx spr_tiles + $1
  ldx <background_palettes stx bg_pals   + $0
  ldx >background_palettes stx bg_pals   + $1
  ldx <sprite_palettes     stx spr_pals  + $0
  ldx >sprite_palettes     stx spr_pals  + $1

  zrx
  chessboard:
    zry
    txz and $01
    jne choose
    txz and $10
    jne chessboard_end
    jmp swap_tile
  choose:
    txz and $10
    jeq chessboard_end
  swap_tile:
    iny
  chessboard_end:
    sty screen00,x
    sty screen01,x
    sty screen10,x
    sty screen11,x
    inx jne chessboard

  ; setup link properties
  ldx $38 stx link_x
  ldy $38 sty link_y
  zry sty link_frame
  ldz $00 stz link_tl_tile
  ldz $80 stz link_tl_attr
  ldz $00 stz link_tr_tile
  ldz $88 stz link_tr_attr
  ldz $01 stz link_bl_tile
  ldz $80 stz link_bl_attr
  ldz $02 stz link_br_tile
  ldz $80 stz link_br_attr

  logic:
    psh
    zry ldx $c0 dly
  ; walk left
    ldz $80 and &1700
    jeq skip_walk_left
    dec link_x
    pop psh ; get time
    and $04
    jne top_left_frame_2
    ldz $07 stz link_tl_tile
    ldz $88 stz link_tl_attr
    ldz $06 stz link_tr_tile
    ldz $88 stz link_tr_attr
    ldz $09 stz link_bl_tile
    ldz $88 stz link_bl_attr
    ldz $08 stz link_br_tile
    ldz $88 stz link_br_attr
    jmp skip_walk_left
  top_left_frame_2:
    ldz $0f stz link_tl_tile
    ldz $88 stz link_tl_attr
    ldz $0e stz link_tr_tile
    ldz $88 stz link_tr_attr
    ldz $11 stz link_bl_tile
    ldz $88 stz link_bl_attr
    ldz $10 stz link_br_tile
    ldz $88 stz link_br_attr
  skip_walk_left:
  ; walk right
    ldz $40 and &1700
    jeq skip_walk_right
    inc link_x
    pop psh ; get time
    and $04
    jne top_right_frame_2
    ldz $06 stz link_tl_tile
    ldz $80 stz link_tl_attr
    ldz $07 stz link_tr_tile
    ldz $80 stz link_tr_attr
    ldz $08 stz link_bl_tile
    ldz $80 stz link_bl_attr
    ldz $09 stz link_br_tile
    ldz $80 stz link_br_attr
    jmp skip_walk_right
  top_right_frame_2:
    ldz $0e stz link_tl_tile
    ldz $80 stz link_tl_attr
    ldz $0f stz link_tr_tile
    ldz $80 stz link_tr_attr
    ldz $10 stz link_bl_tile
    ldz $80 stz link_bl_attr
    ldz $11 stz link_br_tile
    ldz $80 stz link_br_attr
  skip_walk_right:
  ; walk up
    ldz $20 and &1700
    jeq skip_walk_up
    dec link_y
    pop psh ; get time
    and $04
    jne bot_left_frame_2
    ldz $03 stz link_tl_tile
    ldz $80 stz link_tl_attr
    ldz $03 stz link_tr_tile
    ldz $88 stz link_tr_attr
    ldz $04 stz link_bl_tile
    ldz $80 stz link_bl_attr
    ldz $05 stz link_br_tile
    ldz $80 stz link_br_attr
    jmp skip_walk_up
  bot_left_frame_2:
    ldz $03 stz link_tl_tile
    ldz $80 stz link_tl_attr
    ldz $03 stz link_tr_tile
    ldz $88 stz link_tr_attr
    ldz $05 stz link_bl_tile
    ldz $88 stz link_bl_attr
    ldz $04 stz link_br_tile
    ldz $88 stz link_br_attr
  skip_walk_up:
  ; walk down
    ldz $10 and &1700
    jeq skip_walk_down
    inc link_y
    pop psh ; get time
    and $04
    jne bot_right_frame_2
    ldz $00 stz link_tl_tile
    ldz $80 stz link_tl_attr
    ldz $00 stz link_tr_tile
    ldz $88 stz link_tr_attr
    ldz $01 stz link_bl_tile
    ldz $80 stz link_bl_attr
    ldz $02 stz link_br_tile
    ldz $80 stz link_br_attr
    jmp skip_walk_down
  bot_right_frame_2:
    ldz $0a stz link_tl_tile
    ldz $80 stz link_tl_attr
    ldz $0b stz link_tr_tile
    ldz $80 stz link_tr_attr
    ldz $0c stz link_bl_tile
    ldz $80 stz link_bl_attr
    ldz $0d stz link_br_tile
    ldz $80 stz link_br_attr
  skip_walk_down:
  ; move background
    inc cam_x
    inc cam_y
  ; go to next iteration
    pop adc $1
    jmp logic

render:
; update link sprites
  ldz link_tl_tile stz link_tl.tile
  ldz link_tl_attr stz link_tl.attr
  ldz link_tr_tile stz link_tr.tile
  ldz link_tr_attr stz link_tr.attr
  ldz link_bl_tile stz link_bl.tile
  ldz link_bl_attr stz link_bl.attr
  ldz link_br_tile stz link_br.tile
  ldz link_br_attr stz link_br.attr
  ldx link_x ldy link_y
                 stx link_tl.x
                 sty link_tl.y
  clc txz adc $8 stz link_tr.x
                 sty link_tr.y
                 stx link_bl.x
  clc tyz adc $8 stz link_bl.y
  clc txz adc $8 stz link_br.x
  clc tyz adc $8 stz link_br.y
; update camera
  ldx cam_x stx scroll_x
  ldy cam_y sty scroll_y
  ext

background_palettes:
  $60 $07

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
; -------------------
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
  %11111111 %11111111
; -------------------

sprite_tiles:
; --------00---------
  %00000000 %00101010
  %00000000 %10101010
  %00000100 %10111111
  %00000100 %11111111
  %00000101 %11011001
  %00000101 %11011101
  %00000001 %01010101
  %00000010 %10010111
; --------01---------
  %00111111 %11110101
  %11110111 %11111110
  %11010101 %11110111
  %11110111 %11110110
  %11110111 %11110111
  %11111111 %11110110
  %00010101 %01011100
  %00000000 %11111100
; --------02---------
  %01011010 %11111100
  %10101010 %01111100
  %11101001 %01011100
  %11111111 %01010100
  %11101010 %10010000
  %10101010 %00000000
  %00111111 %00000000
  %00000000 %00000000
; --------03---------
  %00000000 %00101010
  %00000000 %10101010
  %00000100 %10101010
  %00000110 %10101010
  %00000111 %10101010
  %00000101 %11111010
  %00000001 %11111110
  %00000011 %10111111
; --------04---------
  %00001111 %10101010
  %00001111 %10101010
  %00000011 %11101010
  %00000010 %10111111
  %00000010 %10101010
  %00000011 %11111010
  %00000011 %11111100
  %00000000 %11110000
; --------05---------
  %10101011 %11000000
  %10101011 %11010000
  %10101011 %11010000
  %11111110 %01010000
  %10101010 %10000000
  %10101011 %00000000
  %00111100 %00000000
  %00000000 %00000000
; --------06---------
  %00000000 %00101010
  %00000010 %10101010
  %00101010 %01101011
  %10101010 %01011111
  %10001010 %01010111
  %00001011 %11010111
  %00000011 %11110101
  %00000000 %10101010
; --------07---------
  %10000000 %00000000
  %11111111 %00000000
  %11111111 %11000000
  %11111111 %00000000
  %01011001 %00001100
  %01011101 %01011100
  %01010101 %00001100
  %01010101 %00001100
; --------08---------
  %00001110 %10101010
  %00111111 %10010101
  %00111111 %11010101
  %00111111 %11010110
  %00001011 %11101011
  %00101010 %10101010
  %00000000 %11111111
  %00000000 %11111111
; --------09---------
  %10101111 %11011100
  %10101011 %11011100
  %10101011 %11001100
  %10101100 %00001100
  %11111100 %00001100
  %10101000 %00000000
  %00000000 %00000000
  %11000000 %00000000
; --------0a---------
  %00000000 %00101010
  %00000000 %10101010
  %00000100 %10111111
  %00000100 %11111111
  %00000101 %11011001
  %00000101 %11011101
  %00000001 %01010101
  %00000000 %10010111
; --------0b---------
  %10101000 %00000000
  %10101010 %00000000
  %11111110 %00010000
  %11111111 %00010000
  %01100111 %01010000
  %01110111 %01010000
  %01010101 %01110000
  %11010110 %11110000
; --------0c---------
  %00001111 %11111101
  %00111101 %11111111
  %00110101 %01111101
  %00111101 %11111101
  %00111101 %11111101
  %00111111 %11111101
  %00000101 %01010100
  %00000000 %00000000
; --------0d---------
  %01011010 %10010000
  %10101010 %10010000
  %11111010 %11000000
  %10111111 %10000000
  %11111010 %10000000
  %10101011 %00000000
  %00111111 %00000000
  %00111111 %00000000
; --------0e---------
  %00000000 %00000000
  %00000000 %00101010
  %00000010 %10101010
  %00101010 %01101011
  %10101010 %01011111
  %10001010 %01010111
  %00001011 %11010111
  %00000011 %11110101
; --------0f---------
  %00000000 %00000000
  %10000000 %00000000
  %11111111 %00000000
  %11111111 %11000000
  %11111111 %00000000
  %01011001 %00000000
  %01011101 %01010000
  %01010101 %00110000
; --------10---------
  %00000000 %10101010
  %00001011 %11101001
  %00001111 %11111101
  %00101111 %11111101
  %00101011 %11111110
  %11111010 %10101011
  %11111110 %10101010
  %00111111 %00000000
; --------11---------
  %01010101 %00110000
  %01011111 %01110000
  %01011011 %01110000
  %01101011 %00110000
  %10101100 %00110000
  %11111110 %00110000
  %10101011 %11000000
  %00111111 %00000000
