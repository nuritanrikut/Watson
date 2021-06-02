/*
Copyright (c) 2011 Pavel Sountsov

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#include <cmath>

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "../widgetz_internal.hpp"

#include "../../macros.hpp"

/*
Title: Theme Stuff
*/

void WZ_DEF_THEME::draw_3d_rectangle( float x1,
                                      float y1,
                                      float x2,
                                      float y2,
                                      float border,
                                      ALLEGRO_COLOR col,
                                      bool invert )
{
    ALLEGRO_VERTEX vtx[6];
    ALLEGRO_COLOR hi, lo;
    int ii;

    if( invert )
    {
        lo = wz_scale_color( col, 1.5 );
        hi = wz_scale_color( col, 0.5 );
    }
    else
    {
        hi = wz_scale_color( col, 1.5 );
        lo = wz_scale_color( col, 0.5 );
    }

    for( ii = 0; ii < 6; ii++ )
    {
        vtx[ii].color = hi;
        vtx[ii].z = 0;
    }

    vtx[0].x = x1 + border;
    vtx[0].y = y1 + border;
    vtx[1].x = x1 + border;
    vtx[1].y = y2 - border;
    vtx[2].x = x1;
    vtx[2].y = y2;
    vtx[3].x = x1;
    vtx[3].y = y1;
    vtx[4].x = x2;
    vtx[4].y = y1;
    vtx[5].x = x2 - border;
    vtx[5].y = y1 + border;
    al_draw_prim( vtx, 0, 0, 0, 6, ALLEGRO_PRIM_TRIANGLE_FAN );
    vtx[0].x = x2 - border;
    vtx[0].y = y2 - border;
    vtx[1].x = x1 + border;
    vtx[1].y = y2 - border;
    vtx[2].x = x1;
    vtx[2].y = y2;
    vtx[3].x = x2;
    vtx[3].y = y2;
    vtx[4].x = x2;
    vtx[4].y = y1;
    vtx[5].x = x2 - border;
    vtx[5].y = y1 + border;

    for( ii = 0; ii < 6; ii++ )
    {
        vtx[ii].color = lo;
    }

    al_draw_prim( vtx, 0, 0, 0, 6, ALLEGRO_PRIM_TRIANGLE_FAN );
    al_draw_filled_rectangle( x1 + border, y1 + border, x2 - border, y2 - border, col );
}

//return the new start
int WZ_DEF_THEME::find_eol( ALLEGRO_USTR *text, ALLEGRO_FONT *font, float max_width, int start, int *end )
{
    int a, b;
    int first = 1;
    int last = 0;
    a = start;

    while( 1 )
    {
        ALLEGRO_USTR_INFO info;
        const ALLEGRO_USTR *token;
        float len;
        /*
		Find the end of current token
		*/
        b = al_ustr_find_set_cstr( text, a, "\t\n " );

        if( b == -1 ) //found nothing
        {
            b = al_ustr_size( text ); //this is the last whole word
            last = 1;
        }

        /*
		Check to see if the token fits
		*/
        token = al_ref_ustr( &info, text, start, b );
        len = al_get_ustr_width( font, token );

        if( len < max_width || first )
        {
            if( last )
            {
                *end = b + 1;
                return -1;
            }
        }
        else //we return the last num
        {
            *end = a - 1;
            return a;
        }

        /*
		Check what character we found
		*/
        {
            int character = al_ustr_get( text, b );

            if( character == '\n' )
            {
                *end = b;
                return b + 1;
            }
        }
        a = b + 1;
        first = 0;
    }
}

/*
Function: wz_draw_single_text

Draws a single line of text
*/
void WZ_DEF_THEME::draw_single_text( float x,
                                     float y,
                                     float width,
                                     float height,
                                     int halign,
                                     int valign,
                                     ALLEGRO_COLOR color,
                                     ALLEGRO_FONT *font,
                                     ALLEGRO_USTR *text )
{
    float xpos;
    float ypos;
    float font_height = al_get_font_line_height( font );

    if( valign == WZ_ALIGN_TOP )
    {
        ypos = y;
    }
    else if( valign == WZ_ALIGN_BOTTOM )
    {
        ypos = y + height - font_height;
    }
    else
    {
        ypos = y + height / 2 - font_height / 2;
    }

    if( halign == WZ_ALIGN_LEFT )
    {
        xpos = x;
        al_draw_ustr( font, color, floorf( xpos ), floorf( ypos ), ALLEGRO_ALIGN_LEFT, text );
    }
    else if( halign == WZ_ALIGN_RIGHT )
    {
        xpos = x + width;
        al_draw_ustr( font, color, floorf( xpos ), floorf( ypos ), ALLEGRO_ALIGN_RIGHT, text );
    }
    else
    {
        xpos = x + width / 2;
        al_draw_ustr( font, color, floorf( xpos ), floorf( ypos ), ALLEGRO_ALIGN_CENTRE, text );
    }
}

