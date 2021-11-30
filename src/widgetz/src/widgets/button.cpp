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
Title: Button

Section: Internal

Function: wz_button_proc

See also:
<wz_widget_proc>
*/
auto WZ_BUTTON::proc( const ALLEGRO_EVENT *event ) -> int
{
    int ret = 1;
    float x;
    float y;

    switch( event->type )
    {
        case WZ_LOSE_FOCUS:
        {
            ret = 0;
            break;
        }
        case WZ_DRAW:
        {
            if( this->flags & WZ_STATE_HIDDEN )
            {
                ret = 0;
            }
            else
            {
                int flags = 0;
                if( this->flags & WZ_STATE_DISABLED )
                {
                    flags |= WZ_STYLE_DISABLED;
                }
                else if( this->flags & WZ_STATE_HAS_FOCUS )
                {
                    flags |= WZ_STYLE_FOCUSED;
                }

                if( this->down )
                {
                    flags |= WZ_STYLE_DOWN;
                }

                this->theme->draw_button( this->local_x, this->local_y, this->width, this->height, this->text, flags );
            }

            break;
        }
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_MOVE:
            x = event->touch.x;
            y = event->touch.y;
#endif
        case ALLEGRO_EVENT_MOUSE_AXES:
        {
            if( event->type == ALLEGRO_EVENT_MOUSE_AXES )
            {
                x = event->mouse.x;
                y = event->mouse.y;
            }

            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else if( ( event->mouse.dx != 0 || event->mouse.dy != 0 ) && this->widget_rect_test( x, y )
                     && !( this->flags & WZ_STATE_HAS_FOCUS ) )
            {
                this->ask_parent_for_focus();
            }
            else
            {
                ret = 0;
            }

            break;
        }
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
                this->down = 1;
                this->hold_focus = 1;
            }
            else
            {
                ret = 0;
            }

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
                        this->down = 1;
                    }
                    else
                    {
                        ret = 0;
                    }

                    break;
                }
                default:
                    ret = 0;
            }

            break;
        }
        case ALLEGRO_EVENT_KEY_UP:
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
                    {
                        ret = 0;
                    }

                    break;
                }
                default:
                    ret = 0;
            }

            break;
        }
        case WZ_HANDLE_SHORTCUT:
        {
            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else
            {
                if( !( this->flags & WZ_STATE_HAS_FOCUS ) )
                {
                    this->ask_parent_for_focus();
                }

                this->trigger();
            }

            break;
        }
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_END:
            x = event->touch.x;
            y = event->touch.y;
#endif
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        {
            if( event->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP )
            {
                x = event->mouse.x;
                y = event->mouse.y;
            }

            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else if( this->down == 1 )
            {
                if( this->widget_rect_test( x, y ) )
                {
                    this->trigger();
                }

                this->down = 0;
                this->hold_focus = 0;
            }
            else
            {
                ret = 0;
            }

            break;
        }
        case WZ_DESTROY:
        {
            if( this->own )
            {
                al_ustr_free( this->text );
            }

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
        case WZ_TRIGGER:
        {
            ALLEGRO_EVENT ev;
            this->down = 0;
            wz_craft_event( &ev, WZ_BUTTON_PRESSED, this, (intptr_t)this );
            al_emit_user_event( this->source, &ev, nullptr );
            break;
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

auto WZ_BUTTON::handle_draw() -> int
{
    int ret = 1;
    return ret;
}

/*
Section: Public

Function: wz_create_button

Creates a button.

Parameters:

own - Set to 1 if you want the widget to own the text

Inherits From:
<WZ_WIDGET>

See Also:
<WZ_BUTTON>

<wz_create_widget>
*/
WZ_BUTTON::WZ_BUTTON( WZ_WIDGET *parent,
                      float x,
                      float y,
                      float width,
                      float height,
                      ALLEGRO_USTR *text,
                      int own,
                      int id )
    : WZ_BOX( parent, x, y, width, height, id )
{
    this->down = 0;
    this->own = own;
    this->text = text;
}
