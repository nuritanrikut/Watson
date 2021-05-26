#define ALLEGRO_UNSTABLE
// INTEGRATION WITH ANDROID
//#define _DEBUG

/*

 Watson, a logic game.
 by Koro (1/2016)

 Todo
 - fix splash screen in android (don't update until screen position is correct)
 - fix sound (use multiple samples to avoid delay?)
 - thicker arrows in bitmaps
 - add android keyboard input to highscore table
 - finish tutorial
 
 
 NOTE: in android, to draw text to a bitmap without issues we need the bitmap to be created detached from the screen.
 so we set a null target bitmap before creating them and then we use al_convert_bitmaps
 */

#include "main.hpp"

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

#include "macros.hpp"
#include "game.hpp"
#include "sound.hpp"
#include "tiled_block.hpp"
#include "board.hpp"
#include "allegro_stuff.hpp"
#include "game.hpp"
#include "bitmaps.hpp"
#include "dialog.hpp"
#include "text.hpp"
#include "gui.hpp"

#define FPS 60.0

// defaults:
Settings set = {
    6, // number_of_columns
    6, // column_height
    0, // advanced
    0, // sound_mute
    0, // type_of_tiles
    0, // fat_fingers
    0, // restart
    0  // saved
};

// this is mainly for testing, not actually used. use emit_event(EVENT_TYPE) to emit user events.
#define BASE_USER_EVENT_TYPE ALLEGRO_GET_EVENT_TYPE( 'c', 'c', 'c', 'c' )
#define EVENT_REDRAW ( BASE_USER_EVENT_TYPE + 1 )
ALLEGRO_EVENT_SOURCE user_event_src;

ALLEGRO_EVENT_QUEUE *event_queue = NULL;
WZ_WIDGET *guis[10];
int gui_n = 0;

float fixed_dt = 1.0 / FPS;
float RESIZE_DELAY = 0.04;
float BLINK_DELAY = .3;
float BLINK_TIME = .5;
int swap_mouse_buttons = 0;

GAME_STATE game_state;
int desktop_xsize, desktop_ysize;
int fullscreen;

struct Panel_State
{
    int tile[8][8][8];
    struct Panel_State *parent;
};

struct Panel_State *undo = NULL;

// Prototypes
void draw_stuff( Board *board );
void handle_mouse_click( Game *game, Board *board, TiledBlock *tiled_block, int x, int y, int mclick );
void update_board( Game *game, Board *board );
void mouse_grab( Board *board, int mx, int my );
void mouse_drop( Board *board, int mx, int my );
TiledBlock *settings_block( void );
TiledBlock *get_TiledBlock_at( Board *board, int x, int y );
void show_hint( Game *game, Board *board );
void update_guessed( Game *game );
void execute_undo( Game *game );
void save_state( Game *game );
void destroy_undo();
void switch_solve_puzzle( Game *game, Board *board );
int save_game_f( Game *game, Board *board );
int load_game_f( Game *game, Board *board );
void explain_clue( Board *board, Clue *clue );

//void blink_TB(TiledBlock *tiled_block){
//    ALLEGRO_BITMAP *screen = screenshot();
//    int x,y;
//
//    get_TiledBlock_offset(tiled_block, &x, &y);
//    al_draw_filled_rectangle(x,y, x+tiled_block->width, y+tiled_block->height, al_premul_rgba(255,255,255,150));
//    al_flip_display();
//    al_rest(0.1);
//    al_draw_bitmap(screen,0,0,0);
//    al_flip_display();
//    al_rest(0.2);
//    al_draw_bitmap(screen,0,0,0);
//    al_draw_filled_rectangle(x,y,x+ tiled_block->width, y+tiled_block->height, al_premul_rgba(255,255,255,150));
//    al_flip_display();
//    al_rest(0.1);
//    al_draw_bitmap(screen,0,0,0);
//    al_flip_display();
//    al_destroy_bitmap(screen);
//
//}

//ALLEGRO_TRANSFORM T;
//al_identity_transform(&T);
//al_scale_transform(&T, 5, 5);
//al_use_transform(&T);
//al_draw_filled_rectangle(0,0,80, 20, BLACK_COLOR);
//al_draw_textf(default_font, WHITE_COLOR, 0,0, ALLEGRO_ALIGN_LEFT, "%d, %d | %d, %d", nset.number_of_columns, nset.column_height, set->number_of_columns, set->column_height);
//al_identity_transform(&T);
//al_use_transform(&T);

void halt( ALLEGRO_EVENT_QUEUE *queue )
{
    ALLEGRO_DISPLAY *disp = al_get_current_display();
    al_acknowledge_drawing_halt( disp );
    deblog( "ACKNOWLEDGED HALT" );
    ALLEGRO_EVENT ev;
    al_set_default_voice( NULL ); // otherwise it keeps streaming when the app is on background
    do
    {
        al_wait_for_event( queue, &ev );
    } while( ev.type != ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING );
    al_restore_default_mixer();
    al_acknowledge_drawing_resume( disp );
    deblog( "ACKNOWLEDGED RESUME" );
    al_rest( 0.01 );
    al_flush_event_queue( queue );
}

// debug: show solution
void switch_solve_puzzle( Game *game, Board *board )
{
    SWAP( game->guess, game->puzzle );
    update_board( game, board );
}

void emit_event( int event_type )
{
    static ALLEGRO_EVENT user_event = { 0 };

    user_event.type = event_type;
    al_emit_user_event( &user_event_src, &user_event, NULL );
}

void add_gui( WZ_WIDGET *base, WZ_WIDGET *gui )
{
    wz_register_sources( gui, event_queue );
    if( base )
    {
        if( base->last_child )
            wz_enable( base->last_child, 0 );
        wz_attach( gui, base );
        wz_update( base, 0 );
        gui_n++;
    }
    else
    {
        wz_update( gui, 0 );
    }

    emit_event( EVENT_REDRAW );
}

void remove_gui( WZ_WIDGET *wgt )
{
    if( wgt->prev_sib )
        wz_enable( wgt->prev_sib, 1 );
    wz_destroy( wgt );
    gui_n--;
    emit_event( EVENT_REDRAW );
}

