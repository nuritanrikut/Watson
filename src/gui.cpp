#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "gui.hpp"

#include <cstdio>
#include <cmath>
#include <algorithm>

#include <spdlog/spdlog.h>

#include "widgetz/widgetz.hpp"
#include "allegro_stuff.hpp"
#include "text.hpp"
#include "game_data.hpp"

const char ABOUT_TEXT[] =
    "Watson v%s - %s.\n"
    "C version by Koro. Ported to C++ by nuritanrikut\n"
    "\n"
    "Watson is an open source clone of \"Sherlock\", an old game by Evertt Kaser which is itself based on the a "
    "classic puzzle known as \"Zebra puzzle\" or \"Einstein's riddle\".\n"
    "\n"
    "The game has been compiled in Windows, Mac OS, Linux and Android thanks to the amazing Allegro 5 library. Big "
    "thanks to the friendly folks from Freenode #allegro for all the tips and advice. The game GUI uses the WidgetZ "
    "library by SiegeLord.\n"
    "\n"
    "The tile settings_current is rendered from TTF fonts obtained from fontlibrary.org. The icons are from "
    "www.icons8.com. These "
    "icons are located in <appdir>/icons and can be replaced. The sounds are from freesound.org. The text font is "
    "Linux Libertine by Philipp H. Poll. All these assets can be found in their corresponding directories."
    "\n"
    "Watson is licensed under the GPLv3 (this does not include the artwork cited above). The source code and some "
    "binary releases can be found at https://github.com/nuritanrikut/Watson"
    "\n"
    "\n"
    "Note: the \"extra hard\" mode generates (much) harder puzzles, which require indirect logic and hasn't been "
    "properly tested.\n";

const char HELP_TEXT[] =
    "Watson is a puzzle similar to the classic \"Zebra puzzle\" or \"Einstein's Riddle\". The goal is to figure out "
    "which item goes where on the board.\n"
    "The main panel has a number of columns, each dividided into blocks of same-type items. Each item in a block is "
    "secretly assigned to a different column, without repetition. Some of these assignments may be partially revealed "
    "at the beginning. Use the clues provided (right and bottom panels) to deduce which item goes where. \n"
    "\n"
    "QUICK REFERENCE\n"
    "Press the lightbulb for a hint. You can understand the game logic from it.\n"
    "Click/tap on a clue to read about its meaning.\n"
    "Click/tap on a panel tile to rule it out. Hold-click on hidden tiles to bring them back.\n"
    "Hold-click on a panel tile to assign it to a given column.\n"
    "Double-click/tap on a clue to hide/show (if you don't need it anymore).\n"
    "Drag the clues to rearrange.\n"
    "\n"
    "There is an undo button avialable. Go to settings to switch the tileset and other additional options."
    "\n"
    "Keyboard shortcuts: R to start again, ESC to quit, U to undo, C to get a hint, T to switch tiles. You can resize "
    "the window or press F to go fullscreen.\n"
    "\n"
    "DEBUG: S: show/hide solution.\n";

constexpr double GUI_XFACTOR = 0.5;
constexpr double GUI_YFACTOR = 0.5;

WZ_SKIN_THEME *skin_theme = nullptr;
ALLEGRO_FONT *gui_font = nullptr;
int gui_font_h;
WZ_WIDGET *base_gui = nullptr;

char hi_name[10][64];
double hi_score[10];
int hi_pos = -1;

enum
{
    BUTTON_ROWS,
    BUTTON_COLS,
    BUTTON_OK,
    BUTTON_CANCEL,
    BUTTON_SOUND,
    BUTTON_ADVANCED,
    BUTTON_RESTART,
    BUTTON_ABOUT,
    BUTTON_EXIT,
    BUTTON_TILES,
    GROUP_COLS,
    GROUP_ROWS,
    BUTTON_SAVE,
    BUTTON_LOAD,
    BUTTON_ZOOM,
    BUTTON_SETTINGS,
    BUTTON_PARAMS,
    BUTTON_CLOSE,
    BUTTON_SAVE_NOW,
    BUTTON_EXIT_NOW,
    BUTTON_LOAD_NOW,
    BUTTON_RESTART_NOW,
    BUTTON_EXTRA_HARD,
    BUTTON_RESET_PARAMS,
    GUI_SETTINGS,
    GUI_PARAMS,
    EDITBOX_HISCORE,
};

Gui::Gui( Settings &settings_c, Settings &settings_n ) : settings_current( settings_c ), settings_new( settings_n )
{
    event_queue = nullptr;
    gui_n = 0;
}

// work in progress
// actually highscores must include string + int. Maybe do one file for each mode.
void Gui::get_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score )
{
    ALLEGRO_PATH *path;
    ALLEGRO_FILE *fp;
    char filename[100];
    int i;

    path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );
    snprintf( filename, 99, "Watson%dx%d-%d.hi", number_of_columns, h, advanced );
    al_set_path_filename( path, filename );

    fp = al_fopen( al_path_cstr( path, '/' ), "rb" );
    if( !fp || !( al_fread( fp, name, 64 * sizeof( char ) * 10 ) == 64 * sizeof( char ) * 10 )
        || !( al_fread( fp, score, 10 * sizeof( double ) ) == 10 * sizeof( double ) ) )
    {
        SPDLOG_ERROR( "Error reading {}", (char *)al_path_cstr( path, '/' ) );
        memset( name, 0, 64 * sizeof( char ) * 10 );
        for( i = 0; i < 10; i++ )
        {
            score[i] = 10 * 60 * 60;
        }
    }

    al_fclose( fp );
    al_destroy_path( path );
}

