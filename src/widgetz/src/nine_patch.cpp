/*
Taken from Allegro Nine Patch library. See LICENSE for copying information.
*/

#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

#include "../widgetz_nine_patch.hpp"

#include "../../macros.hpp"

static auto init_nine_patch_side( NINE_PATCH_SIDE *ps, ALLEGRO_BITMAP *bmp, int vertical ) -> bool
{
    const int len = vertical ? al_get_bitmap_height( bmp ) : al_get_bitmap_width( bmp );

    ps->m.resize( 8 );

    int s = -1;
    int t = 0;
    int n = 0;
    int z = -1;
    for( int i = 1; i < len; ++i )
    {
        int zz;
        ALLEGRO_COLOR c = vertical ? al_get_pixel( bmp, 0, i ) : al_get_pixel( bmp, i, 0 );
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
        al_unmap_rgba( c, &r, &g, &b, &a );

        if( i == len - 1 )
        {
            zz = -1;
        }
        else if( r == 0 && g == 0 && b == 0 && a == 255 )
        {
            zz = 0;
        }
        else if( a == 0 || r + g + b + a == 255 * 4 )
        {
            zz = 1;
        }
        else
        {
            return false;
        }

        if( z != zz )
        {
            if( s != -1 )
            {
                ps->m[n].offset = s;
                ps->m[n].length = i - s;

                if( z == 0 )
                {
                    ps->m[n].ratio = 1;
                    t += ps->m[n].length;
                }
                else
                {
                    ps->m[n].ratio = 0;
                }

                ++n;
            }

            s = i;
            z = zz;
        }

        if( n == ps->m.size() )
        {
            ps->m.resize( ps->m.size() * 2 );
        }
    }

    if( n != ps->m.size() )
    {
        ps->m.resize( n );
    }

    ps->count = n;
    ps->fix = len - 2 - t;

    for( int i = 0; i < n; ++i )
    {
        if( ps->m[i].ratio )
        {
            ps->m[i].ratio = ps->m[i].length / (float)t;
        }
    }

    return true;
}

auto wz_create_nine_patch_bitmap( ALLEGRO_BITMAP *bmp, bool owns_bitmap ) -> WZ_NINE_PATCH_BITMAP *
{
    int i = 0;
    ALLEGRO_COLOR c;

    auto *p9 = new WZ_NINE_PATCH_BITMAP();
    p9->bmp = bmp;
    p9->destroy_bmp = owns_bitmap;
    p9->cached_dw = 0;
    p9->cached_dh = 0;
    p9->mutex = al_create_mutex();
    p9->width = al_get_bitmap_width( bmp ) - 2;
    p9->height = al_get_bitmap_height( bmp ) - 2;
    al_lock_bitmap( bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY );

    if( p9->width <= 0 || p9->height <= 0 )
    {
        goto bad_bitmap;
    }

    /* make sure all four corners are transparent */
#define _check_pixel( x, y ) \
    c = al_get_pixel( bmp, x, y ); \
    if( c.a != 0 && c.r + c.g + c.b + c.a != 4 ) \
        goto bad_bitmap;
    _check_pixel( 0, 0 );
    _check_pixel( al_get_bitmap_width( bmp ) - 1, 0 );
    _check_pixel( 0, al_get_bitmap_height( bmp ) - 1 );
    _check_pixel( al_get_bitmap_width( bmp ) - 1, al_get_bitmap_height( bmp ) - 1 );
#undef _check_pixel

    p9->padding.top = p9->padding.right = p9->padding.bottom = p9->padding.left = -1;
    i = 1;

    while( i < al_get_bitmap_width( bmp ) )
    {
        c = al_get_pixel( bmp, i, al_get_bitmap_height( bmp ) - 1 );

        if( c.r + c.g + c.b == 0 && c.a == 1 )
        {
            if( p9->padding.left == -1 )
            {
                p9->padding.left = i - 1;
            }
            else if( p9->padding.right != -1 )
            {
                goto bad_bitmap;
            }
        }
        else if( c.a == 0 || c.r + c.g + c.b + c.a == 4 )
        {
            if( p9->padding.left != -1 && p9->padding.right == -1 )
            {
                p9->padding.right = al_get_bitmap_width( bmp ) - i - 1;
            }
        }

        ++i;
    }

    i = 1;

    while( i < al_get_bitmap_height( bmp ) )
    {
        c = al_get_pixel( bmp, al_get_bitmap_width( bmp ) - 1, i );

        if( c.r + c.g + c.b == 0 && c.a == 1 )
        {
            if( p9->padding.top == -1 )
            {
                p9->padding.top = i - 1;
            }
            else if( p9->padding.bottom != -1 )
            {
                goto bad_bitmap;
            }
        }
        else if( c.a == 0 || c.r + c.g + c.b + c.a == 4 )
        {
            if( p9->padding.top != -1 && p9->padding.bottom == -1 )
            {
                p9->padding.bottom = al_get_bitmap_height( bmp ) - i - 1;
            }
        }

        ++i;
    }

    if( !init_nine_patch_side( &p9->h, bmp, 0 ) || !init_nine_patch_side( &p9->v, bmp, 1 ) )
    {
    bad_bitmap:
        al_destroy_mutex( p9->mutex );

        p9->h.m.clear();
        p9->v.m.clear();

        delete p9;
        p9 = nullptr;
    }

    al_unlock_bitmap( bmp );
    return p9;
}

