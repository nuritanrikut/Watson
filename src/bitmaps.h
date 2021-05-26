//
//  bitmaps.h
//  watson
//
//  Created by koro on 2/4/16.
//  Copyright (c) 2016 koro. All rights reserved.
//

#ifndef __watson__bitmaps__
#define __watson__bitmaps__

#include <stdio.h>
#include "game.h"
#include "macros.h"
#include "board.h"
#include "TiledBlock.h"

void destroy_board_bitmaps( Board *board );
void destroy_all_bitmaps( Board *board );
int init_bitmaps( Board *board );
int init_bitmaps_classic( Board *board );
int update_bitmaps( Game *game, Board *board );
int update_font_bitmaps( Game *game, Board *board );
void fit_board( Board *board );
ALLEGRO_BITMAP *create_title_bmp( void );
//void explain_clue(Game *game, Board *board, int i);
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

#endif /* defined(__watson__bitmaps__) */