/*
Function: wz_draw_multi_text

Draws multiple lines of text, wrapping it as necessary
*/
void WZ_DEF_THEME::draw_multi_text( float x,
                                    float y,
                                    float width,
                                    float height,
                                    int halign,
                                    int valign,
                                    ALLEGRO_COLOR color,
                                    ALLEGRO_FONT *font,
                                    ALLEGRO_USTR *text )
{
    float cur_y = y;
    float text_height = al_get_font_line_height( font );
    float total_height = 0;

    if( valign == WZ_ALIGN_BOTTOM || valign == WZ_ALIGN_CENTRE )
    {
        int ret = 0;

        do
        {
            int start = ret;
            int end;
            ret = find_eol( text, font, width, start, &end );
            total_height += text_height;
        } while( ret > 0 );
    }

    if( valign == WZ_ALIGN_BOTTOM )
    {
        cur_y = y + height - total_height;
    }
    else if( valign == WZ_ALIGN_CENTRE )
    {
        cur_y = y + ( height - total_height ) / 2;
    }

    {
        int ret = 0;

        do
        {
            int start = ret;
            int end;
            ret = find_eol( text, font, width, start, &end );
            {
                ALLEGRO_USTR_INFO info;
                const ALLEGRO_USTR *token = al_ref_ustr( &info, text, start, end );
                draw_single_text( x, cur_y, width, height, halign, WZ_ALIGN_TOP, color, font, (ALLEGRO_USTR *)token );
            }
            cur_y += text_height;
        } while( ret > 0 );
    }
}

void WZ_DEF_THEME::draw_box( float x, float y, float width, float height, int style )
{
    al_draw_filled_rectangle( x, y, x + width, y + height, wz_scale_color( this->color1, 0.5 ) );

    if( style & WZ_STYLE_FOCUSED )
        al_draw_rectangle( x, y, x + width, y + height, wz_scale_color( this->color1, 1.5 ), 1 );
    else
        al_draw_rectangle( x, y, x + width, y + height, this->color1, 1 );
}

void WZ_DEF_THEME::draw_button( float x, float y, float width, float height, ALLEGRO_USTR *text, int style )
{
    ALLEGRO_COLOR button_col;
    ALLEGRO_COLOR text_col;
    bool invert = false;
    button_col = this->color1;
    text_col = this->color2;

    if( style & WZ_STYLE_FOCUSED )
    {
        button_col = wz_scale_color( this->color1, 1.25 );
    }

    if( style & WZ_STYLE_DISABLED )
    {
        button_col = wz_scale_color( this->color1, 0.5 );
        text_col = wz_scale_color( this->color2, 0.5 );
    }

    if( style & WZ_STYLE_DOWN )
    {
        invert = true;
    }

    draw_3d_rectangle( x, y, x + width, y + height, 2, button_col, invert );
    draw_multi_text( x, y, width, height, WZ_ALIGN_CENTRE, WZ_ALIGN_CENTRE, text_col, this->font, text );
}

void WZ_DEF_THEME::draw_textbox( float x,
                                 float y,
                                 float width,
                                 float height,
                                 int halign,
                                 int valign,
                                 ALLEGRO_USTR *text,
                                 int style )
{
    ALLEGRO_COLOR text_col;

    if( style & WZ_STYLE_DISABLED )
        text_col = wz_scale_color( this->color2, 0.5 );
    else
        text_col = this->color2;

    draw_multi_text( x, y, width, height, halign, valign, text_col, this->font, text );
}

