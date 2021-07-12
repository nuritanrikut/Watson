#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "game.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_audio.h>

#include <spdlog/spdlog.h>

#include "macros.hpp"
#include "game_data.hpp"
#include "sound.hpp"
#include "tiled_block.hpp"
#include "board.hpp"
#include "allegro_stuff.hpp"
#include "bitmaps.hpp"
#include "dialog.hpp"
#include "text.hpp"
#include "gui.hpp"

Game::Game()
    : set(),
      nset(), // settings for new game
      gui( set, nset ),
      display( nullptr ),
      noexit( true ),
      restart( 0 ),
      redraw( false ),
      mouse_move( false ),
      keypress( false ),
      resizing( false ),
      resize_update( false ),
      mouse_button_down( 0 ),
      win_gui( false ),
      tb_down( nullptr ),
      tb_up( nullptr ),
      mouse_up_time( 0 ),
      mouse_down_time( 0 ),
      wait_for_double_click( false ),
      hold_click_check( 0 ),
      mbdown_x( 0 ),
      mbdown_y( 0 ),
      touch_down( false ),
      resize_time( 0 ),
      old_time( 0 ),
      blink_time( 0 ),
      play_time( 0 ),
      swap_mouse_buttons( false ),
      game_state( GAME_NULL ),
      desktop_width( 0 ),
      desktop_height( 0 ),
      fullscreen( false ),
      game_data(),
      board(),
      undo( nullptr )
{
}

bool Game::init()
{
    // seed random number generator. comment out for debug
    srand( (unsigned int)time( nullptr ) );

    SPDLOG_DEBUG( "Watson v" PRE_VERSION " - " PRE_DATE " has started." );
    if( init_allegro() )
        return false;

#ifndef _WIN32
    // use anti-aliasing if available (seems to cause problems in windows)
    al_set_new_display_option( ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST );
    al_set_new_display_option( ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST );
#endif

    fullscreen = false;

    // use vsync if available
    al_set_new_display_option( ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST );

    get_desktop_resolution( 0, &desktop_width, &desktop_height );

    if( fullscreen )
    {
        al_set_new_display_flags( ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL );
        display = al_create_display( desktop_width, desktop_height );
    }
    else
    {
        al_set_new_display_flags( ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE | ALLEGRO_OPENGL );
        display = al_create_display( 800, 600 );
    }

    if( !display )
    {
        SPDLOG_ERROR( "Failed to create display!" );
        return false;
    }
    else
    {
        SPDLOG_DEBUG( "Display created." );
    }

    al_set_target_backbuffer( display );
    al_clear_to_color( BLACK_COLOR );

    al_set_window_title( display, "Watson" );
    al_init_user_event_source( &gui.user_event_src );

    if( !load_game_f() )
    {
        set.saved = 1;
        SPDLOG_DEBUG( "Saved game found." );
    }
    else
    {
        SPDLOG_DEBUG( "No saved game found." );
    }

    return true;
}

bool Game::run()
{
    game_state = GAME_INTRO;

    gui.init_guis( 0, 0, al_get_display_width( display ), al_get_display_height( display ) );

    while( noexit )
    {
        game_loop();
    }

    return true;
}

bool Game::cleanup()
{
    destroy_everything();
    al_destroy_display( display );
    al_destroy_event_queue( gui.event_queue );

    return true;
}

void Game::draw_stuff()
{
    //xxx todo: there's still the issue that the timer and some fonts appear broken in some android devices
    // ex: samsung galaxy s2
    // probably has to do with memory->video bitmaps
    int x, y;

    al_clear_to_color( BLACK_COLOR );

    if( game_state == GAME_INTRO )
    {
        draw_title();
    }
    else
    {
        draw_TiledBlock( &board.all, 0, 0 );

        if( board.rule_out )
        {
            if( board.blink )
            {
                highlight_TiledBlock( board.rule_out );
                highlight_TiledBlock( board.highlight );
            }
        }
        else
        {
            if( board.highlight )
                highlight_TiledBlock( board.highlight );
        }

        if( board.dragging )
        { // redraw tile to get it on top
            get_TiledBlock_offset( board.dragging->parent, &x, &y );
            draw_TiledBlock( board.dragging, x, y );
        }

        if( board.zoom )
        {
            al_draw_filled_rectangle( 0, 0, board.max_width, board.max_height, al_premul_rgba( 0, 0, 0, 150 ) );
            al_use_transform( &board.zoom_transform );
            // draw dark background in case of transparent elements
            al_draw_filled_rectangle( board.zoom->x,
                                      board.zoom->y,
                                      board.zoom->x + board.zoom->width,
                                      board.zoom->y + board.zoom->height,
                                      board.zoom->parent->bg_color );
            draw_TiledBlock( board.zoom, 0, 0 );
            al_use_transform( &board.identity_transform );
        }
    }

    gui.draw_guis();
}

void Game::handle_mouse_click_panel_tile( TiledBlock *tiled_block, int mclick )
{
    if( game_state != GAME_PLAYING )
        return;

    int k = tiled_block->index;
    int j = tiled_block->parent->index;
    int i = tiled_block->parent->parent->index;
    if( mclick == 1 )
    {
        save_state();
        if( game_data.tile[i][j][k] )
        { // hide tile
            hide_tile_and_check( &game_data, i, j, k );
            if( !set.sound_mute )
                play_sound( SOUND_HIDE_TILE );
        }
    }
    else if( ( mclick == 2 ) || ( mclick == 4 ) )
    { // hold or right click
        if( game_data.tile[i][j][k] )
        {
            save_state();
            guess_tile( &game_data, i, j, k );
            if( !set.sound_mute )
                play_sound( SOUND_GUESS_TILE );
        }
        else
        { // tile was hidden, unhide
            if( !is_guessed( &game_data, j, k ) )
            {
                game_data.tile[i][j][k] = 1;
                if( !set.sound_mute )
                    play_sound( SOUND_UNHIDE_TILE );
            }
        }
    }
    update_board();
}

