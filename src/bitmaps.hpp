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
int init_bitmaps( Board *board );
int init_bitmaps_classic();
int update_bitmaps( GameData *game_data, Board *board );
int update_font_bitmaps( GameData *game_data, Board *board );
void fit_board( Board *board );
ALLEGRO_BITMAP *create_title_bmp( void );
void destroy_settings_bitmaps( Board *board );
void create_settings_bitmaps( Board *board );
void update_timer( int seconds, Board *board );
void show_info_text( Board *board, ALLEGRO_USTR *msg ); //msg will be freed
void show_info_text_b( Board *board, const char *msg, ... );
void clear_info_panel( Board *board );
void draw_title( void );
void convert_grayscale( ALLEGRO_BITMAP *bmp );
void create_font_symbols( Board *board );

// globals
extern char symbol_char[9][8][6];

// debug
ALLEGRO_BITMAP *get_clue_bitmap( Board *board, Clue *clue );
