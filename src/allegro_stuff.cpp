#include "allegro_stuff.hpp"

#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_memfile.h>

#include <spdlog/spdlog.h>

#include "sound.hpp"

ALLEGRO_FONT *default_font = NULL;
MemFile text_font_mem = { 0 };
MemFile tile_font_mem = { 0 };
const char *TILE_FONT_FILE = "fonts/tiles.ttf";
const char *TEXT_FONT_FILE = "fonts/text_font.ttf";

struct Buffer_USTR
{
    ALLEGRO_USTR *ustr;
    Buffer_USTR *next;
};

Buffer_USTR *buffer_ustr = NULL;

int init_fonts( void )
{
    text_font_mem = create_memfile( TEXT_FONT_FILE );
    if( !text_font_mem.mem )
    {
        SPDLOG_ERROR( "Error creating memfile for text font" );
        return -1;
    }

    tile_font_mem = create_memfile( TILE_FONT_FILE );
    if( !tile_font_mem.mem )
    {
        SPDLOG_ERROR( "Error creating memfile for tile font" );
        return -1;
    }

    return 0;
}

ALLEGRO_FONT *load_font_mem( MemFile font_mem, const char *filename, int size )
{
    // filename is only to detect extension
    ALLEGRO_FILE *fp = NULL;
    ALLEGRO_FONT *font;

    fp = al_open_memfile( font_mem.mem, font_mem.size, "r" );
    if( !fp )
        return NULL;

    font = al_load_ttf_font_f( fp, filename, size, 0 );
    return font;
}

int init_allegro( void )
{
    ALLEGRO_PATH *path;
    int no_input = 1;

    if( !al_init() )
    {
        SPDLOG_ERROR( "failed to initalize allegro!\n" );
        return -1;
    }

#ifdef ALLEGRO_ANDROID
    al_android_set_apk_file_interface();
#endif

    SPDLOG_DEBUG( "initialized allegro" );
    path = al_get_standard_path( ALLEGRO_RESOURCES_PATH );
    al_change_directory( al_path_cstr( path, '/' ) ); // change the working directory
    al_destroy_path( path );

    if( !al_install_keyboard() )
    {
        SPDLOG_DEBUG( "Failed to initialize keyboard!\n" );
    }

    if( !al_install_mouse() )
    {
        SPDLOG_DEBUG( "Failed to initialize mouse.\n" );
    }
    else
        no_input = 0;

    init_sound(); // I don't care if there was an error here.

    if( !al_install_touch_input() )
    {
        SPDLOG_DEBUG( "Failed to initialize touch input.\n" );
    }
    else
        no_input = 0;

    if( no_input )
    {
        SPDLOG_ERROR( "No input found. Exitting." );
        return -1;
    }

    SPDLOG_DEBUG( "initializing sound" );
    init_sound(); // I don't care if there was an error here.
    SPDLOG_DEBUG( "initialized sound" );

    al_init_image_addon();
    SPDLOG_DEBUG( "initialized image addon" );
    al_init_font_addon();
    SPDLOG_DEBUG( "initialized font addon" );
    al_init_ttf_addon();
    SPDLOG_DEBUG( "initialized ttf addon" );

    if( !al_init_primitives_addon() )
    {
        SPDLOG_ERROR( "Failed to initialize primitives addon" );
        return -1;
    }

    SPDLOG_DEBUG( "initialized primitives addon" );

    if( init_fonts() )
        return -1;
    return 0;
};

MemFile create_memfile( const char *filename )
{
    MemFile ret = { 0 };

    ALLEGRO_FILE *fp = al_fopen( filename, "rb" );

    if( !fp )
    {
        SPDLOG_ERROR( "Error opening %s", filename );
        return ret;
    }

    ret.size = al_fsize( fp );
    ret.mem = malloc( ret.size );

    if( !ret.mem )
    {
        SPDLOG_ERROR( "Error allocating %zd bytes for memfile %s", ret.size, filename );
        al_fclose( fp );
        return ret;
    }

    if( al_fread( fp, ret.mem, ret.size ) != ret.size )
    {
        ret.mem = NULL;
        SPDLOG_ERROR( "Error reading %s", filename );
    }

    al_fclose( fp );
    return ret;
}

// adapter = 0 for first desktop
void get_desktop_resolution( int adapter, int *width, int *height )
{
    ALLEGRO_MONITOR_INFO info;
    al_get_monitor_info( adapter, &info );

    *width = info.x2 - info.x1;
    *height = info.y2 - info.y1;
};

