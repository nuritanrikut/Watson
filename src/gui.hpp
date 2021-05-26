#pragma once

#include "board.hpp"
#include "main.hpp"

// Prototypes

//void draw_center_textbox_wait(const char *text, float width_factor, Board *board, ALLEGRO_EVENT_QUEUE *queue);

//int show_settings(Settings *set, Board *board, ALLEGRO_EVENT_QUEUE *queue);
//void draw_multiline_wz_box(const char *text, int cx, int cy, int width, int max_height);
//int yes_no_gui(Board *board, ALLEGRO_USTR *text, int center_x, int center_y, int min_width, ALLEGRO_EVENT_QUEUE *queue);

void draw_guis( void );
int handle_gui_event( ALLEGRO_EVENT *event );
int gui_send_event( ALLEGRO_EVENT *event );
void init_theme( void );
void init_guis( int x, int y, int width, int height );
void destroy_base_gui( void );
void remove_all_guis( void );
void update_guis( int x, int y, int width, int height );
void confirm_restart( Settings *new_set );
void confirm_exit( void );
void confirm_save( void );
void confirm_load( void );
void show_settings( void );
void show_help( void );
void show_about( void );
void show_win_gui( double time );
void draw_text_gui( ALLEGRO_USTR *text );

void update_base_gui( float dt );

void params_gui( Board *board, ALLEGRO_EVENT_QUEUE *queue );
// wrapper for WidgetZ gui to use cstr instead of ustr
