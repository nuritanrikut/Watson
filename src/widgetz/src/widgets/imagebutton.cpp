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

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <spdlog/spdlog.h>
#include "../../widgetz_internal.hpp"

#include "../../../macros.hpp"

/*
Title: Image Button

Section: Internal

Function: wz_button_proc

See also:
<wz_widget_proc>
*/
int WZ_IMAGE_BUTTON::proc( const ALLEGRO_EVENT *event )
{
    int ret = 1;

    switch( event->type )
    {
        case WZ_DRAW:
        {
            ALLEGRO_BITMAP *image = 0;

            if( this->flags & WZ_STATE_HIDDEN )
                break;
            else if( this->flags & WZ_STATE_DISABLED )
                image = this->disabled;
            else
            {
                if( this->down )
                    image = this->down;
                else if( this->flags & WZ_STATE_HAS_FOCUS )
                    image = this->focused;
                else
                    image = this->normal;
            }

            this->theme->draw_image( this->local_x, this->local_y, this->width, this->height, image );
            break;
        }
        default:
            ret = 0;
    }

    if( ret == 0 )
        ret = WZ_BUTTON::proc( event );

    return ret;
}

/*
Section: Public

Function: wz_create_image_button

Creates an image button.

Parameters:

normal - Image drawn when the button is in its default state
down - Image drawn when the button is pressed down
focused - Image drawn when the button is focused
disabled - Image drawn when the button is disabled

Inherits From:
<WZ_BUTTON>

See Also:
<WZ_IMAGE_BUTTON>

<wz_create_button>
*/
WZ_IMAGE_BUTTON::WZ_IMAGE_BUTTON( WZ_WIDGET *parent,
                                  float x,
                                  float y,
                                  float width,
                                  float height,
                                  ALLEGRO_BITMAP *normal,
                                  ALLEGRO_BITMAP *down,
                                  ALLEGRO_BITMAP *focused,
                                  ALLEGRO_BITMAP *disabled,
                                  int id )
    : WZ_BUTTON( parent, x, y, width, height, nullptr, 0, id )
{
    this->normal = normal;
    this->down = down;
    this->focused = focused;
    this->disabled = disabled;
}