void draw_stuff( Board *board )
{
    //xxx todo: there's still the issue that the timer and some fonts appear broken in some android devices
    // ex: samsung galaxy s2
    // probably has to do with memory->video bitmaps
    int x, y;

    al_clear_to_color( BLACK_COLOR ); // (board->bg_color);

    if( game_state == GAME_INTRO )
    {
        draw_title();
    }
    else
    {
        draw_TiledBlock( &board->all, 0, 0 );

        if( board->rule_out )
        {
            if( board->blink )
            {
                highlight_TiledBlock( board->rule_out );
                highlight_TiledBlock( board->highlight );
            }
        }
        else
        {
            if( board->highlight )
                highlight_TiledBlock( board->highlight );
        }

        if( board->dragging )
        { // redraw tile to get it on top
            get_TiledBlock_offset( board->dragging->parent, &x, &y );
            draw_TiledBlock( board->dragging, x, y ); //board->dragging->parent->x, board->dragging->parent->y);
        }

        if( board->zoom )
        {
            al_draw_filled_rectangle( 0, 0, board->max_xsize, board->max_ysize, al_premul_rgba( 0, 0, 0, 150 ) );
            al_use_transform( &board->zoom_transform );
            // draw dark background in case of transparent elements
            al_draw_filled_rectangle( board->zoom->x,
                                      board->zoom->y,
                                      board->zoom->x + board->zoom->width,
                                      board->zoom->y + board->zoom->height,
                                      board->zoom->parent->bg_color );
            draw_TiledBlock( board->zoom, 0, 0 );
            al_use_transform( &board->identity_transform );
        }
    }

    draw_guis();
};

void animate_win( Board *board )
{
    int k, ii, jj, kk = 0;
    int x, y;
    ALLEGRO_BITMAP *currbuf = al_get_target_bitmap();
    ALLEGRO_BITMAP *bmp = al_clone_bitmap( currbuf );
    ALLEGRO_EVENT ev;
    int arr[64];

    al_set_target_bitmap( bmp );
    al_clear_to_color( BLACK_COLOR );
    draw_stuff( board );
    al_set_target_bitmap( currbuf );

    for( k = 0; k < board->number_of_columns * board->column_height; k++ )
    {
        arr[k] = k;
    }

    shuffle( arr, board->number_of_columns * board->column_height );

    for( k = 0; k < board->number_of_columns * board->column_height; k++ )
    {
        al_clear_to_color( BLACK_COLOR );
        al_draw_bitmap( bmp, 0, 0, 0 );
        for( kk = 0; kk <= k; kk++ )
        {
            ii = arr[kk] / board->column_height;
            jj = arr[kk] % board->column_height;
            get_TiledBlock_offset( board->panel.sub[ii]->sub[jj], &x, &y );
            al_draw_filled_rectangle( x,
                                      y,
                                      x + board->panel.sub[ii]->sub[jj]->width,
                                      y + board->panel.sub[ii]->sub[jj]->height,
                                      al_premul_rgba( 0, 0, 0, 200 ) );
        }
        al_flip_display();
        if( !set.sound_mute )
        {
            play_sound( SOUND_STONE );
        }
        while( al_get_next_event( event_queue, &ev ) )
        {
            if( ev.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING )
            {
                halt( event_queue );
                al_acknowledge_resize( al_get_current_display() );
                return;
                // should do something else here
            }
        }
        al_rest( max( 0.15, 0.6 * ( 1 - sqrt( (float)k / ( board->column_height * board->number_of_columns ) ) ) ) );
    }
}

void draw_generating_puzzle( Settings *settings, Board *board )
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

    draw_text_gui( msg );
}

int switch_tiles( Game *game, Board *board, ALLEGRO_DISPLAY *display )
{
    // cycle through tyle types (font, bitmap, classic)
    deblog( "Swtiching tiles." );
    al_set_target_backbuffer( display );
    destroy_all_bitmaps( board );
    board->type_of_tiles = ( board->type_of_tiles + 1 ) % 3;
    if( init_bitmaps( board ) )
    { // cycle through all three options
        board->type_of_tiles = ( board->type_of_tiles + 1 ) % 3;
        if( init_bitmaps( board ) )
            board->type_of_tiles = ( board->type_of_tiles + 1 ) % 3;
        if( init_bitmaps( board ) )
        {
            errlog( "Error switching tiles." );
            exit( -1 );
        }
    }
    board->max_xsize = al_get_display_width( display );
    board->max_ysize = al_get_display_height( display );
    create_board( game, board, 0 );
    al_set_target_backbuffer( display );
    update_board( game, board );
    al_convert_bitmaps();
    al_clear_to_color( BLACK_COLOR );
    al_flip_display();
    return 0;
}

void win_or_lose( Game *game, Board *board )
{
    if( check_solution( game ) )
    {
        game_state = GAME_OVER;
        show_info_text( board, al_ustr_new( "Elementary, watson!" ) );
        animate_win( board );
        if( !set.sound_mute )
            play_sound( SOUND_WIN );
    }
    else
    {
        show_info_text( board, al_ustr_new( "Something is wrong. Try again, go to settings to start a new puzzle." ) );
        if( !set.sound_mute )
            play_sound( SOUND_WRONG );
        execute_undo( game );
        update_board( game, board );
        emit_event( EVENT_REDRAW );
    }
}

void destroy_everything( Board *board )
{
    destroy_board( board );
    destroy_sound();
    destroy_undo();
    remove_all_guis();
    destroy_base_gui();
}

