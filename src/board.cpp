#include "board.hpp"

#include <algorithm>

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>

#include <spdlog/spdlog.h>

#include "allegro_stuff.hpp"
#include "bitmaps.hpp"

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

int PANEL_TILE_COLUMNS[9] = { 0, 1, 2, 2, 2, 3, 3, 4, 4 };
int PANEL_TILE_ROWS[9] = { 0, 1, 1, 2, 2, 2, 2, 2, 2 };

Board::Board()
    : number_of_columns( 0 ),
      column_height( 0 ),
      width( 0 ),
      height( 0 ),
      max_width( 0 ),
      max_height( 0 ),
      panel(),
      vclue(),
      hclue(),
      info_panel(),
      time_panel(),
      all(),
      clue_tiledblock( nullptr ),
      dragging( nullptr ),
      highlight( nullptr ),
      rule_out( nullptr ),
      zoom( nullptr ),
      zoom_transform(),
      zoom_transform_inv(),
      identity_transform(),
      blink( false ),
      panel_tile_size( 0 ),
      clue_unit_size( 0 ),
      clue_unit_space( 0 ),
      number_of_hclues( 0 ),
      number_of_vclues( 0 ),
      dragging_origin_x( 0 ),
      dragging_origin_y( 0 ),
      dragging_relative_position_of_grabbing_x( 0 ),
      dragging_relative_position_of_grabbing_y( 0 ),
      type_of_tiles( 0 ),
      time_start( 0.0 ),
      bg_color(),
      time_bmp( nullptr ),
      info_text_bmp( nullptr ),
      text_font( nullptr )
{
}

void Board::destroy_board()
{ // note that vclue.sub and hclue.sub are destroyed elsewhere
    int i, j;

    for( i = 0; i < number_of_columns; i++ )
    {
        for( j = 0; j < column_height; j++ )
        {
            nfree( panel.sub[i]->sub[j]->sub );
        }
        nfree( panel.sub[i]->sub );
    }
    nfree( panel.sub );

    for( i = 0; i < time_panel.number_of_subblocks; i++ )
        nfree( time_panel.sub[i] );

    destroy_board_clue_blocks();
    destroy_all_bitmaps( this );
    nfree( all.sub );
    nfree( clue_bmp );
    nfree( clue_tiledblock );
}

void Board::destroy_board_clue_blocks()
{
    int i;

    if( hclue.sub )
    {
        for( i = 0; i < hclue.number_of_subblocks; i++ )
        {
            nfree( hclue.sub[i] );
        }
        nfree( hclue.sub );
    }

    if( vclue.sub )
    {
        for( i = 0; i < vclue.number_of_subblocks; i++ )
        {
            nfree( vclue.sub[i] );
        }
        nfree( vclue.sub );
    }
}

void Board::clear_info_panel()
{
    info_panel.bmp = NULL;
}

