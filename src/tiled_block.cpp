#include "tiled_block.hpp"

#include <allegro5/allegro_primitives.h>

TiledBlock::TiledBlock( TiledBlock::BLOCK_TYPE type_,
                        TiledBlock *parent_,
                        ALLEGRO_COLOR background,
                        ALLEGRO_COLOR border,
                        TiledBlock::Visibility visibility )
    : x( 0 ),
      y( 0 ),
      width( 0 ),
      height( 0 ),
      margin( 0 ),
      border_color( border ),
      background_color( background ),
      draw_border( 0 ),
      number_of_subblocks( 0 ),
      parent( parent_ ),
      type( type_ ),
      index( 0 ),
      hidden( visibility ),
      bmp( nullptr )
{
}

// find the tile at x,y. Returns in path[] an array of integers starting at path[0] representing the
// nested sequence of subblocks that leads to it (depth=0 means the main tile doesn't match)

int get_TiledBlock_tile( TiledBlock *tiled_block, int x, int y, int *path )
{
    int m;
    int depth = 0;

    if( ( x >= tiled_block->x ) && ( x < tiled_block->x + tiled_block->width ) && ( y >= tiled_block->y )
        && ( y < tiled_block->y + tiled_block->height ) )
    {
        for( m = 0; m < tiled_block->number_of_subblocks; m++ )
        {
            depth = get_TiledBlock_tile( tiled_block->sub[m], x - tiled_block->x, y - tiled_block->y, path + 1 );
            if( depth )
            {
                *path = m;
                break;
            }
        }
        return depth + 1;
    }
    return 0;
}

// get pointer to tiledblock at position x,y
TiledBlock *get_TiledBlock( TiledBlock *tiled_block, int x, int y )
{
    int m;
    TiledBlock *rt = nullptr;

    if( ( x >= tiled_block->x ) && ( x < tiled_block->x + tiled_block->width ) && ( y >= tiled_block->y )
        && ( y < tiled_block->y + tiled_block->height ) )
    {
        for( m = 0; m < tiled_block->number_of_subblocks; m++ )
        {
            rt = get_TiledBlock( tiled_block->sub[m], x - tiled_block->x, y - tiled_block->y );
            if( rt && ( rt->hidden != TiledBlock::Visibility::TotallyHidden ) )
                return rt;
        }
        return tiled_block;
    }
    else
        return nullptr;
}

// Draw the tiled block in the target allegro display
void draw_TiledBlock( TiledBlock *tiled_block, int x, int y )
{
    int i;

    if( tiled_block->bmp && ( tiled_block->hidden != TiledBlock::Visibility::TotallyHidden ) )
    {
        if( tiled_block->hidden != TiledBlock::Visibility::Visible )
        {
            al_draw_tinted_bitmap(
                *( tiled_block->bmp ), al_map_rgba_f( 0.1, 0.1, 0.1, 0.1 ), tiled_block->x + x, tiled_block->y + y, 0 );
        }
        else
        {
            al_draw_bitmap( *( tiled_block->bmp ), tiled_block->x + x, tiled_block->y + y, 0 );
        }
    }
    else
    {
        al_draw_filled_rectangle( tiled_block->x + x,
                                  tiled_block->y + y,
                                  tiled_block->x + x + tiled_block->width,
                                  tiled_block->y + y + tiled_block->height,
                                  tiled_block->background_color );
    }

    if( tiled_block->draw_border )
        al_draw_rectangle( tiled_block->x + x,
                           tiled_block->y + y,
                           tiled_block->x + x + tiled_block->width,
                           tiled_block->y + y + tiled_block->height,
                           tiled_block->border_color,
                           tiled_block->draw_border );

    for( i = 0; i < tiled_block->number_of_subblocks; i++ )
    {
        if( tiled_block->sub[i] )
            draw_TiledBlock( tiled_block->sub[i], tiled_block->x + x, tiled_block->y + y );
    }
}

void get_TiledBlock_offset( TiledBlock *tiled_block, int *x, int *y )
{
    *x = tiled_block->x;
    *y = tiled_block->y;

    while( tiled_block->parent )
    {
        tiled_block = tiled_block->parent;
        *x += tiled_block->x;
        *y += tiled_block->y;
    }
}

void highlight_TiledBlock( TiledBlock *tiled_block )
{
    int x, y;
    get_TiledBlock_offset( tiled_block, &x, &y );
    for( int i = 0; i < 8; i++ )
    {
        al_draw_rectangle( x, y, x + tiled_block->width, y + tiled_block->height, al_premul_rgba_f( 1, 0, 0, 0.2 ), i );
    }
    al_draw_filled_rectangle( x, y, x + tiled_block->width, y + tiled_block->height, al_premul_rgba_f( 1, 1, 1, 0.3 ) );
}