int toggle_fullscreen( Game *game, Board *board, ALLEGRO_DISPLAY **display )
{
    ALLEGRO_DISPLAY *newdisp;
    float display_factor;

    get_desktop_resolution( 0, &desktop_xsize, &desktop_ysize );

    if( !fullscreen )
    {
        deblog( "Entering full screen mode." );
        al_set_new_display_flags( ALLEGRO_FULLSCREEN | ALLEGRO_OPENGL );
        display_factor = 1;
    }
    else
    {
        deblog( "Exiting full screen mode." );
        al_set_new_display_flags( ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE | ALLEGRO_OPENGL );
        display_factor = 0.9;
    }

    newdisp = al_create_display( desktop_xsize * display_factor, desktop_ysize * display_factor );
    if( !newdisp )
    {
        fprintf( stderr, "Error switching fullscreen mode.\n" );
        return 0;
    }

    //    init_fonts(); // do this after creating display OK

    SWITCH( fullscreen );
    destroy_board( board );

    al_destroy_display( *display );

    *display = newdisp;
    al_set_target_backbuffer( *display );
    board->max_xsize = desktop_xsize * display_factor;
    board->max_ysize = desktop_ysize * display_factor;

    create_board( game, board, fullscreen ? 2 : 1 );
    al_set_target_backbuffer( *display );

    if( !fullscreen )
    {
        al_resize_display( *display, board->xsize, board->ysize );
        al_set_window_position( *display, ( desktop_xsize - board->xsize ) / 2, ( desktop_ysize - board->ysize ) / 2 );
        al_acknowledge_resize( *display );
        al_set_target_backbuffer( *display );
    }

    update_board( game, board );
    al_convert_bitmaps();
    return 1;
}