void WZ_DEF_THEME::draw_scroll( float x,
                                float y,
                                float width,
                                float height,
                                float fraction,
                                float slider_size,
                                int style )
{
    int vertical = height > width;
    ALLEGRO_COLOR col;
    float xpos;
    float ypos;
    float slider_w;
    float slider_h;

    if( style & WZ_STYLE_FOCUSED )
    {
        col = wz_scale_color( this->color1, 1.5 );
    }
    else if( style & WZ_STYLE_DISABLED )
    {
        col = wz_scale_color( this->color1, 0.5 );
    }
    else
    {
        col = this->color1;
    }

    if( vertical )
    {
        float max_size = 0.9f * height;
        slider_h = slider_size > max_size ? max_size : slider_size;
        slider_w = width;
        xpos = x;
        ypos = y + fraction * ( height - slider_h );
        draw_3d_rectangle(
            xpos - 4 + width / 2, y, xpos + 4 + width / 2, y + height, 2, wz_scale_color( this->color1, 0.75 ), true );
    }
    else
    {
        float max_size = 0.9f * width;
        slider_h = height;
        slider_w = slider_size > max_size ? max_size : slider_size;
        xpos = x + fraction * ( width - slider_w );
        ypos = y;
        draw_3d_rectangle(
            x, ypos - 4 + height / 2, x + width, ypos + 4 + height / 2, 2, wz_scale_color( this->color1, 0.75 ), true );
    }

    draw_3d_rectangle( xpos, ypos, xpos + slider_w, ypos + slider_h, 1, col, false );
}

void WZ_DEF_THEME::draw_editbox( float x,
                                 float y,
                                 float width,
                                 float height,
                                 int cursor_pos,
                                 ALLEGRO_USTR *text,
                                 int style )
{
    int len = wz_get_text_pos( this->font, text, width - 4 );
    int cx, cy, cw, ch;
    int len2 = al_ustr_length( text );
    int offset;
    ALLEGRO_USTR_INFO info;
    const ALLEGRO_USTR *token;
    ALLEGRO_COLOR border_col;
    ALLEGRO_COLOR text_col;
    len = len + 1 > len2 ? len2 : len + 1;
    offset = al_ustr_offset( text, len );
    token = al_ref_ustr( &info, text, 0, offset );
    border_col = this->color1;
    text_col = this->color2;

    if( style & WZ_STYLE_FOCUSED )
    {
        border_col = wz_scale_color( this->color1, 1.5 );
    }

    if( style & WZ_STYLE_DISABLED )
    {
        border_col = wz_scale_color( this->color1, 0.5 );
        text_col = wz_scale_color( this->color2, 0.5 );
    }

    draw_3d_rectangle( x, y, x + width, y + height, 1, border_col, true );
    al_get_clipping_rectangle( &cx, &cy, &cw, &ch );
    al_set_clipping_rectangle( x + 2, y + 2, width - 4, height - 4 );
    draw_single_text( x + 2,
                      y + 2,
                      width - 4,
                      height - 4,
                      WZ_ALIGN_LEFT,
                      WZ_ALIGN_CENTRE,
                      text_col,
                      this->font,
                      (ALLEGRO_USTR *)token );
    al_set_clipping_rectangle( cx, cy, cw, ch );

    if( style & WZ_STYLE_FOCUSED )
    {
        if( ( (int)( al_get_time() / 0.5f ) ) % 2 == 0 )
        {
            float len;
            float halfheight;
            offset = al_ustr_offset( text, cursor_pos );
            token = al_ref_ustr( &info, text, 0, offset );
            len = al_get_ustr_width( this->font, token );
            halfheight = al_get_font_line_height( this->font ) / 2.0f;
            al_draw_line( x + 2 + len,
                          y + 2 + height / 2 - halfheight,
                          x + 2 + len,
                          y + 2 + height / 2 + halfheight,
                          text_col,
                          1 );
        }
    }
}

void WZ_DEF_THEME::draw_image( float x, float y, float width, float height, ALLEGRO_BITMAP *image )
{
    float ix = x + ( width - al_get_bitmap_width( image ) ) / 2;
    float iy = y + ( height - al_get_bitmap_height( image ) ) / 2;
    al_draw_bitmap( image, ix, iy, 0 );
}

ALLEGRO_FONT *WZ_DEF_THEME::get_font( int font_num )
{
    return this->font;
}

WZ_DEF_THEME::WZ_DEF_THEME() : color1( NULL_COLOR ), color2( NULL_COLOR ), font( nullptr ) { }
WZ_DEF_THEME::WZ_DEF_THEME( ALLEGRO_COLOR color1_, ALLEGRO_COLOR color2_, ALLEGRO_FONT *font_ )
    : color1( color1_ ),
      color2( color2_ ),
      font( font_ )
{
}

WZ_DEF_THEME::~WZ_DEF_THEME() { }

WZ_DEF_THEME *get_def_theme()
{
    static WZ_DEF_THEME instance;
    return &instance;
}