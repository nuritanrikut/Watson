#include "board.hpp"
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include "allegro_stuff.hpp"
#include "bitmaps.hpp"
#include "main.hpp"

// spacing, in fraction of corresponding display dimensions
float INFO_PANEL_PORTION = 0.1;
float VCLUEBOX_PORTION = .25;
float HCLUEBOX_PORTION = .25;
int VCLUEBOX_MARGIN = 4; // pixels for now
int HCLUEBOX_MARGIN = 4;
int PANEL_MARGIN = 4;
int PANEL_COLUMN_SPACE = 4;
int PANEL_BLOCK_MARGIN = 4;
int PANEL_TILE_SPACE = 0;
int CLUE_UNIT_SPACE = 0;
int CLUE_TILE_MARGIN = 3;
int VCLUE_COLUMN_SPACE = 0;
int HCLUE_ROW_SPACE = 0;
int INFO_PANEL_MARGIN = 4;
float H_FLEX_FACTOR = 0.03;

// general tile settings
#define BG_COLOR NULL_COLOR
#define PANEL_BD_COLOR NULL_COLOR
#define PANEL_BG_COLOR DARK_GREY_COLOR
#define PANEL_COLUMN_BG_COLOR DARK_GREY_COLOR
#define PANEL_COLUMN_BD_COLOR GREY_COLOR
#define CLUE_TILE_BG_COLOR NULL_COLOR //al_map_rgb_f(.2, .2, .2)
#define CLUE_TILE_BD_COLOR NULL_COLOR
#define PANEL_TILE_BD_COLOR NULL_COLOR
#define CLUE_PANEL_BG_COLOR DARK_GREY_COLOR
#define CLUE_PANEL_BD_COLOR GREY_COLOR
#define INFO_PANEL_BG_COLOR DARK_GREY_COLOR
#define INFO_PANEL_BD_COLOR GREY_COLOR
#define TIME_PANEL_BG_COLOR DARK_GREY_COLOR
#define TIME_PANEL_BD_COLOR GREY_COLOR

#define TIME_PANEL_BUTTONS 4

int PANEL_TILE_COLUMNS[9] = { 0, 1, 2, 2, 2, 3, 3, 4, 4 };
int PANEL_TILE_ROWS[9] = { 0, 1, 1, 2, 2, 2, 2, 2, 2 };

void destroy_board( Board *board )
{ // note that board->vclue.sub and board->hclue.sub are destroyed elsewhere
    int i, j;

    for( i = 0; i < board->number_of_columns; i++ )
    {
        for( j = 0; j < board->column_height; j++ )
        {
            nfree( board->panel.sub[i]->sub[j]->sub );
        }
        nfree( board->panel.sub[i]->sub );
    }
    nfree( board->panel.sub );

    for( i = 0; i < board->time_panel.number_of_subblocks; i++ )
        nfree( board->time_panel.sub[i] );

    destroy_board_clue_blocks( board );
    destroy_all_bitmaps( board );
    nfree( board->all.sub );
    nfree( board->clue_bmp );
    nfree( board->clue_tiledblock );
}

void destroy_board_clue_blocks( Board *board )
{
    int i;

    if( board->hclue.sub )
    {
        for( i = 0; i < board->hclue.number_of_subblocks; i++ )
        {
            nfree( board->hclue.sub[i] );
        }
        nfree( board->hclue.sub );
    }

    if( board->vclue.sub )
    {
        for( i = 0; i < board->vclue.number_of_subblocks; i++ )
        {
            nfree( board->vclue.sub[i] );
        }
        nfree( board->vclue.sub );
    }
}