int main( int argc, char **argv )
{
    Game game = { 0 };
    Board board = { 0 };
    ALLEGRO_EVENT ev;
    ALLEGRO_DISPLAY *display = NULL;
    double resize_time, old_time;
    double blink_time = 0, play_time = 0;
    int noexit, mouse_click, redraw, mouse_move, keypress, resizing, resize_update, mouse_button_down, restart, win_gui;
    float max_display_factor;
    TiledBlock *tb_down = NULL, *tb_up = NULL;
    double mouse_up_time = 0, mouse_down_time = 0;
    int wait_for_double_click = 0, hold_click_check = 0;
    float DELTA_DOUBLE_CLICK = 0.2;
    float DELTA_SHORT_CLICK = 0.1;
    float DELTA_HOLD_CLICK = 0.3;
    int mbdown_x, mbdown_y, touch_down;

    // seed random number generator. comment out for debug
    srand( (unsigned int)time( NULL ) );

    deblog( "Watson v" PRE_VERSION " - " PRE_DATE " has started." );
    if( init_allegro() )
        return -1;

#ifndef _WIN32
    // use anti-aliasing if available (seems to cause problems in windows)
    al_set_new_display_option( ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST );
    al_set_new_display_option( ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST );
#endif

    fullscreen = 0;

    // use vsync if available
    al_set_new_display_option( ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST );

    get_desktop_resolution( 0, &desktop_xsize, &desktop_ysize );

    if( !MOBILE )
    {
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
    }
    else
    {
        al_set_new_display_option(
            ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_SUGGEST );

        al_set_new_display_flags( ALLEGRO_FULLSCREEN_WINDOW );
        display = al_create_display( desktop_xsize, desktop_ysize );
        set.fat_fingers = 1;
    }

    if( !display )
    {
        errlog( "Failed to create display!" );
        return -1;
    }
    else
    {
        deblog( "Display created." );
    }

    al_set_target_backbuffer( display );
    al_clear_to_color( BLACK_COLOR );

    al_set_window_title( display, "Watson" );
    al_init_user_event_source( &user_event_src );

    if( !load_game_f( &game, &board ) )
    {
        set.saved = 1;
        deblog( "Saved game found." );
    }
    else
    {
        deblog( "No saved game found." );
    }

    restart = 0;
    game_state = GAME_INTRO;

    init_guis( 0, 0, al_get_display_width( display ), al_get_display_height( display ) );

RESTART:

    if( restart )
    {
        deblog( "Restarting game." );
        remove_all_guis();
    }

    if( restart != 2 )
    {                                 // 2 is for loaded game
        game.advanced = set.advanced; // use "what if" depth 1?
        game.number_of_columns = set.number_of_columns;
        game.column_height = set.column_height;
        game.time = 0;
        draw_stuff( &board );
        draw_generating_puzzle( &set, &board );
        al_flip_display();
        create_game_with_clues( &game );
    }
    else
    { // board should be updated only after destroying the board
        deblog( "Resuming loaded game." );
        set.advanced = game.advanced;
        set.number_of_columns = game.number_of_columns;
        set.column_height = game.column_height;
    }

    if( restart == 1 )
        game.time = 0; // new game, otherwise it's a load game

    get_desktop_resolution( 0, &desktop_xsize, &desktop_ysize );

    if( !fullscreen && !MOBILE )
    {
        max_display_factor = 0.9;
    }
    else
        max_display_factor = 1;

    if( restart )
    {
        al_set_target_backbuffer( display );
        destroy_board( &board );
        destroy_undo();
        al_set_target_backbuffer( display );
    }

    restart = 0;

    board.max_xsize = desktop_xsize * max_display_factor;
    board.max_ysize = desktop_ysize * max_display_factor; // change this later to something adequate
    board.type_of_tiles = set.type_of_tiles;
    board.number_of_columns = game.number_of_columns;
    board.column_height = game.column_height;

    if( create_board( &game, &board, 1 ) )
    {
        errlog( "Failed to create game board." );
        return -1;
    }

    if( !MOBILE && !fullscreen )
    {
        al_set_target_backbuffer( display );
        al_resize_display( display, board.xsize, board.ysize );
        al_set_window_position( display, ( desktop_xsize - board.xsize ) / 2, ( desktop_ysize - board.ysize ) / 2 );
        al_acknowledge_resize( display );
        al_set_target_backbuffer( display );
    }

    al_convert_bitmaps(); // turn bitmaps to memory bitmaps after resize (bug in allegro doesn't autoconvert)

    update_guis( board.all.x, board.all.y, board.xsize, board.ysize );

    if( !event_queue )
    {
        event_queue = al_create_event_queue();
        if( !event_queue )
        {
            errlog( "failed to create event queue." );
            al_destroy_display( display );
            return -1;
        }

        al_register_event_source( event_queue, al_get_display_event_source( display ) );
        if( al_is_keyboard_installed() )
            al_register_event_source( event_queue, al_get_keyboard_event_source() );
        if( al_is_mouse_installed() )
            al_register_event_source( event_queue, al_get_mouse_event_source() );
        if( al_is_touch_input_installed() )
        {
            al_register_event_source( event_queue, al_get_touch_input_event_source() );
            al_set_mouse_emulation_mode( ALLEGRO_MOUSE_EMULATION_NONE );
        }

        al_register_event_source( event_queue, &user_event_src );
    }

    update_board( &game, &board );
    al_set_target_backbuffer( display );

    //  initialize flags
    redraw = 1;
    mouse_click = 0;
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
    al_flush_event_queue( event_queue );
    play_time = old_time = al_get_time();

    while( noexit )
    {
        double dt = al_current_time() - old_time;
        al_rest( fixed_dt - dt ); //rest at least fixed_dt
        dt = al_get_time() - old_time;
        if( game_state == GAME_PLAYING )
            game.time += dt;
        old_time = al_get_time();
        // al_wait_for_event(event_queue, &ev);

        update_base_gui( dt );

        while( al_get_next_event( event_queue, &ev ) )
        { // empty out the event queue
            if( ev.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING )
            {
                deblog( "RECEIVED HALT" );
                halt( event_queue );
                resize_update = 1;
            }

            if( gui_n && gui_send_event( &ev ) )
                continue;

            switch( ev.type )
            {
                case ALLEGRO_EVENT_DISPLAY_CLOSE:
                    emit_event( EVENT_EXIT );
                    break;

                case ALLEGRO_EVENT_DISPLAY_RESIZE:
                    if( board.dragging )
                        mouse_drop( &board, -1, -1 );
                    if( fullscreen )
                        break;
                    al_acknowledge_resize( display );
                    resizing = 1;
                    resize_time = al_get_time();
                    break;

                case EVENT_REDRAW:
                    redraw = 1;
                    break;

                case EVENT_SWITCH_TILES:
                    switch_tiles( &game, &board, display );
                    al_flush_event_queue( event_queue );
                    redraw = 1;
                    break;

                case EVENT_RESTART:
                    restart = 1;
                    set = nset;
                    goto RESTART;

                case EVENT_EXIT:
                    noexit = 0;
                    break;

                case EVENT_SAVE:
                    if( game_state != GAME_PLAYING )
                        break;
                    if( !save_game_f( &game, &board ) )
                    {
                        show_info_text( &board, al_ustr_new( "Game saved" ) );
                        set.saved = 1;
                    }
                    else
                    {
                        show_info_text( &board, al_ustr_new( "Error: game could not be saved." ) );
                    }
                    break;

                case EVENT_LOAD:
                    if( load_game_f( &game, &board ) )
                    {
                        show_info_text( &board, al_ustr_new( "Error game could not be loaded." ) );
                    }
                    else
                    {
                        restart = 2;
                        goto RESTART;
                    }
                    break;

                case EVENT_SETTINGS:
                    show_settings();
                    emit_event( EVENT_REDRAW );
                    break;

                case ALLEGRO_EVENT_TOUCH_BEGIN:
                    if( gui_n )
                        break;
                    ev.mouse.x = ev.touch.x;
                    ev.mouse.y = ev.touch.y;
                    ev.mouse.button = 1;
                    touch_down = 1;
                case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                    if( gui_n )
                        break;
                    if( mouse_button_down )
                        break;
                    mouse_down_time = al_get_time(); // workaround ev.any.timestamp for touch;
                    mbdown_x = ev.mouse.x;
                    mbdown_y = ev.mouse.y;
                    tb_down = get_TiledBlock_at( &board, ev.mouse.x, ev.mouse.y );

                    if( board.zoom && !tb_down )
                    {
                        board.zoom = NULL;
                        redraw = 1;
                        break;
                    }

                    if( wait_for_double_click )
                    {
                        wait_for_double_click = 0;
                        if( ( tb_up == tb_down ) && ( mouse_down_time - mouse_up_time < DELTA_DOUBLE_CLICK ) )
                        {
                            handle_mouse_click( &game, &board, tb_down, ev.mouse.x, ev.mouse.y, 3 ); // double click
                            break;
                        }
                    }
                    mouse_button_down = ev.mouse.button;
                    break;

                case ALLEGRO_EVENT_TOUCH_END:
                    ev.mouse.x = ev.touch.x;
                    ev.mouse.y = ev.touch.y;
                    ev.mouse.button = 1;
                    touch_down = 0;
                case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
                    if( wait_for_double_click )
                        wait_for_double_click = 0;

                    if( board.dragging )
                    {
                        mouse_drop( &board, ev.mouse.x, ev.mouse.y );
                        mouse_button_down = 0;
                    }

                    if( hold_click_check == 2 )
                    {
                        hold_click_check = 0;
                        mouse_button_down = 0;
                        break;
                    }
                    hold_click_check = 0;

                    if( !mouse_button_down )
                        break;

                    mouse_up_time = al_get_time(); //workaround ev.any.timestamp for touch;
                    tb_up = get_TiledBlock_at( &board, ev.mouse.x, ev.mouse.y );
                    if( ( tb_up ) && ( tb_up == tb_down ) )
                    {
                        if( ( ( tb_up->type == TB_HCLUE_TILE ) || ( tb_up->type == TB_VCLUE_TILE ) )
                            && ( mouse_button_down == 1 ) )
                        {
                            if( mouse_up_time - mouse_down_time < DELTA_SHORT_CLICK )
                            {
                                wait_for_double_click = 1;
                            }
                        }
                        if( !wait_for_double_click )
                            handle_mouse_click( &game, &board, tb_up, ev.mouse.x, ev.mouse.y, mouse_button_down );
                    }
                    mouse_button_down = 0;
                    break;

                case ALLEGRO_EVENT_TOUCH_MOVE:
                    if( gui_n )
                        break;
                    if( !ev.touch.primary )
                        break;
                    ev.mouse.x = ev.touch.x;
                    ev.mouse.y = ev.touch.y;
                    //                    ev.mouse.dx = ev.touch.dx;
                    //                    ev.mouse.dy = ev.touch.dy;
                case ALLEGRO_EVENT_MOUSE_AXES:
                    if( gui_n )
                        break;
                    if( board.dragging )
                    {
                        board.dragging->x = ev.mouse.x + board.dragging_relative_position_of_grabbing_x;
                        board.dragging->y = ev.mouse.y + board.dragging_relative_position_of_grabbing_y;
                    }
                    mouse_move = 1;
                    // don't grab if movement was small
                    if( ( abs( ev.mouse.x - mbdown_x ) < 10 ) && ( ( ev.mouse.y - mbdown_y ) < 10 ) )
                        break;

                    if( mouse_button_down && !hold_click_check )
                        if( tb_down && ( ( tb_down->type == TB_HCLUE_TILE ) || ( tb_down->type == TB_VCLUE_TILE ) ) )
                        {
                            handle_mouse_click( &game, &board, tb_down, mbdown_x, mbdown_y, 4 );
                            hold_click_check = 1;
                        }
                    break;

                case ALLEGRO_EVENT_KEY_CHAR:
                    if( gui_n )
                        break;
                    keypress = 1;
                    switch( ev.keyboard.keycode )
                    {
                        case ALLEGRO_KEY_ESCAPE:
                            confirm_exit();
                            break;
                        case ALLEGRO_KEY_BACK: // android: back key
                            show_settings();
                            break;
                        case ALLEGRO_KEY_R:
                            confirm_restart( &set );
                            break;
                        case ALLEGRO_KEY_S: // debug: show solution
                            if( game_state != GAME_PLAYING )
                                break;
                            switch_solve_puzzle( &game, &board );
                            redraw = 1;
                            break;
                        case ALLEGRO_KEY_T:
                            emit_event( EVENT_SWITCH_TILES );
                            break;
                        case ALLEGRO_KEY_H:
                            show_help();
                            break;
                        case ALLEGRO_KEY_C:
                            show_hint( &game, &board );
                            redraw = 1;
                            break;
                        case ALLEGRO_KEY_F:
                            if( toggle_fullscreen( &game, &board, &display ) )
                            {
                                al_register_event_source( event_queue, al_get_display_event_source( display ) );
                            }
                            al_flush_event_queue( event_queue );
                            redraw = 1;
                            break;
                        case ALLEGRO_KEY_U:
                            if( game_state != GAME_PLAYING )
                                break;
                            execute_undo( &game );
                            update_board( &game, &board );
                            // why flush?
                            al_flush_event_queue( event_queue );
                            redraw = 1;
                            break;
                        case ALLEGRO_KEY_SPACE:
                            // tests:
                            //params_gui(&game, &board, event_queue);
                            // game.time = 1;
                            //show_win_gui(game.time);
                            break;
                    }
                    break;
            }
        } // while(al_get_next_event(event_queue, &ev));

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
            if( MOBILE )
            {
                destroy_all_bitmaps( &board );
                init_bitmaps( &board );
            }
            else
                destroy_board_bitmaps( &board );
            board.max_xsize = al_get_display_width( display );
            board.max_ysize = al_get_display_height( display );
            create_board( &game, &board, 0 );
            update_guis( board.all.x, board.all.y, board.xsize, board.ysize );
            al_set_target_backbuffer( display );
            update_board( &game, &board );
            al_convert_bitmaps(); // turn bitmaps to video bitmaps
            redraw = 1;
            // android workaround, try removing:
            al_clear_to_color( BLACK_COLOR );
            al_flip_display();
        }

        if( resizing ) // skip redraw and other stuff
            continue;

        if( wait_for_double_click && ( al_get_time() - mouse_up_time > DELTA_DOUBLE_CLICK ) )
        {
            wait_for_double_click = 0;
            tb_down = get_TiledBlock_at( &board, mbdown_x, mbdown_y );
            handle_mouse_click( &game, &board, tb_down, mbdown_x, mbdown_y, 1 ); // single click
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
                    tb_up = get_TiledBlock_at( &board, tbdx, tbdy );

                    if( tb_up == tb_down )
                    {
                        handle_mouse_click( &game, &board, tb_up, tbdx, tbdy, 4 ); // hold click
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
                update_timer( (int)game.time, &board ); // this draws on a timer bitmap

                if( game.guessed == game.column_height * game.number_of_columns )
                {
                    win_or_lose( &game, &board ); // check if player has won
                    al_flush_event_queue( event_queue );
                }
                emit_event( EVENT_REDRAW );
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
            show_win_gui( game.time );
            win_gui = 1;
        }

        if( redraw )
        {
            redraw = 0;
            al_set_target_backbuffer( display );
            draw_stuff( &board );
            al_flip_display();
        }

    } // end of game loop

    destroy_everything( &board );
    al_destroy_display( display );
    al_destroy_event_queue( event_queue );
    return ( 0 );
}

