#pragma once

#include <cstdio>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "macros.hpp"

struct MemFile
{
    void *mem;
    int64_t size;
};

// Prototypes

// Init mouse, kbd, addons, set path to resources path (or cwd), etc
auto init_allegro() -> int;

// adapter = 0 for first desktop
void get_desktop_resolution( int adapter, int *width, int *height );

// get best fullscreen resolution
void get_highest_resolution( int *width, int *height );

void wait_for_keypress();

// wait for any input
// if queue = NULL will use own queue, with installed input devices
// otherwise uses provided queue with registered input devices
void wait_for_input( ALLEGRO_EVENT_QUEUE *queue );
auto create_memfile( const char *filename ) -> MemFile;
auto init_fonts() -> int;
auto load_font_mem( MemFile font_mem, const char *filename, int size ) -> ALLEGRO_FONT *;

auto new_ustr( const char *str ) -> ALLEGRO_USTR *;
void free_ustr();

// clones the target bitmap (usually the display backbuffer)
auto screenshot() -> ALLEGRO_BITMAP *;
auto screenshot_part( int x, int y, int width, int height ) -> ALLEGRO_BITMAP *;

auto scaled_clone_bitmap( ALLEGRO_BITMAP *source, int width, int height ) -> ALLEGRO_BITMAP *;

// variables
extern ALLEGRO_FONT *default_font;
extern MemFile text_font_mem;
extern MemFile tile_font_mem;
extern const char *TILE_FONT_FILE;
extern const char *TEXT_FONT_FILE;