void calc_nine_patch_offsets( NINE_PATCH_SIDE *ps, int len )
{
    int j = 0;
    int dest_offset = 0;
    int remaining_stretch = len - ps->fix;

    for( int i = 0; i < ps->count; ++i )
    {
        ps->m[i].dest_offset = dest_offset;

        if( ps->m[i].ratio == 0 )
        {
            ps->m[i].dest_length = ps->m[i].length;
        }
        else
        {
            ps->m[i].dest_length = ( len - ps->fix ) * ps->m[i].ratio;
            remaining_stretch -= ps->m[i].dest_length;
            j = i;
        }

        dest_offset += ps->m[i].dest_length;
    }

    if( remaining_stretch )
    {
        ps->m[j].dest_length += remaining_stretch;

        if( j + 1 < ps->count )
        {
            ps->m[j + 1].dest_offset += remaining_stretch;
        }
    }
}

void wz_draw_tinted_nine_patch_bitmap( WZ_NINE_PATCH_BITMAP *p9, ALLEGRO_COLOR tint, int dx, int dy, int dw, int dh )
{
    bool release_drawing = false;

    /* don't draw bitmaps that are smaller than the fixed area */
    if( dw < p9->h.fix || dh < p9->v.fix )
    {
        return;
    }

    /* if the bitmap is the same size as the origin, then draw it as-is */
    if( dw == p9->width && dh == p9->height )
    {
        al_draw_tinted_bitmap_region( p9->bmp, tint, 1, 1, dw, dh, dx, dy, 0 );
        return;
    }

    /* due to the caching mechanism, multiple threads cannot draw this image at the same time */
    al_lock_mutex( p9->mutex );

    /* only recalculate the offsets if they have changed since the last draw */
    if( p9->cached_dw != dw || p9->cached_dh != dh )
    {
        calc_nine_patch_offsets( &p9->h, dw );
        calc_nine_patch_offsets( &p9->v, dh );
        p9->cached_dw = dw;
        p9->cached_dh = dh;
    }

    if( !al_is_bitmap_drawing_held() )
    {
        release_drawing = true;
        al_hold_bitmap_drawing( true );
    }

    /* draw each region */
    for( int i = 0; i < p9->v.count; ++i )
    {
        for( int j = 0; j < p9->h.count; ++j )
        {
            al_draw_tinted_scaled_bitmap( p9->bmp,
                                          tint,
                                          p9->h.m[j].offset,
                                          p9->v.m[i].offset,
                                          p9->h.m[j].length,
                                          p9->v.m[i].length,
                                          dx + p9->h.m[j].dest_offset,
                                          dy + p9->v.m[i].dest_offset,
                                          p9->h.m[j].dest_length,
                                          p9->v.m[i].dest_length,
                                          0 );
        }
    }

    al_unlock_mutex( p9->mutex );

    if( release_drawing )
    {
        al_hold_bitmap_drawing( false );
    }
}

void wz_draw_nine_patch_bitmap( WZ_NINE_PATCH_BITMAP *p9, int dx, int dy, int dw, int dh )
{
    wz_draw_tinted_nine_patch_bitmap( p9, al_map_rgb_f( 1, 1, 1 ), dx, dy, dw, dh );
}

auto wz_create_bitmap_from_nine_patch( WZ_NINE_PATCH_BITMAP *p9, int width, int h ) -> ALLEGRO_BITMAP *
{
    ALLEGRO_BITMAP *bmp = al_create_bitmap( width, h );
    ALLEGRO_STATE s;

    if( !bmp )
    {
        return nullptr;
    }

    al_store_state( &s, ALLEGRO_STATE_TARGET_BITMAP );
    al_set_target_bitmap( bmp );
    al_clear_to_color( al_map_rgba( 0, 0, 0, 0 ) );
    wz_draw_nine_patch_bitmap( p9, 0, 0, width, h );
    al_restore_state( &s );
    return bmp;
}

auto wz_load_nine_patch_bitmap( const char *filename ) -> WZ_NINE_PATCH_BITMAP *
{
    ALLEGRO_BITMAP *bmp = al_load_bitmap( filename );
    return bmp ? wz_create_nine_patch_bitmap( bmp, true ) : nullptr;
}

auto wz_get_nine_patch_bitmap_width( const WZ_NINE_PATCH_BITMAP *p9 ) -> int
{
    return p9->width;
}

auto wz_get_nine_patch_bitmap_height( const WZ_NINE_PATCH_BITMAP *p9 ) -> int
{
    return p9->height;
}

auto wz_get_nine_patch_bitmap_min_width( const WZ_NINE_PATCH_BITMAP *p9 ) -> int
{
    return p9->h.fix;
}

auto wz_get_nine_patch_bitmap_min_height( const WZ_NINE_PATCH_BITMAP *p9 ) -> int
{
    return p9->v.fix;
}

auto wz_get_nine_patch_bitmap_source( const WZ_NINE_PATCH_BITMAP *p9 ) -> ALLEGRO_BITMAP *
{
    return p9->bmp;
}

auto wz_get_nine_patch_padding( const WZ_NINE_PATCH_BITMAP *p9 ) -> WZ_NINE_PATCH_PADDING
{
    return p9->padding;
}

void wz_destroy_nine_patch_bitmap( WZ_NINE_PATCH_BITMAP *p9 )
{
    if( p9 == nullptr )
    {
        return;
    }

    if( p9->destroy_bmp )
    {
        al_destroy_bitmap( p9->bmp );
    }

    al_destroy_mutex( p9->mutex );
    p9->h.m.clear();
    p9->v.m.clear();
    delete p9;
}