void update_guessed( Game *game )
{
    int i, j, k, count, val;

    game->guessed = 0;

    for( i = 0; i < game->number_of_columns; i++ )
    {
        for( j = 0; j < game->column_height; j++ )
        {
            count = 0;
            val = -1;
            for( k = 0; k < game->number_of_columns; k++ )
            {
                if( game->tile[i][j][k] )
                {
                    count++;
                    val = k;
                    if( count > 1 )
                        break;
                }
            }
            if( count == 1 )
            {
                game->guess[i][j] = val;
                game->guessed++;
            }
            else
                game->guess[i][j] = -1;
        }
    }
}

void update_board( Game *game, Board *board )
{
    int i, j, k;

    for( i = 0; i < game->number_of_columns; i++ )
    {
        for( j = 0; j < game->column_height; j++ )
        {
            if( game->guess[i][j] >= 0 )
            {
                board->panel.sub[i]->sub[j]->number_of_subblocks = 0;
                board->panel.sub[i]->sub[j]->bmp = &( board->guess_bmp[j][game->guess[i][j]] );
            }
            else
            {
                board->panel.sub[i]->sub[j]->number_of_subblocks = board->number_of_columns;
                board->panel.sub[i]->sub[j]->bmp = NULL;
                for( k = 0; k < game->number_of_columns; k++ )
                {
                    if( game->tile[i][j][k] )
                    {
                        board->panel.sub[i]->sub[j]->sub[k]->hidden = 0;
                    }
                    else
                    {
                        board->panel.sub[i]->sub[j]->sub[k]->hidden = 1;
                    }
                }
            }
        }
    }
}

void swap_clues( Board *board, TiledBlock *c1, TiledBlock *c2 )
{
    SWAP( c1->bmp, c2->bmp );

    if( c1->index >= 0 )
        board->clue_tiledblock[c1->index] = c2;
    if( c2->index >= 0 )
        board->clue_tiledblock[c2->index] = c1;
    SWAP( c1->index, c2->index );
    SWAP( c1->hidden, c2->hidden );
};

