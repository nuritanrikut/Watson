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

#include "../widgetz_internal.hpp"

#include "../../macros.hpp"

/*
Title: Internal

These are functions that are useful if you need to create additional widgets
*/

/*
Function: wz_ask_parent_for_focus

Asks the parent to defocus everyone but the widget that calls this function.

Returns:
1 if the widget already has focus, or succesfully obtained the focus

0 if the widget cannot be focused
*/
auto WZ_WIDGET::ask_parent_for_focus() -> int
{
    if( this->flags & WZ_STATE_HAS_FOCUS )
        return 1;

    if( this->flags & WZ_STATE_NOTWANT_FOCUS )
        return 0;

    if( this->flags & WZ_STATE_DISABLED )
        return 0;

    if( this->flags & WZ_STATE_HIDDEN )
        return 0;

    if( this->parent == nullptr )
    {
        ALLEGRO_EVENT event;
        wz_craft_event( &event, WZ_TAKE_FOCUS, this, 0 );
        this->send_event( &event );
    }
    else
    {
        ALLEGRO_EVENT event;

        if( !( this->parent->flags & WZ_STATE_HAS_FOCUS ) )
        {
            this->parent->ask_parent_for_focus();
        }

        wz_craft_event( &event, WZ_WANT_FOCUS, this, 0 );
        this->parent->send_event( &event );
    }

    return 1;
}

/*
Function: wz_ask_parent_to_focus_next

Asks the parent to focus the next child if possible
*/
void WZ_WIDGET::ask_parent_to_focus_next()
{
    WZ_WIDGET *child;

    if( this->parent == nullptr )
        return;

    child = this->next_sib;

    while( child )
    {
        if( child->ask_parent_for_focus() )
            return;

        child = child->next_sib;
    }

    child = this->parent->first_child;

    while( child != this )
    {
        if( child->ask_parent_for_focus() )
            return;

        child = child->next_sib;
    }
}

/*
Function: wz_ask_parent_to_focus_prev

Asks the parent to focus the previous child if possible
*/
void WZ_WIDGET::ask_parent_to_focus_prev()
{
    WZ_WIDGET *child;

    if( this->parent == nullptr )
        return;

    child = this->prev_sib;

    while( child )
    {
        if( child->ask_parent_for_focus() )
            return;

        child = child->prev_sib;
    }

    child = this->parent->last_child;

    while( child != this )
    {
        if( child->ask_parent_for_focus() )
            return;

        child = child->prev_sib;
    }
}

/*
Function: wz_get_widget_dir

A function that returns a widget that is located the closest to the passed widget in some given direction.

Parameters:
dir - 0 is up, 1 is right, 2 is down, 3 is left

Returns:

The widget it found, or the passed widget if it found nothing
*/
auto WZ_WIDGET::get_widget_dir( int dir ) -> WZ_WIDGET *
{
    float least_dev = 100000;
    WZ_WIDGET *ret = this;
    WZ_WIDGET *child;

    if( this->parent == nullptr )
        return this;

    child = this->parent->first_child;

    while( child )
    {
        float dev = 1000000;

        switch( dir )
        {
            case 0:
            {
                if( child->y + child->height < this->y )
                {
                    dev = this->y - ( child->y + child->height ) + fabs( this->x - child->x );
                }

                break;
            }
            case 1:
            {
                if( child->x > this->x + this->width )
                {
                    dev = child->x - ( this->x + this->width ) + fabs( this->y - child->y );
                }

                break;
            }
            case 2:
            {
                if( child->y > this->y + this->height )
                {
                    dev = child->y - ( this->y + this->height ) + fabs( this->x - child->x );
                }

                break;
            }
            default:
            {
                if( child->x + child->width < this->x )
                {
                    dev = this->x - ( child->x + child->width ) + fabs( this->y - child->y );
                }

                break;
            }
        }

        if( child != this && dev < least_dev && !( child->flags & WZ_STATE_NOTWANT_FOCUS )
            && !( child->flags & WZ_STATE_DISABLED ) && !( child->flags & WZ_STATE_HIDDEN ) )
        {
            least_dev = dev;
            ret = child;
        }

        child = child->next_sib;
    }

    return ret;
}

/*
Function: wz_craft_event

Crafts a simple GUI event.

Parameters:
type - Type of the event
source - The widget that launched the event, or 0 if it has no source
data - data you wish to attach to the event
*/
void wz_craft_event( ALLEGRO_EVENT *event, int type, WZ_WIDGET *source, intptr_t data )
{
    event->user.type = type;
    event->user.timestamp = al_get_time();
    event->user.data1 = source == nullptr ? -1 : source->id;
    event->user.data2 = (intptr_t)source;
    event->user.data3 = data;
}

/*
Function: wz_get_text_pos

Parameters:

text - the text you want to search
x - the length you want to match

Returns:

The character position such that the text length of the string up to that character
is as close as possible to the passed length.
*/
auto wz_get_text_pos( ALLEGRO_FONT *font, ALLEGRO_USTR *text, float x ) -> int
{
    int ii = 0;
    int len = al_ustr_length( text );
    float width = al_get_ustr_width( font, text );

    if( x > width )
    {
        return len + 1;
    }

    if( x < 0 )
    {
        return 0;
    }
    else
    {
        float old_diff = x;
        float diff;
        ALLEGRO_USTR_INFO info;

        for( ii = 0; ii <= len; ii++ )
        {
            int offset = al_ustr_offset( text, ii );
            const ALLEGRO_USTR *str = al_ref_ustr( &info, text, 0, offset );
            diff = fabs( x - al_get_ustr_width( font, str ) );

            if( diff > old_diff )
            {
                return ii - 1;
            }

            old_diff = diff;
        }
    }

    return ii - 1;
}