void Gui::save_highscores( int number_of_columns, int h, int advanced, char ( *name )[64], double *score )
{
    ALLEGRO_PATH *path;
    ALLEGRO_FILE *fp;
    char filename[100];

    path = al_get_standard_path( ALLEGRO_USER_DATA_PATH );
    SPDLOG_TRACE( "ALLEGRO_USER_DATA_PATH = {}", al_path_cstr( path, '/' ) );

    if( !al_make_directory( al_path_cstr( path, '/' ) ) )
    {
        SPDLOG_ERROR( "could not open or create path {}", al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        return;
    }

    snprintf( filename, 99, "Watson%dx%d-%d.hi", number_of_columns, h, advanced );
    al_set_path_filename( path, filename );
    fp = al_fopen( al_path_cstr( path, '/' ), "wb" );
    if( !fp )
    {
        SPDLOG_ERROR( "Couldn't open {} for writing", (char *)al_path_cstr( path, '/' ) );
        al_destroy_path( path );
        al_fclose( fp );
        return;
    }

    al_fwrite( fp, name, 64 * sizeof( char ) * 10 );
    al_fwrite( fp, score, sizeof( double ) * 10 );
    al_fclose( fp );

    SPDLOG_DEBUG( "Saved highscores at {}", al_path_cstr( path, '/' ) );
    al_destroy_path( path );
}

// even if wgt->own = 1, the original function duplicates the string
// this avoids a memory leak
void Gui::wz_set_text_own( WZ_WIDGET *wgt, ALLEGRO_USTR *text )
{
    wgt->set_text( text );
    al_ustr_free( text );
}

void Gui::draw_guis( void )
{
    WZ_WIDGET *gui;

    if( !base_gui )
        return;

    gui = base_gui->first_child;

    while( gui )
    {
        gui->draw();
        gui = gui->next_sib;
    }
}

// unused
void Gui::init_theme_noskin( void )
{
    if( skin_theme )
        delete skin_theme;
    auto def_theme = get_def_theme();
    skin_theme = new WZ_SKIN_THEME( def_theme );

    gui_font = load_font_mem( text_font_mem, TEXT_FONT_FILE, -gui_font_h );
    skin_theme->font = gui_font;
    skin_theme->color1 = al_map_rgb( 150, 150, 150 );
    skin_theme->color2 = WHITE_COLOR;
}

void Gui::init_theme( void )
{
    if( skin_theme )
        delete skin_theme;
    auto def_skin_theme = get_skin_theme();
    skin_theme = new WZ_SKIN_THEME( def_skin_theme );

    gui_font = load_font_mem( text_font_mem, TEXT_FONT_FILE, -gui_font_h );
    skin_theme->font = gui_font;
    skin_theme->color1 = GUI_BG_COLOR;
    skin_theme->color2 = GUI_TEXT_COLOR;
    skin_theme->button_up_bitmap = al_load_bitmap( "data/button_up.png" );
    skin_theme->button_down_bitmap = al_load_bitmap( "data/button_down.png" );
    skin_theme->box_bitmap = al_load_bitmap( "data/box.png" );
    skin_theme->editbox_bitmap = al_load_bitmap( "data/editbox.png" );
    skin_theme->scroll_track_bitmap = al_load_bitmap( "data/scroll_track.png" );
    skin_theme->slider_bitmap = al_load_bitmap( "data/slider.png" );
    skin_theme->init();
}

void Gui::scale_gui( float factor )
{
    gui_font_h *= factor;
    al_destroy_font( gui_font );
    gui_font = load_font_mem( text_font_mem, TEXT_FONT_FILE, -gui_font_h );
    skin_theme->font = gui_font;
}

void Gui::destroy_theme()
{
    ndestroy_bitmap( skin_theme->button_up_bitmap );
    ndestroy_bitmap( skin_theme->button_down_bitmap );
    ndestroy_bitmap( skin_theme->box_bitmap );
    ndestroy_bitmap( skin_theme->editbox_bitmap );
    ndestroy_bitmap( skin_theme->scroll_track_bitmap );
    ndestroy_bitmap( skin_theme->slider_bitmap );
    al_destroy_font( gui_font );
    gui_font = nullptr;
    skin_theme->font = nullptr;
}

WZ_WIDGET *Gui::new_widget( int id, int x, int y )
{
    WZ_WIDGET *wgt = new WZ_WIDGET( nullptr, x, y, 0, 0, id );
    wgt->set_theme( skin_theme );
    return wgt;
}

void Gui::update_guis( int x, int y, int width, int height )
{
    gui_font_h = height / 30;
    al_destroy_font( gui_font );
    gui_font = load_font_mem( text_font_mem, TEXT_FONT_FILE, -gui_font_h );
    base_gui->resize( (float)height / base_gui->height );
    base_gui->x = x;
    base_gui->y = y;
    base_gui->width = width;
    base_gui->height = height;

    skin_theme->font = gui_font;
    base_gui->update( 0 );
    emit_event( EVENT_REDRAW );
}

void Gui::init_guis( int x, int y, int width, int height )
{
    gui_font_h = height / 35;
    init_theme();
    base_gui = new_widget( -1, x, y );
    base_gui->width = width;
    base_gui->height = height;
}

void Gui::add_gui( WZ_WIDGET *base, WZ_WIDGET *gui )
{
    gui->register_sources( event_queue );
    if( base )
    {
        if( base->last_child )
            base->last_child->enable( 0 );
        gui->attach( base );
        base->update( 0 );
        gui_n++;
    }
    else
    {
        gui->update( 0 );
    }

    emit_event( EVENT_REDRAW );
}

void Gui::remove_gui( WZ_WIDGET *wgt )
{
    if( wgt->prev_sib )
        wgt->prev_sib->enable( 1 );
    wgt->destroy();
    gui_n--;
    emit_event( EVENT_REDRAW );
}

void Gui::remove_all_guis( void )
{
    while( base_gui->last_child )
        remove_gui( base_gui->last_child );
}

void Gui::destroy_base_gui()
{
    remove_all_guis();
    base_gui->destroy();
    destroy_theme();
}

WZ_WIDGET *Gui::create_msg_gui( int id, ALLEGRO_USTR *msg )
{
    int width = base_gui->width / 2;
    int height = gui_font_h * get_multiline_text_lines( gui_font, width, al_cstr( msg ) );
    int i;
    int but_w = 6 * gui_font_h;
    int but_h = gui_font_h * 1.5;
    WZ_WIDGET *wgt, *gui;

    for( i = 0; i < 3; i++ )
    {
        if( height + gui_font_h * 3 + but_h > base_gui->height )
        {
            width += base_gui->width / 4.1;
            height = gui_font_h * get_multiline_text_lines( gui_font, width, al_cstr( msg ) );
        }
        else
        {
            break;
        }
    }

    gui = new_widget(
        id, ( base_gui->width - width - 2 * gui_font_h ) / 2, ( base_gui->height - ( 4 * gui_font_h + height ) ) / 2 );

    wgt = new WZ_BOX( gui, 0, 0, width + 2 * gui_font_h, height + 4 * gui_font_h, -1 );
    wgt->flags |= WZ_STATE_NOTWANT_FOCUS;

    new WZ_TEXTBOX( gui, gui_font_h, gui_font_h, width, height, WZ_ALIGN_LEFT, WZ_ALIGN_TOP, msg, 1, -1 );
    wgt = new WZ_BUTTON( gui,
                         gui_font_h + ( width - but_w * 1.5 ),
                         height + gui_font_h * 2,
                         but_w,
                         but_h,
                         al_ustr_new( "OK" ),
                         1,
                         BUTTON_CLOSE );
    wgt->set_shortcut( ALLEGRO_KEY_ESCAPE, 0 );

    return gui;
}

WZ_WIDGET *Gui::create_yesno_gui( int id, int button_ok_id, int button_cancel_id, ALLEGRO_USTR *msg )
{
    int but_w = 6 * gui_font_h;
    int but_h = gui_font_h * 1.5;
    int width = std::max( int( base_gui->width / 3 ), 2 * but_w + 3 * gui_font_h );
    int height = gui_font_h * get_multiline_text_lines( gui_font, width, al_cstr( msg ) );

    WZ_WIDGET *wgt, *gui = new_widget( id,
                                       ( base_gui->width - width - 2 * gui_font_h ) / 2,
                                       ( base_gui->height - ( 3 * gui_font_h + but_h + height ) ) / 2 );

    wgt = new WZ_BOX( gui, 0, 0, width + 2 * gui_font_h, height + 3 * gui_font_h + but_h, -1 );
    wgt->flags |= WZ_STATE_NOTWANT_FOCUS;

    new WZ_TEXTBOX( gui, gui_font_h, gui_font_h, width, height, WZ_ALIGN_CENTRE, WZ_ALIGN_TOP, msg, 1, -1 );
    new WZ_BUTTON( gui,
                   gui_font_h + ( width - 2 * but_w ) / 3,
                   2 * gui_font_h + height,
                   but_w,
                   but_h,
                   al_ustr_new( "OK" ),
                   1,
                   button_ok_id );
    wgt = new WZ_BUTTON( gui,
                         gui_font_h + but_w + 2 * ( width - 2 * but_w ) / 3,
                         2 * gui_font_h + height,
                         but_w,
                         but_h,
                         al_ustr_new( "Cancel" ),
                         1,
                         button_cancel_id );
    wgt->set_shortcut( ALLEGRO_KEY_ESCAPE, 0 );
    return gui;
}

WZ_WIDGET *Gui::create_settings_gui( void )
{
    int fh = gui_font_h;
    int rh = 2.5 * fh; // row height
    int rn = 0;        // current row
    int gui_w = fh * 20;
    int rows = 7;
    int gui_h = rows * rh;
    int but_w = 30;
    int but_sw = 15;
    int but_h;
    int sep = 10;
    int i;

    settings_new = settings_current;

    but_w = std::max( al_get_text_width( skin_theme->font, "Save game" ), but_w );
    but_w = std::max( al_get_text_width( skin_theme->font, "Load game" ), but_w );
    but_w = std::max( al_get_text_width( skin_theme->font, "Switch tiles" ), but_w );
    but_w = std::max( al_get_text_width( skin_theme->font, "About Watson" ), but_w );
    but_h = fh * 2;

    but_sw = std::max( al_get_text_width( skin_theme->font, "zoom" ), but_sw );
    but_sw += fh;
    but_w += 2 * fh;

    gui_w = std::max( gui_w, 3 * ( but_w + sep ) + sep * 3 );

    // main gui
    WZ_WIDGET *gui = new_widget( GUI_SETTINGS, ( base_gui->width - gui_w ) / 2, ( base_gui->height - rows * rh ) / 2 );

    {
        WZ_WIDGET *wgt = new WZ_BOX( gui, 0, 0, gui_w, gui_h + sep, -1 );
        wgt->flags |= WZ_STATE_NOTWANT_FOCUS;
    }

    auto create_fill_layout = []( WZ_WIDGET *parent,
                                  float x,
                                  float y,
                                  float width,
                                  float height,
                                  float hspace,
                                  float vspace,
                                  int halign,
                                  int valign,
                                  int id ) {
        WZ_WIDGET *wgt = new WZ_FILL_LAYOUT( parent, x, y, width, height, hspace, vspace, halign, valign, id );
        wgt->flags |= WZ_STATE_HIDDEN;

        return wgt;
    };

    // about button
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, sep, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_TOP, -1 );
    new WZ_TEXTBOX( gui,
                    0,
                    0,
                    al_get_text_width( skin_theme->font, "Settings" ),
                    but_h,
                    WZ_ALIGN_LEFT,
                    WZ_ALIGN_CENTRE,
                    al_ustr_new( "Settings" ),
                    1,
                    -1 );

    // number of rows multitoggle
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, sep, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );
    new WZ_TEXTBOX( gui,
                    0,
                    0,
                    al_get_text_width( skin_theme->font, "Columns: " ),
                    but_h,
                    WZ_ALIGN_RIGHT,
                    WZ_ALIGN_CENTRE,
                    al_ustr_new( "Rows:" ),
                    1,
                    -1 );
    for( i = 0; i < 5; i++ )
    {
        WZ_TOGGLE *wgt =
            new WZ_TOGGLE( gui, 0, 0, but_h, but_h, al_ustr_newf( "%c", '4' + i ), 1, GROUP_ROWS, BUTTON_ROWS );
        if( 4 + i == settings_current.column_height )
            wgt->down = 1;
    }

    // number of columns multitoggle
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, sep, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );
    new WZ_TEXTBOX( gui,
                    0,
                    0,
                    al_get_text_width( skin_theme->font, "Columns: " ),
                    but_h,
                    WZ_ALIGN_RIGHT,
                    WZ_ALIGN_CENTRE,
                    al_ustr_new( "Columns:" ),
                    1,
                    -1 );
    for( i = 0; i < 5; i++ )
    {
        WZ_TOGGLE *wgt =
            new WZ_TOGGLE( gui, 0, 0, but_h, but_h, al_ustr_newf( "%c", '4' + i ), 1, GROUP_COLS, BUTTON_COLS );
        if( 4 + i == settings_current.number_of_columns )
            wgt->down = 1;
    }

    // sound + swtich tiles + zoom
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, sep, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );
    {
        WZ_TOGGLE *wgt = new WZ_TOGGLE( gui, 0, 0, but_w, but_h, al_ustr_new( "Zoom" ), 1, -1, BUTTON_ZOOM );
        wgt->down = settings_current.fat_fingers;
    }

    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Switch tiles" ), 1, BUTTON_TILES );
    {
        WZ_TOGGLE *button_mute =
            new WZ_TOGGLE( gui,
                           0,
                           0,
                           but_w,
                           but_h,
                           settings_current.sound_mute ? al_ustr_new( "Sound: off" ) : al_ustr_new( "Sound: on" ),
                           1,
                           -1,
                           BUTTON_SOUND );
        button_mute->down = !settings_current.sound_mute;
    }

    // advanced + save + load buttons
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, sep, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );
    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Advanced" ), 1, BUTTON_PARAMS );
    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Save game" ), 1, BUTTON_SAVE );
    {
        WZ_TOGGLE *wgt = new WZ_TOGGLE( gui, 0, 0, but_w, but_h, al_ustr_new( "Load game" ), 1, -1, BUTTON_LOAD );
        wgt->down = !settings_current.saved;
    }

    // restart/exit/switch tiles buttons
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, sep, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );
    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "New game" ), 1, BUTTON_RESTART );
    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Exit game" ), 1, BUTTON_EXIT );
    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "About Watson" ), 1, BUTTON_ABOUT );

    // ok/cancel buttons
    create_fill_layout( gui, 0, rh * ( rn++ ), gui_w, rh, but_w / 3, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );

    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "OK" ), 1, BUTTON_OK );
    {
        WZ_BUTTON *wgt = new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Cancel" ), 1, BUTTON_CLOSE );
        // escape key cancels and exits
        wgt->set_shortcut( ALLEGRO_KEY_ESCAPE, 0 );
    }

    return gui;
}