TiledBlock *get_TiledBlock_at( Board *board, int x, int y )
{
    float xx = x, yy = y;
    TiledBlock *tiled_block;

    if( board->zoom )
    {
        al_transform_coordinates( &board->zoom_transform_inv, &xx, &yy );
        tiled_block = get_TiledBlock( board->zoom, xx, yy );
        if( tiled_block && ( tiled_block->parent == board->zoom ) )
            return tiled_block;
        else
            return NULL; // else return get_TiledBlock(&board->all, x, y);
    }

    return get_TiledBlock( &board->all, x, y );
}

void mouse_grab( Board *board, int mx, int my )
{
    emit_event( EVENT_REDRAW );
    board->dragging = get_TiledBlock_at( board, mx, my );
    if( board->dragging && board->dragging->bmp )
    {
        if( ( board->dragging->type == TB_HCLUE_TILE ) || ( board->dragging->type == TB_VCLUE_TILE ) )
        {
            board->dragging_origin_x = board->dragging->x;
            board->dragging_origin_y = board->dragging->y;
            board->dragging_relative_position_of_grabbing_x = board->dragging->x - mx + 5;
            board->dragging_relative_position_of_grabbing_y = board->dragging->y - my + 5;
            board->dragging->x = mx + board->dragging_relative_position_of_grabbing_x;
            board->dragging->y = my + board->dragging_relative_position_of_grabbing_y;
            // if(!set.sound_mute) play_sound(SOUND_UNHIDE_TILE);
            return;
        }
    }

    board->dragging = NULL;
    return;
};

void mouse_drop( Board *board, int mx, int my )
{
    TiledBlock *tiled_block;

    emit_event( EVENT_REDRAW );
    if( !board->dragging )
        return;
    board->dragging->x = board->dragging_origin_x;
    board->dragging->y = board->dragging_origin_y;

    tiled_block = get_TiledBlock_at( board, mx, my );
    if( tiled_block && ( tiled_block->type == board->dragging->type ) )
    {
        swap_clues( board, board->dragging, tiled_block );
        if( board->highlight == board->dragging )
            board->highlight = tiled_block;
        else if( board->highlight == tiled_block )
            board->highlight = board->dragging;
        if( !set.sound_mute )
            play_sound( SOUND_HIDE_TILE );
    }
    board->dragging = NULL;
    clear_info_panel( board );
    return;
};

void destroy_undo()
{
    struct Panel_State *foo;

    while( undo )
    {
        foo = undo->parent;
        free( undo );
        undo = foo;
    }
}

void execute_undo( Game *game )
{
    Panel_State *undo_old;

    if( !undo )
        return;
    memcpy( &game->tile, &undo->tile, sizeof( game->tile ) );
    undo_old = undo->parent;
    free( undo );
    undo = undo_old;
    if( !set.sound_mute )
        play_sound( SOUND_UNHIDE_TILE );
    update_guessed( game );
}

void save_state( Game *game )
{
    Panel_State *foo = new Panel_State();
    foo->parent = undo;
    undo = foo;
    memcpy( &undo->tile, &game->tile, sizeof( undo->tile ) );
}

void explain_clue( Board *board, Clue *clue )
{
    char *b0, *b1, *b2;
    b0 = symbol_char[clue->j[0]][clue->k[0]];
    b1 = symbol_char[clue->j[1]][clue->k[1]];
    b2 = symbol_char[clue->j[2]][clue->k[2]];

    switch( clue->rel )
    {
        case CONSECUTIVE:
            show_info_text(
                board,
                al_ustr_newf(
                    "The column of %s is between %s and %s, but they could be on either side.", b1, b0, b2 ) );
            break;
        case NEXT_TO:
            show_info_text(
                board,
                al_ustr_newf(
                    "The columns of %s and %s are next to each other, but they could be on either side.", b0, b1 ) );
            break;
        case NOT_NEXT_TO:
            show_info_text( board, al_ustr_newf( "The column of %s is NOT next to the column of %s.", b0, b1 ) );
            break;
        case NOT_MIDDLE:
            show_info_text(
                board,
                al_ustr_newf(
                    "There is exactly one column between %s and %s, and %s is NOT in that column.", b0, b2, b1 ) );
            break;
        case ONE_SIDE:
            show_info_text( board, al_ustr_newf( "The column of %s is strictly to the left of %s.", b0, b1 ) );
            break;
        case TOGETHER_2:
            show_info_text( board, al_ustr_newf( "%s and %s are on the same column.", b0, b1 ) );
            break;
        case TOGETHER_3:
            show_info_text( board, al_ustr_newf( "%s, %s and %s are on the same column.", b0, b1, b2 ) );
            break;
        case NOT_TOGETHER:
            show_info_text( board, al_ustr_newf( "%s and %s are NOT on the same column.", b0, b1 ) );
            break;
        case TOGETHER_NOT_MIDDLE:
            show_info_text(
                board, al_ustr_newf( "%s and %s are on the same column, and %s is NOT in that column.", b0, b2, b1 ) );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            show_info_text( board,
                            al_ustr_newf( "%s is on the same column of either %s or %s, but NOT BOTH.", b0, b1, b2 ) );
            break;
        default:
            break;
    }
}

