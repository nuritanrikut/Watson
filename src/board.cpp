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
      panel( TiledBlock::BLOCK_TYPE::TB_PANEL,
             &all,
             PANEL_BACKGROUND_COLOR,
             PANEL_BORDER_COLOR,
             TiledBlock::Visibility::Visible ),
      vclue( TiledBlock::BLOCK_TYPE::TB_ALL, &all, NULL_COLOR, NULL_COLOR, TiledBlock::Visibility::Visible ),
      hclue( TiledBlock::BLOCK_TYPE::TB_ALL, &all, NULL_COLOR, NULL_COLOR, TiledBlock::Visibility::Visible ),
      info_panel( TiledBlock::BLOCK_TYPE::TB_ALL, &all, NULL_COLOR, NULL_COLOR, TiledBlock::Visibility::Visible ),
      time_panel( TiledBlock::BLOCK_TYPE::TB_ALL, &all, NULL_COLOR, NULL_COLOR, TiledBlock::Visibility::Visible ),
      all( TiledBlock::BLOCK_TYPE::TB_ALL, nullptr, NULL_COLOR, NULL_COLOR, TiledBlock::Visibility::Visible ),
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
      background_color(),
      time_bmp( nullptr ),
      info_text_bmp( nullptr ),
      text_font( nullptr )
{
}

void Board::destroy_board()
{ // note that vclue.sub and hclue.sub are destroyed elsewhere
    int i;
    int j;

    for( i = 0; i < number_of_columns; i++ )
    {
        for( j = 0; j < column_height; j++ )
        {
            for( int k = 0; k < number_of_columns; k++ )
            {
                delete panel.sub[i]->sub[j]->sub[k];
            }
            panel.sub[i]->sub[j]->sub.clear();
            delete panel.sub[i]->sub[j];
        }
        panel.sub[i]->sub.clear();
        delete panel.sub[i];
    }
    panel.sub.clear();

    for( i = 0; i < time_panel.number_of_subblocks; i++ )
    {
        delete time_panel.sub[i];
    }
    time_panel.sub.clear();

    destroy_board_clue_blocks();
    destroy_all_bitmaps( this );
    all.sub.clear();
    clue_bmp.clear();
    clue_tiledblock.clear();
}

void Board::destroy_board_clue_blocks()
{
    if( !hclue.sub.empty() )
    {
        for( int i = 0; i < hclue.number_of_subblocks; i++ )
        {
            delete hclue.sub[i];
        }
        hclue.sub.clear();
    }

    if( !vclue.sub.empty() )
    {
        for( int i = 0; i < vclue.number_of_subblocks; i++ )
        {
            delete vclue.sub[i];
        }
        vclue.sub.clear();
    }
}

void Board::clear_info_panel()
{
    info_panel.bmp = nullptr;
}

