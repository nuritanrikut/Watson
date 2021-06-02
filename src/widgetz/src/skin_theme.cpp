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

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

#include "../widgetz_internal.hpp"
#include "../widgetz_nine_patch.hpp"

#include "../../macros.hpp"

/*
Title: Skinnable Theme Stuff
*/

/*
Function: wz_init_skin_theme

Initializes a this this. Do not pass wz_skin_theme to this function, but make a copy of it first. You should initialize the bitmap variables inside the <WZ_SKIN_THEME> struct before calling
this function. Don't forget to call <wz_destroy_skin_theme> on the this when you are done with it.
*/
void WZ_SKIN_THEME::init()
{
    skin_pad = 0;

    if( this->button_up_bitmap != 0 )
        this->button_up_patch = wz_create_nine_patch_bitmap( this->button_up_bitmap, false );
    else
        this->button_up_patch = 0;

    if( this->button_down_bitmap != 0 )
        this->button_down_patch = wz_create_nine_patch_bitmap( this->button_down_bitmap, false );
    else
        this->button_down_patch = 0;

    if( this->box_bitmap != 0 )
        this->box_patch = wz_create_nine_patch_bitmap( this->box_bitmap, false );
    else
        this->box_patch = 0;

    if( this->editbox_bitmap != 0 )
        this->editbox_patch = wz_create_nine_patch_bitmap( this->editbox_bitmap, false );
    else
        this->editbox_patch = 0;

    if( this->scroll_track_bitmap != 0 )
        this->scroll_track_patch = wz_create_nine_patch_bitmap( this->scroll_track_bitmap, false );
    else
        this->scroll_track_patch = 0;

    if( this->slider_bitmap != 0 )
        this->slider_patch = wz_create_nine_patch_bitmap( this->slider_bitmap, false );
    else
        this->slider_patch = 0;
}

/*
Function: wz_destroy_skin_theme

Destroys a this this that has been initialized by <wz_init_skin_theme>. Note that it does not free the passed pointer.
*/
WZ_SKIN_THEME::~WZ_SKIN_THEME()
{
    wz_destroy_nine_patch_bitmap( this->button_up_patch );
    wz_destroy_nine_patch_bitmap( this->button_down_patch );
    wz_destroy_nine_patch_bitmap( this->box_patch );
    wz_destroy_nine_patch_bitmap( this->editbox_patch );
    wz_destroy_nine_patch_bitmap( this->scroll_track_patch );
    wz_destroy_nine_patch_bitmap( this->slider_patch );
}

/* Returns the padding corrected in case the passed rectangle was too small */
WZ_NINE_PATCH_PADDING WZ_SKIN_THEME::draw_tinted_patch( WZ_NINE_PATCH_BITMAP *p9,
                                                        ALLEGRO_COLOR tint,
                                                        float x,
                                                        float y,
                                                        float width,
                                                        float height )
{
    WZ_NINE_PATCH_PADDING pad = wz_get_nine_patch_padding( p9 );
    float min_w = wz_get_nine_patch_bitmap_min_width( p9 );
    float min_h = wz_get_nine_patch_bitmap_min_height( p9 );
    float nx = x;
    float ny = y;
    float nw = width;
    float nh = height;

    if( width < min_w )
    {
        nw = min_w;
        nx = x + width / 2 - nw / 2;
    }

    if( height < min_h )
    {
        nh = min_h;
        ny = y + height / 2 - nh / 2;
    }

    wz_draw_tinted_nine_patch_bitmap( p9, tint, nx, ny, nw, nh );
    pad.left -= x - nx;
    pad.top -= y - ny;
    pad.right -= x - nx; /* Might be wrong */
    pad.top -= y - ny;
    return pad;
}

void WZ_SKIN_THEME::draw_box( float x, float y, float width, float height, int style )
{
    ALLEGRO_COLOR col;

    if( this->box_patch )
    {
        if( style & WZ_STYLE_FOCUSED )
            col = wz_scale_color( this->color1, 1.5 );
        else if( style & WZ_STYLE_DISABLED )
            col = wz_scale_color( this->color1, 0.5 );
        else
            col = this->color1;

        draw_tinted_patch( this->box_patch, col, x, y, width, height );
    }
}

void WZ_SKIN_THEME::draw_button( float x, float y, float width, float height, ALLEGRO_USTR *text, int style )
{
    ALLEGRO_COLOR button_col;
    ALLEGRO_COLOR text_col;
    WZ_NINE_PATCH_PADDING pad;
    button_col = this->color1;
    text_col = this->color2;
    pad.left = 0;
    pad.right = 0;
    pad.top = 0;
    pad.bottom = 0;

#if defined( ALLEGRO_ANDROID ) || defined( ALLEGRO_IPHONE )
    if( ( style & WZ_STYLE_FOCUSED ) && !( style & WZ_STYLE_DOWN ) )
#else
    if( ( style & WZ_STYLE_FOCUSED ) )
#endif
    {
        button_col = wz_scale_color( this->color1, 1.25 );
    }

    if( style & WZ_STYLE_DISABLED )
    {
        button_col = wz_scale_color( this->color1, 0.5 );
        text_col = wz_scale_color( this->color2, 0.5 );
    }

    if( this->button_up_patch && this->button_down_patch )
    {
        if( style & WZ_STYLE_DOWN )
        {
            pad = draw_tinted_patch( this->button_down_patch, button_col, x, y, width, height );
        }
        else
        {
            pad = draw_tinted_patch( this->button_up_patch, button_col, x, y, width, height );
        }
    }

    draw_multi_text( x + pad.left,
                     y + pad.top,
                     width - ( pad.left + pad.right ),
                     height - ( pad.top + pad.bottom ),
                     WZ_ALIGN_CENTRE,
                     WZ_ALIGN_CENTRE,
                     text_col,
                     this->font,
                     text );
}