void Game::handle_mouse_click_panel_block( TiledBlock *tiled_block, int mclick )
{
    if( game_state != GAME_PLAYING )
        return;

    if( ( ( mclick == 2 ) || ( mclick == 4 ) )
        && ( game_data.guess[tiled_block->parent->index][tiled_block->index] >= 0 ) )
    {
        // we found guessed block - unguess it
        save_state();
        unguess_tile( &game_data, tiled_block->parent->index, tiled_block->index );
        if( !set.sound_mute )
            play_sound( SOUND_UNHIDE_TILE );
    }
    update_board();
}

void Game::handle_mouse_click_clue_tile( TiledBlock *tiled_block, int mx, int my, int mclick )
{
    if( game_state != GAME_PLAYING )
        return;

    if( tiled_block->bmp && ( tiled_block->index >= 0 ) )
    {
        if( ( mclick == 2 ) || ( mclick == 3 ) )
        { // toggle hide-show clue on double or right click
            tiled_block->hidden = tiled_block->hidden == TiledBlock::Visibility::Visible
                                      ? TiledBlock::Visibility::PartiallyHidden
                                      : TiledBlock::Visibility::Visible;
            game_data.clue[tiled_block->index].hidden = !game_data.clue[tiled_block->index].hidden;
            if( !set.sound_mute )
                play_sound( SOUND_HIDE_TILE );
        }
        else if( mclick == 1 )
        { // explain clue in info panel
            if( tiled_block->hidden != TiledBlock::Visibility::Visible )
            {
                if( !set.sound_mute )
                    play_sound( SOUND_CLICK );
                explain_clue( &game_data.clue[tiled_block->index] );
                board.highlight = tiled_block; // highlight clue
            }
        }
        else if( mclick == 4 )
        { // hold-click
            mouse_grab( mx, my );
        }
    }
}

void Game::handle_mouse_click_button_clue()
{
    if( game_state != GAME_PLAYING )
        return;

    if( !set.sound_mute )
        play_sound( SOUND_CLICK );
    show_hint();
}

void Game::handle_mouse_click_button_settings()
{
    if( !set.sound_mute )
        play_sound( SOUND_CLICK );
    gui.show_settings();
}

void Game::handle_mouse_click_button_help()
{
    if( !set.sound_mute )
        play_sound( SOUND_CLICK );
    gui.show_help();
}

void Game::handle_mouse_click_button_undo()
{
    execute_undo();
    update_board();
}

void Game::handle_mouse_click( TiledBlock *tiled_block, int mx, int my, int mclick )
{
    if( game_state == GAME_INTRO )
    {
        game_state = GAME_PLAYING;
        return;
    }

    if( swap_mouse_buttons )
    {
        if( mclick == 1 )
            mclick = 2;
        else if( mclick == 2 )
            mclick = 1;
    }

    board.clear_info_panel(); // remove text if there was any

    board.highlight = nullptr;
    board.rule_out = nullptr;

    gui.emit_event( EVENT_REDRAW );

    if( !tiled_block )
        return;

    if( game_state == GAME_OVER )
    {
        show_info_text( &board, al_ustr_new( "Press R to start a new puzzle." ) );
    }

    if( set.fat_fingers )
    {
        if( !board.zoom || ( tiled_block->parent != board.zoom ) )
        {
            if( ( ( tiled_block->parent ) && ( tiled_block->parent->type == TB_TIME_PANEL ) )
                || ( tiled_block->type == TB_TIME_PANEL ) )
                zoom_TB( &board.time_panel );
            else if( tiled_block->type == TB_PANEL_TILE )
                zoom_TB( board.zoom = tiled_block->parent );
            return;
        }
    }

    board.zoom = nullptr;

    switch( tiled_block->type )
    { // which board component was clicked
        case TB_PANEL_TILE:
            handle_mouse_click_panel_tile( tiled_block, mclick );
            break;

        case TB_PANEL_BLOCK:
            handle_mouse_click_panel_block( tiled_block, mclick );
            break;

        case TB_HCLUE_TILE:
        case TB_VCLUE_TILE:
            // check that this is a real clue
            handle_mouse_click_clue_tile( tiled_block, mx, my, mclick );
            break;

        case TB_BUTTON_CLUE: // time panel
            handle_mouse_click_button_clue();
            break;

        case TB_BUTTON_SETTINGS:
            handle_mouse_click_button_settings();
            break;

        case TB_BUTTON_HELP:
            handle_mouse_click_button_help();
            break;

        case TB_BUTTON_UNDO:
            handle_mouse_click_button_undo();
            break;

        case TB_BUTTON_TILES:
            gui.emit_event( EVENT_SWITCH_TILES );
            break;

        default:
            break;
    }
}

void Game::update_board()
{
    for( int i = 0; i < game_data.number_of_columns; i++ )
    {
        auto column = board.panel.sub[i];

        for( int j = 0; j < game_data.column_height; j++ )
        {
            auto block = column->sub[j];

            if( game_data.guess[i][j] >= 0 )
            {
                block->number_of_subblocks = 0;
                block->bmp = &( board.guess_bmp[j][game_data.guess[i][j]] );
            }
            else
            {
                block->number_of_subblocks = board.number_of_columns;
                block->bmp = nullptr;

                for( int k = 0; k < game_data.number_of_columns; k++ )
                {
                    if( game_data.tile[i][j][k] )
                    {
                        block->sub[k]->hidden = TiledBlock::Visibility::Visible;
                    }
                    else
                    {
                        block->sub[k]->hidden = TiledBlock::Visibility::PartiallyHidden;
                    }
                }
            }
        }
    }
}

void Game::mouse_grab( int mx, int my )
{
    gui.emit_event( EVENT_REDRAW );
    board.dragging = get_TiledBlock_at( mx, my );

    if( !board.dragging )
        return;

    if( !board.dragging->bmp )
    {
        board.dragging = nullptr;
        return;
    }

    if( ( board.dragging->type == TB_HCLUE_TILE ) || ( board.dragging->type == TB_VCLUE_TILE ) )
    {
        board.dragging_origin_x = board.dragging->x;
        board.dragging_origin_y = board.dragging->y;
        board.dragging_relative_position_of_grabbing_x = board.dragging->x - mx + 5;
        board.dragging_relative_position_of_grabbing_y = board.dragging->y - my + 5;
        board.dragging->x = mx + board.dragging_relative_position_of_grabbing_x;
        board.dragging->y = my + board.dragging_relative_position_of_grabbing_y;
        return;
    }

    board.dragging = nullptr;
}

