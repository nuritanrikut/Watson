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
Title: Fill Layout

Section: Internal

Function: wz_fill_layout_proc

See also:
<wz_widget_proc>
*/
auto WZ_FILL_LAYOUT::proc( const ALLEGRO_EVENT *event ) -> int
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
                this->theme->draw_box( this->local_x,
                                       this->local_y,
                                       this->width,
                                       this->height,
                                       this->flags & WZ_STATE_DISABLED ? WZ_STYLE_DISABLED : 0 );
            }

            break;
        }
        case WZ_TAKE_FOCUS:
        {
            this->ask_parent_to_focus_next();
            ret = 0;
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
            this->h_spacing *= factor;
            this->v_spacing *= factor;
            break;
        }
        case WZ_UPDATE:
        {
            float total_width = this->h_spacing;
            float total_height = this->v_spacing;
            float max_height = 0;
            float x = 0;
            float y = 0;
            int row = 0;
            WZ_WIDGET *child = this->next_sib;
            WZ_WIDGET *row_start = child;
            WZ_WIDGET *it;

            while( child )
            {
                if( child->flags & WZ_STATE_LAYOUT )
                {
                    break;
                }

                if( !( child->flags & WZ_STATE_HIDDEN ) )
                {
                    if( total_width + child->width > this->width )
                    {
                        if( total_width > 2 * this->h_spacing + 0.1 )
                        {
                            /*
							I.e. we actually added something
							*/
                            float x = 0;
                            it = row_start;

                            if( this->h_align == WZ_ALIGN_LEFT )
                            {
                                x = this->h_spacing;
                            }
                            else if( this->h_align == WZ_ALIGN_RIGHT )
                            {
                                x = this->width - total_width + this->h_spacing;
                            }
                            else
                            {
                                x = ( this->width - total_width ) / 2 + this->h_spacing;
                            }

                            while( it != child )
                            {
                                if( !( it->flags & WZ_STATE_HIDDEN ) )
                                {
                                    it->x = x + this->x;
                                    x += it->width + this->h_spacing;

                                    if( it->height > max_height )
                                    {
                                        max_height = it->height;
                                    }
                                }

                                it = it->next_sib;
                            }

                            total_width = this->h_spacing;
                            total_height += max_height + this->v_spacing;
                            max_height = 0;
                            row += 1;
                            row_start = child;
                        }
                    }

                    total_width += child->width + this->h_spacing;
                    child->y = row;
                }

                child = child->next_sib;
            }

            it = row_start;
            x = 0;

            if( this->h_align == WZ_ALIGN_LEFT )
            {
                x = this->h_spacing;
            }
            else if( this->h_align == WZ_ALIGN_RIGHT )
            {
                x = this->width - total_width + this->h_spacing;
            }
            else
            {
                x = ( this->width - total_width ) / 2 + this->h_spacing;
            }

            while( it != nullptr )
            {
                if( it->flags & WZ_STATE_LAYOUT )
                {
                    break;
                }

                if( !( it->flags & WZ_STATE_HIDDEN ) )
                {
                    it->x = x + this->x;
                    x += it->width + this->h_spacing;

                    if( it->height > max_height )
                    {
                        max_height = it->height;
                    }
                }

                it = it->next_sib;
            }

            total_height += max_height + this->v_spacing;
            /*
			And now, arrange them in the vertical direction
			*/
            row = 0;
            max_height = 0;
            child = this->next_sib;

            if( this->v_align == WZ_ALIGN_TOP )
            {
                y = this->v_spacing;
            }
            else if( this->v_align == WZ_ALIGN_BOTTOM )
            {
                y = this->height - total_height + this->v_spacing;
            }
            else
            {
                y = ( this->height - total_height ) / 2 + this->v_spacing;
            }

            while( child )
            {
                if( child->flags & WZ_STATE_LAYOUT )
                {
                    break;
                }

                if( !( child->flags & WZ_STATE_HIDDEN ) )
                {
                    if( child->y != row )
                    {
                        y += max_height + this->v_spacing;
                        max_height = 0;
                        row += 1;
                    }

                    if( child->height > max_height )
                    {
                        max_height = child->height;
                    }

                    child->y = y + this->y;
                }

                child = child->next_sib;
            }

            break;
        }
        default:
            ret = 0;
    }

    if( ret == 0 )
    {
        ret = WZ_BOX::proc( event );
    }

    return ret;
}

/*
Section: Public

Function: wz_create_fill_layout

Creates a fill layout meta-widget.

Inherits From:
<WZ_WIDGET>

See Also:
<WZ_FILL_LAYOUT>

<wz_create_widget>
*/
WZ_FILL_LAYOUT::WZ_FILL_LAYOUT( WZ_WIDGET *parent,
                                float x,
                                float y,
                                float width,
                                float height,
                                float hspace,
                                float vspace,
                                int halign,
                                int valign,
                                int id )
    : WZ_BOX( parent, x, y, width, height, id )
{
    this->flags |= WZ_STATE_LAYOUT;
    this->flags |= WZ_STATE_NOTWANT_FOCUS;
    this->h_spacing = hspace;
    this->v_spacing = vspace;
    this->h_align = halign;
    this->v_align = valign;
    this->ask_parent_to_focus_next();
}