WZ_WIDGET *Gui::create_win_gui( double time )
{
    // Initialize Allegro 5 and the font routines
    int gui_w, gui_h, but_w, lh;
    int i, j;

    settings_new = settings_current;

    lh = 1.2 * gui_font_h;
    gui_w = al_get_text_width( gui_font, "You solved the puzzle in 000:000:000" ) + 4 * lh + 2;
    but_w = 2 * lh + std::max( al_get_text_width( gui_font, "Settings" ), al_get_text_width( gui_font, "New game" ) );

    // 13 lines of text + 1.5 for button + 2 for margin = lh*16 (+17 * vspace?)
    gui_h = 16.5 * lh + 2;

    WZ_WIDGET *gui = new_widget( -1, ( base_gui->width - gui_w ) / 2, ( base_gui->height - gui_h ) / 2 );
    new WZ_FILL_LAYOUT( gui, 0, 0, gui_w, gui_h, lh, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_TOP, -1 );
    new WZ_TEXTBOX( gui, 0, 0, gui_w - 2 * lh - 2, lh, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, al_ustr_new( "" ), 1, -1 );

    get_highscores( settings_current.number_of_columns,
                    settings_current.column_height,
                    settings_current.advanced,
                    hi_name,
                    (double *)hi_score );
    if( time > 0 )
    {
        for( i = 0; i < 10; i++ )
        {
            if( hi_score[i] > time )
                break;
        }
    }
    else
    {
        i = 10;
    }

    if( time > 0 )
        new WZ_TEXTBOX(
            gui,
            0,
            0,
            gui_w - 2 * lh - 2,
            lh,
            WZ_ALIGN_CENTRE,
            WZ_ALIGN_CENTRE,
            al_ustr_newf( "You solved the puzzle in %02d:%02d:%02d", (int)time / 3600, (int)time / 60, (int)time % 60 ),
            1,
            -1 );

    new WZ_TEXTBOX(
        gui,
        0,
        0,
        gui_w - 2 * lh - 2,
        lh,
        WZ_ALIGN_CENTRE,
        WZ_ALIGN_CENTRE,
        al_ustr_newf(
            "Best times for %d x %d board:", settings_current.number_of_columns, settings_current.column_height ),
        1,
        -1 );

    if( !( time > 0 ) )
        new WZ_TEXTBOX( gui, 0, 0, gui_w - 2 * lh - 2, lh, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, al_ustr_new( "" ), 1, -1 );

    for( j = 0; j < i; j++ )
    {
        new WZ_TEXTBOX( gui,
                        0,
                        0,
                        ( gui_w - 4 * lh - 2 ) / 2.5,
                        lh,
                        WZ_ALIGN_LEFT,
                        WZ_ALIGN_CENTRE,
                        al_ustr_new( hi_name[j] ),
                        1,
                        -1 );
        new WZ_TEXTBOX(
            gui, 0, 0, ( gui_w - 4 * lh - 2 ) / 5, lh, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, al_ustr_new( "" ), 1, -1 );
        new WZ_TEXTBOX(
            gui,
            0,
            0,
            ( gui_w - 4 * lh - 2 ) / 2.5,
            lh,
            WZ_ALIGN_LEFT,
            WZ_ALIGN_CENTRE,
            al_ustr_newf(
                "%02d:%02d:%02d", (int)hi_score[j] / 3600, ( (int)hi_score[j] / 60 ) % 60, (int)hi_score[j] % 60 ),
            1,
            -1 );
    }

    if( i >= 10 )
    {
        hi_pos = -1;
    }
    else
    {
        hi_pos = i;
        new WZ_EDITBOX( gui, 0, 0, ( gui_w - 4 * lh - 2 ) / 2.5, lh, al_ustr_new( "your name" ), 1, EDITBOX_HISCORE );
        new WZ_TEXTBOX(
            gui, 0, 0, ( gui_w - 4 * lh - 2 ) / 5, lh, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, al_ustr_new( "" ), 1, -1 );
        new WZ_TEXTBOX( gui,
                        0,
                        0,
                        ( gui_w - 4 * lh - 2 ) / 2.5,
                        lh,
                        WZ_ALIGN_LEFT,
                        WZ_ALIGN_CENTRE,
                        al_ustr_newf( "%02d:%02d:%02d", (int)time / 3600, ( (int)time / 60 ) % 60, (int)time % 60 ),
                        1,
                        -1 );

        for( j = i; j < 9; j++ )
        {
            new WZ_TEXTBOX( gui,
                            0,
                            0,
                            ( gui_w - 4 * lh - 2 ) / 2.5,
                            lh,
                            WZ_ALIGN_LEFT,
                            WZ_ALIGN_CENTRE,
                            al_ustr_new( hi_name[j] ),
                            1,
                            -1 );
            new WZ_TEXTBOX(
                gui, 0, 0, ( gui_w - 4 * lh - 2 ) / 5, lh, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, al_ustr_new( "" ), 1, -1 );
            new WZ_TEXTBOX(
                gui,
                0,
                0,
                ( gui_w - 4 * lh - 2 ) / 2.5,
                lh,
                WZ_ALIGN_LEFT,
                WZ_ALIGN_CENTRE,
                al_ustr_newf(
                    "%02d:%02d:%02d", (int)hi_score[j] / 3600, ( (int)hi_score[j] / 60 ) % 60, (int)hi_score[j] % 60 ),
                1,
                -1 );
        }
        memcpy( &hi_name[i + 1], &hi_name[i], 64 * sizeof( char ) * ( 9 - i ) );
        memcpy( &hi_score[i + 1], &hi_score[i], sizeof( double ) * ( 9 - i ) );
        hi_score[i] = time;

        hi_name[i][0] = '\0';
    }

    new WZ_TEXTBOX( gui, 0, 0, gui_w * 0.9, lh, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, al_ustr_new( "" ), 1, -1 );

    new WZ_BUTTON( gui, 0, 0, but_w, 1.5 * lh, al_ustr_new( "New Game" ), 1, BUTTON_RESTART_NOW );
    WZ_BUTTON *wgt = new WZ_BUTTON( gui, 0, 0, but_w, 1.5 * lh, al_ustr_new( "Settings" ), 1, BUTTON_SETTINGS );

    // avoid focus on the settings button
    gui->update( 0 );
    wgt->focus( 0 );

    return gui;
}

