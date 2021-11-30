/*
Taken from Allegro Nine Patch library. See LICENSE for copying information.
*/

#pragma once

#include <vector>
#include <allegro5/allegro.h>

struct NINE_PATCH_MARK
{
    int offset;
    int length;
    int dest_offset;
    int dest_length;
    float ratio;
};

struct NINE_PATCH_SIDE
{
    std::vector<NINE_PATCH_MARK> m;
    int count;
    int fix;
};

struct WZ_NINE_PATCH_PADDING
{
    int top, right, bottom, left;
};

struct WZ_NINE_PATCH_BITMAP
{
    ALLEGRO_BITMAP *bmp;
    NINE_PATCH_SIDE h, v;
    WZ_NINE_PATCH_PADDING padding;
    bool destroy_bmp;
    int width, height;
    int cached_dw, cached_dh;
    ALLEGRO_MUTEX *mutex;
};

auto wz_create_nine_patch_bitmap( ALLEGRO_BITMAP *bmp, bool owns_bitmap ) -> WZ_NINE_PATCH_BITMAP *;
auto wz_load_nine_patch_bitmap( const char *filename ) -> WZ_NINE_PATCH_BITMAP *;
void wz_draw_nine_patch_bitmap( WZ_NINE_PATCH_BITMAP *p9, int dx, int dy, int dw, int dh );
void wz_draw_tinted_nine_patch_bitmap( WZ_NINE_PATCH_BITMAP *p9, ALLEGRO_COLOR tint, int dx, int dy, int dw, int dh );
auto wz_create_bitmap_from_nine_patch( WZ_NINE_PATCH_BITMAP *p9, int width, int height ) -> ALLEGRO_BITMAP *;

auto wz_get_nine_patch_bitmap_width( const WZ_NINE_PATCH_BITMAP *p9 ) -> int;
auto wz_get_nine_patch_bitmap_height( const WZ_NINE_PATCH_BITMAP *p9 ) -> int;

auto wz_get_nine_patch_bitmap_min_width( const WZ_NINE_PATCH_BITMAP *p9 ) -> int;
auto wz_get_nine_patch_bitmap_min_height( const WZ_NINE_PATCH_BITMAP *p9 ) -> int;

auto wz_get_nine_patch_bitmap_source( const WZ_NINE_PATCH_BITMAP *p9 ) -> ALLEGRO_BITMAP *;
auto wz_get_nine_patch_padding( const WZ_NINE_PATCH_BITMAP *p9 ) -> WZ_NINE_PATCH_PADDING;

void wz_destroy_nine_patch_bitmap( WZ_NINE_PATCH_BITMAP *p9 );
