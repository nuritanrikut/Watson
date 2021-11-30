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
Title: Box

Section: Internal

Function: wz_box_proc

See also:
<wz_widget_proc>
*/
auto WZ_BOX::proc( const ALLEGRO_EVENT *event ) -> int
{
    int ret = 1;

    switch( event->type )
    {
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        {
            if( event->mouse.button == 1 && this->widget_rect_test( event->mouse.x, event->mouse.y ) )
            {
                this->ask_parent_for_focus();
            }

            ret = 0;
            break;
        }
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_BEGIN:
        {
            if( this->widget_rect_test( event->touch.x, event->touch.y ) )
            {
                this->ask_parent_for_focus();
            }

            ret = 0;
            break;
        }
#endif
        case WZ_DRAW:
        {
            if( this->flags & WZ_STATE_HIDDEN )
            {
                ret = 0;
            }
            else
            {
                int flags = ( this->flags & WZ_STATE_HAS_FOCUS ) ? WZ_STYLE_FOCUSED : 0;
                flags |= ( this->flags & WZ_STATE_DISABLED ) ? WZ_STYLE_DISABLED : 0;
                this->theme->draw_box( this->local_x, this->local_y, this->width, this->height, flags );
            }
        }

        default:
            ret = 0;
    }

    if( ret == 0 )
    {
        ret = WZ_WIDGET::proc( event );
    }

    return ret;
}

/*
Section: Public

Function: wz_create_box

Creates a box.

See Also:
<WZ_WIDGET>

<wz_create_widget>
*/

WZ_BOX::WZ_BOX( WZ_WIDGET *parent, float x, float y, float width, float height, int id )
    : WZ_WIDGET( parent, x, y, width, height, id )
{
}