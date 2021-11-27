#pragma once

#include "board.hpp"

#include "settings.hpp"

class Gui
{
public:
    Gui( Settings &settings_current, Settings &settings_new );

    void get_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score );
    void save_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score );
    void draw_guis();

    int handle_gui_event( ALLEGRO_EVENT *event );
    void handle_gui_event_text_changed( ALLEGRO_EVENT *event, WZ_WIDGET *wgt, WZ_WIDGET *gui );
    void handle_gui_event_button_pressed( ALLEGRO_EVENT *event, WZ_WIDGET *wgt, WZ_WIDGET *gui );
    void handle_gui_event_settings( ALLEGRO_EVENT *event, WZ_WIDGET *wgt, WZ_WIDGET *gui );
    void handle_gui_event_params( ALLEGRO_EVENT *event, WZ_WIDGET *wgt, WZ_WIDGET *gui );

    int gui_send_event( ALLEGRO_EVENT *event );
    void init_theme();
    void init_guis( int x, int y, int width, int height );
    void destroy_base_gui();
    void add_gui( WZ_WIDGET *base, WZ_WIDGET *gui );
    void remove_gui( WZ_WIDGET *base );
    void remove_all_guis();
    void update_guis( int x, int y, int width, int height );
    void confirm_restart( Settings *new_set );
    void confirm_exit();
    void confirm_save();
    void confirm_load();
    void show_settings();
    void show_help();
    void show_about();
    void show_win_gui( double time );
    void draw_text_gui( ALLEGRO_USTR *text );

    void emit_event( int event_type );
    void update_base_gui( float dt );

    void params_gui( Board *board, ALLEGRO_EVENT_QUEUE *queue );

    ALLEGRO_EVENT_SOURCE user_event_src;

    ALLEGRO_EVENT_QUEUE *event_queue;
    WZ_WIDGET *guis[10];

    int gui_n;
    int gui_font_h;
    WZ_WIDGET *base_gui;

    WZ_SKIN_THEME *skin_theme;
    ALLEGRO_FONT *gui_font;

    char hi_name[10][64];
    double hi_score[10];
    int hi_pos;

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
    void create_win_gui_high_scores( double time, WZ_WIDGET *gui, int gui_w );
    WZ_WIDGET *create_win_gui( double time );
    WZ_WIDGET *create_params_gui();
    WZ_WIDGET *create_text_gui( ALLEGRO_USTR *text );
    void show_params();
};