WZ_WIDGET *Gui::create_params_gui()
{
    int i;
    ALLEGRO_USTR *rel[NUMBER_OF_RELATIONS];
    int gui_h = 0, gui_w = 0, rel_w = 0, but_w = 0, but_h = 0;
    int lh = gui_font_h * 1.5;

    rel[0] = al_ustr_new( "NEXT_TO" );
    rel[1] = al_ustr_new( "NOT_NEXT_TO" );
    rel[2] = al_ustr_new( "ONE_SIDE" );
    rel[3] = al_ustr_new( "CONSECUTIVE" );
    rel[4] = al_ustr_new( "NOT_MIDDLE" );
    rel[5] = al_ustr_new( "TOGETHER_2" );
    rel[6] = al_ustr_new( "TOGETHER_3" );
    rel[7] = al_ustr_new( "NOT_TOGETHER" );
    rel[8] = al_ustr_new( "TOGETHER_NOT_MIDDLE" );
    rel[9] = al_ustr_new( "TFWOO" );
    rel[10] = al_ustr_new( "REVEAL" );

    but_w = al_get_text_width( gui_font, "Extra hard" ) + 2 * gui_font_h;

    for( i = 0; i <= 10; i++ )
    {
        rel_w = std::max( rel_w, al_get_text_width( gui_font, al_cstr( rel[i] ) ) );
    }

    gui_w = std::max( int( base_gui->width / 2.5 ),
                      2 * gui_font_h + al_get_text_width( gui_font, "Clue type distribution for puzzle creation" ) );
    but_h = lh;
    gui_h = ( NUMBER_OF_RELATIONS + 2 ) * lh;

    WZ_WIDGET *gui = new_widget( GUI_PARAMS,
                                 ( base_gui->width - gui_w ) / 2,
                                 ( base_gui->height - gui_h - ( 2 * but_h + 1.5 * gui_font_h ) ) / 2 );

    new WZ_FILL_LAYOUT( gui, 0, 0, gui_w, gui_h, gui_font_h, 0, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, -1 );

    new WZ_TEXTBOX( gui,
                    0,
                    0,
                    gui_w - 2 * gui_font_h - 2,
                    lh * 2,
                    WZ_ALIGN_CENTRE,
                    WZ_ALIGN_CENTRE,
                    al_ustr_new( "Clue type distribution for puzzle creation" ),
                    1,
                    -1 );

    for( i = 0; i < NUMBER_OF_RELATIONS; i++ )
    {
        new WZ_TEXTBOX( gui, 0, 0, rel_w, lh, WZ_ALIGN_RIGHT, WZ_ALIGN_TOP, rel[i], 1, -1 );
        WZ_SCROLL *wgt =
            new WZ_SCROLL( gui, 0, 0, gui_w - rel_w - 3 * gui_font_h - 2, lh / 2, 50, gui_font_h, i + 1024 );
        wgt->cur_pos = REL_PERCENT[i];
    }

    // buttons
    new WZ_FILL_LAYOUT( gui,
                        0,
                        gui_h,
                        gui_w,
                        2 * but_h + 1.5 * gui_font_h,
                        ( gui_w - 2 * but_w ) / 3.1,
                        gui_font_h / 2,
                        WZ_ALIGN_CENTRE,
                        WZ_ALIGN_LEFT,
                        -1 );

    {
        WZ_TOGGLE *wgt =
            new WZ_TOGGLE( gui, 0, 0, but_w, but_h, al_ustr_new( "Extra hard" ), 1, -1, BUTTON_EXTRA_HARD );
        wgt->down = settings_new.advanced ? 1 : 0;
    }

    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Reset" ), 1, BUTTON_RESET_PARAMS );
    new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "OK" ), 1, BUTTON_OK );

    {
        WZ_BUTTON *wgt = new WZ_BUTTON( gui, 0, 0, but_w, but_h, al_ustr_new( "Cancel" ), 1, BUTTON_CLOSE );
        wgt->set_shortcut( ALLEGRO_KEY_ESCAPE, 0 );
    }

    return gui;
}

