/* Koro's macros */
#pragma once

#pragma region helper functions

#define SWITCH( x ) ( ( x = ( x ) ? ( 0 ) : ( 1 ) ) )

#define nfree( x ) \
    do \
    { \
        free( ( x ) ); \
        ( x ) = NULL; \
    } while( 0 )
#define ndestroy_bitmap( x ) \
    do \
    { \
        al_destroy_bitmap( ( x ) ); \
        ( x ) = NULL; \
    } while( 0 )

#define al_rewind( x ) al_fseek( x, 0, ALLEGRO_SEEK_SET )

#pragma endregion

#pragma region version related

#define PRE_VERSION "0.9.0"
#define PRE_DATE "2021-05-27"

#pragma endregion

#pragma region font related

#define DEFAULT_FONT_FILE "fonts/text_font_fixed.ttf"

#define BF_CODEPOINT_START 0x0860

#define MAX_BF_BITMAPS 32

#pragma endregion

#pragma region game play related

#define MAX_CLUES 100

#pragma endregion

#pragma region colors

#define NULL_COLOR ( al_map_rgba_f( 0, 0, 0, 0 ) )
#define WHITE_COLOR ( al_map_rgba_f( 1, 1, 1, 1 ) )
#define WINE_COLOR ( al_map_rgba_f( 0.4, 0.1, 0.1, 1 ) )
#define GREY_COLOR ( al_map_rgba_f( 0.5, 0.5, 0.5, 1 ) )
#define WINDOW_BG_COLOR ( al_map_rgba_f( 0.3, 0.3, 0.3, 1 ) )
#define WINDOW_BD_COLOR ( al_map_rgba_f( 1, 1, 1, 1 ) )
#define DARK_GREY_COLOR ( al_map_rgba_f( .2, .2, .2, 1 ) )
#define BLACK_COLOR ( al_map_rgb( 0, 0, 0 ) )

#define SYMBOL_COLOR al_map_rgba( 255, 0, 0, 255 )

#define BG_COLOR NULL_COLOR
#define PANEL_BD_COLOR NULL_COLOR
#define PANEL_BG_COLOR DARK_GREY_COLOR
#define PANEL_COLUMN_BG_COLOR DARK_GREY_COLOR
#define PANEL_COLUMN_BD_COLOR GREY_COLOR
#define CLUE_TILE_BG_COLOR NULL_COLOR
#define CLUE_TILE_BD_COLOR NULL_COLOR
#define PANEL_TILE_BD_COLOR NULL_COLOR
#define CLUE_PANEL_BG_COLOR DARK_GREY_COLOR
#define CLUE_PANEL_BD_COLOR GREY_COLOR
#define INFO_PANEL_BG_COLOR DARK_GREY_COLOR
#define INFO_PANEL_BD_COLOR GREY_COLOR
#define TIME_PANEL_BG_COLOR DARK_GREY_COLOR
#define TIME_PANEL_BD_COLOR GREY_COLOR

#define GUI_BG_COLOR al_map_rgb( 100, 100, 100 )
#define GUI_TEXT_COLOR al_map_rgb( 255, 255, 255 )

#pragma endregion

#pragma region events

#define BASE_USER_EVENT_TYPE ALLEGRO_GET_EVENT_TYPE( 'c', 'c', 'c', 'c' )
#define EVENT_REDRAW ( BASE_USER_EVENT_TYPE + 1 )
#define EVENT_SWITCH_TILES ( BASE_USER_EVENT_TYPE + 2 )
#define EVENT_RESTART ( BASE_USER_EVENT_TYPE + 3 )
#define EVENT_EXIT ( BASE_USER_EVENT_TYPE + 4 )
#define EVENT_LOAD ( BASE_USER_EVENT_TYPE + 5 )
#define EVENT_SAVE ( BASE_USER_EVENT_TYPE + 6 )
#define EVENT_SETTINGS ( BASE_USER_EVENT_TYPE + 7 )

#pragma endregion

#pragma region gui

#define TIME_PANEL_BUTTONS 4

#pragma endregion

#pragma region audio

#define RESERVED_SAMPLES 64

#pragma endregion

#pragma region Widget states
/*
Various constants defined by WidgetZ

Constants: Widget States

WZ_STATE_HIDDEN           - Widget is hidden
WZ_STATE_DISABLED         - Widget is disabled
WZ_STATE_HAS_FOCUS        - Widget has focus
WZ_STATE_NOTWANT_FOCUS    - Widget does not want focus
WZ_STATE_LAYOUT           - Widget is a layout widget
*/
#define WZ_STATE_HIDDEN 02
#define WZ_STATE_DISABLED 04
#define WZ_STATE_HAS_FOCUS 010
#define WZ_STATE_NOTWANT_FOCUS 020
#define WZ_STATE_LAYOUT 040

#pragma endregion

#pragma region Widget styles

/*
Constants: Widget Styles

WZ_STYLE_DISABLED          - Widget is disabled
WZ_STYLE_FOCUSED           - Widget has focus
WZ_STYLE_DOWN              - Widget is down
*/
#define WZ_STYLE_DISABLED 02
#define WZ_STYLE_FOCUSED 04
#define WZ_STYLE_DOWN 020

#pragma endregion