void Game::mouse_drop( int mx, int my )
{
    gui.emit_event( EVENT_REDRAW );

    if( !board.dragging )
        return;

    board.dragging->x = board.dragging_origin_x;
    board.dragging->y = board.dragging_origin_y;

    TiledBlock *tiled_block = get_TiledBlock_at( mx, my );

    if( tiled_block && ( tiled_block->type == board.dragging->type ) )
    {
        swap_clues( board.dragging, tiled_block );
        if( board.highlight == board.dragging )
            board.highlight = tiled_block;
        else if( board.highlight == tiled_block )
            board.highlight = board.dragging;
        if( !set.sound_mute )
            play_sound( SOUND_HIDE_TILE );
    }

    board.dragging = nullptr;
    board.clear_info_panel();
}

TiledBlock *Game::get_TiledBlock_at( int x, int y )
{
    if( !board.zoom )
        return get_TiledBlock( &board.all, x, y );

    float xx = x, yy = y;
    al_transform_coordinates( &board.zoom_transform_inv, &xx, &yy );

    TiledBlock *tiled_block = get_TiledBlock( board.zoom, xx, yy );

    if( tiled_block && ( tiled_block->parent == board.zoom ) )
        return tiled_block;
    else
        return nullptr;
}

void Game::show_hint()
{
    if( game_state != GAME_PLAYING )
        return;
    if( !check_panel_correctness( &game_data ) )
    {
        show_info_text( &board, al_ustr_new( "Something is wrong. An item was ruled out incorrectly." ) );
        return;
    }

    int hint = get_hint( &game_data );
    if( !hint )
    {
        show_info_text( &board, al_ustr_new( "No hint available." ) );
        return;
    }

    board.highlight = board.clue_tiledblock[hint & 255];
    board.rule_out = board.panel.sub[( hint >> 15 ) & 7]->sub[( hint >> 12 ) & 7]->sub[( hint >> 9 ) & 7];

    char *b0 = symbol_char[game_data.clue[hint & 255].j[0]][game_data.clue[hint & 255].k[0]];
    char *b1 = symbol_char[game_data.clue[hint & 255].j[1]][game_data.clue[hint & 255].k[1]];
    char *b2 = symbol_char[game_data.clue[hint & 255].j[2]][game_data.clue[hint & 255].k[2]];
    char *b3 = symbol_char[( hint >> 12 ) & 7][( hint >> 9 ) & 7];

    show_info_text( &board, get_hint_info_text( game_data.clue[hint & 255].rel, b0, b1, b2, b3 ) );
}

void Game::update_guessed()
{
    int i, j, k, count, val;

    game_data.guessed = 0;

    for( i = 0; i < game_data.number_of_columns; i++ )
    {
        for( j = 0; j < game_data.column_height; j++ )
        {
            count = 0;
            val = -1;
            for( k = 0; k < game_data.number_of_columns; k++ )
            {
                if( game_data.tile[i][j][k] )
                {
                    count++;
                    val = k;
                    if( count > 1 )
                        break;
                }
            }
            if( count == 1 )
            {
                game_data.guess[i][j] = val;
                game_data.guessed++;
            }
            else
                game_data.guess[i][j] = -1;
        }
    }
}

void Game::execute_undo()
{
    if( !undo )
        return;

    memcpy( &game_data.tile, &undo->tile, sizeof( game_data.tile ) );

    PanelState *undo_old = undo->parent;

    delete undo;

    undo = undo_old;

    if( !set.sound_mute )
        play_sound( SOUND_UNHIDE_TILE );

    update_guessed();
}

void Game::save_state()
{
    PanelState *foo = new PanelState();

    foo->parent = undo;
    undo = foo;

    memcpy( &undo->tile, &game_data.tile, sizeof( undo->tile ) );
}

void Game::destroy_undo()
{
    PanelState *foo;

    while( undo )
    {
        foo = undo->parent;
        delete undo;
        undo = foo;
    }
}

void Game::switch_solve_puzzle()
{
    std::swap( game_data.guess, game_data.puzzle );
    update_board();
}

int Game::save_game_f()
{
    ALLEGRO_PATH *path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );

    SPDLOG_DEBUG( "ALLEGRO_USER_DATA_PATH = %s", al_path_cstr( path, '/' ) );

    if( !al_make_directory( al_path_cstr( path, '/' ) ) )
    {
        SPDLOG_ERROR( "could not open or create path %s.\n", al_path_cstr( path, '/' ) );
        return -1;
    }

    al_set_path_filename( path, "Watson.sav" );

    ALLEGRO_FILE *fp = al_fopen( al_path_cstr( path, '/' ), "wb" );
    if( !fp )
    {
        SPDLOG_ERROR( "Couldn't open %s for writing.\n", (char *)al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        return -1;
    }
    al_fwrite( fp, &game_data.number_of_columns, sizeof( game_data.number_of_columns ) );
    al_fwrite( fp, &game_data.column_height, sizeof( game_data.column_height ) );
    al_fwrite( fp, &game_data.puzzle, sizeof( game_data.puzzle ) );
    al_fwrite( fp, &game_data.clue_n, sizeof( game_data.clue_n ) );
    al_fwrite( fp, &game_data.clue, sizeof( game_data.clue ) );
    al_fwrite( fp, &game_data.tile, sizeof( game_data.tile ) );
    al_fwrite( fp, &game_data.time, sizeof( game_data.time ) );
    al_fclose( fp );

    SPDLOG_DEBUG( "Saved game at %s.", al_path_cstr( path, '/' ) );
    al_destroy_path( path );
    return 0;
}