WZ_WIDGET *Gui::create_text_gui( ALLEGRO_USTR *text )
{
    int width = base_gui->width / 3;
    int height = gui_font_h * get_multiline_text_lines( gui_font, width, al_cstr( text ) );
    int i;

    for( i = 0; i < 3; i++ )
    {
        if( height + gui_font_h * 2 > base_gui->height )
        {
            width += base_gui->width / 6;
            height = gui_font_h * get_multiline_text_lines( gui_font, width, al_cstr( text ) );
        }
        else
        {
            break;
        }
    }

    WZ_WIDGET *gui = new_widget(
        -1, ( base_gui->width - width - 2 * gui_font_h ) / 2, ( base_gui->height - height - 2 * gui_font_h ) / 2 );

    {
        WZ_BOX *wgt = new WZ_BOX( gui, 0, 0, width + 2 * gui_font_h, height + 2 * gui_font_h, -1 );
        wgt->flags |= WZ_STATE_NOTWANT_FOCUS;
    }

    new WZ_TEXTBOX( gui, gui_font_h, gui_font_h, width, height, WZ_ALIGN_LEFT, WZ_ALIGN_TOP, text, 1, -1 );

    gui->update( 0 );
    return gui;
}

void Gui::confirm_restart( Settings *new_set )
{
    settings_new = *new_set;
    add_gui( base_gui,
             create_yesno_gui( -1,
                               BUTTON_RESTART_NOW,
                               BUTTON_CLOSE,
                               al_ustr_newf( "Start new %dx%d%s game?",
                                             settings_new.number_of_columns,
                                             settings_new.column_height,
                                             settings_new.advanced ? " advanced" : "" ) ) );
}

