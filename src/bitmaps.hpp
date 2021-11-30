//
//  bitmaps.h
//  watson
//
//  Created by koro on 2/4/16.
//  Copyright (c) 2016 koro. All rights reserved.
//

#pragma once

#include <cstdio>

#include "game_data.hpp"
#include "macros.hpp"
#include "board.hpp"
#include "tiled_block.hpp"

void destroy_board_bitmaps( Board *board );
void destroy_all_bitmaps( Board *board );
auto init_bitmaps( Board *board ) -> int;
auto init_bitmaps_classic() -> int;
auto update_bitmaps( GameData *game_data, Board *board ) -> int;
auto update_font_bitmaps( GameData *game_data, Board *board ) -> int;
void fit_board( Board *board );
auto create_title_bmp() -> ALLEGRO_BITMAP *;
void destroy_settings_bitmaps( Board *board );
void create_settings_bitmaps( Board *board );
void update_timer( int seconds, Board *board );
void show_info_text( Board *board, ALLEGRO_USTR *msg ); //msg will be freed
void show_info_text_b( Board *board, const char *msg, ... );
void clear_info_panel( Board *board );
void draw_title();
void convert_grayscale( ALLEGRO_BITMAP *bmp );
void create_font_symbols( Board *board );

// globals
extern char symbol_char[9][8][6];

// debug
auto get_clue_bitmap( Board *board, Clue *clue ) -> ALLEGRO_BITMAP *;