int Game::load_game_f()
{
    ALLEGRO_PATH *path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );

    al_set_path_filename( path, "Watson.sav" );

    ALLEGRO_FILE *fp = al_fopen( al_path_cstr( path, '/' ), "rb" );
    if( !fp )
    {
        SPDLOG_ERROR( "Couldn't open %s for reading.", (char *)al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        return -1;
    }

    if( al_fread( fp, &game_data.number_of_columns, sizeof( game_data.number_of_columns ) )
        != sizeof( game_data.number_of_columns ) )
    {
        al_destroy_path( path );
        SPDLOG_ERROR( "Error reading %s.", (char *)al_path_cstr( path, '/' ) );
        return -1;
    }

    al_fread( fp, &game_data.column_height, sizeof( game_data.column_height ) );
    al_fread( fp, &game_data.puzzle, sizeof( game_data.puzzle ) );
    al_fread( fp, &game_data.clue_n, sizeof( game_data.clue_n ) );
    al_fread( fp, &game_data.clue, sizeof( game_data.clue ) );
    al_fread( fp, &game_data.tile, sizeof( game_data.tile ) );
    al_fread( fp, &game_data.time, sizeof( game_data.time ) );
    al_fclose( fp );

    update_guessed();

    al_destroy_path( path );

    return 0;
}

ALLEGRO_USTR *Game::get_hint_info_text( RELATION relation, char *b0, char *b1, char *b2, char *b3 )
{
    const char *fmt = nullptr;
    switch( relation )
    {
        case CONSECUTIVE:
            fmt = "The column of %s is between %s and %s, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b1, b0, b2, b3 );
            break;
        case NEXT_TO:
            fmt = "The columns of %s and %s are next to each other, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b3 );
            break;
        case NOT_NEXT_TO:
            fmt = "The column of %s is NOT next to the column of %s, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b3 );
            break;
        case NOT_MIDDLE:
            fmt = "There is exactly one column between %s and %s, and %s is NOT in that "
                  "column, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b2, b1, b3 );
            break;
        case ONE_SIDE:
            fmt = "The column of %s is strictly to the left of %s, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b3 );
            break;
        case TOGETHER_2:
            fmt = "%s and %s are on the same column, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b3 );
            break;
        case TOGETHER_3:
            fmt = "%s, %s and %s are on the same column, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b2, b3 );
            break;
        case NOT_TOGETHER:
            fmt = "%s and %s are NOT on the same column, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b3 );
            break;
        case TOGETHER_NOT_MIDDLE:
            fmt = "%s and %s are on the same column, and %s is NOT in that column, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b2, b1, b3 );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            fmt = "%s is on the same column of either %s or %s, but NOT BOTH, so we can rule out %s from here.";
            return al_ustr_newf( fmt, b0, b1, b2, b3 );
            break;

        case REVEAL:
        case NUMBER_OF_RELATIONS:
        default:
            return nullptr;
            break;
    }
}
void Game::explain_clue( Clue *clue )
{
    char *b0 = symbol_char[clue->j[0]][clue->k[0]];
    char *b1 = symbol_char[clue->j[1]][clue->k[1]];
    char *b2 = symbol_char[clue->j[2]][clue->k[2]];

    ALLEGRO_USTR *clue_explanation = nullptr;
    const char *fmt = nullptr;

    switch( clue->rel )
    {
        case CONSECUTIVE:
            fmt = "The column of %s is between %s and %s, but they could be on either side.";
            clue_explanation = al_ustr_newf( fmt, b1, b0, b2 );
            break;
        case NEXT_TO:
            fmt = "The columns of %s and %s are next to each other, but they could be on either side.";
            clue_explanation = al_ustr_newf( fmt, b0, b1 );
            break;
        case NOT_NEXT_TO:
            fmt = "The column of %s is NOT next to the column of %s.";
            clue_explanation = al_ustr_newf( fmt, b0, b1 );
            break;
        case NOT_MIDDLE:
            fmt = "There is exactly one column between %s and %s, and %s is NOT in that column.";
            clue_explanation = al_ustr_newf( fmt, b0, b2, b1 );
            break;
        case ONE_SIDE:
            fmt = "The column of %s is strictly to the left of %s.";
            clue_explanation = al_ustr_newf( fmt, b0, b1 );
            break;
        case TOGETHER_2:
            fmt = "%s and %s are on the same column.";
            clue_explanation = al_ustr_newf( fmt, b0, b1 );
            break;
        case TOGETHER_3:
            fmt = "%s, %s and %s are on the same column.";
            clue_explanation = al_ustr_newf( fmt, b0, b1, b2 );
            break;
        case NOT_TOGETHER:
            fmt = "%s and %s are NOT on the same column.";
            clue_explanation = al_ustr_newf( fmt, b0, b1 );
            break;
        case TOGETHER_NOT_MIDDLE:
            fmt = "%s and %s are on the same column, and %s is NOT in that column.";
            clue_explanation = al_ustr_newf( fmt, b0, b2, b1 );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            fmt = "%s is on the same column of either %s or %s, but NOT BOTH.";
            clue_explanation = al_ustr_newf( fmt, b0, b1, b2 );
            break;
        default:
            break;
    }

    if( clue_explanation )
        show_info_text( &board, clue_explanation );
}

void Game::swap_clues( TiledBlock *c1, TiledBlock *c2 )
{
    std::swap( c1->bmp, c2->bmp );

    if( c1->index >= 0 )
        board.clue_tiledblock[c1->index] = c2;
    if( c2->index >= 0 )
        board.clue_tiledblock[c2->index] = c1;

    std::swap( c1->index, c2->index );
    std::swap( c1->hidden, c2->hidden );
};

