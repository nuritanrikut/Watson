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
Title: Widget

Section: Internal

Function: wz_widget_proc

Callback function that handles the operations of a general widget.
All widget procs call this function as their default handler.

Returns:

1 if the event was handled by the widget, 0 otherwise
*/
int WZ_WIDGET::proc( const ALLEGRO_EVENT *event )
{
    int ret = 1;

    switch( event->type )
    {
        case WZ_HIDE:
        {
            this->flags |= WZ_STATE_HIDDEN;
            break;
        }
        case WZ_SHOW:
        {
            this->flags &= ~WZ_STATE_HIDDEN;
            break;
        }
        case WZ_DISABLE:
        {
            this->flags |= WZ_STATE_DISABLED;
            break;
        }
        case WZ_ENABLE:
        {
            this->flags &= ~WZ_STATE_DISABLED;
            break;
        }
        case WZ_UPDATE_POSITION:
        {
            if( this->parent )
            {
                this->local_x = this->parent->local_x + this->x;
                this->local_y = this->parent->local_y + this->y;
            }
            else
            {
                this->local_x = this->x;
                this->local_y = this->y;
            }

            break;
        }
        case WZ_DESTROY:
        {
            al_destroy_user_event_source( this->source );
            delete this->source;
            delete this;
            break;
        }
        case WZ_LOSE_FOCUS:
        {
            this->flags &= ~WZ_STATE_HAS_FOCUS;
            break;
        }
        case WZ_TAKE_FOCUS:
        {
            this->focus( 0 );
            this->flags |= WZ_STATE_HAS_FOCUS;

            if( this->first_child )
                this->first_child->focus( 1 );

            break;
        }
        case WZ_WANT_FOCUS:
        {
            WZ_WIDGET *child = this->first_child;
            ALLEGRO_EVENT ev;
            int all_unfocused = 1;

            while( child )
            {
                if( child->hold_focus )
                {
                    all_unfocused = 0;
                    break;
                }

                child->focus( 0 );
                child = child->next_sib;
            }

            if( all_unfocused )
            {
                wz_craft_event( &ev, WZ_TAKE_FOCUS, this, 0 );
                WZ_WIDGET *wgt = (WZ_WIDGET *)event->user.data2;
                wgt->send_event( &ev );
            }

            break;
        }
        case WZ_RESIZE:
        {
            float factor = *(float *)&event->user.data3;
            this->x *= factor;
            this->y *= factor;
            this->width *= factor;
            this->height *= factor;
            this->local_x *= factor;
            this->local_y *= factor;
            break;
        }

        case ALLEGRO_EVENT_KEY_CHAR:
        {
            if( event->keyboard.keycode == this->shortcut.keycode
                && ( ( event->keyboard.modifiers & this->shortcut.modifiers ) || this->shortcut.modifiers == 0 ) )
            {
                ALLEGRO_EVENT ev;
                wz_craft_event( &ev, WZ_HANDLE_SHORTCUT, this, 0 );
                this->send_event( &ev );
            }
            else
            {
                switch( event->keyboard.keycode )
                {
                    case ALLEGRO_KEY_TAB:
                    {
                        if( this->first_child != 0 )
                        {
                            ret = 0;
                        }
                        else if( event->keyboard.modifiers & 1 )
                        {
                            this->ask_parent_to_focus_prev();
                        }
                        else
                        {
                            this->ask_parent_to_focus_next();
                        }

                        break;
                    }
                    case ALLEGRO_KEY_UP:
                    {
                        if( this->first_child != 0 )
                        {
                            ret = 0;
                        }
                        else if( this->parent != 0 )
                        {
                            this->get_widget_dir( 0 )->ask_parent_for_focus();
                        }
                        else
                            ret = 0;

                        break;
                    }
                    case ALLEGRO_KEY_RIGHT:
                    {
                        if( this->first_child != 0 )
                        {
                            ret = 0;
                        }
                        else if( this->parent != 0 )
                        {
                            this->get_widget_dir( 1 )->ask_parent_for_focus();
                        }
                        else
                            ret = 0;

                        break;
                    }
                    case ALLEGRO_KEY_DOWN:
                    {
                        if( this->first_child != 0 )
                        {
                            ret = 0;
                        }
                        else if( this->parent != 0 )
                        {
                            this->get_widget_dir( 2 )->ask_parent_for_focus();
                        }
                        else
                            ret = 0;

                        break;
                    }
                    case ALLEGRO_KEY_LEFT:
                    {
                        if( this->first_child != 0 )
                        {
                            ret = 0;
                        }
                        else if( this->parent != 0 )
                        {
                            this->get_widget_dir( 3 )->ask_parent_for_focus();
                        }
                        else
                            ret = 0;

                        break;
                    }
                    default:
                        ret = 0;
                }
            }

            break;
        }
        default:
            ret = 0;
    }

    return ret;
}

/*
Function: wz_set_theme

Sets the theme to a widget, setting it for every widget in the widget tree

Parameters:

theme - Widget theme to use

See also:

<WZ_DEF_THEME>
*/
void WZ_WIDGET::set_theme( WZ_THEME *theme )
{
    this->theme = theme;
    WZ_WIDGET *child = this->first_child;

    while( child )
    {
        child->set_theme( theme );
        child = child->next_sib;
    }
}

/*
Section: Public

Function: wz_create_widget

Creates a standard widget. You generally don't need to use this, unless you want a do-nothing dummy widget.

Parameters:
id - The id of this widget, which will be used to identify it when it sends events to the user's queue.
Pass -1 to automatically assign an id. If automatically assigned, id will equal the id of its previous sibling
plus one, or if it has none, the id of its parent plus one. If it has no parent, its id will be 0.

See Also:
<WZ_WIDGET>
*/
WZ_WIDGET::WZ_WIDGET( WZ_WIDGET *parent, float x, float y, float width, float height, int id )
{
    this->x = x;
    this->y = y;
    this->height = height;
    this->width = width;
    this->flags = 1;
    this->theme = get_def_theme();
    this->parent = nullptr;
    this->last_child = nullptr;
    this->first_child = nullptr;
    this->next_sib = nullptr;
    this->prev_sib = nullptr;
    this->id = id;
    this->hold_focus = 0;

    this->source = new ALLEGRO_EVENT_SOURCE();
    al_init_user_event_source( this->source );
    this->shortcut.modifiers = 0;
    this->shortcut.keycode = -1;
    this->attach( parent );
    this->ask_parent_for_focus();
}
