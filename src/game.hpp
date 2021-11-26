#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#define ALLEGRO_UNSTABLE

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

#include "allegro_stuff.hpp"
#include "bitmaps.hpp"
#include "board.hpp"
#include "dialog.hpp"
#include "game_data.hpp"
#include "gui.hpp"
#include "macros.hpp"
#include "sound.hpp"
#include "text.hpp"
#include "tiled_block.hpp"

#include "board.hpp"

constexpr double FPS = 60.0;
constexpr double DELTA_DOUBLE_CLICK = 0.2;
constexpr double DELTA_SHORT_CLICK = 0.1;
constexpr double DELTA_HOLD_CLICK = 0.3;
constexpr double RESIZE_DELAY = 0.04;
constexpr double BLINK_DELAY = 0.3;
constexpr double BLINK_TIME = 0.05;
constexpr double FIXED_DT = 1.0 / FPS;

struct PanelState
{
    int tile[8][8][8];
    PanelState *parent;
};

class Game
{
public:
    Game();

    bool init();
    bool run();
    bool cleanup();

private:
    void halt( ALLEGRO_EVENT_QUEUE *queue );
    void emit_event( int event_type );

    void handle_mouse_click( TiledBlock *tiled_block, int mx, int my, int mclick );
    void handle_mouse_click_panel_tile( TiledBlock *tiled_block, int mclick );
    void handle_mouse_click_panel_block( TiledBlock *tiled_block, int mclick );
    void handle_mouse_click_clue_tile( TiledBlock *tiled_block, int mx, int my, int mclick );
    void handle_mouse_click_button_clue();
    void handle_mouse_click_button_settings();
    void handle_mouse_click_button_help();
    void handle_mouse_click_button_undo();

    void mouse_grab( int mx, int my );
    void mouse_drop( int mx, int my );
    TiledBlock *get_TiledBlock_at( int x, int y );
    void destroy_undo();
    ALLEGRO_USTR *get_hint_info_text( RELATION relation, char *b0, char *b1, char *b2, char *b3 );
    void explain_clue( Clue *clue );
    void game_loop();
    void destroy_everything();
    int toggle_fullscreen();
    void handle_allegro_event_display_close();
    void handle_allegro_event_display_resize();
    void handle_allegro_event_redraw();
    void handle_event_switch_tiles();
    void draw_stuff();
    void update_board();
    void show_hint();
    void update_guessed();
    void execute_undo();
    void save_state();
    void switch_solve_puzzle();
    int save_game_f();
    int load_game_f();
    void swap_clues( TiledBlock *c1, TiledBlock *c2 );
    void zoom_TB( TiledBlock *tiled_block );
    void animate_win();
    void draw_generating_puzzle( Settings *settings );
    int switch_tiles();
    void win_or_lose();
    void handle_event_restart();
    void handle_event_exit();
    void handle_event_save();
    bool handle_event_load();
    void handle_event_settings();
    void handle_allegro_event_touch_begin( ALLEGRO_EVENT &ev );
    void handle_allegro_event_mouse_button_down( ALLEGRO_EVENT &ev );
    void handle_allegro_event_touch_end( ALLEGRO_EVENT &ev );
    void handle_allegro_event_mouse_button_up( ALLEGRO_EVENT &ev );
    void handle_allegro_event_touch_move( ALLEGRO_EVENT &ev );
    void handle_allegro_event_mouse_axes( ALLEGRO_EVENT &ev );
    void handle_allegro_event_key_char( ALLEGRO_EVENT &ev );
    void handle_events();
    void game_inner_loop();

private:
    Settings set;
    Settings nset; // settings for new game

    Gui gui;

    ALLEGRO_DISPLAY *display;

    bool noexit;
    RESTART_STATE restart;
    bool redraw;
    bool mouse_move;
    bool keypress;
    bool resizing;
    bool resize_update;
    int mouse_button_down; // 0=left mouse button, 1=right mouse button
    bool win_gui;

    // tiled_block where mouse was pressed
    TiledBlock *tb_down;

    // tiled_block where mouse was released
    TiledBlock *tb_up;

    double mouse_up_time;
    double mouse_down_time;

    bool wait_for_double_click;
    HOLD_CLICK_CHECK hold_click_check;

    // pos where mouse was pressed
    int mbdown_x;
    int mbdown_y;

    bool touch_down;

    double resize_time;
    double old_time;
    double blink_time;
    double play_time;
    bool swap_mouse_buttons;

    GAME_STATE game_state;
    int desktop_width;
    int desktop_height;

    bool fullscreen;

    GameData game_data;
    Board board;

    PanelState *undo;
};