void Game::zoom_TB( TiledBlock *tiled_block )
{
    if( !tiled_block )
        return;

    int dw = al_get_display_width( al_get_current_display() );
    int dh = al_get_display_height( al_get_current_display() );

    int x;
    int y;
    get_TiledBlock_offset( tiled_block, &x, &y );

    double scale_factor = 2.5;

    int tr_x = -( scale_factor - 1 ) * ( x + tiled_block->width / 2 );
    if( scale_factor * x + tr_x < 0 )
        tr_x = -scale_factor * x;
    else if( scale_factor * ( x + tiled_block->width ) + tr_x > dw )
        tr_x = dw - scale_factor * ( x + tiled_block->width );

    int tr_y = -( scale_factor - 1 ) * ( y + tiled_block->height / 2 );
    if( scale_factor * y + tr_y < 0 )
        tr_y = -scale_factor * y;
    else if( scale_factor * ( y + tiled_block->height ) + tr_y > dh )
        tr_y = dh - scale_factor * ( y + tiled_block->height );

    al_identity_transform( &board.identity_transform );
    al_identity_transform( &board.zoom_transform );

    al_build_transform( &board.zoom_transform, tr_x, tr_y, scale_factor, scale_factor, 0 );
    if( tiled_block->parent )
        get_TiledBlock_offset( tiled_block->parent, &x, &y );
    al_translate_transform( &board.zoom_transform, scale_factor * x, scale_factor * y );

    board.zoom_transform_inv = board.zoom_transform;
    al_invert_transform( &board.zoom_transform_inv );

    board.zoom = tiled_block;

    if( !set.sound_mute )
        play_sound( SOUND_CLICK );
}

void Game::halt( ALLEGRO_EVENT_QUEUE *queue )
{
    ALLEGRO_DISPLAY *disp = al_get_current_display();
    al_acknowledge_drawing_halt( disp );

    SPDLOG_DEBUG( "ACKNOWLEDGED HALT" );

    // otherwise it keeps streaming when the app is on background
    al_set_default_voice( nullptr );

    ALLEGRO_EVENT ev;
    do
    {
        al_wait_for_event( queue, &ev );
    } while( ev.type != ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING );

    al_restore_default_mixer();

    al_acknowledge_drawing_resume( disp );

    SPDLOG_DEBUG( "ACKNOWLEDGED RESUME" );

    al_rest( 0.01 );

    al_flush_event_queue( queue );
}

void Game::animate_win()
{
    ALLEGRO_BITMAP *currbuf = al_get_target_bitmap();
    ALLEGRO_BITMAP *bmp = al_clone_bitmap( currbuf );

    al_set_target_bitmap( bmp );
    al_clear_to_color( BLACK_COLOR );
    draw_stuff();
    al_set_target_bitmap( currbuf );

    int arr[64];
    for( int k = 0; k < board.number_of_columns * board.column_height; k++ )
    {
        arr[k] = k;
    }

    shuffle( arr, board.number_of_columns * board.column_height );

    for( int k = 0; k < board.number_of_columns * board.column_height; k++ )
    {
        al_clear_to_color( BLACK_COLOR );

        al_draw_bitmap( bmp, 0, 0, 0 );

        for( int kk = 0; kk <= k; kk++ )
        {
            int ii = arr[kk] / board.column_height;
            int jj = arr[kk] % board.column_height;

            int x, y;
            get_TiledBlock_offset( board.panel.sub[ii]->sub[jj], &x, &y );

            al_draw_filled_rectangle( x,
                                      y,
                                      x + board.panel.sub[ii]->sub[jj]->width,
                                      y + board.panel.sub[ii]->sub[jj]->height,
                                      al_premul_rgba( 0, 0, 0, 200 ) );
        }

        al_flip_display();

        if( !set.sound_mute )
        {
            play_sound( SOUND_STONE );
        }

        ALLEGRO_EVENT ev;
        while( al_get_next_event( gui.event_queue, &ev ) )
        {
            if( ev.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING )
            {
                halt( gui.event_queue );
                al_acknowledge_resize( al_get_current_display() );
                return;
                // should do something else here
            }
        }

        al_rest( std::max( 0.15, 0.6 * ( 1 - sqrt( (float)k / ( board.column_height * board.number_of_columns ) ) ) ) );
    }
}

void Game::draw_generating_puzzle( Settings *settings )
{
    if( game_state == GAME_INTRO )
        return;

    const char *fmt = nullptr;

    if( !settings->advanced )
        fmt = "Generating %d x %d puzzle, please wait...";
    else
        fmt = "Generating %d x %d advanced puzzle, please wait (this could take a while)...";

    ALLEGRO_USTR *msg = al_ustr_newf( fmt, settings->number_of_columns, settings->column_height );

    gui.draw_text_gui( msg );
}

int Game::switch_tiles()
{
    // cycle through tyle types (font, bitmap, classic)
    SPDLOG_DEBUG( "Swtiching tiles." );

    al_set_target_backbuffer( display );

    destroy_all_bitmaps( &board );

    board.type_of_tiles = ( board.type_of_tiles + 1 ) % 3;

    if( init_bitmaps( &board ) )
    {
        // cycle through all three options
        board.type_of_tiles = ( board.type_of_tiles + 1 ) % 3;

        if( init_bitmaps( &board ) )
            board.type_of_tiles = ( board.type_of_tiles + 1 ) % 3;

        if( init_bitmaps( &board ) )
        {
            SPDLOG_ERROR( "Error switching tiles." );
            exit( -1 );
        }
    }

    board.max_width = al_get_display_width( display );
    board.max_height = al_get_display_height( display );

    board.create_board( &game_data, Board::CreateMode::Update );

    al_set_target_backbuffer( display );

    update_board();

    al_convert_memory_bitmaps();
    al_clear_to_color( BLACK_COLOR );
    al_flip_display();

    return 0;
}

void Game::win_or_lose()
{
    if( check_solution( &game_data ) )
    {
        game_state = GAME_OVER;

        show_info_text( &board, al_ustr_new( "Elementary, watson!" ) );
        animate_win();

        if( !set.sound_mute )
            play_sound( SOUND_WIN );
    }
    else
    {
        show_info_text( &board, al_ustr_new( "Something is wrong. Try again, go to settings to start a new puzzle." ) );

        if( !set.sound_mute )
            play_sound( SOUND_WRONG );

        execute_undo();
        update_board();

        gui.emit_event( EVENT_REDRAW );
    }
}

void Game::destroy_everything()
{
    board.destroy_board();
    destroy_sound();
    destroy_undo();
    gui.remove_all_guis();
    gui.destroy_base_gui();
}

