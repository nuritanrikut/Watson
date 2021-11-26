#pragma once

#include <vector>

#include <cstdio>
#include "game_data.hpp"
#include "tiled_block.hpp"
#include "macros.hpp"
#include <allegro5/allegro_font.h>
#include "widgetz/widgetz.hpp"

struct Board
{
    Board();

    enum class CreateMode
    {
        Update = 0,
        Create = 1,
        CreateFullscreen = 2
    };

    int create_board( GameData *game_data, CreateMode mode );
    void destroy_board();
    void destroy_board_clue_blocks();
    void clear_info_panel();

    int number_of_columns;
    int column_height;

    int width;
    int height;
    int max_width;
    int max_height;

    TiledBlock panel;
    TiledBlock vclue;
    TiledBlock hclue;
    TiledBlock info_panel;
    TiledBlock time_panel;
    TiledBlock all;
    std::vector<TiledBlock *> clue_tiledblock; // pointer to the tiled block where clue is
    TiledBlock *dragging;
    TiledBlock *highlight;
    TiledBlock *rule_out;
    TiledBlock *zoom;

    ALLEGRO_TRANSFORM zoom_transform;
    ALLEGRO_TRANSFORM zoom_transform_inv;
    ALLEGRO_TRANSFORM identity_transform;

    bool blink;
    int panel_tile_size;
    int clue_unit_size;
    int clue_unit_space;
    int number_of_hclues;
    int number_of_vclues;

    // initial position of TiledBlock being dragged
    int dragging_origin_x;
    int dragging_origin_y;

    int dragging_relative_position_of_grabbing_x;
    int dragging_relative_position_of_grabbing_y;

    int type_of_tiles; // 0 = use ttf font, 1 = use file bitmaps, 2 = use classic tiles from grid

    double time_start;

    ALLEGRO_COLOR background_color;

    ALLEGRO_BITMAP *panel_tile_bmp[8][8];
    ALLEGRO_BITMAP *guess_bmp[8][8];
    ALLEGRO_BITMAP *clue_unit_bmp[8][8];
    ALLEGRO_BITMAP *symbol_bmp[8];
    std::vector<ALLEGRO_BITMAP *> clue_bmp;
    ALLEGRO_BITMAP *button_bmp[4];
    ALLEGRO_BITMAP *button_bmp_scaled[4];
    ALLEGRO_BITMAP *time_bmp;
    ALLEGRO_BITMAP *info_text_bmp;

    ALLEGRO_FONT *text_font;
};
