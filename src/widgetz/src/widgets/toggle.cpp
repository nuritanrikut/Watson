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
Title: Toggle Button

Section: Internal

Function: wz_toggle_button_proc

See also:
<wz_widget_proc>
*/
auto WZ_TOGGLE::proc( const ALLEGRO_EVENT *event ) -> int
{
    int ret = 1;
    float x, y;

    switch( event->type )
    {
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_BEGIN:
            x = event->touch.x;
            y = event->touch.y;
#endif
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        {
            if( event->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN )
            {
                x = event->mouse.x;
                y = event->mouse.y;
            }
            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else if( this->widget_rect_test( x, y ) )
            {
                this->ask_parent_for_focus();
                this->trigger();
            }
            else
                ret = 0;

            break;
        }
        case ALLEGRO_EVENT_KEY_DOWN:
        {
            switch( event->keyboard.keycode )
            {
                case ALLEGRO_KEY_ENTER:
                {
                    if( this->flags & WZ_STATE_DISABLED )
                    {
                        ret = 0;
                    }
                    else if( this->flags & WZ_STATE_HAS_FOCUS )
                    {
                        this->trigger();
                    }
                    else
                        ret = 0;

                    break;
                }
                default:
                    ret = 0;
            }

            break;
        }
        case WZ_LOSE_FOCUS:
        {
            return WZ_WIDGET::proc( event );
            break;
        }
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_END:
#endif
        case ALLEGRO_EVENT_KEY_UP:
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        {
            return WZ_WIDGET::proc( event );
            break;
        }
        case WZ_TOGGLE_GROUP:
        {
            if( event->user.data3 == this->group )
            {
                this->down = 0;
            }

            break;
        }
        case WZ_TRIGGER:
        {
            ALLEGRO_EVENT ev;

            if( this->group == -1 )
            {
                /*
				A simple toggle button
				*/
                this->down = !this->down;
            }
            else if( this->parent )
            {
                /*
				Disable the group, and then enable self
				*/
                wz_craft_event( &ev, WZ_TOGGLE_GROUP, this, this->group );
                this->parent->broadcast_event( &ev );
                this->down = 1;
            }

            wz_craft_event( &ev, WZ_BUTTON_PRESSED, this, 0 );
            al_emit_user_event( this->source, &ev, nullptr );
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

Function: wz_create_toggle_button

Parameters:

own - Set to 1 if you want the widget to own the text (it'll make a local copy)

See Also:
<wz_create_widget>

Inherits From:
<WZ_BUTTON>

See Also:
<WZ_TOGGLE>

<wz_create_widget>
*/
WZ_TOGGLE::WZ_TOGGLE( WZ_WIDGET *parent,
                      float x,
                      float y,
                      float width,
                      float height,
                      ALLEGRO_USTR *text,
                      int own,
                      int group,
                      int id )
    : WZ_BUTTON( parent, x, y, width, height, text, own, id )
{
    this->group = group;
}