int Game::toggle_fullscreen()
{
    get_desktop_resolution( 0, &desktop_width, &desktop_height );

    float display_factor;

    if( !fullscreen )
    {
        SPDLOG_DEBUG( "Entering full screen mode." );
        al_set_new_display_flags( ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL );
        display_factor = 1;
    }
    else
    {
        SPDLOG_DEBUG( "Exiting full screen mode." );
        al_set_new_display_flags( ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE | ALLEGRO_OPENGL );
        display_factor = 0.9;
    }

    ALLEGRO_DISPLAY *new_display = al_create_display( desktop_width * display_factor, desktop_height * display_factor );
    if( !new_display )
    {
        fprintf( stderr, "Error switching fullscreen mode.\n" );
        return 0;
    }

    fullscreen = !fullscreen;
    board.destroy_board();

    al_destroy_display( display );

    display = new_display;
    al_set_target_backbuffer( display );

    board.max_width = desktop_width * display_factor;
    board.max_height = desktop_height * display_factor;

    board.create_board( &game_data, fullscreen ? Board::CreateMode::CreateFullscreen : Board::CreateMode::Create );

    al_set_target_backbuffer( display );

    if( !fullscreen )
    {
        al_resize_display( display, board.width, board.height );
        al_set_window_position( display, ( desktop_width - board.width ) / 2, ( desktop_height - board.height ) / 2 );
        al_acknowledge_resize( display );
        al_set_target_backbuffer( display );
    }

    update_board();
    al_convert_memory_bitmaps();

    return 1;
}

void Game::handle_allegro_event_display_close()
{
    gui.emit_event( EVENT_EXIT );
}

void Game::handle_allegro_event_display_resize()
{
    if( board.dragging )
        mouse_drop( -1, -1 );

    if( fullscreen )
        return;

    al_acknowledge_resize( display );

    resizing = true;
    resize_time = al_get_time();
}

void Game::handle_allegro_event_redraw()
{
    redraw = true;
}

void Game::handle_event_switch_tiles()
{
    switch_tiles();

    al_flush_event_queue( gui.event_queue );

    redraw = true;
}

void Game::handle_event_restart()
{
    restart = 1;

    set = nset;
}

void Game::handle_event_exit()
{
    noexit = false;
}

void Game::handle_event_save()
{
    if( game_state != GAME_PLAYING )
        return;

    if( !save_game_f() )
    {
        show_info_text( &board, al_ustr_new( "Game saved" ) );
        set.saved = 1;
    }
    else
    {
        show_info_text( &board, al_ustr_new( "Error: game could not be saved." ) );
    }
}

bool Game::handle_event_load()
{
    if( load_game_f() )
    {
        show_info_text( &board, al_ustr_new( "Error game could not be loaded." ) );
        return false;
    }
    else
    {
        restart = 2;
        return true;
    }
}

void Game::handle_event_settings()
{
    gui.show_settings();
    gui.emit_event( EVENT_REDRAW );
}

void Game::handle_allegro_event_touch_begin( ALLEGRO_EVENT &ev )
{
    if( gui.gui_n )
        return;

    ev.mouse.x = ev.touch.x;
    ev.mouse.y = ev.touch.y;
    ev.mouse.button = 1;

    touch_down = true;
}

void Game::handle_allegro_event_mouse_button_down( ALLEGRO_EVENT &ev )
{
    if( gui.gui_n )
        return;

    if( mouse_button_down )
        return;

    mouse_down_time = al_get_time(); // workaround ev.any.timestamp for touch;
    mbdown_x = ev.mouse.x;
    mbdown_y = ev.mouse.y;
    tb_down = get_TiledBlock_at( ev.mouse.x, ev.mouse.y );

    if( board.zoom && !tb_down )
    {
        board.zoom = nullptr;
        redraw = true;
        return;
    }

    if( wait_for_double_click )
    {
        wait_for_double_click = false;
        if( ( tb_up == tb_down ) && ( mouse_down_time - mouse_up_time < DELTA_DOUBLE_CLICK ) )
        {
            handle_mouse_click( tb_down, ev.mouse.x, ev.mouse.y, 3 ); // double click
            return;
        }
    }

    mouse_button_down = ev.mouse.button;
}

void Game::handle_allegro_event_touch_end( ALLEGRO_EVENT &ev )
{
    ev.mouse.x = ev.touch.x;
    ev.mouse.y = ev.touch.y;
    ev.mouse.button = 1;

    touch_down = false;
}

void Game::handle_allegro_event_mouse_button_up( ALLEGRO_EVENT &ev )
{
    if( wait_for_double_click )
        wait_for_double_click = false;

    if( board.dragging )
    {
        mouse_drop( ev.mouse.x, ev.mouse.y );
        mouse_button_down = 0;
    }

    if( hold_click_check == 2 )
    {
        hold_click_check = 0;
        mouse_button_down = 0;
        return;
    }

    hold_click_check = 0;

    if( !mouse_button_down )
        return;

    mouse_up_time = al_get_time(); //workaround ev.any.timestamp for touch;
    tb_up = get_TiledBlock_at( ev.mouse.x, ev.mouse.y );

    if( ( tb_up ) && ( tb_up == tb_down ) )
    {
        bool is_a_clue_tile = ( tb_up->type == TB_HCLUE_TILE ) || ( tb_up->type == TB_VCLUE_TILE );
        bool is_left_mouse_button_down = mouse_button_down == 1;
        bool down_event_is_short = mouse_up_time - mouse_down_time < DELTA_SHORT_CLICK;

        wait_for_double_click = is_a_clue_tile && is_left_mouse_button_down && down_event_is_short;

        if( !wait_for_double_click )
            handle_mouse_click( tb_up, ev.mouse.x, ev.mouse.y, mouse_button_down );
    }

    mouse_button_down = 0;
}

void Game::handle_allegro_event_touch_move( ALLEGRO_EVENT &ev )
{
    if( gui.gui_n )
        return;

    if( !ev.touch.primary )
        return;

    ev.mouse.x = ev.touch.x;
    ev.mouse.y = ev.touch.y;
}

