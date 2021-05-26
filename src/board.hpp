#pragma once

#include <cstdio>
#include "game_data.hpp"
#include "tiled_block.hpp"
#include "macros.hpp"
#include <allegro5/allegro_font.h>
#include "widgetz/widgetz.hpp"

struct Board
{
    Board();

    int create_board( GameData *game_data, int mode ); // mode = 0: update, 1: create
    void destroy_board();
    void destroy_board_clue_blocks();
    void clear_info_panel();

    int number_of_columns;
    int column_height;
    int xsize, ysize;
    int max_xsize, max_ysize;
    TiledBlock panel;
    TiledBlock vclue;
    TiledBlock hclue;
    TiledBlock info_panel;
    TiledBlock time_panel;
    TiledBlock all;
    TiledBlock **clue_tiledblock; // pointer to the tiled block where clue is
    TiledBlock *dragging;
    TiledBlock *highlight;
    TiledBlock *rule_out;
    TiledBlock *zoom;
    ALLEGRO_TRANSFORM zoom_transform;
    ALLEGRO_TRANSFORM zoom_transform_inv;
    ALLEGRO_TRANSFORM identity_transform;
    int blink;
    int panel_tile_size;
    int clue_unit_size;
    int clue_unit_space;
    int number_of_hclues;
    int number_of_vclues;
    int dragging_origin_x, dragging_origin_y; // initial position of TiledBlock being dragged
    int dragging_relative_position_of_grabbing_x, dragging_relative_position_of_grabbing_y;
    int type_of_tiles; // 0 = use ttf font, 1 = use file bitmaps, 2 = use classic tiles from grid
    float time_start;
    int restart;
    ALLEGRO_COLOR bg_color;
    ALLEGRO_BITMAP *panel_tile_bmp[8][8];
    ALLEGRO_BITMAP *guess_bmp[8][8];
    ALLEGRO_BITMAP *clue_unit_bmp[8][8];
    ALLEGRO_BITMAP *symbol_bmp[8];
    ALLEGRO_BITMAP **clue_bmp;
    ALLEGRO_BITMAP *button_bmp[4];
    ALLEGRO_BITMAP *button_bmp_scaled[4];
    ALLEGRO_BITMAP *time_bmp;
    ALLEGRO_BITMAP *info_text_bmp;
    ALLEGRO_FONT *text_font;
};

enum BLOCK_TYPE
{
    TB_OTHER = 0, // don't change this
    TB_PANEL,
    TB_PANEL_COLUMN,
    TB_PANEL_BLOCK,
    TB_PANEL_TILE,
    TB_HCLUEBOX,
    TB_HCLUE_TILE,
    TB_VCLUEBOX,
    TB_VCLUE_TILE,
    TB_INFO_PANEL,
    TB_TIME_PANEL,
    TB_BUTTON_SETTINGS,
    TB_BUTTON_HELP,
    TB_BUTTON_CLUE,
    TB_TIMER,
    TB_ALL,
    TB_BUTTON_UNDO,
    TB_BUTTON_TILES
};
