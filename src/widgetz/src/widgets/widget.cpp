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
            ret = handle_hide();
            break;
        case WZ_SHOW:
            ret = handle_show();
            break;
        case WZ_DISABLE:
            ret = handle_disable();
            break;
        case WZ_ENABLE:
            ret = handle_enable();
            break;
        case WZ_UPDATE_POSITION:
            ret = handle_update_position();
            break;
        case WZ_DESTROY:
            ret = handle_destroy();
            break;
        case WZ_LOSE_FOCUS:
            ret = handle_lose_focus();
            break;
        case WZ_TAKE_FOCUS:
            ret = handle_take_focus();
            break;
        case WZ_WANT_FOCUS:
            ret = handle_want_focus( event );
            break;
        case WZ_RESIZE:
            ret = handle_resize( event );
            break;
        case ALLEGRO_EVENT_KEY_CHAR:
            ret = handle_key_char( event );
            break;
        default:
            ret = 0;
    }

    return ret;
}

int WZ_WIDGET::handle_hide()
{
    int ret = 1;
    this->flags |= WZ_STATE_HIDDEN;
    return ret;
}

int WZ_WIDGET::handle_show()
{
    int ret = 1;
    this->flags &= ~WZ_STATE_HIDDEN;
    return ret;
}

int WZ_WIDGET::handle_disable()
{
    int ret = 1;
    this->flags |= WZ_STATE_DISABLED;
    return ret;
}

int WZ_WIDGET::handle_enable()
{
    int ret = 1;
    this->flags &= ~WZ_STATE_DISABLED;
    return ret;
}

int WZ_WIDGET::handle_update_position()
{
    int ret = 1;
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
    return ret;
}

int WZ_WIDGET::handle_destroy()
{
    int ret = 1;
    al_destroy_user_event_source( this->source );
    delete this->source;
    delete this;
    return ret;
}

int WZ_WIDGET::handle_lose_focus()
{
    int ret = 1;
    this->flags &= ~WZ_STATE_HAS_FOCUS;
    return ret;
}

int WZ_WIDGET::handle_take_focus()
{
    int ret = 1;
    this->focus( 0 );
    this->flags |= WZ_STATE_HAS_FOCUS;

    if( this->first_child )
        this->first_child->focus( 1 );
    return ret;
}

int WZ_WIDGET::handle_want_focus( const ALLEGRO_EVENT *event )
{
    int ret = 1;
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
    return ret;
}

int WZ_WIDGET::handle_resize( const ALLEGRO_EVENT *event )
{
    int ret = 1;
    float factor = *(float *)&event->user.data3;
    this->x *= factor;
    this->y *= factor;
    this->width *= factor;
    this->height *= factor;
    this->local_x *= factor;
    this->local_y *= factor;
    return ret;
}

int WZ_WIDGET::handle_key_char( const ALLEGRO_EVENT *event )
{
    if( event->keyboard.keycode == this->shortcut.keycode
        && ( ( event->keyboard.modifiers & this->shortcut.modifiers ) || this->shortcut.modifiers == 0 ) )
    {
        ALLEGRO_EVENT ev;
        wz_craft_event( &ev, WZ_HANDLE_SHORTCUT, this, 0 );
        this->send_event( &ev );
        return 1;
    }

    if( !( ( event->keyboard.keycode == ALLEGRO_KEY_TAB ) || ( event->keyboard.keycode == ALLEGRO_KEY_UP )
           || ( event->keyboard.keycode == ALLEGRO_KEY_RIGHT ) || ( event->keyboard.keycode == ALLEGRO_KEY_DOWN )
           || ( event->keyboard.keycode == ALLEGRO_KEY_LEFT ) ) )
        return 0;

    if( this->first_child != 0 || !this->parent )
    {
        return 0;
    }

    if( event->keyboard.keycode == ALLEGRO_KEY_TAB )
    {
        if( event->keyboard.modifiers & 1 )
        {
            this->ask_parent_to_focus_prev();
        }
        else
        {
            this->ask_parent_to_focus_next();
        }
    }
    else if( event->keyboard.keycode == ALLEGRO_KEY_UP )
    {
        this->get_widget_dir( 0 )->ask_parent_for_focus();
    }
    else if( event->keyboard.keycode == ALLEGRO_KEY_RIGHT )
    {
        this->get_widget_dir( 1 )->ask_parent_for_focus();
    }
    else if( event->keyboard.keycode == ALLEGRO_KEY_DOWN )
    {
        this->get_widget_dir( 2 )->ask_parent_for_focus();
    }
    else if( event->keyboard.keycode == ALLEGRO_KEY_LEFT )
    {
        this->get_widget_dir( 3 )->ask_parent_for_focus();
    }

    return 1;
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