void Game::handle_allegro_event_mouse_axes( ALLEGRO_EVENT &ev )
{
    if( gui.gui_n )
        return;

    if( board.dragging )
    {
        board.dragging->x = ev.mouse.x + board.dragging_relative_position_of_grabbing_x;
        board.dragging->y = ev.mouse.y + board.dragging_relative_position_of_grabbing_y;
    }

    mouse_move = true;

    // don't grab if movement was small
    if( ( abs( ev.mouse.x - mbdown_x ) < 10 ) && ( abs( ev.mouse.y - mbdown_y ) < 10 ) )
        return;

    if( mouse_button_down && !hold_click_check )
    {
        if( tb_down && ( ( tb_down->type == TB_HCLUE_TILE ) || ( tb_down->type == TB_VCLUE_TILE ) ) )
        {
            handle_mouse_click( tb_down, mbdown_x, mbdown_y, 4 );
            hold_click_check = 1;
        }
    }
}

void Game::handle_allegro_event_key_char( ALLEGRO_EVENT &ev )
{
    if( gui.gui_n )
        return;

    keypress = true;
    switch( ev.keyboard.keycode )
    {
        case ALLEGRO_KEY_ESCAPE:
            gui.confirm_exit();
            break;
        case ALLEGRO_KEY_BACK: // android: back key
            gui.show_settings();
            break;
        case ALLEGRO_KEY_R:
            gui.confirm_restart( &set );
            break;
        case ALLEGRO_KEY_S: // debug: show solution
            if( game_state != GAME_PLAYING )
                break;
            switch_solve_puzzle();
            redraw = true;
            break;
        case ALLEGRO_KEY_T:
            gui.emit_event( EVENT_SWITCH_TILES );
            break;
        case ALLEGRO_KEY_H:
            gui.show_help();
            break;
        case ALLEGRO_KEY_C:
            show_hint();
            redraw = true;
            break;
        case ALLEGRO_KEY_F:
            if( toggle_fullscreen() )
            {
                al_register_event_source( gui.event_queue, al_get_display_event_source( display ) );
            }
            al_flush_event_queue( gui.event_queue );
            redraw = true;
            break;
        case ALLEGRO_KEY_U:
            if( game_state != GAME_PLAYING )
                break;
            execute_undo();
            update_board();
            // why flush?
            al_flush_event_queue( gui.event_queue );
            redraw = true;
            break;
        default:
            break;
    }
}

void Game::handle_events()
{
    // empty out the event queue
    ALLEGRO_EVENT ev;
    while( al_get_next_event( gui.event_queue, &ev ) )
    {
        if( ev.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING )
        {
            SPDLOG_DEBUG( "RECEIVED HALT" );
            halt( gui.event_queue );
            resize_update = true;
        }

        if( gui.gui_n && gui.gui_send_event( &ev ) )
        {
            continue;
        }

        switch( ev.type )
        {
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                handle_allegro_event_display_close();
                break;

            case ALLEGRO_EVENT_DISPLAY_RESIZE:
                handle_allegro_event_display_resize();
                break;

            case EVENT_REDRAW:
                handle_allegro_event_redraw();
                break;

            case EVENT_SWITCH_TILES:
                handle_event_switch_tiles();
                break;

            case EVENT_RESTART:
                handle_event_restart();
                return;

            case EVENT_EXIT:
                handle_event_exit();
                break;

            case EVENT_SAVE:
                handle_event_save();
                break;

            case EVENT_LOAD:
                if( !handle_event_load() )
                    return;
                break;

            case EVENT_SETTINGS:
                handle_event_settings();
                break;

            case ALLEGRO_EVENT_TOUCH_BEGIN:
                handle_allegro_event_touch_begin( ev );
                [[fallthrough]];
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                handle_allegro_event_mouse_button_down( ev );
                break;

            case ALLEGRO_EVENT_TOUCH_END:
                handle_allegro_event_touch_end( ev );
                [[fallthrough]];
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                handle_allegro_event_mouse_button_up( ev );
                break;

            case ALLEGRO_EVENT_TOUCH_MOVE:
                handle_allegro_event_touch_move( ev );
                [[fallthrough]];
            case ALLEGRO_EVENT_MOUSE_AXES:
                handle_allegro_event_mouse_axes( ev );
                break;

            case ALLEGRO_EVENT_KEY_CHAR:
                handle_allegro_event_key_char( ev );
                break;
        }
    }
}

void Game::game_inner_loop()
{
    double dt = al_current_time() - old_time;
    al_rest( FIXED_DT - dt ); //rest at least fixed_dt
    dt = al_get_time() - old_time;
    if( game_state == GAME_PLAYING )
        game_data.time += dt;
    old_time = al_get_time();

    gui.update_base_gui( dt );

    handle_events();
    if( restart )
        return;

    if( resizing )
    {
        if( al_get_time() - resize_time > RESIZE_DELAY )
        {
            resizing = false;
            resize_update = true;
        }
    }

    if( resize_update )
    {
        resize_update = false;
        al_acknowledge_resize( display );
        destroy_board_bitmaps( &board );
        board.max_width = al_get_display_width( display );
        board.max_height = al_get_display_height( display );
        board.create_board( &game_data, Board::CreateMode::Update );
        gui.update_guis( board.all.x, board.all.y, board.width, board.height );
        al_set_target_backbuffer( display );
        update_board();
        al_convert_memory_bitmaps(); // turn bitmaps to video bitmaps
        redraw = true;
        // android workaround, try removing:
        al_clear_to_color( BLACK_COLOR );
        al_flip_display();
    }

    if( resizing ) // skip redraw and other stuff
        return;

    if( wait_for_double_click && ( al_get_time() - mouse_up_time > DELTA_DOUBLE_CLICK ) )
    {
        wait_for_double_click = false;
        tb_down = get_TiledBlock_at( mbdown_x, mbdown_y );
        handle_mouse_click( tb_down, mbdown_x, mbdown_y, 1 ); // single click
    }

    if( mouse_button_down && !hold_click_check && !board.dragging )
    {
        if( al_get_time() - mouse_down_time > DELTA_HOLD_CLICK )
        {
            hold_click_check = 1;
            if( tb_down )
            {
                int tbdx, tbdy;
                if( touch_down )
                {
                    ALLEGRO_TOUCH_INPUT_STATE touch;
                    al_get_touch_input_state( &touch );
                    tbdx = touch.touches[0].x;
                    tbdy = touch.touches[0].y;
                }
                else
                {
                    ALLEGRO_MOUSE_STATE mouse;
                    al_get_mouse_state( &mouse );
                    tbdx = mouse.x;
                    tbdy = mouse.y;
                }
                tb_up = get_TiledBlock_at( tbdx, tbdy );

                if( tb_up == tb_down )
                {
                    handle_mouse_click( tb_up, tbdx, tbdy, 4 ); // hold click
                    hold_click_check = 2;
                }
            }
        }
    }

    if( mouse_move )
    {
        mouse_move = false;
        if( board.dragging )
            redraw = true;
    }

    if( keypress )
    {
        if( game_state == GAME_INTRO )
            game_state = GAME_PLAYING;
        keypress = false;
    }

    if( old_time - play_time > 1 )
    { // runs every second
        if( game_state == GAME_INTRO )
            game_state = GAME_PLAYING;
        if( game_state == GAME_PLAYING )
        {
            play_time = al_get_time();
            update_timer( (int)game_data.time, &board ); // this draws on a timer bitmap

            if( game_data.guessed == game_data.column_height * game_data.number_of_columns )
            {
                win_or_lose(); // check if player has won
                al_flush_event_queue( gui.event_queue );
            }
            gui.emit_event( EVENT_REDRAW );
        }
    }

    if( board.rule_out && ( al_get_time() - blink_time > BLINK_DELAY ) )
    {
        board.blink = !board.blink;
        blink_time = al_get_time();
        redraw = true;
    }

    if( ( game_state == GAME_OVER ) && noexit && !win_gui )
    {
        gui.show_win_gui( game_data.time );
        win_gui = true;
    }

    if( redraw )
    {
        redraw = false;
        al_set_target_backbuffer( display );
        draw_stuff();
        al_flip_display();
    }
}