void show_hint( Game *game, Board *board )
{
    char *b0, *b1, *b2, *b3;
    int i;

    if( game_state != GAME_PLAYING )
        return;
    if( !check_panel_correctness( game ) )
    {
        show_info_text( board, al_ustr_new( "Something is wrong. An item was ruled out incorrectly." ) );
        return;
    }

    i = get_hint( game );
    if( !i )
    {
        show_info_text( board, al_ustr_new( "No hint available." ) );
        return;
    }

    board->highlight = board->clue_tiledblock[i & 255];
    board->rule_out = board->panel.sub[( i >> 15 ) & 7]->sub[( i >> 12 ) & 7]->sub[( i >> 9 ) & 7];

    b0 = symbol_char[game->clue[i & 255].j[0]][game->clue[i & 255].k[0]];
    b1 = symbol_char[game->clue[i & 255].j[1]][game->clue[i & 255].k[1]];
    b2 = symbol_char[game->clue[i & 255].j[2]][game->clue[i & 255].k[2]];
    b3 = symbol_char[( i >> 12 ) & 7][( i >> 9 ) & 7];

    switch( game->clue[i & 255].rel )
    {
        case CONSECUTIVE:
            show_info_text(
                board,
                al_ustr_newf(
                    "The column of %s is between %s and %s, so we can rule out %s from here.", b1, b0, b2, b3 ) );
            break;
        case NEXT_TO:
            show_info_text(
                board,
                al_ustr_newf(
                    "The columns of %s and %s are next to each other, so we can rule out %s from here.", b0, b1, b3 ) );
            break;
        case NOT_NEXT_TO:
            show_info_text(
                board,
                al_ustr_newf( "The column of %s is NOT next to the column of %s, so we can rule out %s from here.",
                              b0,
                              b1,
                              b3 ) );
            break;
        case NOT_MIDDLE:
            show_info_text( board,
                            al_ustr_newf( "There is exactly one column between %s and %s, and %s is NOT in that "
                                          "column, so we can rule out %s from here.",
                                          b0,
                                          b2,
                                          b1,
                                          b3 ) );
            break;
        case ONE_SIDE:
            show_info_text(
                board,
                al_ustr_newf(
                    "The column of %s is strictly to the left of %s, so we can rule out %s from here.", b0, b1, b3 ) );
            break;
        case TOGETHER_2:
            show_info_text(
                board,
                al_ustr_newf( "%s and %s are on the same column, so we can rule out %s from here.", b0, b1, b3 ) );
            break;
        case TOGETHER_3:
            show_info_text(
                board,
                al_ustr_newf(
                    "%s, %s and %s are on the same column, so we can rule out %s from here.", b0, b1, b2, b3 ) );
            break;
        case NOT_TOGETHER:
            show_info_text(
                board,
                al_ustr_newf( "%s and %s are NOT on the same column, so we can rule out %s from here.", b0, b1, b3 ) );
            break;
        case TOGETHER_NOT_MIDDLE:
            show_info_text(
                board,
                al_ustr_newf(
                    "%s and %s are on the same column, and %s is NOT in that column, so we can rule out %s from here.",
                    b0,
                    b2,
                    b1,
                    b3 ) );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            show_info_text(
                board,
                al_ustr_newf(
                    "%s is on the same column of either %s or %s, but NOT BOTH, so we can rule out %s from here.",
                    b0,
                    b1,
                    b2,
                    b3 ) );
            break;

        default:
            break;
    }
}

void zoom_TB( Board *board, TiledBlock *tiled_block )
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

    al_identity_transform( &board->identity_transform );
    al_identity_transform( &board->zoom_transform );
    al_build_transform( &board->zoom_transform, tr_x, tr_y, c, c, 0 );
    if( tiled_block->parent )
        get_TiledBlock_offset( tiled_block->parent, &x, &y );
    al_translate_transform( &board->zoom_transform, c * x, c * y );
    board->zoom_transform_inv = board->zoom_transform;
    al_invert_transform( &board->zoom_transform_inv );
    board->zoom = tiled_block;
    if( !set.sound_mute )
        play_sound( SOUND_CLICK );
}

void handle_mouse_click( Game *game, Board *board, TiledBlock *tiled_block, int mx, int my, int mclick )
{
    int i, j, k;

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

    clear_info_panel( board ); // remove text if there was any

    if( board->highlight )
    {
        board->highlight = NULL;
    }

    if( board->rule_out )
    {
        board->rule_out = NULL;
    }

    emit_event( EVENT_REDRAW );

    if( !tiled_block )
        return;

    if( game_state == GAME_OVER )
    {
        show_info_text( board, al_ustr_new( "Press R to start a new puzzle." ) );
    }

    if( set.fat_fingers )
    {
        if( !board->zoom || ( tiled_block->parent != board->zoom ) )
        {
            if( ( ( tiled_block->parent ) && ( tiled_block->parent->type == TB_TIME_PANEL ) )
                || ( tiled_block->type == TB_TIME_PANEL ) )
            {
                zoom_TB( board, &board->time_panel );
                return;
            }
            else if( tiled_block->type == TB_PANEL_TILE )
            {
                zoom_TB( board, board->zoom = tiled_block->parent );
                return;
            }
        }
    }

    if( board->zoom )
        board->zoom = NULL;

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
                save_state( game );
                if( game->tile[i][j][k] )
                { // hide tile
                    hide_tile_and_check( game, i, j, k );
                    if( !set.sound_mute )
                        play_sound( SOUND_HIDE_TILE );
                }
            }
            else if( ( mclick == 2 ) || ( mclick == 4 ) )
            { // hold or right click
                if( game->tile[i][j][k] )
                {
                    save_state( game );
                    guess_tile( game, i, j, k );
                    if( !set.sound_mute )
                        play_sound( SOUND_GUESS_TILE );
                }
                else
                { // tile was hidden, unhide
                    if( !is_guessed( game, j, k ) )
                    {
                        game->tile[i][j][k] = 1;
                        if( !set.sound_mute )
                            play_sound( SOUND_UNHIDE_TILE );
                    }
                }
            }
            update_board( game, board );
            break;

        case TB_PANEL_BLOCK:
            if( game_state != GAME_PLAYING )
                break;
            if( ( ( mclick == 2 ) || ( mclick == 4 ) )
                && ( game->guess[tiled_block->parent->index][tiled_block->index] >= 0 ) )
            {
                // we found guessed block - unguess it
                save_state( game );
                unguess_tile( game, tiled_block->parent->index, tiled_block->index );
                if( !set.sound_mute )
                    play_sound( SOUND_UNHIDE_TILE );
            }
            update_board( game, board );
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
                    SWITCH( game->clue[tiled_block->index].hidden );
                    if( !set.sound_mute )
                        play_sound( SOUND_HIDE_TILE );
                }
                else if( mclick == 1 )
                { // explain clue in info panel
                    //                    {
                    //                    //xxx DEBUG text
                    //                    char str[1000];
                    //                        snprintf(str, 999, "Clue number %d", tiled_block->index);
                    //                    show_info_text_b(board,str);
                    //                    }
                    if( !tiled_block->hidden )
                    {
                        if( !set.sound_mute )
                            play_sound( SOUND_CLICK );
                        explain_clue( board, &game->clue[tiled_block->index] );
                        board->highlight = tiled_block; // highlight clue
                    }
                }
                else if( mclick == 4 )
                { // hold-click
                    mouse_grab( board, mx, my );
                    //                    if(MOBILE){
                    //                        board->highlight = tiled_block;
                    //                        show_info_text(board, "Tap somewhere in the clue box to move this clue");
                    //
                    //                    }
                }
            }
            break;

        case TB_BUTTON_CLUE: // time panel
            if( game_state != GAME_PLAYING )
                break;
            if( !set.sound_mute )
                play_sound( SOUND_CLICK );
            show_hint( game, board );
            break;
        case TB_BUTTON_SETTINGS:
            if( !set.sound_mute )
                play_sound( SOUND_CLICK );
            show_settings();
            break;

        case TB_BUTTON_HELP:
            if( !set.sound_mute )
                play_sound( SOUND_CLICK );
            show_help();
            break;

        case TB_BUTTON_UNDO:
            execute_undo( game );
            update_board( game, board );
            break;

        case TB_BUTTON_TILES:
            emit_event( EVENT_SWITCH_TILES );
            break;

        default:
            break;
    }
}