//xxx todo: better board generation. Fix tile size first, then compute everything?
// mode: 1 = create, 0 = update, 2 = create fullscreen
auto Board::create_board( GameData *game_data, CreateMode mode ) -> int
{
    int column_w;
    int column_h;
    int block_w;
    int block_h;
    int block_space;
    int hclue_tile_w;
    int hclue_tile_h;
    int vclue_tile_w;
    int vclue_tile_h;
    int panel_tile_w;
    int panel_tile_h;
    int cus;

    clue_unit_space = CLUE_UNIT_SPACE;

    if( mode != CreateMode::Update )
    {
        clue_bmp.resize( game_data->clue_n, nullptr );
        clue_tiledblock.resize( game_data->clue_n, nullptr );
    }

    if( max_height * INFO_PANEL_PORTION - 2 * INFO_PANEL_MARGIN < 32 )
    { // guarantee height of 32 pixels in info panel
        max_height = ( 32 + 2 * INFO_PANEL_MARGIN ) / INFO_PANEL_PORTION;
    }

    width = max_width;
    height = max_height * ( 1.0 - INFO_PANEL_PORTION );

    background_color = BACKGROUND_COLOR;
    dragging = nullptr;
    highlight = nullptr;
    rule_out = nullptr;

    // panel dimensions
    panel.x = PANEL_MARGIN;
    panel.y = PANEL_MARGIN;
    panel.width = ( width - 2 * PANEL_MARGIN - width * HCLUEBOX_PORTION - 2 * HCLUEBOX_MARGIN );
    panel.height = ( height - 2 * PANEL_MARGIN - height * VCLUEBOX_PORTION - 2 * VCLUEBOX_MARGIN );
    panel.margin = PANEL_MARGIN;
    panel.draw_border = 1;
    panel.number_of_subblocks = number_of_columns; // number_of_columns subpanels
    panel.index = 0;
    panel.bmp = nullptr;

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
    if( mode != CreateMode::Update )
    {
        panel.sub.resize( number_of_columns, nullptr );
    }

    for( int i = 0; i < number_of_columns; i++ )
    {
        if( mode != CreateMode::Update )
        {
            if( !panel.sub[i] )
            {
                panel.sub[i] = new TiledBlock( TiledBlock::BLOCK_TYPE::TB_PANEL_COLUMN,
                                               &panel,
                                               PANEL_COLUMN_BACKGROUND_COLOR,
                                               PANEL_COLUMN_BORDER_COLOR,
                                               TiledBlock::Visibility::Visible );
            }
            panel.sub[i]->draw_border = 1; // draw boundary
            panel.sub[i]->number_of_subblocks = column_height;
            panel.sub[i]->sub.resize( column_height, nullptr );
            panel.sub[i]->index = i;
            panel.sub[i]->bmp = nullptr; // no background image
        }

        panel.sub[i]->width = column_w;
        panel.sub[i]->height = panel.height;
        panel.sub[i]->margin = 0;
        panel.sub[i]->x = i * ( panel.sub[i]->width + PANEL_COLUMN_SPACE ); // position relative to parent
        panel.sub[i]->y = 0;

        // panel blocks
        for( int j = 0; j < column_height; j++ )
        {
            if( mode != CreateMode::Update )
            {
                if( !panel.sub[i]->sub[j] )
                {
                    panel.sub[i]->sub[j] = new TiledBlock( TiledBlock::BLOCK_TYPE::TB_PANEL_BLOCK,
                                                           panel.sub[i],
                                                           NULL_COLOR,
                                                           NULL_COLOR,
                                                           TiledBlock::Visibility::Visible );
                }
                panel.sub[i]->sub[j]->draw_border = 0;
                panel.sub[i]->sub[j]->number_of_subblocks = number_of_columns;
                panel.sub[i]->sub[j]->sub.resize( number_of_columns, nullptr );
                panel.sub[i]->sub[j]->index = j;
                panel.sub[i]->sub[j]->bmp = nullptr;
            }
            panel.sub[i]->sub[j]->margin = PANEL_BLOCK_MARGIN;
            panel.sub[i]->sub[j]->width = block_w;
            panel.sub[i]->sub[j]->height = PANEL_TILE_ROWS[number_of_columns] * panel_tile_h
                                           + ( PANEL_TILE_ROWS[number_of_columns] - 1 ) * PANEL_TILE_SPACE;
            panel.sub[i]->sub[j]->x = PANEL_BLOCK_MARGIN;
            panel.sub[i]->sub[j]->y =
                j * ( panel.sub[i]->sub[j]->height + block_space + 2 * PANEL_BLOCK_MARGIN ) + PANEL_BLOCK_MARGIN;

            // panel tiles
            for( int k = 0; k < number_of_columns; k++ )
            {
                if( mode != CreateMode::Update )
                {
                    if( !panel.sub[i]->sub[j]->sub[k] )
                    {
                        panel.sub[i]->sub[j]->sub[k] = new TiledBlock( TiledBlock::BLOCK_TYPE::TB_PANEL_TILE,
                                                                       panel.sub[i]->sub[j],
                                                                       NULL_COLOR,
                                                                       PANEL_TILE_BORDER_COLOR,
                                                                       TiledBlock::Visibility::Visible );
                    }
                    panel.sub[i]->sub[j]->sub[k]->draw_border = 1;
                    panel.sub[i]->sub[j]->sub[k]->number_of_subblocks = 0;
                    panel.sub[i]->sub[j]->sub[k]->sub.clear();
                    panel.sub[i]->sub[j]->sub[k]->index = k;
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
    vclue.background_color = CLUE_PANEL_BACKGROUND_COLOR;
    vclue.border_color = CLUE_PANEL_BORDER_COLOR;
    vclue.draw_border = 1;
    vclue.bmp = nullptr;
    vclue.width = panel.width;
    vclue.height = height - panel.height - 2 * panel.margin - 2 * vclue.margin;
    vclue.parent = &all;
    vclue.sub.clear();             // later change
    vclue.number_of_subblocks = 0; // later change
    vclue.hidden = TiledBlock::Visibility::Visible;

    // HCluebox dimension
    hclue.margin = HCLUEBOX_MARGIN;
    hclue.x = panel.x + panel.width + panel.margin + hclue.margin;
    hclue.y = panel.y;
    hclue.width = width - panel.width - 2 * panel.margin - 2 * hclue.margin;
    hclue.height = height - 2 * HCLUEBOX_MARGIN;
    hclue.background_color = CLUE_PANEL_BACKGROUND_COLOR;
    hclue.border_color = CLUE_PANEL_BORDER_COLOR;
    hclue.draw_border = 1;
    hclue.parent = &all;
    hclue.number_of_subblocks = 0; // later change
    hclue.sub.clear();             // later change
    hclue.hidden = TiledBlock::Visibility::Visible;

    // count number of vclues and hclues
    if( mode != CreateMode::Update )
    {
        number_of_vclues = 0;
        number_of_hclues = 0;
        for( int i = 0; i < game_data->clue_n; i++ )
        {
            if( is_vclue( game_data->clues[i].rel ) )
            {
                number_of_vclues++;
            }
            else
            {
                number_of_hclues++;
            }
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
    vclue.sub.resize( vclue.number_of_subblocks, nullptr );
    for( int i = 0; i < vclue.number_of_subblocks; i++ )
    {
        if( !vclue.sub[i] )
        {
            vclue.sub[i] = new TiledBlock( TiledBlock::BLOCK_TYPE::TB_VCLUE_TILE,
                                           &( vclue ),
                                           CLUE_TILE_BACKGROUND_COLOR,
                                           CLUE_TILE_BORDER_COLOR,
                                           TiledBlock::Visibility::Visible );
        }
        vclue.sub[i]->width = vclue_tile_w;
        vclue.sub[i]->height = vclue_tile_h;
        vclue.sub[i]->margin = CLUE_TILE_MARGIN;
        vclue.sub[i]->x = vclue.sub[i]->margin + i * ( vclue.sub[i]->width + 2 * vclue.sub[i]->margin );
        vclue.sub[i]->y = vclue.sub[i]->margin;
        vclue.sub[i]->draw_border = 1;
        vclue.sub[i]->number_of_subblocks = 0;
        vclue.sub[i]->sub.clear();
        vclue.sub[i]->index = -1;
        vclue.sub[i]->bmp = nullptr;
    }

    //create Hclue tiles
    hclue.sub.resize( hclue.number_of_subblocks, nullptr );
    for( int i = 0; i < hclue.number_of_subblocks; i++ )
    {
        if( !hclue.sub[i] )
        {
            hclue.sub[i] = new TiledBlock( TiledBlock::BLOCK_TYPE::TB_HCLUE_TILE,
                                           &( hclue ),
                                           CLUE_TILE_BACKGROUND_COLOR,
                                           CLUE_TILE_BORDER_COLOR,
                                           TiledBlock::Visibility::Visible );
        }
        hclue.sub[i]->width = hclue_tile_w;
        hclue.sub[i]->height = hclue_tile_h;
        hclue.sub[i]->margin = CLUE_TILE_MARGIN;
        hclue.sub[i]->x = hclue.sub[i]->margin;
        hclue.sub[i]->y = hclue.sub[i]->margin + i * ( hclue.sub[i]->height + 2 * hclue.sub[i]->margin );
        hclue.sub[i]->draw_border = 1;
        hclue.sub[i]->number_of_subblocks = 0;
        hclue.sub[i]->sub.clear();
        hclue.sub[i]->index = -1;
        hclue.sub[i]->bmp = nullptr;
    }

    //load Hclues
    {
        int j = 0;
        int k = 0;
        for( int i = 0; i < game_data->clue_n; i++ )
        {
            if( is_vclue( game_data->clues[i].rel ) )
            {
                vclue.sub[j]->index = i;
                vclue.sub[j]->hidden = game_data->clues[i].hidden ? TiledBlock::Visibility::PartiallyHidden
                                                                  : TiledBlock::Visibility::Visible;
                vclue.sub[j]->bmp = &( clue_bmp[i] );
                clue_tiledblock[i] = vclue.sub[j];
                j++;
            }
            else
            {
                hclue.sub[k]->index = i;
                hclue.sub[k]->hidden = game_data->clues[i].hidden ? TiledBlock::Visibility::PartiallyHidden
                                                                  : TiledBlock::Visibility::Visible;
                hclue.sub[k]->bmp = &( clue_bmp[i] );
                clue_tiledblock[i] = hclue.sub[k];
                k++;
            }
        }
    }

    //resize board to fit tight
    width = hclue.x + hclue.width + hclue.margin;
    height = std::max( hclue.height + 2 * hclue.margin, vclue.y + vclue.height + vclue.margin );

    for( int i = 1; i < number_of_columns; i++ )
    {
        panel.sub[i]->x += i * std::min( int( max_width * H_FLEX_FACTOR ), ( max_width - width ) / number_of_columns );
    }
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
    info_panel.border_color = INFO_PANEL_BORDER_COLOR;
    info_panel.background_color = INFO_PANEL_BACKGROUND_COLOR;
    info_panel.draw_border = 1;
    info_panel.number_of_subblocks = 0;
    info_panel.sub.clear();
    info_panel.parent = &all;
    info_panel.type = TiledBlock::BLOCK_TYPE::TB_INFO_PANEL;
    info_panel.index = 0;
    info_panel.hidden = TiledBlock::Visibility::Visible;
    info_panel.bmp = nullptr;

    // time panel
    time_panel.y = info_panel.y;
    time_panel.x = hclue.x;
    time_panel.width = hclue.width;
    time_panel.height = info_panel.height;
    time_panel.margin = info_panel.margin;
    time_panel.background_color = TIME_PANEL_BACKGROUND_COLOR;
    time_panel.border_color = TIME_PANEL_BORDER_COLOR;
    time_panel.draw_border = 1;
    time_panel.number_of_subblocks = 1 + TIME_PANEL_BUTTONS;
    time_panel.parent = &all;
    time_panel.type = TiledBlock::BLOCK_TYPE::TB_TIME_PANEL;
    time_panel.index = 0;
    time_panel.hidden = TiledBlock::Visibility::Visible;
    time_panel.bmp = nullptr;

    if( mode != CreateMode::Update )
    {
        // if board is being created
        time_panel.sub.resize( 6, nullptr );
    }

    // timer
    if( !time_panel.sub[0] )
    {
        time_panel.sub[0] = new TiledBlock( TiledBlock::BLOCK_TYPE::TB_TIMER,
                                            &time_panel,
                                            TIME_PANEL_BACKGROUND_COLOR,
                                            NULL_COLOR,
                                            TiledBlock::Visibility::Visible );
    }
    time_panel.sub[0]->x = 2;
    time_panel.sub[0]->y = 4;
    time_panel.sub[0]->height = 16;
    time_panel.sub[0]->width = time_panel.width - 4;
    time_panel.sub[0]->margin = 0;
    time_panel.sub[0]->draw_border = 0;
    time_panel.sub[0]->number_of_subblocks = 0;
    time_panel.sub[0]->sub.clear();
    time_panel.sub[0]->index = 0;
    time_panel.sub[0]->bmp = &time_bmp;

    static constexpr std::array types = { TiledBlock::BLOCK_TYPE::TB_TIMER,
                                          TiledBlock::BLOCK_TYPE::TB_BUTTON_CLUE,
                                          TiledBlock::BLOCK_TYPE::TB_BUTTON_HELP,
                                          TiledBlock::BLOCK_TYPE::TB_BUTTON_SETTINGS,
                                          TiledBlock::BLOCK_TYPE::TB_BUTTON_UNDO };

    for( int i = 0; i < TIME_PANEL_BUTTONS; i++ )
    { // buttons
        if( !time_panel.sub[i + 1] )
        {
            time_panel.sub[i + 1] =
                new TiledBlock( types[i + 1], &time_panel, NULL_COLOR, WHITE_COLOR, TiledBlock::Visibility::Visible );
        }
        time_panel.sub[i + 1]->height = std::min( ( time_panel.height - 24 ), time_panel.width / 5 );
        time_panel.sub[i + 1]->width = time_panel.sub[i + 1]->height;
        time_panel.sub[i + 1]->y = ( ( time_panel.height + ( time_panel.sub[0]->y + time_panel.sub[0]->height ) )
                                     - time_panel.sub[i + 1]->height )
                                   / 2;
        time_panel.sub[i + 1]->x =
            ( ( i + 1 ) * time_panel.width + ( i - TIME_PANEL_BUTTONS ) * time_panel.sub[i + 1]->width )
            / ( TIME_PANEL_BUTTONS + 1 );
        time_panel.sub[i + 1]->margin = 0;
        time_panel.sub[i + 1]->draw_border = 0;
        time_panel.sub[i + 1]->number_of_subblocks = 0;
        time_panel.sub[i + 1]->sub.clear();
        time_panel.sub[i + 1]->bmp = &button_bmp_scaled[i];
        time_panel.sub[i + 1]->index = 0;
    }

    // collect TiledBlocks into an array for convenience
    // and create settings block
    if( mode != CreateMode::Update )
    { // only if board is being created
        all.x = 0;
        all.y = 0;
        all.margin = 0;
        all.border_color = NULL_COLOR;
        all.background_color = background_color;
        all.draw_border = 0;
        all.parent = nullptr;
        all.number_of_subblocks = 5;
        all.sub.resize( all.number_of_subblocks, nullptr );
        all.type = TiledBlock::BLOCK_TYPE::TB_ALL;
        all.index = 0;
        all.hidden = TiledBlock::Visibility::Visible;
        all.bmp = nullptr;

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

    if( ( mode != CreateMode::Create ) )
    { // only for update or fullscreen or mobile
        all.x = ( max_width - width ) / 2;
        all.y = ( max_height - height ) / 2;
    }

    if( mode != CreateMode::Update )
    {
        if( init_bitmaps( this ) )
        {
            return -1;
        }
    }

    if( update_bitmaps( game_data, this ) )
    {
        return -1;
    }

    create_font_symbols( this );

    return 0;
}