void Gui::confirm_exit( void )
{
    add_gui( base_gui, create_yesno_gui( -1, BUTTON_EXIT_NOW, BUTTON_CLOSE, al_ustr_newf( "Exit game?" ) ) );
}

void Gui::show_help( void )
{
    add_gui( base_gui, create_msg_gui( -1, al_ustr_new( HELP_TEXT ) ) );
}

void Gui::show_about( void )
{
    add_gui( base_gui, create_msg_gui( -1, al_ustr_newf( ABOUT_TEXT, PRE_VERSION, PRE_DATE ) ) );
}

void Gui::confirm_save( void )
{
    add_gui(
        base_gui,
        create_yesno_gui(
            -1, BUTTON_SAVE_NOW, BUTTON_CLOSE, al_ustr_new( "Save game? This will overwrite\na previous save." ) ) );
}

void Gui::confirm_load( void )
{
    add_gui( base_gui,
             create_yesno_gui(
                 -1, BUTTON_LOAD_NOW, BUTTON_CLOSE, al_ustr_new( "Discard current progress and load saved game?" ) ) );
}

void Gui::show_settings( void )
{
    add_gui( base_gui, create_settings_gui() );
}

void Gui::show_win_gui( double time )
{
    add_gui( base_gui, create_win_gui( time ) );
}