// get best fullscreen resolution
void get_highest_resolution( int *width, int *height )
{
    ALLEGRO_DISPLAY_MODE disp_data;

    *width = 0;
    *height = 0;

    for( int i = 0; i < al_get_num_display_modes(); i++ )
    {
        al_get_display_mode( i, &disp_data );
        if( *width < disp_data.width )
            *width = disp_data.width;
    }

    if( ( *width == disp_data.width ) && ( *height < disp_data.height ) )
    {
        *height = disp_data.height;
    }
}

// quick helpful thingy
void wait_for_keypress()
{
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();

    al_register_event_source( queue, al_get_keyboard_event_source() );
    al_wait_for_event( queue, NULL );
    al_destroy_event_queue( queue );
}

//wait for keypress or mouse click
void wait_for_input( ALLEGRO_EVENT_QUEUE *queue )
{
    bool own_queue = queue == nullptr;

    if( own_queue )
    {
        queue = al_create_event_queue();

        if( al_is_keyboard_installed() )
            al_register_event_source( queue, al_get_keyboard_event_source() );
        if( al_is_mouse_installed() )
            al_register_event_source( queue, al_get_mouse_event_source() );
        if( al_is_touch_input_installed() )
            al_register_event_source( queue, al_get_touch_input_event_source() );
    }

    bool done = false;
    while( !done )
    {
        ALLEGRO_EVENT ev;
        while( !al_peek_next_event( queue, &ev ) )
            al_rest( 0.001 );

        switch( ev.type )
        {
            case ALLEGRO_EVENT_DISPLAY_RESIZE:
            case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
                if( !own_queue )
                    done = true;
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            case ALLEGRO_EVENT_KEY_CHAR:
            case ALLEGRO_EVENT_TOUCH_BEGIN:
                done = true;
            default:
                al_drop_next_event( queue );
        }
    }

    if( own_queue )
        al_destroy_event_queue( queue );
}

ALLEGRO_USTR *new_ustr( const char *str )
{
    Buffer_USTR *buf = new Buffer_USTR();
    buf->ustr = al_ustr_new( str );
    buf->next = buffer_ustr;
    buffer_ustr = buf;
    return buf->ustr;
}

void free_ustr( void )
{
    while( buffer_ustr )
    {
        al_ustr_free( buffer_ustr->ustr );
        buffer_ustr = buffer_ustr->next;
    }
}

ALLEGRO_BITMAP *screenshot()
{
    ALLEGRO_BITMAP *ret = al_clone_bitmap( al_get_target_bitmap() );
    return ret;
}

ALLEGRO_BITMAP *screenshot_part( int x, int y, int width, int height )
{
    int store = al_get_new_bitmap_format();

    ALLEGRO_BITMAP *currbuf = al_get_target_bitmap();

    al_set_new_bitmap_format( ALLEGRO_PIXEL_FORMAT_RGB_888 );

    ALLEGRO_BITMAP *ret = al_create_bitmap( width, height );

    al_set_target_bitmap( ret );
    al_draw_bitmap_region( currbuf, x, y, width, height, 0, 0, 0 );
    al_set_target_bitmap( currbuf );

    al_set_new_bitmap_format( store );

    return ret;
}

ALLEGRO_BITMAP *scaled_clone_bitmap( ALLEGRO_BITMAP *source, int width, int height )
{
    ALLEGRO_BITMAP *currbuf = al_get_target_bitmap();
    ALLEGRO_BITMAP *ret = al_create_bitmap( width, height );

    al_set_target_bitmap( ret );
    al_clear_to_color( NULL_COLOR );
    al_draw_scaled_bitmap(
        source, 0, 0, al_get_bitmap_width( source ), al_get_bitmap_height( source ), 0, 0, width, height, 0 );
    al_set_target_bitmap( currbuf );
    
    return ret;
}

// unused
void convert_memory_bitmap( ALLEGRO_BITMAP *bmp )
{
    int bflags = al_get_new_bitmap_flags();
    al_set_new_bitmap_flags( ALLEGRO_MEMORY_BITMAP );
    al_convert_bitmap( bmp );
    al_set_new_bitmap_flags( bflags );
}

void convert_video_bitmap( ALLEGRO_BITMAP *bmp )
{
    int bflags = al_get_new_bitmap_flags();
    al_set_new_bitmap_flags( ALLEGRO_CONVERT_BITMAP );
    al_convert_bitmap( bmp );
    al_set_new_bitmap_flags( bflags );
}