// work in progress
// actually highscores must include string + int. Maybe do one file for each mode.
void get_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score )
{
    ALLEGRO_PATH *path;
    ALLEGRO_FILE *fp;
    char filename[100];
    int i;

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
#endif

    path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );
    snprintf( filename, 99, "Watson%dx%d-%d.hi", number_of_columns, h, advanced );
    al_set_path_filename( path, filename );

    fp = al_fopen( al_path_cstr( path, '/' ), "rb" );
    if( !fp || !( al_fread( fp, name, 64 * sizeof( char ) * 10 ) == 64 * sizeof( char ) * 10 )
        || !( al_fread( fp, score, 10 * sizeof( double ) ) == 10 * sizeof( double ) ) )
    {
        errlog( "Error reading %s.", (char *)al_path_cstr( path, '/' ) );
        memset( name, 0, 64 * sizeof( char ) * 10 );
        for( i = 0; i < 10; i++ )
        {
            score[i] = 600;
        }
    }

    al_fclose( fp );
    al_destroy_path( path );
}

void save_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score )
{
    ALLEGRO_PATH *path;
    ALLEGRO_FILE *fp;
    char filename[100];

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
#endif

    path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );
    deblog( "ALLEGRO_USER_DATA_PATH = %s", al_path_cstr( path, '/' ) );

    if( !al_make_directory( al_path_cstr( path, '/' ) ) )
    {
        errlog( "could not open or create path %s.\n", al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        return;
    }

    snprintf( filename, 99, "Watson%dx%d-%d.hi", number_of_columns, h, advanced );
    al_set_path_filename( path, filename );
    fp = al_fopen( al_path_cstr( path, '/' ), "wb" );
    if( !fp )
    {
        errlog( "Couldn't open %s for writing.\n", (char *)al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        al_fclose( fp );
        return;
    }

    al_fwrite( fp, name, 64 * sizeof( char ) * 10 );
    al_fwrite( fp, score, sizeof( double ) * 10 );
    al_fclose( fp );

    deblog( "Saved highscores at %s.", al_path_cstr( path, '/' ) );
    al_destroy_path( path );
}

// work in progress
int save_game_f( Game *game, Board *board )
{
    ALLEGRO_PATH *path;
    ALLEGRO_FILE *fp;

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
#endif

    path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );
    deblog( "ALLEGRO_USER_DATA_PATH = %s", al_path_cstr( path, '/' ) );

    if( !al_make_directory( al_path_cstr( path, '/' ) ) )
    {
        errlog( "could not open or create path %s.\n", al_path_cstr( path, '/' ) );
        return -1;
    }

    al_set_path_filename( path, "Watson.sav" );
    fp = al_fopen( al_path_cstr( path, '/' ), "wb" );
    if( !fp )
    {
        errlog( "Couldn't open %s for writing.\n", (char *)al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        return -1;
    }
    al_fwrite( fp, &game->number_of_columns, sizeof( game->number_of_columns ) );
    al_fwrite( fp, &game->column_height, sizeof( game->column_height ) );
    al_fwrite( fp, &game->puzzle, sizeof( game->puzzle ) );
    al_fwrite( fp, &game->clue_n, sizeof( game->clue_n ) );
    al_fwrite( fp, &game->clue, sizeof( game->clue ) );
    al_fwrite( fp, &game->tile, sizeof( game->tile ) );
    al_fwrite( fp, &game->time, sizeof( game->time ) );
    al_fclose( fp );

    deblog( "Saved game at %s.", al_path_cstr( path, '/' ) );
    al_destroy_path( path );
    return 0;
}

int load_game_f( Game *game, Board *board )
{
    ALLEGRO_PATH *path;
    ALLEGRO_FILE *fp;
    //    ALLEGRO_FS_ENTRY *entry;

#ifdef ALLEGRO_ANDROID
    al_set_standard_file_interface();
#endif
    path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );
    al_set_path_filename( path, "Watson.sav" );
    fp = al_fopen( al_path_cstr( path, '/' ), "rb" );
    if( !fp )
    {
        errlog( "Couldn't open %s for reading.", (char *)al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        return -1;
    }

    if( al_fread( fp, &game->number_of_columns, sizeof( game->number_of_columns ) )
        != sizeof( game->number_of_columns ) )
    {
        al_destroy_path( path );
        errlog( "Error reading %s.", (char *)al_path_cstr( path, '/' ) );
        return -1;
    }

    al_fread( fp, &game->column_height, sizeof( game->column_height ) );
    al_fread( fp, &game->puzzle, sizeof( game->puzzle ) );
    al_fread( fp, &game->clue_n, sizeof( game->clue_n ) );
    al_fread( fp, &game->clue, sizeof( game->clue ) );
    al_fread( fp, &game->tile, sizeof( game->tile ) );
    al_fread( fp, &game->time, sizeof( game->time ) );
    al_fclose( fp );

    update_guessed( game );
    /* delete saved game after reading
    entry = al_create_fs_entry(al_path_cstr(path, '/'));
    al_remove_fs_entry(entry);
    */
    al_destroy_path( path );

    return 0;
}