void Gui::show_params( void )
{
    add_gui( base_gui, create_params_gui() );
}

int Gui::handle_gui_event( ALLEGRO_EVENT *event )
{
    WZ_WIDGET *wgt = (WZ_WIDGET *)event->user.data2;
    WZ_WIDGET *gui = (WZ_WIDGET *)wgt->parent;

    if( !gui )
    {
        SPDLOG_DEBUG( "Can't handle unparented gui" );
        return 0;
    }

    if( event->type == WZ_TEXT_CHANGED )
    {
        if( wgt->id == EDITBOX_HISCORE )
        {
            strncpy( hi_name[hi_pos], al_cstr( ( (WZ_TEXTBOX *)wgt )->text ), 63 );
            wgt->flags &= ~WZ_STATE_HAS_FOCUS;
            wgt->flags |= WZ_STATE_NOTWANT_FOCUS;
            save_highscores( settings_current.number_of_columns,
                             settings_current.column_height,
                             settings_current.advanced,
                             hi_name,
                             hi_score );
            remove_gui( gui );
            add_gui( base_gui, create_win_gui( -1 ) );
        }
    }

    // general buttons
    if( event->type == WZ_BUTTON_PRESSED )
    {
        switch( wgt->id )
        {
            case BUTTON_CLOSE:
                remove_gui( gui );
                break;

            case BUTTON_EXIT_NOW:
                emit_event( EVENT_EXIT );
                break;

            case BUTTON_RESTART_NOW:
                emit_event( EVENT_RESTART );
                break;

            case BUTTON_SAVE_NOW:
                emit_event( EVENT_SAVE );
                remove_gui( gui );
                break;

            case BUTTON_LOAD_NOW:
                emit_event( EVENT_LOAD );
                remove_gui( gui );
                break;
            case BUTTON_SETTINGS:
                add_gui( base_gui, create_settings_gui() );
                break;
        }
    }

    if( gui->id == GUI_SETTINGS )
    {
        if( ( event->type == ALLEGRO_EVENT_KEY_CHAR ) && ( event->keyboard.keycode == ALLEGRO_KEY_BACK ) )
        { // android "back" key
            remove_gui( gui );
        }

        if( event->type == WZ_BUTTON_PRESSED )
        {
            switch( wgt->id )
            {
                case BUTTON_EXIT:
                    confirm_exit();
                    break;

                case BUTTON_OK:
                    if( ( settings_new.number_of_columns != settings_current.number_of_columns )
                        || ( settings_new.column_height != settings_current.column_height )
                        || ( settings_new.advanced != settings_current.advanced ) )
                    {
                        confirm_restart( &settings_new );
                    }
                    else
                    {
                        settings_new = settings_current;
                        remove_gui( gui );
                    }
                    break;

                case BUTTON_RESTART:
                    confirm_restart( &settings_new );
                    break;

                case BUTTON_TILES:
                    emit_event( EVENT_SWITCH_TILES );
                    break;

                case BUTTON_ZOOM:
                    settings_current.fat_fingers = !settings_current.fat_fingers;
                    settings_new.fat_fingers = settings_current.fat_fingers;
                    break;

                case BUTTON_ABOUT:
                    show_about();
                    break;

                case BUTTON_SOUND:
                    settings_current.sound_mute = !settings_current.sound_mute;
                    if( settings_current.sound_mute )
                    {
                        wz_set_text_own( (WZ_WIDGET *)event->user.data2, al_ustr_new( "Sound: off" ) );
                    }
                    else
                    {
                        wz_set_text_own( (WZ_WIDGET *)event->user.data2, al_ustr_new( "Sound: on" ) );
                    }
                    settings_new.sound_mute = settings_current.sound_mute;
                    break;

                case BUTTON_PARAMS:
                    show_params();
                    break;

                case BUTTON_SAVE:
                    if( settings_current.saved )
                        confirm_save();
                    else
                        emit_event( EVENT_SAVE );
                    break;

                case BUTTON_LOAD:
                    if( !settings_current.saved )
                    { // if no saved game exists, revert button press
                        ( (WZ_BUTTON *)wgt )->down = 1;
                    }
                    else
                    {
                        confirm_load();
                    }
                    break;

                case BUTTON_ROWS:
                    settings_new.column_height = atoi( al_cstr( ( (WZ_BUTTON *)wgt )->text ) );
                    break;

                case BUTTON_COLS:
                    settings_new.number_of_columns = atoi( al_cstr( ( (WZ_BUTTON *)wgt )->text ) );
                    break;
            }
        }
    }
    else if( gui->id == GUI_PARAMS )
    {
        if( event->type == WZ_BUTTON_PRESSED )
        {
            if( wgt->id == BUTTON_EXTRA_HARD )
            {
                settings_new.advanced = !settings_new.advanced;
            }
            else if( wgt->id == BUTTON_OK )
            {
                wgt = gui->first_child;
                while( wgt )
                {
                    if( wgt->id >= 1024 ) // is slider
                    {
                        REL_PERCENT[wgt->id - 1024] = ( (WZ_SCROLL *)wgt )->cur_pos;
                    }
                    wgt = wgt->next_sib;
                }

                ALLEGRO_USTR *msg = al_ustr_newf(
                    "WARNING: The puzzle generation parameters are here for debug purposes and altering "
                    "them may make the game unbalanced. You can reset these settings later.\n\n%s",
                    settings_new.advanced ? "You also turned on extra hard mode. This mode is not recommended. "
                                            "Puzzle generation will be slower, and solving the puzzles may "
                                            "require thinking ahead many steps and very indirect reasoning."
                                          : "" );
                remove_gui( gui );
                add_gui( base_gui, create_msg_gui( -1, msg ) );
            }
            else if( wgt->id == BUTTON_RESET_PARAMS )
            {
                wgt = gui->first_child;
                reset_rel_params();
                settings_new.advanced = settings_current.advanced = 0;

                // update sliders and button
                while( wgt )
                {
                    if( wgt->id >= 1024 ) // is slider
                    {
                        ( (WZ_SCROLL *)wgt )->cur_pos = REL_PERCENT[wgt->id - 1024];
                    }
                    else if( wgt->id == BUTTON_EXTRA_HARD )
                    {
                        ( (WZ_BUTTON *)wgt )->down = 0;
                    }

                    wgt = wgt->next_sib;
                }
            }
        }
    }

    return 0;
}

void Gui::update_base_gui( float dt )
{
    base_gui->update( dt );
}

int Gui::gui_send_event( ALLEGRO_EVENT *event )
{
    int ret = 0;

    if( base_gui->last_child->send_event( event ) )
    {
        emit_event( EVENT_REDRAW );
        ret = 1;
    }

    if( event->type == WZ_BUTTON_PRESSED || event->type == WZ_TEXT_CHANGED )
    {
        handle_gui_event( event );
    }

    return ret;
}

void Gui::draw_text_gui( ALLEGRO_USTR *text )
{
    WZ_WIDGET *gui = create_text_gui( text );

    gui->update( 0 );
    gui->draw();
    gui->destroy();
}

void Gui::emit_event( int event_type )
{
    static ALLEGRO_EVENT user_event = { 0 };

    user_event.type = event_type;
    al_emit_user_event( &user_event_src, &user_event, nullptr );
}