//xxx todo: better board generation. Fix tile size first, then compute everything?
// mode: 1 = create, 0 = update, 2 = create fullscreen
int Board::create_board( GameData *game_data, int mode )
{
    int i, j, k;
    int column_w, column_h, block_w, block_h, block_space;
    int hclue_tile_w, hclue_tile_h, vclue_tile_w, vclue_tile_h;
    int panel_tile_w, panel_tile_h;
    int cus;

    clue_unit_space = CLUE_UNIT_SPACE;

    if( mode )
    {
        clue_bmp = static_cast<ALLEGRO_BITMAP **>( malloc( game_data->clue_n * sizeof( *clue_bmp ) ) );
        clue_tiledblock = static_cast<TiledBlock **>( malloc( game_data->clue_n * sizeof( *clue_tiledblock ) ) );
    }

    if( max_height * INFO_PANEL_PORTION - 2 * INFO_PANEL_MARGIN < 32 ) // guarantee height of 32 pixels in info panel
        max_height = ( 32 + 2 * INFO_PANEL_MARGIN ) / INFO_PANEL_PORTION;

    width = max_width;
    height = max_height * ( 1.0 - INFO_PANEL_PORTION );

    bg_color = BG_COLOR;
    dragging = NULL;
    highlight = NULL;
    rule_out = NULL;

    // panel dimensions
    panel.x = PANEL_MARGIN;
    panel.y = PANEL_MARGIN;
    panel.width = ( width - 2 * PANEL_MARGIN - width * HCLUEBOX_PORTION - 2 * HCLUEBOX_MARGIN );
    panel.height = ( height - 2 * PANEL_MARGIN - height * VCLUEBOX_PORTION - 2 * VCLUEBOX_MARGIN );
    panel.margin = PANEL_MARGIN;
    panel.bd_color = PANEL_BD_COLOR;
    panel.bg_color = PANEL_BG_COLOR;
    panel.bd = 1;
    panel.number_of_subblocks = number_of_columns; // number_of_columns subpanels
    panel.parent = &all;
    panel.type = TB_PANEL;
    panel.index = 0;
    panel.hidden = 0;
    panel.bmp = NULL;

    // for nons-square panel tiles:
    panel_tile_w = ( ( panel.width - ( number_of_columns - 1 ) * PANEL_COLUMN_SPACE ) / number_of_columns
                     - 2 * PANEL_BLOCK_MARGIN - ( PANEL_TILE_COLUMNS[number_of_columns] - 1 ) * PANEL_TILE_SPACE )
                   / ( PANEL_TILE_COLUMNS[number_of_columns] );
    panel_tile_h =
        ( panel.height
          - column_height * ( ( PANEL_TILE_ROWS[number_of_columns] - 1 ) * PANEL_TILE_SPACE + 2 * PANEL_BLOCK_MARGIN ) )
        / ( column_height * PANEL_TILE_ROWS[number_of_columns] ); //ok go

    // make square tiles
    panel_tile_size = std::min( panel_tile_w, panel_tile_h );
    panel_tile_w = panel_tile_size; // to use below
    panel_tile_h = panel_tile_size; // to use below

    block_w = PANEL_TILE_COLUMNS[number_of_columns] * panel_tile_w
              + ( PANEL_TILE_COLUMNS[number_of_columns] - 1 ) * PANEL_TILE_SPACE;
    block_h = PANEL_TILE_ROWS[number_of_columns] * panel_tile_h
              + ( PANEL_TILE_ROWS[number_of_columns] - 1 ) * PANEL_TILE_SPACE;
    column_w = ( block_w + 2 * PANEL_BLOCK_MARGIN );
    block_space = 0; // don't separate blocks too much
    column_h = column_height * ( block_h + 2 * PANEL_BLOCK_MARGIN );

    // adjust panel dimensions to account for integer arithmetic
    panel.width = number_of_columns * column_w + ( number_of_columns - 1 ) * PANEL_COLUMN_SPACE;
    panel.height = column_h;

    // panel columns
    if( mode )
        panel.sub = static_cast<TiledBlock **>( malloc( number_of_columns * sizeof( struct TiledBlock * ) ) );
    for( i = 0; i < number_of_columns; i++ )
    {
        if( mode )
        {
            panel.sub[i] = static_cast<TiledBlock *>( malloc( sizeof( struct TiledBlock ) ) );
            panel.sub[i]->bg_color = PANEL_COLUMN_BG_COLOR;
            panel.sub[i]->bd_color = PANEL_COLUMN_BD_COLOR;
            panel.sub[i]->parent = &panel;
            panel.sub[i]->bd = 1; // draw boundary
            panel.sub[i]->number_of_subblocks = column_height;
            panel.sub[i]->sub = static_cast<TiledBlock **>( malloc( column_height * sizeof( struct TiledBlock * ) ) );
            panel.sub[i]->type = TB_PANEL_COLUMN;
            panel.sub[i]->index = i;
            panel.sub[i]->hidden = 0;
            panel.sub[i]->bmp = NULL; // no background image
        }

        panel.sub[i]->width = column_w;
        panel.sub[i]->height = panel.height;
        panel.sub[i]->margin = 0;
        panel.sub[i]->x = i * ( panel.sub[i]->width + PANEL_COLUMN_SPACE ); // position relative to parent
        panel.sub[i]->y = 0;

        // panel blocks
        for( j = 0; j < column_height; j++ )
        {
            if( mode )
            {
                panel.sub[i]->sub[j] = static_cast<TiledBlock *>( malloc( sizeof( struct TiledBlock ) ) );
                panel.sub[i]->sub[j]->bd_color = NULL_COLOR;
                panel.sub[i]->sub[j]->bg_color = NULL_COLOR;
                panel.sub[i]->sub[j]->bd = 0;
                panel.sub[i]->sub[j]->number_of_subblocks = number_of_columns;
                panel.sub[i]->sub[j]->sub =
                    static_cast<TiledBlock **>( malloc( number_of_columns * sizeof( struct TiledBlock * ) ) );
                panel.sub[i]->sub[j]->parent = panel.sub[i];
                panel.sub[i]->sub[j]->type = TB_PANEL_BLOCK;
                panel.sub[i]->sub[j]->index = j;
                panel.sub[i]->sub[j]->hidden = 0;
                panel.sub[i]->sub[j]->bmp = NULL;
            }
            panel.sub[i]->sub[j]->margin = PANEL_BLOCK_MARGIN;
            panel.sub[i]->sub[j]->width = block_w;
            panel.sub[i]->sub[j]->height = PANEL_TILE_ROWS[number_of_columns] * panel_tile_h
                                           + ( PANEL_TILE_ROWS[number_of_columns] - 1 ) * PANEL_TILE_SPACE;
            panel.sub[i]->sub[j]->x = PANEL_BLOCK_MARGIN;
            panel.sub[i]->sub[j]->y =
                j * ( panel.sub[i]->sub[j]->height + block_space + 2 * PANEL_BLOCK_MARGIN ) + PANEL_BLOCK_MARGIN;

            // panel tiles
            for( k = 0; k < number_of_columns; k++ )
            {
                if( mode )
                {
                    panel.sub[i]->sub[j]->sub[k] = static_cast<TiledBlock *>( malloc( sizeof( struct TiledBlock ) ) );
                    panel.sub[i]->sub[j]->sub[k]->bd_color = PANEL_TILE_BD_COLOR;
                    panel.sub[i]->sub[j]->sub[k]->bg_color = NULL_COLOR;
                    panel.sub[i]->sub[j]->sub[k]->bd = 1;
                    panel.sub[i]->sub[j]->sub[k]->number_of_subblocks = 0;
                    panel.sub[i]->sub[j]->sub[k]->sub = NULL;
                    panel.sub[i]->sub[j]->sub[k]->parent = panel.sub[i]->sub[j];
                    panel.sub[i]->sub[j]->sub[k]->type = TB_PANEL_TILE;
                    panel.sub[i]->sub[j]->sub[k]->index = k;
                    panel.sub[i]->sub[j]->sub[k]->hidden = 0;
                }
                panel.sub[i]->sub[j]->sub[k]->margin = 0;
                panel.sub[i]->sub[j]->sub[k]->width = panel_tile_w;
                panel.sub[i]->sub[j]->sub[k]->height = panel_tile_h;
                panel.sub[i]->sub[j]->sub[k]->x = ( k % PANEL_TILE_COLUMNS[number_of_columns] ) * panel_tile_w;
                panel.sub[i]->sub[j]->sub[k]->y = ( k / PANEL_TILE_COLUMNS[number_of_columns] ) * panel_tile_h;
                panel.sub[i]->sub[j]->sub[k]->bmp = &( panel_tile_bmp[j][k] );
            }
        }
    }

    // VCluebox dimensions
    vclue.margin = VCLUEBOX_MARGIN;
    vclue.y = panel.y + panel.height + panel.margin + vclue.margin;
    vclue.x = vclue.margin;
    vclue.bg_color = CLUE_PANEL_BG_COLOR;
    vclue.bd_color = CLUE_PANEL_BD_COLOR;
    vclue.bd = 1;
    vclue.bmp = NULL;
    vclue.width = panel.width;
    vclue.height = height - panel.height - 2 * panel.margin - 2 * vclue.margin;
    vclue.parent = &all;
    vclue.sub = NULL;              // later change
    vclue.number_of_subblocks = 0; // later change
    vclue.hidden = 0;

    // HCluebox dimension
    hclue.margin = HCLUEBOX_MARGIN;
    hclue.x = panel.x + panel.width + panel.margin + hclue.margin;
    hclue.y = panel.y;
    hclue.width = width - panel.width - 2 * panel.margin - 2 * hclue.margin;
    hclue.height = height - 2 * HCLUEBOX_MARGIN;
    hclue.bg_color = CLUE_PANEL_BG_COLOR;
    hclue.bd_color = CLUE_PANEL_BD_COLOR;
    hclue.bd = 1;
    hclue.parent = &all;
    hclue.number_of_subblocks = 0; // later change
    hclue.sub = NULL;              // later change
    hclue.hidden = 0;

    // count number of vclues and hclues
    if( mode )
    {
        number_of_vclues = 0;
        number_of_hclues = 0;
        for( i = 0; i < game_data->clue_n; i++ )
        {
            if( is_vclue( game_data->clue[i].rel ) )
                number_of_vclues++;
            else
                number_of_hclues++;
        }
    }
    //xxx todo: allow multiple columns/rows of hclues/vclues

    cus = std::min( hclue.width - 2 * CLUE_TILE_MARGIN - 2 * CLUE_UNIT_SPACE,
                    vclue.height - 2 * CLUE_TILE_MARGIN - 2 * CLUE_UNIT_SPACE )
          / 3;
    cus = std::min( cus,
                    ( panel.height + 2 * CLUE_UNIT_SPACE + 2 * CLUE_TILE_MARGIN + VCLUEBOX_MARGIN + panel.margin
                      - 2 * CLUE_TILE_MARGIN * number_of_hclues )
                        / ( std::max( number_of_hclues - 3, 1 ) ) );
    clue_unit_size = std::min( cus, ( panel.width / ( std::max( number_of_vclues, 1 ) ) - 2 * CLUE_TILE_MARGIN ) );

    vclue_tile_h = clue_unit_size * 3 + 2 * CLUE_UNIT_SPACE;
    vclue_tile_w = clue_unit_size;
    hclue_tile_h = clue_unit_size;
    hclue_tile_w = clue_unit_size * 3 + 2 * CLUE_UNIT_SPACE;

    // update hclue and vclue to fit tight
    hclue.width = hclue_tile_w + 2 * CLUE_TILE_MARGIN;
    hclue.height = panel.height + panel.margin + vclue.margin + vclue_tile_h + 2 * CLUE_TILE_MARGIN;
    vclue.height = vclue_tile_h + 2 * CLUE_TILE_MARGIN;
    vclue.width = panel.width;
    vclue.number_of_subblocks = vclue.width / ( clue_unit_size + 2 * CLUE_TILE_MARGIN );
    hclue.number_of_subblocks = hclue.height / ( clue_unit_size + 2 * CLUE_TILE_MARGIN );

    //fit tight again
    hclue.height = hclue.number_of_subblocks * ( hclue_tile_h + 2 * CLUE_TILE_MARGIN );
    vclue.width = vclue.number_of_subblocks * ( vclue_tile_w + 2 * CLUE_TILE_MARGIN );

    //create Vclue tiles
    vclue.sub = static_cast<TiledBlock **>( malloc( vclue.number_of_subblocks * sizeof( struct TiledBlock * ) ) );
    for( i = 0; i < vclue.number_of_subblocks; i++ )
    {
        vclue.sub[i] = static_cast<TiledBlock *>( malloc( sizeof( struct TiledBlock ) ) );
        vclue.sub[i]->width = vclue_tile_w;
        vclue.sub[i]->height = vclue_tile_h;
        vclue.sub[i]->margin = CLUE_TILE_MARGIN;
        vclue.sub[i]->x = vclue.sub[i]->margin + i * ( vclue.sub[i]->width + 2 * vclue.sub[i]->margin );
        vclue.sub[i]->y = vclue.sub[i]->margin;
        vclue.sub[i]->bd_color = CLUE_TILE_BD_COLOR;
        vclue.sub[i]->bg_color = CLUE_TILE_BG_COLOR;
        vclue.sub[i]->bd = 1;
        vclue.sub[i]->number_of_subblocks = 0;
        vclue.sub[i]->sub = NULL;
        vclue.sub[i]->parent = &( vclue );
        vclue.sub[i]->type = TB_VCLUE_TILE;
        vclue.sub[i]->index = -1;
        vclue.sub[i]->hidden = 0;
        vclue.sub[i]->bmp = NULL;
    }

    //create Hclue tiles
    hclue.sub = static_cast<TiledBlock **>( malloc( hclue.number_of_subblocks * sizeof( struct TiledBlock * ) ) );
    for( i = 0; i < hclue.number_of_subblocks; i++ )
    {
        hclue.sub[i] = static_cast<TiledBlock *>( malloc( sizeof( struct TiledBlock ) ) );
        hclue.sub[i]->width = hclue_tile_w;
        hclue.sub[i]->height = hclue_tile_h;
        hclue.sub[i]->margin = CLUE_TILE_MARGIN;
        hclue.sub[i]->x = hclue.sub[i]->margin;
        hclue.sub[i]->y = hclue.sub[i]->margin + i * ( hclue.sub[i]->height + 2 * hclue.sub[i]->margin );
        ;
        hclue.sub[i]->bd_color = CLUE_TILE_BD_COLOR;
        hclue.sub[i]->bg_color = CLUE_TILE_BG_COLOR;
        hclue.sub[i]->bd = 1;
        hclue.sub[i]->number_of_subblocks = 0;
        hclue.sub[i]->sub = NULL;
        hclue.sub[i]->parent = &( hclue );
        hclue.sub[i]->type = TB_HCLUE_TILE;
        hclue.sub[i]->index = -1;
        hclue.sub[i]->hidden = 0;
        hclue.sub[i]->bmp = NULL;
    }

    //load Hclues
    j = 0;
    k = 0;
    for( i = 0; i < game_data->clue_n; i++ )
    {
        if( is_vclue( game_data->clue[i].rel ) )
        {
            vclue.sub[j]->index = i;
            vclue.sub[j]->hidden = game_data->clue[i].hidden;
            vclue.sub[j]->bmp = &( clue_bmp[i] );
            clue_tiledblock[i] = vclue.sub[j];
            j++;
        }
        else
        {
            hclue.sub[k]->index = i;
            hclue.sub[k]->hidden = game_data->clue[i].hidden;
            hclue.sub[k]->bmp = &( clue_bmp[i] );
            clue_tiledblock[i] = hclue.sub[k];
            k++;
        }
    }

    //resize board to fit tight
    width = hclue.x + hclue.width + hclue.margin;
    height = std::max( hclue.height + 2 * hclue.margin, vclue.y + vclue.height + vclue.margin );

    for( i = 1; i < number_of_columns; i++ )
        panel.sub[i]->x += i * std::min( int( max_width * H_FLEX_FACTOR ), ( max_width - width ) / number_of_columns );
    hclue.x +=
        number_of_columns * std::min( int( max_width * H_FLEX_FACTOR ), ( max_width - width ) / number_of_columns );

    info_panel.height = (float)max_height * INFO_PANEL_PORTION - 2 * INFO_PANEL_MARGIN;
    vclue.y += std::min( ( max_height - height - info_panel.height - 2 * info_panel.margin ) / 2, 30 );
    info_panel.y = vclue.y + vclue.height + vclue.margin + INFO_PANEL_MARGIN
                   + std::min( ( max_height - height - info_panel.height - 2 * INFO_PANEL_MARGIN ) / 2, 30 );
    hclue.y = ( info_panel.y - hclue.height ) / 2;

    // center vclues

    panel.width = panel.sub[number_of_columns - 1]->x + panel.sub[number_of_columns - 1]->width - panel.sub[0]->x;
    vclue.x = ( panel.width - vclue.width ) / 2;

    // info panel
    info_panel.x = panel.x;
    info_panel.width = hclue.x - hclue.margin - INFO_PANEL_MARGIN - panel.x; // assumes margins are equal
    info_panel.margin = INFO_PANEL_MARGIN;
    info_panel.bd_color = INFO_PANEL_BD_COLOR;
    info_panel.bg_color = INFO_PANEL_BG_COLOR;
    info_panel.bd = 1;
    info_panel.number_of_subblocks = 0;
    info_panel.sub = NULL;
    info_panel.parent = &all;
    info_panel.type = TB_INFO_PANEL;
    info_panel.index = 0;
    info_panel.hidden = 0;
    info_panel.bmp = NULL;

    // time panel
    time_panel.y = info_panel.y;
    time_panel.x = hclue.x;
    time_panel.width = hclue.width;
    time_panel.height = info_panel.height;
    time_panel.margin = info_panel.margin;
    time_panel.bg_color = TIME_PANEL_BG_COLOR;
    time_panel.bd_color = TIME_PANEL_BD_COLOR;
    time_panel.bd = 1;
    time_panel.number_of_subblocks = 1 + TIME_PANEL_BUTTONS;
    time_panel.parent = &all;
    time_panel.type = TB_TIME_PANEL;
    time_panel.index = 0;
    time_panel.hidden = 0;
    time_panel.bmp = NULL;

    if( mode )
    { // if board is being created
        time_panel.sub = static_cast<TiledBlock **>( malloc( 6 * sizeof( struct TiledBlock * ) ) );
        for( i = 0; i < 5; i++ )
            time_panel.sub[i] = static_cast<TiledBlock *>( malloc( sizeof( struct TiledBlock ) ) );
    }

    // timer
    time_panel.sub[0]->x = 2;
    time_panel.sub[0]->y = 4;
    time_panel.sub[0]->height = 16;
    time_panel.sub[0]->width = time_panel.width - 4;
    time_panel.sub[0]->margin = 0;
    time_panel.sub[0]->bd_color = NULL_COLOR;
    time_panel.sub[0]->bg_color = TIME_PANEL_BG_COLOR;
    time_panel.sub[0]->bd = 0;
    time_panel.sub[0]->number_of_subblocks = 0;
    time_panel.sub[0]->sub = NULL;
    time_panel.sub[0]->parent = &time_panel;
    time_panel.sub[0]->type = TB_TIMER;
    time_panel.sub[0]->index = 0;
    time_panel.sub[0]->hidden = 0;
    time_panel.sub[0]->bmp = &time_bmp;

    for( i = 0; i < TIME_PANEL_BUTTONS; i++ )
    { // buttons
        time_panel.sub[i + 1]->height = std::min( ( time_panel.height - 24 ), time_panel.width / 5 );
        time_panel.sub[i + 1]->width = time_panel.sub[i + 1]->height;
        time_panel.sub[i + 1]->y = ( ( time_panel.height + ( time_panel.sub[0]->y + time_panel.sub[0]->height ) )
                                     - time_panel.sub[i + 1]->height )
                                   / 2;
        time_panel.sub[i + 1]->x =
            ( ( i + 1 ) * time_panel.width + ( i - TIME_PANEL_BUTTONS ) * time_panel.sub[i + 1]->width )
            / ( TIME_PANEL_BUTTONS + 1 );
        time_panel.sub[i + 1]->margin = 0;
        time_panel.sub[i + 1]->bg_color = NULL_COLOR;
        time_panel.sub[i + 1]->bd_color = WHITE_COLOR;
        time_panel.sub[i + 1]->bd = 0;
        time_panel.sub[i + 1]->number_of_subblocks = 0;
        time_panel.sub[i + 1]->sub = NULL;
        time_panel.sub[i + 1]->parent = &time_panel;
        time_panel.sub[i + 1]->bmp = &button_bmp_scaled[i];
        time_panel.sub[i + 1]->index = 0;
        time_panel.sub[i + 1]->hidden = 0;
    }

    time_panel.sub[1]->type = TB_BUTTON_CLUE;
    time_panel.sub[2]->type = TB_BUTTON_HELP;
    time_panel.sub[3]->type = TB_BUTTON_SETTINGS;
    time_panel.sub[4]->type = TB_BUTTON_UNDO;

    // collect TiledBlocks into an array for convenience
    // and create settings block
    if( mode )
    { // only if board is being created
        all.x = 0;
        all.y = 0;
        all.margin = 0;
        all.bd_color = NULL_COLOR;
        all.bg_color = bg_color;
        all.bd = 0;
        all.parent = NULL;
        all.number_of_subblocks = 5;
        all.sub = static_cast<TiledBlock **>( malloc( all.number_of_subblocks * sizeof( TiledBlock * ) ) );
        all.type = TB_ALL;
        all.index = 0;
        all.hidden = 0;
        all.bmp = NULL;

        all.sub[0] = &panel;
        all.sub[1] = &hclue;
        all.sub[2] = &vclue;
        all.sub[3] = &time_panel;
        all.sub[4] = &info_panel;
    }

    // final size adjustment
    width = hclue.x + hclue.margin + hclue.width;
    height = info_panel.y + info_panel.height + info_panel.margin;
    all.width = width;
    all.height = height;

    if( ( mode != 1 ) )
    { // only for update or fullscreen or mobile
        all.x = ( max_width - width ) / 2;
        all.y = ( max_height - height ) / 2;
    }

    if( mode )
        if( init_bitmaps( this ) )
            return -1;

    if( update_bitmaps( game_data, this ) )
        return -1;

    create_font_symbols( this );

    return 0;
}
