#pragma once

#include "board.hpp"

#include "settings.hpp"

class Gui
{
public:
    Gui( Settings &settings_current, Settings &settings_new );

    void get_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score );
    void save_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score );
    void draw_guis( void );
    int handle_gui_event( ALLEGRO_EVENT *event );
    int gui_send_event( ALLEGRO_EVENT *event );
    void init_theme( void );
    void init_guis( int x, int y, int width, int height );
    void destroy_base_gui( void );
    void add_gui( WZ_WIDGET *base, WZ_WIDGET *gui );
    void remove_gui( WZ_WIDGET *base );
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

    void emit_event( int event_type );
    void update_base_gui( float dt );

    void params_gui( Board *board, ALLEGRO_EVENT_QUEUE *queue );

    ALLEGRO_EVENT_SOURCE user_event_src;

    ALLEGRO_EVENT_QUEUE *event_queue;
    WZ_WIDGET *guis[10];

    int gui_n;

private:
    Settings &settings_current;
    Settings &settings_new;

private:
    void wz_set_text_own( WZ_WIDGET *wgt, ALLEGRO_USTR *text );
    void init_theme_noskin();
    void scale_gui( float factor );
    void destroy_theme();

    WZ_WIDGET *new_widget( int id, int x, int y );
    WZ_WIDGET *create_msg_gui( int id, ALLEGRO_USTR *msg );
    WZ_WIDGET *create_yesno_gui( int id, int button_ok_id, int button_cancel_id, ALLEGRO_USTR *msg );
    WZ_WIDGET *create_settings_gui();
    WZ_WIDGET *create_win_gui( double time );
    WZ_WIDGET *create_params_gui();
    WZ_WIDGET *create_text_gui( ALLEGRO_USTR *text );
    void show_params();
};
