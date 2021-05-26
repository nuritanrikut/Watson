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
      redraw( 0 ),
      mouse_move( 0 ),
      keypress( 0 ),
      resizing( 0 ),
      resize_update( 0 ),
      mouse_button_down( 0 ),
      win_gui( 0 ),
      tb_down( nullptr ),
      tb_up( nullptr ),
      mouse_up_time( 0 ),
      mouse_down_time( 0 ),
      wait_for_double_click( 0 ),
      hold_click_check( 0 ),
      mbdown_x( 0 ),
      mbdown_y( 0 ),
      touch_down( 0 ),
      resize_time( 0 ),
      old_time( 0 ),
      blink_time( 0 ),
      play_time( 0 ),
      swap_mouse_buttons( 0 ),
      game_state( GAME_NULL ),
      desktop_xsize( 0 ),
      desktop_ysize( 0 ),
      fullscreen( 0 ),
      game_data(),
      board(),
      undo( nullptr )
{
}

bool Game::init()
{
    // seed random number generator. comment out for debug
    srand( (unsigned int)time( NULL ) );

    SPDLOG_DEBUG( "Watson v" PRE_VERSION " - " PRE_DATE " has started." );
    if( init_allegro() )
        return false;

#ifndef _WIN32
    // use anti-aliasing if available (seems to cause problems in windows)
    al_set_new_display_option( ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST );
    al_set_new_display_option( ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST );
#endif

    fullscreen = 0;

    // use vsync if available
    al_set_new_display_option( ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST );

    get_desktop_resolution( 0, &desktop_xsize, &desktop_ysize );

    if( fullscreen )
    {
        al_set_new_display_flags( ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL );
        display = al_create_display( desktop_xsize, desktop_ysize );
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
            al_draw_filled_rectangle( 0, 0, board.max_xsize, board.max_ysize, al_premul_rgba( 0, 0, 0, 150 ) );
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

    if( board.highlight )
    {
        board.highlight = NULL;
    }

    if( board.rule_out )
    {
        board.rule_out = NULL;
    }

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
            {
                zoom_TB( &board.time_panel );
                return;
            }
            else if( tiled_block->type == TB_PANEL_TILE )
            {
                zoom_TB( board.zoom = tiled_block->parent );
                return;
            }
        }
    }

    if( board.zoom )
        board.zoom = NULL;

    int i, j, k;
    switch( tiled_block->type )
    { // which board component was clicked
        case TB_PANEL_TILE:
            if( game_state != GAME_PLAYING )
                break;
            k = tiled_block->index;
            j = tiled_block->parent->index;
            i = tiled_block->parent->parent->index;
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
            break;

        case TB_PANEL_BLOCK:
            if( game_state != GAME_PLAYING )
                break;
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
            break;

        case TB_HCLUE_TILE:
        case TB_VCLUE_TILE:
            if( game_state != GAME_PLAYING )
                break;
            // check that this is a real clue
            if( tiled_block->bmp && ( tiled_block->index >= 0 ) )
            {
                if( ( mclick == 2 ) || ( mclick == 3 ) )
                { // toggle hide-show clue on double or right click
                    SWITCH( tiled_block->hidden );
                    SWITCH( game_data.clue[tiled_block->index].hidden );
                    if( !set.sound_mute )
                        play_sound( SOUND_HIDE_TILE );
                }
                else if( mclick == 1 )
                { // explain clue in info panel
                    if( !tiled_block->hidden )
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
            break;

        case TB_BUTTON_CLUE: // time panel
            if( game_state != GAME_PLAYING )
                break;
            if( !set.sound_mute )
                play_sound( SOUND_CLICK );
            show_hint();
            break;
        case TB_BUTTON_SETTINGS:
            if( !set.sound_mute )
                play_sound( SOUND_CLICK );
            gui.show_settings();
            break;

        case TB_BUTTON_HELP:
            if( !set.sound_mute )
                play_sound( SOUND_CLICK );
            gui.show_help();
            break;

        case TB_BUTTON_UNDO:
            execute_undo();
            update_board();
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
    int i, j, k;

    for( i = 0; i < game_data.number_of_columns; i++ )
    {
        auto column = board.panel.sub[i];
        for( j = 0; j < game_data.column_height; j++ )
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
                block->bmp = NULL;
                for( k = 0; k < game_data.number_of_columns; k++ )
                {
                    if( game_data.tile[i][j][k] )
                    {
                        block->sub[k]->hidden = 0;
                    }
                    else
                    {
                        block->sub[k]->hidden = 1;
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
    if( board.dragging && board.dragging->bmp )
    {
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
    }

    board.dragging = NULL;
}

void Game::mouse_drop( int mx, int my )
{
    TiledBlock *tiled_block;

    gui.emit_event( EVENT_REDRAW );
    if( !board.dragging )
        return;
    board.dragging->x = board.dragging_origin_x;
    board.dragging->y = board.dragging_origin_y;

    tiled_block = get_TiledBlock_at( mx, my );
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
    board.dragging = NULL;
    board.clear_info_panel();
}

TiledBlock *Game::get_TiledBlock_at( int x, int y )
{
    float xx = x, yy = y;
    TiledBlock *tiled_block;

    if( board.zoom )
    {
        al_transform_coordinates( &board.zoom_transform_inv, &xx, &yy );
        tiled_block = get_TiledBlock( board.zoom, xx, yy );
        if( tiled_block && ( tiled_block->parent == board.zoom ) )
            return tiled_block;
        else
            return NULL;
    }

    return get_TiledBlock( &board.all, x, y );
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

    int i = get_hint( &game_data );
    if( !i )
    {
        show_info_text( &board, al_ustr_new( "No hint available." ) );
        return;
    }

    board.highlight = board.clue_tiledblock[i & 255];
    board.rule_out = board.panel.sub[( i >> 15 ) & 7]->sub[( i >> 12 ) & 7]->sub[( i >> 9 ) & 7];

    char *b0 = symbol_char[game_data.clue[i & 255].j[0]][game_data.clue[i & 255].k[0]];
    char *b1 = symbol_char[game_data.clue[i & 255].j[1]][game_data.clue[i & 255].k[1]];
    char *b2 = symbol_char[game_data.clue[i & 255].j[2]][game_data.clue[i & 255].k[2]];
    char *b3 = symbol_char[( i >> 12 ) & 7][( i >> 9 ) & 7];

    show_info_text( &board, get_hint_info_text( game_data.clue[i & 255].rel, b0, b1, b2, b3 ) );
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
    PanelState *undo_old;

    if( !undo )
        return;
    memcpy( &game_data.tile, &undo->tile, sizeof( game_data.tile ) );
    undo_old = undo->parent;
    free( undo );
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
        free( undo );
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
    switch( relation )
    {
        case CONSECUTIVE:
            return al_ustr_newf(
                "The column of %s is between %s and %s, so we can rule out %s from here.", b1, b0, b2, b3 );
            break;
        case NEXT_TO:
            return al_ustr_newf(
                "The columns of %s and %s are next to each other, so we can rule out %s from here.", b0, b1, b3 );
            break;
        case NOT_NEXT_TO:
            return al_ustr_newf(
                "The column of %s is NOT next to the column of %s, so we can rule out %s from here.", b0, b1, b3 );
            break;
        case NOT_MIDDLE:
            return al_ustr_newf( "There is exactly one column between %s and %s, and %s is NOT in that "
                                 "column, so we can rule out %s from here.",
                                 b0,
                                 b2,
                                 b1,
                                 b3 );
            break;
        case ONE_SIDE:
            return al_ustr_newf(
                "The column of %s is strictly to the left of %s, so we can rule out %s from here.", b0, b1, b3 );
            break;
        case TOGETHER_2:
            return al_ustr_newf( "%s and %s are on the same column, so we can rule out %s from here.", b0, b1, b3 );
            break;
        case TOGETHER_3:
            return al_ustr_newf(
                "%s, %s and %s are on the same column, so we can rule out %s from here.", b0, b1, b2, b3 );
            break;
        case NOT_TOGETHER:
            return al_ustr_newf( "%s and %s are NOT on the same column, so we can rule out %s from here.", b0, b1, b3 );
            break;
        case TOGETHER_NOT_MIDDLE:
            return al_ustr_newf(
                "%s and %s are on the same column, and %s is NOT in that column, so we can rule out %s from here.",
                b0,
                b2,
                b1,
                b3 );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            return al_ustr_newf(
                "%s is on the same column of either %s or %s, but NOT BOTH, so we can rule out %s from here.",
                b0,
                b1,
                b2,
                b3 );
            break;

        default:
            return nullptr;
            break;
    }
}
void Game::explain_clue( Clue *clue )
{
    char *b0, *b1, *b2;
    b0 = symbol_char[clue->j[0]][clue->k[0]];
    b1 = symbol_char[clue->j[1]][clue->k[1]];
    b2 = symbol_char[clue->j[2]][clue->k[2]];

    switch( clue->rel )
    {
        case CONSECUTIVE:
            show_info_text(
                &board,
                al_ustr_newf(
                    "The column of %s is between %s and %s, but they could be on either side.", b1, b0, b2 ) );
            break;
        case NEXT_TO:
            show_info_text(
                &board,
                al_ustr_newf(
                    "The columns of %s and %s are next to each other, but they could be on either side.", b0, b1 ) );
            break;
        case NOT_NEXT_TO:
            show_info_text( &board, al_ustr_newf( "The column of %s is NOT next to the column of %s.", b0, b1 ) );
            break;
        case NOT_MIDDLE:
            show_info_text(
                &board,
                al_ustr_newf(
                    "There is exactly one column between %s and %s, and %s is NOT in that column.", b0, b2, b1 ) );
            break;
        case ONE_SIDE:
            show_info_text( &board, al_ustr_newf( "The column of %s is strictly to the left of %s.", b0, b1 ) );
            break;
        case TOGETHER_2:
            show_info_text( &board, al_ustr_newf( "%s and %s are on the same column.", b0, b1 ) );
            break;
        case TOGETHER_3:
            show_info_text( &board, al_ustr_newf( "%s, %s and %s are on the same column.", b0, b1, b2 ) );
            break;
        case NOT_TOGETHER:
            show_info_text( &board, al_ustr_newf( "%s and %s are NOT on the same column.", b0, b1 ) );
            break;
        case TOGETHER_NOT_MIDDLE:
            show_info_text(
                &board, al_ustr_newf( "%s and %s are on the same column, and %s is NOT in that column.", b0, b2, b1 ) );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            show_info_text( &board,
                            al_ustr_newf( "%s is on the same column of either %s or %s, but NOT BOTH.", b0, b1, b2 ) );
            break;
        default:
            break;
    }
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
    float c = 2.5;
    int x, y, dw = al_get_display_width( al_get_current_display() ),
              dh = al_get_display_height( al_get_current_display() );
    int tr_x, tr_y;

    if( !tiled_block )
        return;
    get_TiledBlock_offset( tiled_block, &x, &y );

    tr_x = -( c - 1 ) * ( x + tiled_block->width / 2 );
    if( c * x + tr_x < 0 )
        tr_x = -c * x;
    else if( c * ( x + tiled_block->width ) + tr_x > dw )
        tr_x = dw - c * ( x + tiled_block->width );

    tr_y = -( c - 1 ) * ( y + tiled_block->height / 2 );
    if( c * y + tr_y < 0 )
        tr_y = -c * y;
    else if( c * ( y + tiled_block->height ) + tr_y > dh )
        tr_y = dh - c * ( y + tiled_block->height );

    al_identity_transform( &board.identity_transform );
    al_identity_transform( &board.zoom_transform );
    al_build_transform( &board.zoom_transform, tr_x, tr_y, c, c, 0 );
    if( tiled_block->parent )
        get_TiledBlock_offset( tiled_block->parent, &x, &y );
    al_translate_transform( &board.zoom_transform, c * x, c * y );
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
    ALLEGRO_EVENT ev;
    al_set_default_voice( NULL ); // otherwise it keeps streaming when the app is on background
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
    int k, ii, jj, kk = 0;
    int x, y;
    ALLEGRO_BITMAP *currbuf = al_get_target_bitmap();
    ALLEGRO_BITMAP *bmp = al_clone_bitmap( currbuf );
    ALLEGRO_EVENT ev;
    int arr[64];

    al_set_target_bitmap( bmp );
    al_clear_to_color( BLACK_COLOR );
    draw_stuff();
    al_set_target_bitmap( currbuf );

    for( k = 0; k < board.number_of_columns * board.column_height; k++ )
    {
        arr[k] = k;
    }

    shuffle( arr, board.number_of_columns * board.column_height );

    for( k = 0; k < board.number_of_columns * board.column_height; k++ )
    {
        al_clear_to_color( BLACK_COLOR );
        al_draw_bitmap( bmp, 0, 0, 0 );
        for( kk = 0; kk <= k; kk++ )
        {
            ii = arr[kk] / board.column_height;
            jj = arr[kk] % board.column_height;
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
    ALLEGRO_USTR *msg;
    if( game_state == GAME_INTRO )
        return;

    if( !settings->advanced )
        msg = al_ustr_newf(
            "Generating %d x %d puzzle, please wait...", settings->number_of_columns, settings->column_height );
    else
        msg = al_ustr_newf( "Generating %d x %d advanced puzzle, please wait (this could take a while)...",
                            settings->number_of_columns,
                            settings->column_height );

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
    { // cycle through all three options
        board.type_of_tiles = ( board.type_of_tiles + 1 ) % 3;
        if( init_bitmaps( &board ) )
            board.type_of_tiles = ( board.type_of_tiles + 1 ) % 3;
        if( init_bitmaps( &board ) )
        {
            SPDLOG_ERROR( "Error switching tiles." );
            exit( -1 );
        }
    }
    board.max_xsize = al_get_display_width( display );
    board.max_ysize = al_get_display_height( display );
    board.create_board( &game_data, 0 );
    al_set_target_backbuffer( display );
    update_board();
    al_convert_bitmaps();
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
    ALLEGRO_DISPLAY *newdisp;
    float display_factor;

    get_desktop_resolution( 0, &desktop_xsize, &desktop_ysize );

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

    newdisp = al_create_display( desktop_xsize * display_factor, desktop_ysize * display_factor );
    if( !newdisp )
    {
        fprintf( stderr, "Error switching fullscreen mode.\n" );
        return 0;
    }

    SWITCH( fullscreen );
    board.destroy_board();

    al_destroy_display( display );

    display = newdisp;
    al_set_target_backbuffer( display );
    board.max_xsize = desktop_xsize * display_factor;
    board.max_ysize = desktop_ysize * display_factor;

    board.create_board( &game_data, fullscreen ? 2 : 1 );
    al_set_target_backbuffer( display );

    if( !fullscreen )
    {
        al_resize_display( display, board.xsize, board.ysize );
        al_set_window_position( display, ( desktop_xsize - board.xsize ) / 2, ( desktop_ysize - board.ysize ) / 2 );
        al_acknowledge_resize( display );
        al_set_target_backbuffer( display );
    }

    update_board();
    al_convert_bitmaps();
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
    resizing = 1;
    resize_time = al_get_time();
}

void Game::handle_allegro_event_redraw()
{
    redraw = 1;
}

void Game::handle_event_switch_tiles()
{
    switch_tiles();
    al_flush_event_queue( gui.event_queue );
    redraw = 1;
}

void Game::handle_event_restart()
{
    restart = 1;
    set = nset;
}

void Game::handle_event_exit()
{
    noexit = 0;
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
        //goto RESTART;
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
    touch_down = 1;
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
        board.zoom = NULL;
        redraw = 1;
        return;
    }

    if( wait_for_double_click )
    {
        wait_for_double_click = 0;
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
    touch_down = 0;
}

void Game::handle_allegro_event_mouse_button_up( ALLEGRO_EVENT &ev )
{
    if( wait_for_double_click )
        wait_for_double_click = 0;

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
        if( ( ( tb_up->type == TB_HCLUE_TILE ) || ( tb_up->type == TB_VCLUE_TILE ) ) && ( mouse_button_down == 1 ) )
        {
            if( mouse_up_time - mouse_down_time < DELTA_SHORT_CLICK )
            {
                wait_for_double_click = 1;
            }
        }
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
    mouse_move = 1;
    // don't grab if movement was small
    if( ( abs( ev.mouse.x - mbdown_x ) < 10 ) && ( ( ev.mouse.y - mbdown_y ) < 10 ) )
        return;

    if( mouse_button_down && !hold_click_check )
        if( tb_down && ( ( tb_down->type == TB_HCLUE_TILE ) || ( tb_down->type == TB_VCLUE_TILE ) ) )
        {
            handle_mouse_click( tb_down, mbdown_x, mbdown_y, 4 );
            hold_click_check = 1;
        }
}

void Game::handle_allegro_event_key_char( ALLEGRO_EVENT &ev )
{
    if( gui.gui_n )
        return;
    keypress = 1;
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
            redraw = 1;
            break;
        case ALLEGRO_KEY_T:
            gui.emit_event( EVENT_SWITCH_TILES );
            break;
        case ALLEGRO_KEY_H:
            gui.show_help();
            break;
        case ALLEGRO_KEY_C:
            show_hint();
            redraw = 1;
            break;
        case ALLEGRO_KEY_F:
            if( toggle_fullscreen() )
            {
                al_register_event_source( gui.event_queue, al_get_display_event_source( display ) );
            }
            al_flush_event_queue( gui.event_queue );
            redraw = 1;
            break;
        case ALLEGRO_KEY_U:
            if( game_state != GAME_PLAYING )
                break;
            execute_undo();
            update_board();
            // why flush?
            al_flush_event_queue( gui.event_queue );
            redraw = 1;
            break;
        default:
            break;
    }
}

void Game::handle_events()
{
    ALLEGRO_EVENT ev;
    while( al_get_next_event( gui.event_queue, &ev ) )
    { // empty out the event queue
        if( ev.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING )
        {
            SPDLOG_DEBUG( "RECEIVED HALT" );
            halt( gui.event_queue );
            resize_update = 1;
        }

        if( gui.gui_n && gui.gui_send_event( &ev ) )
            continue;

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
                handle_event_load();
                break;

            case EVENT_SETTINGS:
                handle_event_settings();
                break;

            case ALLEGRO_EVENT_TOUCH_BEGIN:
                handle_allegro_event_touch_begin( ev );
                // fallthru
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                handle_allegro_event_mouse_button_down( ev );
                break;

            case ALLEGRO_EVENT_TOUCH_END:
                handle_allegro_event_touch_end( ev );
                // fallthru
            case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                handle_allegro_event_mouse_button_up( ev );
                break;

            case ALLEGRO_EVENT_TOUCH_MOVE:
                handle_allegro_event_touch_move( ev );
                //fallthru
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

    if( resizing )
    {
        if( al_get_time() - resize_time > RESIZE_DELAY )
        {
            resizing = 0;
            resize_update = 1;
        }
    }

    if( resize_update )
    {
        resize_update = 0;
        al_acknowledge_resize( display );
        destroy_board_bitmaps( &board );
        board.max_xsize = al_get_display_width( display );
        board.max_ysize = al_get_display_height( display );
        board.create_board( &game_data, 0 );
        gui.update_guis( board.all.x, board.all.y, board.xsize, board.ysize );
        al_set_target_backbuffer( display );
        update_board();
        al_convert_bitmaps(); // turn bitmaps to video bitmaps
        redraw = 1;
        // android workaround, try removing:
        al_clear_to_color( BLACK_COLOR );
        al_flip_display();
    }

    if( resizing ) // skip redraw and other stuff
        return;

    if( wait_for_double_click && ( al_get_time() - mouse_up_time > DELTA_DOUBLE_CLICK ) )
    {
        wait_for_double_click = 0;
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
        mouse_move = 0;
        if( board.dragging )
            redraw = 1;
    }

    if( keypress )
    {
        if( game_state == GAME_INTRO )
            game_state = GAME_PLAYING;
        keypress = 0;
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
        SWITCH( board.blink );
        blink_time = al_get_time();
        redraw = 1;
    }

    if( ( game_state == GAME_OVER ) && noexit && !win_gui )
    {
        gui.show_win_gui( game_data.time );
        win_gui = 1;
    }

    if( redraw )
    {
        redraw = 0;
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

    get_desktop_resolution( 0, &desktop_xsize, &desktop_ysize );

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

    board.max_xsize = desktop_xsize * max_display_factor;
    board.max_ysize = desktop_ysize * max_display_factor; // change this later to something adequate
    board.type_of_tiles = set.type_of_tiles;
    board.number_of_columns = game_data.number_of_columns;
    board.column_height = game_data.column_height;

    if( board.create_board( &game_data, 1 ) )
    {
        SPDLOG_ERROR( "Failed to create game board." );
        noexit = false;
        return;
    }

    if( !fullscreen )
    {
        al_set_target_backbuffer( display );
        al_resize_display( display, board.xsize, board.ysize );
        al_set_window_position( display, ( desktop_xsize - board.xsize ) / 2, ( desktop_ysize - board.ysize ) / 2 );
        al_acknowledge_resize( display );
        al_set_target_backbuffer( display );
    }

    al_convert_bitmaps(); // turn bitmaps to memory bitmaps after resize (bug in allegro doesn't autoconvert)

    gui.update_guis( board.all.x, board.all.y, board.xsize, board.ysize );

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
    redraw = 1;
    noexit = 1;
    mouse_move = 0;
    restart = 0;
    keypress = 0;
    resizing = 0;
    mouse_button_down = 0;
    resize_update = 0;
    resize_time = 0;
    if( game_state != GAME_INTRO )
        game_state = GAME_PLAYING;
    board.time_start = al_get_time();
    blink_time = 0;
    board.blink = 0;
    mbdown_x = 0;
    mbdown_y = 0;
    touch_down = 0;
    tb_down = tb_up = NULL;
    win_gui = 0;

    show_info_text(
        &board,
        al_ustr_newf( "Click on clue for info. Click %s for help, %s for settings, or %s for a hint at any time.",
                      symbol_char[board.column_height][1],
                      symbol_char[board.column_height][2],
                      symbol_char[board.column_height][0] ) );

    al_set_target_backbuffer( display );
    al_clear_to_color( BLACK_COLOR );
    al_flip_display();
    al_flush_event_queue( gui.event_queue );
    play_time = old_time = al_get_time();

    while( noexit )
    {
        game_inner_loop();
    }
}