void Game::game_loop()
{
    if( restart )
    {
        SPDLOG_DEBUG( "Restarting game" );
        gui.remove_all_guis();
    }

    // 2 is for loaded game
    if( restart != 2 )
    {
        game_data.advanced = set.advanced; // use "what if" depth 1?
        game_data.number_of_columns = set.number_of_columns;
        game_data.column_height = set.column_height;
        game_data.time = 0;
        draw_stuff();
        draw_generating_puzzle( &set );
        al_flip_display();
        create_game_with_clues( &game_data );
    }
    else
    {
        // board should be updated only after destroying the board
        SPDLOG_DEBUG( "Resuming loaded game" );
        set.advanced = game_data.advanced;
        set.number_of_columns = game_data.number_of_columns;
        set.column_height = game_data.column_height;
    }

    if( restart == 1 )
        game_data.time = 0; // new game, otherwise it's a load game

    get_desktop_resolution( 0, &desktop_width, &desktop_height );

    float max_display_factor;

    if( !fullscreen )
        max_display_factor = 0.9;
    else
        max_display_factor = 1;

    if( restart )
    {
        al_set_target_backbuffer( display );
        board.destroy_board();
        destroy_undo();
        al_set_target_backbuffer( display );
    }

    restart = 0;

    board.max_width = desktop_width * max_display_factor;
    board.max_height = desktop_height * max_display_factor; // change this later to something adequate
    board.type_of_tiles = set.type_of_tiles;
    board.number_of_columns = game_data.number_of_columns;
    board.column_height = game_data.column_height;

    if( board.create_board( &game_data, Board::CreateMode::Create ) )
    {
        SPDLOG_ERROR( "Failed to create game board." );
        noexit = false;
        return;
    }

    if( !fullscreen )
    {
        al_set_target_backbuffer( display );
        al_resize_display( display, board.width, board.height );
        al_set_window_position( display, ( desktop_width - board.width ) / 2, ( desktop_height - board.height ) / 2 );
        al_acknowledge_resize( display );
        al_set_target_backbuffer( display );
    }

    al_convert_memory_bitmaps(); // turn bitmaps to memory bitmaps after resize (bug in allegro doesn't autoconvert)

    gui.update_guis( board.all.x, board.all.y, board.width, board.height );

    if( !gui.event_queue )
    {
        gui.event_queue = al_create_event_queue();
        if( !gui.event_queue )
        {
            SPDLOG_ERROR( "failed to create event queue." );
            al_destroy_display( display );
            noexit = false;
            return;
        }

        al_register_event_source( gui.event_queue, al_get_display_event_source( display ) );
        if( al_is_keyboard_installed() )
            al_register_event_source( gui.event_queue, al_get_keyboard_event_source() );
        if( al_is_mouse_installed() )
            al_register_event_source( gui.event_queue, al_get_mouse_event_source() );
        if( al_is_touch_input_installed() )
        {
            al_register_event_source( gui.event_queue, al_get_touch_input_event_source() );
            al_set_mouse_emulation_mode( ALLEGRO_MOUSE_EMULATION_NONE );
        }

        al_register_event_source( gui.event_queue, &gui.user_event_src );
    }

    update_board();
    al_set_target_backbuffer( display );

    //  initialize flags
    redraw = true;
    noexit = true;
    mouse_move = false;
    restart = 0;
    keypress = false;
    resizing = false;
    mouse_button_down = 0;
    resize_update = false;
    resize_time = 0;

    if( game_state != GAME_INTRO )
        game_state = GAME_PLAYING;

    board.time_start = al_get_time();
    blink_time = 0;
    board.blink = false;
    mbdown_x = 0;
    mbdown_y = 0;
    touch_down = false;
    tb_down = tb_up = nullptr;
    win_gui = false;

    const char *fmt = "Click on clue for info. Click %s for help, %s for settings, or %s for a hint at any time.";
    auto last_block = symbol_char[board.column_height];
    ALLEGRO_USTR *msg = al_ustr_newf( fmt, last_block[1], last_block[2], last_block[0] );
    show_info_text( &board, msg );

    al_set_target_backbuffer( display );
    al_clear_to_color( BLACK_COLOR );
    al_flip_display();
    al_flush_event_queue( gui.event_queue );
    play_time = old_time = al_get_time();

    while( noexit )
    {
        game_inner_loop();
        if( restart )
            return;
    }
}
