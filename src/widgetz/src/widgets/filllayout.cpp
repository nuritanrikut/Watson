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

#include "../../widgetz_internal.hpp"

#include "../../../macros.hpp"

/*
Title: Fill Layout

Section: Internal

Function: wz_fill_layout_proc

See also:
<wz_widget_proc>
*/
int wz_fill_layout_proc( WZ_WIDGET *wgt, const ALLEGRO_EVENT *event )
{
    int ret = 1;
    WZ_FILL_LAYOUT *box = (WZ_FILL_LAYOUT *)wgt;

    switch( event->type )
    {
        case WZ_DRAW:
        {
            if( wgt->flags & WZ_STATE_HIDDEN )
            {
                ret = 0;
            }
            else
            {
                wgt->theme->draw_box( wgt->theme,
                                      wgt->local_x,
                                      wgt->local_y,
                                      wgt->width,
                                      wgt->height,
                                      wgt->flags & WZ_STATE_DISABLED ? WZ_STYLE_DISABLED : 0 );
            }

            break;
        }
        case WZ_TAKE_FOCUS:
        {
            wz_ask_parent_to_focus_next( wgt );
            ret = 0;
            break;
        }
        case WZ_RESIZE:
        {
            float factor = *(float *)&event->user.data3;
            wgt->x *= factor;
            wgt->y *= factor;
            wgt->width *= factor;
            wgt->height *= factor;
            wgt->local_x *= factor;
            wgt->local_y *= factor;
            box->h_spacing *= factor;
            box->v_spacing *= factor;
            break;
        }
        case WZ_UPDATE:
        {
            float total_width = box->h_spacing;
            float total_height = box->v_spacing;
            float max_height = 0;
            float x = 0;
            float y = 0;
            int row = 0;
            WZ_WIDGET *child = wgt->next_sib;
            WZ_WIDGET *row_start = child;
            WZ_WIDGET *it;

            while( child )
            {
                if( child->flags & WZ_STATE_LAYOUT )
                    break;

                if( !( child->flags & WZ_STATE_HIDDEN ) )
                {
                    if( total_width + child->width > wgt->width )
                    {
                        if( total_width > 2 * box->h_spacing + 0.1 )
                        {
                            /*
							I.e. we actually added something
							*/
                            float x = 0;
                            it = row_start;

                            if( box->h_align == WZ_ALIGN_LEFT )
                                x = box->h_spacing;
                            else if( box->h_align == WZ_ALIGN_RIGHT )
                                x = wgt->width - total_width + box->h_spacing;
                            else
                                x = ( wgt->width - total_width ) / 2 + box->h_spacing;

                            while( it != child )
                            {
                                if( !( it->flags & WZ_STATE_HIDDEN ) )
                                {
                                    it->x = x + wgt->x;
                                    x += it->width + box->h_spacing;

                                    if( it->height > max_height )
                                    {
                                        max_height = it->height;
                                    }
                                }

                                it = it->next_sib;
                            }

                            total_width = box->h_spacing;
                            total_height += max_height + box->v_spacing;
                            max_height = 0;
                            row += 1;
                            row_start = child;
                        }
                    }

                    total_width += child->width + box->h_spacing;
                    child->y = row;
                }

                child = child->next_sib;
            }

            it = row_start;
            x = 0;

            if( box->h_align == WZ_ALIGN_LEFT )
                x = box->h_spacing;
            else if( box->h_align == WZ_ALIGN_RIGHT )
                x = wgt->width - total_width + box->h_spacing;
            else
                x = ( wgt->width - total_width ) / 2 + box->h_spacing;

            while( it != 0 )
            {
                if( it->flags & WZ_STATE_LAYOUT )
                    break;

                if( !( it->flags & WZ_STATE_HIDDEN ) )
                {
                    it->x = x + wgt->x;
                    x += it->width + box->h_spacing;

                    if( it->height > max_height )
                    {
                        max_height = it->height;
                    }
                }

                it = it->next_sib;
            }

            total_height += max_height + box->v_spacing;
            /*
			And now, arrange them in the vertical direction
			*/
            row = 0;
            max_height = 0;
            child = wgt->next_sib;

            if( box->v_align == WZ_ALIGN_TOP )
                y = box->v_spacing;
            else if( box->v_align == WZ_ALIGN_BOTTOM )
                y = wgt->height - total_height + box->v_spacing;
            else
                y = ( wgt->height - total_height ) / 2 + box->v_spacing;

            while( child )
            {
                if( child->flags & WZ_STATE_LAYOUT )
                    break;

                if( !( child->flags & WZ_STATE_HIDDEN ) )
                {
                    if( child->y != row )
                    {
                        y += max_height + box->v_spacing;
                        max_height = 0;
                        row += 1;
                    }

                    if( child->height > max_height )
                    {
                        max_height = child->height;
                    }

                    child->y = y + wgt->y;
                }

                child = child->next_sib;
            }

            break;
        }
        default:
            ret = 0;
    }

    if( ret == 0 )
        ret = wz_box_proc( wgt, event );

    return ret;
}

/*
Function: wz_init_fill_layout
*/
void wz_init_fill_layout( WZ_FILL_LAYOUT *box,
                          WZ_WIDGET *parent,
                          float x,
                          float y,
                          float width,
                          float height,
                          float hspace,
                          float vspace,
                          int halign,
                          int valign,
                          int id )
{
    WZ_WIDGET *wgt = (WZ_WIDGET *)box;
    wz_init_box( wgt, parent, x, y, width, height, id );
    wgt->proc = wz_fill_layout_proc;
    wgt->flags |= WZ_STATE_LAYOUT;
    wgt->flags |= WZ_STATE_NOTWANT_FOCUS;
    box->h_spacing = hspace;
    box->v_spacing = vspace;
    box->h_align = halign;
    box->v_align = valign;
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
WZ_FILL_LAYOUT *wz_create_fill_layout( WZ_WIDGET *parent,
                                       float x,
                                       float y,
                                       float width,
                                       float height,
                                       float hspace,
                                       float vspace,
                                       int halign,
                                       int valign,
                                       int id )
{
    WZ_FILL_LAYOUT *box = new WZ_FILL_LAYOUT();
    wz_init_fill_layout( box, parent, x, y, width, height, hspace, vspace, halign, valign, id );
    wz_ask_parent_to_focus_next( (WZ_WIDGET *)box );
    return box;
}