void WZ_SKIN_THEME::draw_textbox( float x,
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

void WZ_SKIN_THEME::draw_scroll( float x,
                                 float y,
                                 float width,
                                 float height,
                                 float fraction,
                                 float slider_size,
                                 int style )
{
    ALLEGRO_COLOR col;
    int vertical = height > width;
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
    }
    else
    {
        float max_size = 0.9f * width;
        slider_h = height;
        slider_w = slider_size > max_size ? max_size : slider_size;
        xpos = x + fraction * ( width - slider_w );
        ypos = y;
    }

    if( this->scroll_track_patch )
        draw_tinted_patch( this->scroll_track_patch, this->color1, x, y, width, height );

    if( this->slider_patch )
        draw_tinted_patch( this->slider_patch, col, xpos, ypos, slider_w, slider_h );
}

void WZ_SKIN_THEME::draw_editbox( float x,
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
    ALLEGRO_USTR *token;
    ALLEGRO_COLOR border_col;
    ALLEGRO_COLOR text_col;
    WZ_NINE_PATCH_PADDING pad;
    pad.left = 0;
    pad.right = 0;
    pad.top = 0;
    pad.bottom = 0;
    len = len + 1 > len2 ? len2 : len + 1;
    offset = al_ustr_offset( text, len );
    token = (ALLEGRO_USTR *)al_ref_ustr( &info, text, 0, offset );
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

    if( this->editbox_patch )
    {
        pad = draw_tinted_patch( this->editbox_patch, border_col, x, y, width, height );
    }

    al_get_clipping_rectangle( &cx, &cy, &cw, &ch );
    al_set_clipping_rectangle(
        x + pad.left, y + pad.right, width - ( pad.left + pad.right ), height - ( pad.top + pad.bottom ) );
    draw_single_text( x + pad.left,
                      y + pad.right,
                      width - ( pad.left + pad.right ),
                      height - ( pad.top + pad.bottom ),
                      WZ_ALIGN_LEFT,
                      WZ_ALIGN_CENTRE,
                      text_col,
                      this->font,
                      token );
    al_set_clipping_rectangle( cx, cy, cw, ch );

    if( style & WZ_STYLE_FOCUSED )
    {
        float mh = ( pad.top + height - pad.bottom ) / 2;

        if( ( (int)( al_get_time() / 0.5f ) ) % 2 == 0 )
        {
            float len;
            float halfheight;
            offset = al_ustr_offset( text, cursor_pos );
            token = (ALLEGRO_USTR *)al_ref_ustr( &info, text, 0, offset );
            len = al_get_ustr_width( this->font, token );
            halfheight = al_get_font_line_height( this->font ) / 2.0f;
            al_draw_line(
                x + pad.left + len, y + mh - halfheight, x + pad.left + len, y + mh + halfheight, text_col, 1 );
        }
    }
}

void WZ_SKIN_THEME::draw_image( float x, float y, float width, float height, ALLEGRO_BITMAP *image )
{
    float ix = x + ( width - al_get_bitmap_width( image ) ) / 2;
    float iy = y + ( height - al_get_bitmap_height( image ) ) / 2;
    al_draw_bitmap( image, ix, iy, 0 );
}

ALLEGRO_FONT *WZ_SKIN_THEME::get_font( int font_num )
{
    return this->font;
}

WZ_SKIN_THEME::WZ_SKIN_THEME()
    : WZ_DEF_THEME(),
      skin_pad( 0 ),
      button_up_patch( nullptr ),
      button_down_patch( nullptr ),
      box_patch( nullptr ),
      editbox_patch( nullptr ),
      scroll_track_patch( nullptr ),
      slider_patch( nullptr ),
      button_up_bitmap( nullptr ),
      button_down_bitmap( nullptr ),
      box_bitmap( nullptr ),
      editbox_bitmap( nullptr ),
      scroll_track_bitmap( nullptr ),
      slider_bitmap( nullptr )
{
}

WZ_SKIN_THEME::WZ_SKIN_THEME( WZ_DEF_THEME *other )
    : WZ_DEF_THEME( other->color1, other->color2, other->font ),
      skin_pad( 0 ),
      button_up_patch( nullptr ),
      button_down_patch( nullptr ),
      box_patch( nullptr ),
      editbox_patch( nullptr ),
      scroll_track_patch( nullptr ),
      slider_patch( nullptr ),
      button_up_bitmap( nullptr ),
      button_down_bitmap( nullptr ),
      box_bitmap( nullptr ),
      editbox_bitmap( nullptr ),
      scroll_track_bitmap( nullptr ),
      slider_bitmap( nullptr )
{
}

WZ_SKIN_THEME::WZ_SKIN_THEME( WZ_SKIN_THEME *other )
    : WZ_DEF_THEME( other->color1, other->color2, other->font ),
      skin_pad( other->skin_pad ),
      button_up_patch( other->button_up_patch ),
      button_down_patch( other->button_down_patch ),
      box_patch( other->box_patch ),
      editbox_patch( other->editbox_patch ),
      scroll_track_patch( other->scroll_track_patch ),
      slider_patch( other->slider_patch ),
      button_up_bitmap( other->button_up_bitmap ),
      button_down_bitmap( other->button_down_bitmap ),
      box_bitmap( other->box_bitmap ),
      editbox_bitmap( other->editbox_bitmap ),
      scroll_track_bitmap( other->scroll_track_bitmap ),
      slider_bitmap( other->slider_bitmap )
{
}

WZ_SKIN_THEME *get_skin_theme()
{
    static WZ_SKIN_THEME instance;
    return &instance;
}