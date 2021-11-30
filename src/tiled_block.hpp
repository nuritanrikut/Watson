#pragma once

#include <vector>
#include <cstdio>

#include <allegro5/allegro5.h>

#include "macros.hpp"

struct TiledBlock
{
    enum class Visibility
    {
        Visible = 0,
        PartiallyHidden = 1,
        TotallyHidden = 2,
    };

    enum class BLOCK_TYPE
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

    TiledBlock( BLOCK_TYPE type,
                TiledBlock *parent,
                ALLEGRO_COLOR background,
                ALLEGRO_COLOR border,
                Visibility visibility = Visibility::Visible );

    int x;
    int y;
    int width;
    int height;
    int margin;
    ALLEGRO_COLOR border_color;
    ALLEGRO_COLOR background_color; // set to something if no background bitmap
    int draw_border;
    int number_of_subblocks;
    std::vector<TiledBlock *> sub;
    TiledBlock *parent;
    BLOCK_TYPE type;      // a descriptor
    int index;            // a number identifying it among same-type blocks
    Visibility hidden;    // 1 for semi-hidden, -1 for totally hidden.
    ALLEGRO_BITMAP **bmp; // set to nullptr for filled background
};

// find the tile at x,y. Returns an array of integers starting at path[0] representing the
// nested sequence of subblocks that leads to it. path should be an int array of size
// at least the max depth of the subblock sequence
auto get_TiledBlock_tile( TiledBlock *tiled_block, int x, int y, int *path ) -> int;
void draw_TiledBlock( TiledBlock *tiled_block, int x, int y );
void highlight_TiledBlock( TiledBlock *tiled_block );
void get_TiledBlock_offset( TiledBlock *tiled_block, int *x, int *y );
auto get_TiledBlock( TiledBlock *tiled_block, int x, int y ) -> TiledBlock *;
// returns pointer to new tiled block
auto new_TiledBlock() -> TiledBlock *;
