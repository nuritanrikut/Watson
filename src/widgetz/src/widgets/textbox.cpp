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
appreciated this is not required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <spdlog/spdlog.h>

#include "../../widgetz_internal.hpp"

#include "../../../macros.hpp"

/*
Title: Text Box

Section: Internal

Function: wz_textbox_proc

See also:
<wz_widget_proc>
*/
auto WZ_TEXTBOX::proc( const ALLEGRO_EVENT *event ) -> int
{
    int ret = 1;

    switch( event->type )
    {
        case WZ_DRAW:
        {
            if( this->flags & WZ_STATE_HIDDEN )
            {
                ret = 0;
            }
            else
            {
                int flags = ( this->flags & WZ_STATE_DISABLED ) ? WZ_STYLE_DISABLED : 0;
                this->theme->draw_textbox( this->local_x,
                                           this->local_y,
                                           this->width,
                                           this->height,
                                           this->h_align,
                                           this->v_align,
                                           this->text,
                                           flags );
            }

            break;
        }
        case WZ_DESTROY:
        {
            if( this->own )
                al_ustr_free( this->text );

            ret = 0;
            break;
        }
        case WZ_SET_TEXT:
        {
            if( this->own )
            {
                al_ustr_free( this->text );
                this->text = al_ustr_dup( (ALLEGRO_USTR *)event->user.data3 );
            }
            else
            {
                this->text = (ALLEGRO_USTR *)event->user.data3;
            }

            break;
        }
        default:
            ret = 0;
    }

    if( ret == 0 )
        ret = WZ_WIDGET::proc( event );

    return ret;
}

/*
Section: Public

Function: wz_create_textbox

Creates a text this.

Parameters:

halign - Horizontal text alignment, can be one of: <WZ_ALIGN_LEFT>, <WZ_ALIGN_CENTRE>, <WZ_ALIGN_RIGHT>
valign - Vertical text alignment, can be one of: <WZ_ALIGN_TOP>, <WZ_ALIGN_CENTRE>, <WZ_ALIGN_BOTTOM>
own - Set to 1 if you want the widget to own the text

Inherits From:
<WZ_WIDGET>

See Also:
<WZ_TEXTBOX>

<wz_create_widget>
*/
WZ_TEXTBOX::WZ_TEXTBOX( WZ_WIDGET *parent,
                        float x,
                        float y,
                        float width,
                        float height,
                        int halign,
                        int valign,
                        ALLEGRO_USTR *text,
                        int own,
                        int id )
    : WZ_WIDGET( parent, x, y, width, height, id )
{
    this->flags |= WZ_STATE_NOTWANT_FOCUS;
    this->own = own;
    this->h_align = halign;
    this->v_align = valign;
    this->text = text;
}
