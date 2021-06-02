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
    int x, y, width, height;
    int margin;
    ALLEGRO_COLOR bd_color;
    ALLEGRO_COLOR bg_color; // set to something if no background bitmap
    int bd;
    int number_of_subblocks;
    std::vector<TiledBlock *> sub;
    TiledBlock *parent;
    int type;             // a descriptor
    int index;            // a number identifying it among same-type blocks
    Visibility hidden;    // 1 for semi-hidden, -1 for totally hidden.
    ALLEGRO_BITMAP **bmp; // set to nullptr for filled background
};

// find the tile at x,y. Returns an array of integers starting at path[0] representing the
// nested sequence of subblocks that leads to it. path should be an int array of size
// at least the max depth of the subblock sequence
int get_TiledBlock_tile( TiledBlock *tiled_block, int x, int y, int *path );
void draw_TiledBlock( TiledBlock *tiled_block, int x, int y );
void highlight_TiledBlock( TiledBlock *tiled_block );
void get_TiledBlock_offset( TiledBlock *tiled_block, int *x, int *y );
TiledBlock *get_TiledBlock( TiledBlock *tiled_block, int x, int y );
// returns pointer to new tiled block
TiledBlock *new_TiledBlock( void );