//xxx todo: better board generation. Fix tile size first, then compute everything?
// mode: 1 = create, 0 = update, 2 = create fullscreen
int create_board( Game *game, Board *board, int mode )
{
    int i, j, k;
    int column_w, column_h, block_w, block_h, block_space;
    int hclue_tile_w, hclue_tile_h, vclue_tile_w, vclue_tile_h;
    int panel_tile_w, panel_tile_h;
    int cus;

    board->clue_unit_space = CLUE_UNIT_SPACE;

    if( mode )
    {
        board->clue_bmp = static_cast<ALLEGRO_BITMAP **>( malloc( game->clue_n * sizeof( *board->clue_bmp ) ) );
        board->clue_tiledblock =
            static_cast<TiledBlock **>( malloc( game->clue_n * sizeof( *board->clue_tiledblock ) ) );
    }

    if( board->max_ysize * INFO_PANEL_PORTION - 2 * INFO_PANEL_MARGIN
        < 32 ) // guarantee height of 32 pixels in info panel
        board->max_ysize = ( 32 + 2 * INFO_PANEL_MARGIN ) / INFO_PANEL_PORTION;

    board->xsize = board->max_xsize;
    board->ysize = board->max_ysize * ( 1.0 - INFO_PANEL_PORTION );

    board->bg_color = BG_COLOR;
    board->dragging = NULL;
    board->highlight = NULL;
    board->rule_out = NULL;

    // panel dimensions
    board->panel.x = PANEL_MARGIN;
    board->panel.y = PANEL_MARGIN;
    board->panel.width = ( board->xsize - 2 * PANEL_MARGIN - board->xsize * HCLUEBOX_PORTION - 2 * HCLUEBOX_MARGIN );
    board->panel.height = ( board->ysize - 2 * PANEL_MARGIN - board->ysize * VCLUEBOX_PORTION - 2 * VCLUEBOX_MARGIN );
    board->panel.margin = PANEL_MARGIN;
    board->panel.bd_color = PANEL_BD_COLOR;
    board->panel.bg_color = PANEL_BG_COLOR;
    board->panel.bd = 1;
    board->panel.number_of_subblocks = board->number_of_columns; // number_of_columns subpanels
    board->panel.parent = &board->all;
    board->panel.type = TB_PANEL;
    board->panel.index = 0;
    board->panel.hidden = 0;
    board->panel.bmp = NULL;

    // for nons-square panel tiles:
    panel_tile_w =
        ( ( board->panel.width - ( board->number_of_columns - 1 ) * PANEL_COLUMN_SPACE ) / board->number_of_columns
          - 2 * PANEL_BLOCK_MARGIN - ( PANEL_TILE_COLUMNS[board->number_of_columns] - 1 ) * PANEL_TILE_SPACE )
        / ( PANEL_TILE_COLUMNS[board->number_of_columns] );
    panel_tile_h =
        ( board->panel.height
          - board->column_height
                * ( ( PANEL_TILE_ROWS[board->number_of_columns] - 1 ) * PANEL_TILE_SPACE + 2 * PANEL_BLOCK_MARGIN ) )
        / ( board->column_height * PANEL_TILE_ROWS[board->number_of_columns] ); //ok go

    // make square tiles
    board->panel_tile_size = min( panel_tile_w, panel_tile_h );
    panel_tile_w = board->panel_tile_size; // to use below
    panel_tile_h = board->panel_tile_size; // to use below

    block_w = PANEL_TILE_COLUMNS[board->number_of_columns] * panel_tile_w
              + ( PANEL_TILE_COLUMNS[board->number_of_columns] - 1 ) * PANEL_TILE_SPACE;
    block_h = PANEL_TILE_ROWS[board->number_of_columns] * panel_tile_h
              + ( PANEL_TILE_ROWS[board->number_of_columns] - 1 ) * PANEL_TILE_SPACE;
    column_w = ( block_w + 2 * PANEL_BLOCK_MARGIN );
    // block_space = (board->panel.height - board->column_height*(block_h + 2*PANEL_BLOCK_MARGIN))/(board->column_height-1);
    block_space = 0; // don't separate blocks too much
    column_h = board->column_height * ( block_h + 2 * PANEL_BLOCK_MARGIN );

    // adjust panel dimensions to account for integer arithmetic
    board->panel.width = board->number_of_columns * column_w + ( board->number_of_columns - 1 ) * PANEL_COLUMN_SPACE;
    board->panel.height = column_h;

    // panel columns
    if( mode )
        board->panel.sub =
            static_cast<TiledBlock **>( malloc( board->number_of_columns * sizeof( struct TiledBlock * ) ) );
    for( i = 0; i < board->number_of_columns; i++ )
    {
        if( mode )
        {
            board->panel.sub[i] = static_cast<TiledBlock *>(malloc( sizeof( struct TiledBlock ) ));
            board->panel.sub[i]->bg_color = PANEL_COLUMN_BG_COLOR;
            board->panel.sub[i]->bd_color = PANEL_COLUMN_BD_COLOR;
            board->panel.sub[i]->parent = &board->panel;
            board->panel.sub[i]->bd = 1; // draw boundary
            board->panel.sub[i]->number_of_subblocks = board->column_height;
            board->panel.sub[i]->sub =
                static_cast<TiledBlock **>( malloc( board->column_height * sizeof( struct TiledBlock * ) ) );
            board->panel.sub[i]->type = TB_PANEL_COLUMN;
            board->panel.sub[i]->index = i;
            board->panel.sub[i]->hidden = 0;
            board->panel.sub[i]->bmp = NULL; // no background image
        }

        board->panel.sub[i]->width = column_w;
        board->panel.sub[i]->height = board->panel.height;
        board->panel.sub[i]->margin = 0;
        board->panel.sub[i]->x = i * ( board->panel.sub[i]->width + PANEL_COLUMN_SPACE ); // position relative to parent
        board->panel.sub[i]->y = 0;

        // panel blocks
        for( j = 0; j < board->column_height; j++ )
        {
            if( mode )
            {
                board->panel.sub[i]->sub[j] = static_cast<TiledBlock *>(malloc( sizeof( struct TiledBlock ) ));
                board->panel.sub[i]->sub[j]->bd_color = NULL_COLOR;
                board->panel.sub[i]->sub[j]->bg_color = NULL_COLOR;
                board->panel.sub[i]->sub[j]->bd = 0;
                board->panel.sub[i]->sub[j]->number_of_subblocks = board->number_of_columns;
                board->panel.sub[i]->sub[j]->sub =
                    static_cast<TiledBlock **>( malloc( board->number_of_columns * sizeof( struct TiledBlock * ) ) );
                board->panel.sub[i]->sub[j]->parent = board->panel.sub[i];
                board->panel.sub[i]->sub[j]->type = TB_PANEL_BLOCK;
                board->panel.sub[i]->sub[j]->index = j;
                board->panel.sub[i]->sub[j]->hidden = 0;
                board->panel.sub[i]->sub[j]->bmp = NULL;
            }
            board->panel.sub[i]->sub[j]->margin = PANEL_BLOCK_MARGIN;
            board->panel.sub[i]->sub[j]->width = block_w;
            board->panel.sub[i]->sub[j]->height =
                PANEL_TILE_ROWS[board->number_of_columns] * panel_tile_h
                + ( PANEL_TILE_ROWS[board->number_of_columns] - 1 ) * PANEL_TILE_SPACE;
            board->panel.sub[i]->sub[j]->x = PANEL_BLOCK_MARGIN;
            board->panel.sub[i]->sub[j]->y =
                j * ( board->panel.sub[i]->sub[j]->height + block_space + 2 * PANEL_BLOCK_MARGIN ) + PANEL_BLOCK_MARGIN;

            // panel tiles
            for( k = 0; k < board->number_of_columns; k++ )
            {
                if( mode )
                {
                    board->panel.sub[i]->sub[j]->sub[k] = static_cast<TiledBlock *>(malloc( sizeof( struct TiledBlock ) ));
                    board->panel.sub[i]->sub[j]->sub[k]->bd_color = PANEL_TILE_BD_COLOR;
                    board->panel.sub[i]->sub[j]->sub[k]->bg_color = NULL_COLOR;
                    board->panel.sub[i]->sub[j]->sub[k]->bd = 1;
                    board->panel.sub[i]->sub[j]->sub[k]->number_of_subblocks = 0;
                    board->panel.sub[i]->sub[j]->sub[k]->sub = NULL;
                    board->panel.sub[i]->sub[j]->sub[k]->parent = board->panel.sub[i]->sub[j];
                    board->panel.sub[i]->sub[j]->sub[k]->type = TB_PANEL_TILE;
                    board->panel.sub[i]->sub[j]->sub[k]->index = k;
                    board->panel.sub[i]->sub[j]->sub[k]->hidden = 0;
                }
                board->panel.sub[i]->sub[j]->sub[k]->margin = 0;
                board->panel.sub[i]->sub[j]->sub[k]->width = panel_tile_w;
                board->panel.sub[i]->sub[j]->sub[k]->height = panel_tile_h;
                board->panel.sub[i]->sub[j]->sub[k]->x =
                    ( k % PANEL_TILE_COLUMNS[board->number_of_columns] ) * panel_tile_w;
                board->panel.sub[i]->sub[j]->sub[k]->y =
                    ( k / PANEL_TILE_COLUMNS[board->number_of_columns] ) * panel_tile_h;
                board->panel.sub[i]->sub[j]->sub[k]->bmp = &( board->panel_tile_bmp[j][k] );
            }
        }
    }

    // VCluebox dimensions
    board->vclue.margin = VCLUEBOX_MARGIN;
    board->vclue.y = board->panel.y + board->panel.height + board->panel.margin + board->vclue.margin;
    board->vclue.x = board->vclue.margin;
    board->vclue.bg_color = CLUE_PANEL_BG_COLOR;
    board->vclue.bd_color = CLUE_PANEL_BD_COLOR;
    board->vclue.bd = 1;
    board->vclue.bmp = NULL;
    board->vclue.width = board->panel.width;
    board->vclue.height = board->ysize - board->panel.height - 2 * board->panel.margin - 2 * board->vclue.margin;
    board->vclue.parent = &board->all;
    board->vclue.sub = NULL;              // later change
    board->vclue.number_of_subblocks = 0; // later change
    board->vclue.hidden = 0;

    // HCluebox dimension
    board->hclue.margin = HCLUEBOX_MARGIN;
    board->hclue.x = board->panel.x + board->panel.width + board->panel.margin + board->hclue.margin;
    board->hclue.y = board->panel.y;
    board->hclue.width = board->xsize - board->panel.width - 2 * board->panel.margin - 2 * board->hclue.margin;
    board->hclue.height = board->ysize - 2 * HCLUEBOX_MARGIN;
    board->hclue.bg_color = CLUE_PANEL_BG_COLOR;
    board->hclue.bd_color = CLUE_PANEL_BD_COLOR;
    board->hclue.bd = 1;
    board->hclue.parent = &board->all;
    board->hclue.number_of_subblocks = 0; // later change
    board->hclue.sub = NULL;              // later change
    board->hclue.hidden = 0;

    // count number of vclues and hclues
    if( mode )
    {
        board->number_of_vclues = 0;
        board->number_of_hclues = 0;
        for( i = 0; i < game->clue_n; i++ )
        {
            if( is_vclue( game->clue[i].rel ) )
                board->number_of_vclues++;
            else
                board->number_of_hclues++;
        }
    }
    //xxx todo: allow multiple columns/rows of hclues/vclues

    cus = min( board->hclue.width - 2 * CLUE_TILE_MARGIN - 2 * CLUE_UNIT_SPACE,
               board->vclue.height - 2 * CLUE_TILE_MARGIN - 2 * CLUE_UNIT_SPACE )
          / 3;
    cus = min( cus,
               ( board->panel.height + 2 * CLUE_UNIT_SPACE + 2 * CLUE_TILE_MARGIN + VCLUEBOX_MARGIN
                 + board->panel.margin - 2 * CLUE_TILE_MARGIN * board->number_of_hclues )
                   / ( max( board->number_of_hclues - 3, 1 ) ) );
    board->clue_unit_size =
        min( cus, ( board->panel.width / ( max( board->number_of_vclues, 1 ) ) - 2 * CLUE_TILE_MARGIN ) );

    //    board->clue_unit_size = min((board->hclue.height - 2*board->number_of_hclues*CLUE_TILE_MARGIN)/board->number_of_hclues, (board->vclue.width - 2*board->number_of_vclues*CLUE_TILE_MARGIN)/board->number_of_vclues);
    //    board->clue_unit_size = min(cus, board->clue_unit_size);
    vclue_tile_h = board->clue_unit_size * 3 + 2 * CLUE_UNIT_SPACE;
    vclue_tile_w = board->clue_unit_size;
    hclue_tile_h = board->clue_unit_size;
    hclue_tile_w = board->clue_unit_size * 3 + 2 * CLUE_UNIT_SPACE;

    // update hclue and vclue to fit tight
    board->hclue.width = hclue_tile_w + 2 * CLUE_TILE_MARGIN;
    board->hclue.height =
        board->panel.height + board->panel.margin + board->vclue.margin + vclue_tile_h + 2 * CLUE_TILE_MARGIN;
    board->vclue.height = vclue_tile_h + 2 * CLUE_TILE_MARGIN;
    board->vclue.width = board->panel.width;
    board->vclue.number_of_subblocks = board->vclue.width / ( board->clue_unit_size + 2 * CLUE_TILE_MARGIN );
    board->hclue.number_of_subblocks = board->hclue.height / ( board->clue_unit_size + 2 * CLUE_TILE_MARGIN );

    //fit tight again
    board->hclue.height = board->hclue.number_of_subblocks * ( hclue_tile_h + 2 * CLUE_TILE_MARGIN );
    board->vclue.width = board->vclue.number_of_subblocks * ( vclue_tile_w + 2 * CLUE_TILE_MARGIN );

    //create Vclue tiles
    board->vclue.sub =
        static_cast<TiledBlock **>( malloc( board->vclue.number_of_subblocks * sizeof( struct TiledBlock * ) ) );
    for( i = 0; i < board->vclue.number_of_subblocks; i++ )
    {
        board->vclue.sub[i] = static_cast<TiledBlock *>(malloc( sizeof( struct TiledBlock ) ));
        board->vclue.sub[i]->width = vclue_tile_w;
        board->vclue.sub[i]->height = vclue_tile_h;
        board->vclue.sub[i]->margin = CLUE_TILE_MARGIN;
        board->vclue.sub[i]->x =
            board->vclue.sub[i]->margin + i * ( board->vclue.sub[i]->width + 2 * board->vclue.sub[i]->margin );
        board->vclue.sub[i]->y = board->vclue.sub[i]->margin;
        board->vclue.sub[i]->bd_color = CLUE_TILE_BD_COLOR;
        board->vclue.sub[i]->bg_color = CLUE_TILE_BG_COLOR;
        board->vclue.sub[i]->bd = 1;
        board->vclue.sub[i]->number_of_subblocks = 0;
        board->vclue.sub[i]->sub = NULL;
        board->vclue.sub[i]->parent = &( board->vclue );
        board->vclue.sub[i]->type = TB_VCLUE_TILE;
        board->vclue.sub[i]->index = -1;
        board->vclue.sub[i]->hidden = 0;
        board->vclue.sub[i]->bmp = NULL;
    }

    //create Hclue tiles
    board->hclue.sub = static_cast<TiledBlock **>(malloc( board->hclue.number_of_subblocks * sizeof( struct TiledBlock * ) ));
    for( i = 0; i < board->hclue.number_of_subblocks; i++ )
    {
        board->hclue.sub[i] = static_cast<TiledBlock *>(malloc( sizeof( struct TiledBlock ) ));
        board->hclue.sub[i]->width = hclue_tile_w;
        board->hclue.sub[i]->height = hclue_tile_h;
        board->hclue.sub[i]->margin = CLUE_TILE_MARGIN;
        board->hclue.sub[i]->x = board->hclue.sub[i]->margin;
        board->hclue.sub[i]->y =
            board->hclue.sub[i]->margin + i * ( board->hclue.sub[i]->height + 2 * board->hclue.sub[i]->margin );
        ;
        board->hclue.sub[i]->bd_color = CLUE_TILE_BD_COLOR;
        board->hclue.sub[i]->bg_color = CLUE_TILE_BG_COLOR;
        board->hclue.sub[i]->bd = 1;
        board->hclue.sub[i]->number_of_subblocks = 0;
        board->hclue.sub[i]->sub = NULL;
        board->hclue.sub[i]->parent = &( board->hclue );
        board->hclue.sub[i]->type = TB_HCLUE_TILE;
        board->hclue.sub[i]->index = -1;
        board->hclue.sub[i]->hidden = 0;
        board->hclue.sub[i]->bmp = NULL;
    }

    //load Hclues
    j = 0;
    k = 0;
    for( i = 0; i < game->clue_n; i++ )
    {
        if( is_vclue( game->clue[i].rel ) )
        {
            board->vclue.sub[j]->index = i;
            board->vclue.sub[j]->hidden = game->clue[i].hidden;
            board->vclue.sub[j]->bmp = &( board->clue_bmp[i] );
            board->clue_tiledblock[i] = board->vclue.sub[j];
            j++;
        }
        else
        {
            board->hclue.sub[k]->index = i;
            board->hclue.sub[k]->hidden = game->clue[i].hidden;
            board->hclue.sub[k]->bmp = &( board->clue_bmp[i] );
            board->clue_tiledblock[i] = board->hclue.sub[k];
            k++;
        }
    }

    //resize board to fit tight
    //    fit_board(board);
    board->xsize = board->hclue.x + board->hclue.width + board->hclue.margin;
    board->ysize = max( board->hclue.height + 2 * board->hclue.margin,
                        board->vclue.y + board->vclue.height + board->vclue.margin );

    for( i = 1; i < board->number_of_columns; i++ )
        board->panel.sub[i]->x += i
                                  * min( ( board->max_xsize * H_FLEX_FACTOR ),
                                         ( board->max_xsize - board->xsize ) / board->number_of_columns );
    board->hclue.x +=
        board->number_of_columns
        * min( ( board->max_xsize * H_FLEX_FACTOR ), ( board->max_xsize - board->xsize ) / board->number_of_columns );

    board->info_panel.height = (float)board->max_ysize * INFO_PANEL_PORTION - 2 * INFO_PANEL_MARGIN;
    board->vclue.y +=
        min( ( board->max_ysize - board->ysize - board->info_panel.height - 2 * board->info_panel.margin ) / 2, 30 );
    board->info_panel.y =
        board->vclue.y + board->vclue.height + board->vclue.margin + INFO_PANEL_MARGIN
        + min( ( board->max_ysize - board->ysize - board->info_panel.height - 2 * INFO_PANEL_MARGIN ) / 2, 30 );
    board->hclue.y = ( board->info_panel.y - board->hclue.height ) / 2;

    // center vclues

    board->panel.width = board->panel.sub[board->number_of_columns - 1]->x
                         + board->panel.sub[board->number_of_columns - 1]->width - board->panel.sub[0]->x;
    board->vclue.x = ( board->panel.width - board->vclue.width ) / 2;
    //	board->panel.x = (board->hclue.x - board->panel.width) / 2;

    // info panel
    board->info_panel.x = board->panel.x; //  INFO_PANEL_MARGIN;
    board->info_panel.width = board->hclue.x - board->hclue.margin - INFO_PANEL_MARGIN
                              - board->panel.x; // board->vclue.width; // assumes margins are equal
    board->info_panel.margin = INFO_PANEL_MARGIN;
    board->info_panel.bd_color = INFO_PANEL_BD_COLOR;
    board->info_panel.bg_color = INFO_PANEL_BG_COLOR;
    board->info_panel.bd = 1;
    board->info_panel.number_of_subblocks = 0;
    board->info_panel.sub = NULL;
    board->info_panel.parent = &board->all;
    board->info_panel.type = TB_INFO_PANEL;
    board->info_panel.index = 0;
    board->info_panel.hidden = 0;
    board->info_panel.bmp = NULL;

    // time panel
    board->time_panel.y = board->info_panel.y;
    board->time_panel.x = board->hclue.x;
    board->time_panel.width = board->hclue.width;
    board->time_panel.height = board->info_panel.height;
    board->time_panel.margin = board->info_panel.margin;
    board->time_panel.bg_color = TIME_PANEL_BG_COLOR;
    board->time_panel.bd_color = TIME_PANEL_BD_COLOR;
    board->time_panel.bd = 1;
    board->time_panel.number_of_subblocks = 1 + TIME_PANEL_BUTTONS;
    board->time_panel.parent = &board->all;
    board->time_panel.type = TB_TIME_PANEL;
    board->time_panel.index = 0;
    board->time_panel.hidden = 0;
    board->time_panel.bmp = NULL;

    if( mode )
    { // if board is being created
        board->time_panel.sub = static_cast<TiledBlock **>(malloc( 6 * sizeof( struct TiledBlock * ) ));
        for( i = 0; i < 5; i++ )
            board->time_panel.sub[i] = static_cast<TiledBlock *>(malloc( sizeof( struct TiledBlock ) ));
    }

    // timer
    board->time_panel.sub[0]->x = 2;
    board->time_panel.sub[0]->y = 4;
    board->time_panel.sub[0]->height = 16;
    board->time_panel.sub[0]->width = board->time_panel.width - 4;
    board->time_panel.sub[0]->margin = 0;
    board->time_panel.sub[0]->bd_color = NULL_COLOR;
    board->time_panel.sub[0]->bg_color = TIME_PANEL_BG_COLOR;
    board->time_panel.sub[0]->bd = 0;
    board->time_panel.sub[0]->number_of_subblocks = 0;
    board->time_panel.sub[0]->sub = NULL;
    board->time_panel.sub[0]->parent = &board->time_panel;
    board->time_panel.sub[0]->type = TB_TIMER;
    board->time_panel.sub[0]->index = 0;
    board->time_panel.sub[0]->hidden = 0;
    board->time_panel.sub[0]->bmp = &board->time_bmp;

    for( i = 0; i < TIME_PANEL_BUTTONS; i++ )
    { // buttons
        board->time_panel.sub[i + 1]->height = min( ( board->time_panel.height - 24 ), board->time_panel.width / 5 );
        board->time_panel.sub[i + 1]->width = board->time_panel.sub[i + 1]->height;
        board->time_panel.sub[i + 1]->y =
            ( ( board->time_panel.height + ( board->time_panel.sub[0]->y + board->time_panel.sub[0]->height ) )
              - board->time_panel.sub[i + 1]->height )
            / 2;
        board->time_panel.sub[i + 1]->x =
            ( ( i + 1 ) * board->time_panel.width + ( i - TIME_PANEL_BUTTONS ) * board->time_panel.sub[i + 1]->width )
            / ( TIME_PANEL_BUTTONS + 1 );
        board->time_panel.sub[i + 1]->margin = 0;
        board->time_panel.sub[i + 1]->bg_color = NULL_COLOR;
        board->time_panel.sub[i + 1]->bd_color = WHITE_COLOR;
        board->time_panel.sub[i + 1]->bd = 0;
        board->time_panel.sub[i + 1]->number_of_subblocks = 0;
        board->time_panel.sub[i + 1]->sub = NULL;
        board->time_panel.sub[i + 1]->parent = &board->time_panel;
        board->time_panel.sub[i + 1]->bmp = &board->button_bmp_scaled[i];
        board->time_panel.sub[i + 1]->index = 0;
        board->time_panel.sub[i + 1]->hidden = 0;
    }

    board->time_panel.sub[1]->type = TB_BUTTON_CLUE;
    board->time_panel.sub[2]->type = TB_BUTTON_HELP;
    board->time_panel.sub[3]->type = TB_BUTTON_SETTINGS;
    board->time_panel.sub[4]->type = TB_BUTTON_UNDO;

    // collect TiledBlocks into an array for convenience
    // and create settings block
    if( mode )
    { // only if board is being created
        board->all.x = 0;
        board->all.y = 0;
        board->all.margin = 0;
        board->all.bd_color = NULL_COLOR;
        board->all.bg_color = board->bg_color;
        board->all.bd = 0;
        board->all.parent = NULL;
        board->all.number_of_subblocks = 5;
        board->all.sub = static_cast<TiledBlock **>(malloc( board->all.number_of_subblocks * sizeof( struct TiledBlock * ) ));
        board->all.type = TB_ALL;
        board->all.index = 0;
        board->all.hidden = 0;
        board->all.bmp = NULL;

        board->all.sub[0] = &board->panel;
        board->all.sub[1] = &board->hclue;
        board->all.sub[2] = &board->vclue;
        board->all.sub[3] = &board->time_panel;
        board->all.sub[4] = &board->info_panel;
    }

    // final size adjustment
    board->xsize = board->hclue.x + board->hclue.margin + board->hclue.width;
    board->ysize = board->info_panel.y + board->info_panel.height + board->info_panel.margin;
    board->all.width = board->xsize;
    board->all.height = board->ysize;

    if( ( mode != 1 ) || MOBILE )
    { // only for update or fullscreen or mobile
        board->all.x = ( board->max_xsize - board->xsize ) / 2;
        board->all.y = ( board->max_ysize - board->ysize ) / 2;
    }

    if( mode )
        if( init_bitmaps( board ) )
            return -1;

    if( update_bitmaps( game, board ) )
        return -1;

    create_font_symbols( board );

    return 0;
}
