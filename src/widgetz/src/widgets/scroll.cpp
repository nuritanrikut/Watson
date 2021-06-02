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

/* Returns 1 if the position changed, 0 otherwise*/
int WZ_SCROLL::set_scroll_pos( float x, float y )
{
    float fraction;
    int old_pos;

    if( this->height > this->width )
    {
        float max_size = 0.9f * this->height;
        float slider_size = this->slider_size > max_size ? max_size : this->slider_size;
        fraction = ( (float)( y - this->local_y - slider_size / 2 ) ) / ( (float)this->height - slider_size );
    }
    else
    {
        float max_size = 0.9f * this->width;
        float slider_size = this->slider_size > max_size ? max_size : this->slider_size;
        fraction = ( (float)( x - this->local_x - slider_size / 2 ) ) / ( (float)this->width - slider_size );
    }

    old_pos = this->cur_pos;
    this->cur_pos = (int)( ( (float)this->max_pos ) * fraction + 0.5f );

    if( this->cur_pos < 0 )
        this->cur_pos = 0;

    if( this->cur_pos > this->max_pos )
        this->cur_pos = this->max_pos;

    return old_pos != this->cur_pos;
}

/*
Title: Scroll Bar

Section: Internal

Function: wz_scroll_proc

See also:
<wz_widget_proc>
*/
int WZ_SCROLL::proc( const ALLEGRO_EVENT *event )
{
    int ret = 1;
    float x, y;
    int vertical = this->height > this->width;

    switch( event->type )
    {
        case WZ_DRAW:
        {
            int flags = 0;
            float fraction;

            if( this->flags & WZ_STATE_HIDDEN )
            {
                ret = 0;
            }
            else if( this->flags & WZ_STATE_DISABLED )
            {
                flags = WZ_STYLE_DISABLED;
            }
            else if( this->flags & WZ_STATE_HAS_FOCUS )
            {
                flags = WZ_STYLE_FOCUSED;
            }

            fraction = ( (float)this->cur_pos ) / ( (float)this->max_pos );
            this->theme->draw_scroll(
                this->local_x, this->local_y, this->width, this->height, fraction, this->slider_size, flags );
            break;
        }
        case WZ_SET_SCROLL_POS:
        {
            this->cur_pos = event->user.data3;

            if( this->cur_pos < 0 )
                this->cur_pos = 0;

            if( this->cur_pos > this->max_pos )
                this->cur_pos = this->max_pos;

            break;
        }
        case WZ_SET_SCROLL_MAX_POS:
        {
            this->max_pos = event->user.data3;

            if( this->max_pos < 0 )
                this->max_pos = 0;

            if( this->cur_pos > this->max_pos )
                this->cur_pos = this->max_pos;

            break;
        }
        case WZ_HANDLE_SHORTCUT:
        {
            this->ask_parent_for_focus();
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
            else if( event->mouse.dx != 0 || event->mouse.dy != 0 )
            {
                ret = 0;
                if( this->widget_rect_test( x, y ) && !( this->flags & WZ_STATE_HAS_FOCUS ) )
                {
                    this->ask_parent_for_focus();
                    ret = 1;
                }

                if( this->down )
                {
                    if( set_scroll_pos( x, y ) )
                    {
                        this->trigger();
                        ret = 1;
                    }
                }
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
                this->hold_focus = 1;

                if( set_scroll_pos( x, y ) )
                    this->trigger();

                this->down = 1;
            }
            else
                ret = 0;

            break;
        }
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_END:
#endif
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
        {
            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else
            {
                if( this->down )
                {
                    this->down = 0;
                    this->hold_focus = 0;
                }
                else
                {
                    ret = 0;
                }
            }
            break;
        }
        case ALLEGRO_EVENT_KEY_CHAR:
        {
            int old_pos = this->cur_pos;

            switch( event->keyboard.keycode )
            {
                case ALLEGRO_KEY_LEFT:
                {
                    if( !vertical && this->cur_pos > 0 )
                        this->cur_pos--;
                    else
                        ret = 0;

                    break;
                }
                case ALLEGRO_KEY_RIGHT:
                {
                    if( !vertical && this->cur_pos < this->max_pos )
                        this->cur_pos++;
                    else
                        ret = 0;

                    break;
                }
                case ALLEGRO_KEY_UP:
                {
                    if( vertical && this->cur_pos > 0 )
                        this->cur_pos--;
                    else
                        ret = 0;

                    break;
                }
                case ALLEGRO_KEY_DOWN:
                {
                    if( vertical && this->cur_pos < this->max_pos )
                        this->cur_pos++;
                    else
                        ret = 0;

                    break;
                }
                default:
                    ret = 0;
            }

            if( old_pos != this->cur_pos )
            {
                this->trigger();
            }

            break;
        }
        case WZ_TRIGGER:
        {
            ALLEGRO_EVENT ev;

            if( !( this->flags & WZ_STATE_HAS_FOCUS ) )
            {
                this->ask_parent_for_focus();
            }

            wz_craft_event( &ev, WZ_SCROLL_POS_CHANGED, this, this->cur_pos );
            al_emit_user_event( this->source, &ev, 0 );
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

Function: wz_create_scroll

Creates a scroll bar.

Inherits From:
<WZ_WIDGET>

See Also:
<WZ_SCROLL>

<wz_create_widget>
*/
WZ_SCROLL::WZ_SCROLL( WZ_WIDGET *parent,
                      float x,
                      float y,
                      float width,
                      float height,
                      int max_pos,
                      int slider_size,
                      int id )
    : WZ_WIDGET( parent, x, y, width, height, id )
{
    this->cur_pos = 0;
    this->max_pos = max_pos;
    this->down = 0;
    this->slider_size = slider_size;
}
