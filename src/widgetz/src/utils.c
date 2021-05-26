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

#include "../widgetz_internal.h"

/*
Title: Public

These are the functions you use to interact with the WidgetZ widgets.
*/

/*
Function: wz_set_shortcut

Sets the shorcut combination for a widget.

Parameters:

keycode - Allegro keycode to use
modifiers - bitfield of modifier flags to match. You can combine multiple flags via binary OR.
*/
void wz_set_shortcut( WZ_WIDGET *wgt, int keycode, int modifiers )
{
    wgt->shortcut.keycode = keycode;
    wgt->shortcut.modifiers = modifiers;
}

/*
Function: wz_register_sources

Registers the widget tree with an Allegro event queue. Now, any event generated by this widget and any
of its children will be registered by it.

Parameters:

queue - Allegro event queue to use
*/
void wz_register_sources( WZ_WIDGET *wgt, ALLEGRO_EVENT_QUEUE *queue )
{
    WZ_WIDGET *child;
    al_register_event_source( queue, wgt->source );
    child = wgt->first_child;

    while( child )
    {
        wz_register_sources( child, queue );
        child = child->next_sib;
    }
}

/*
Function: wz_set_theme

Sets the theme to a widget, setting it for every widget in the widget tree

Parameters:

theme - Widget theme to use

See also:

<WZ_DEF_THEME>
*/
void wz_set_theme( WZ_WIDGET *wgt, WZ_THEME *theme )
{
    WZ_WIDGET *child;
    wgt->theme = theme;
    child = wgt->first_child;

    while( child )
    {
        wz_set_theme( child, theme );
        child = child->next_sib;
    }
}

/*
Function: wz_detach

Detaches a widget tree from its parent
*/
void wz_detach( WZ_WIDGET *wgt )
{
    if( wgt->parent == 0 )
        return;

    if( wgt->next_sib != 0 )
    {
        if( wgt->prev_sib != 0 )
        {
            wgt->prev_sib->next_sib = wgt->next_sib;
            wgt->next_sib->prev_sib = wgt->prev_sib;
        }
        else
        {
            wgt->next_sib->prev_sib = 0;
            wgt->parent->first_child = wgt->next_sib;
        }
    }
    else if( wgt->prev_sib != 0 )
    {
        if( wgt->next_sib != 0 )
        {
            wgt->next_sib->prev_sib = wgt->prev_sib;
            wgt->prev_sib->next_sib = wgt->next_sib;
        }
        else
        {
            wgt->prev_sib->next_sib = 0;
            wgt->parent->last_child = wgt->prev_sib;
        }
    }
    else
    {
        wgt->parent->first_child = 0;
        wgt->parent->last_child = 0;
    }

    wgt->parent = 0;
    wgt->next_sib = 0;
    wgt->prev_sib = 0;
}

/*
Function: wz_attach

Attaches a widget tree to its parent
*/
void wz_attach( WZ_WIDGET *wgt, WZ_WIDGET *parent )
{
    if( parent == 0 )
        return;

    wz_detach( wgt );
    wgt->parent = parent;

    if( parent->last_child != 0 )
    {
        parent->last_child->next_sib = wgt;
        wgt->prev_sib = parent->last_child;
    }

    if( parent->first_child == 0 )
        parent->first_child = wgt;

    parent->last_child = wgt;
    wz_set_theme( wgt, parent->theme );
}

/*
Function: wz_attach_after

Attaches a widget to a parent after a specific widget (the parent is taken from sib)

Parameters:

sib - Widget after which this widget will be attached. It's parent is used as the parent of this widget
*/
void wz_attach_after( WZ_WIDGET *wgt, WZ_WIDGET *sib )
{
    if( sib->parent == 0 )
        return;

    wz_detach( wgt );

    if( sib->next_sib )
    {
        sib->next_sib->prev_sib = wgt;
    }
    else
    {
        sib->parent->last_child = wgt;
    }

    wgt->next_sib = sib->next_sib;
    sib->next_sib = wgt;
    wgt->prev_sib = sib;
    wgt->parent = sib->parent;
}

/*
Function: wz_attach_before

Attaches a widget to a parent before a specific widget (the parent is taken from sib)

Parameters:

sib - Widget before which this widget will be attached. It's parent is used as the parent of this widget
*/
void wz_attach_before( WZ_WIDGET *wgt, WZ_WIDGET *sib )
{
    if( sib->parent == 0 )
        return;

    wz_detach( wgt );

    if( sib->prev_sib )
    {
        sib->prev_sib->next_sib = wgt;
    }
    else
    {
        sib->parent->first_child = wgt;
    }

    wgt->prev_sib = sib->prev_sib;
    sib->prev_sib = wgt;
    wgt->next_sib = sib;
    wgt->parent = sib->parent;
}

/*
Function: wz_send_event

Sends an event to a widget, and to any of its focused children, propagating down the tree.
The first widget that processes the event breaks the process.
Note that the first widget gets the event whether it is focused or not.

Returns:
1 if the event was handled by some widget, 0 if it was not
*/
int wz_send_event( WZ_WIDGET *wgt, const ALLEGRO_EVENT *event )
{
    WZ_WIDGET *child = wgt->first_child;

    if( wgt->proc( wgt, event ) )
        return 1;

    while( child )
    {
        if( ( child->flags & WZ_STATE_HAS_FOCUS ) && wz_send_event( child, event ) )
            return 1;

        child = child->next_sib;
    }

    /*
	See if the unfocused ones would like some too
	*/
    child = wgt->first_child;

    while( child )
    {
        if( wz_send_event( child, event ) )
            return 1;

        child = child->next_sib;
    }

    return 0;
}

/*
Function: wz_broadcast_event

Broadcasts an event to a widget, and to all of its children, propagating down the tree

Returns:
1 if the event was handled by some widget, 0 if it was not
*/
int wz_broadcast_event( WZ_WIDGET *wgt, const ALLEGRO_EVENT *event )
{
    int ret = 0;
    WZ_WIDGET *child = wgt->first_child;
    ret |= wgt->proc( wgt, event );

    while( child )
    {
        WZ_WIDGET *next = child->next_sib;
        ret |= wz_broadcast_event( child, event );
        child = next;
    }

    return ret;
}

/*
Function: wz_update

Updates the widget tree, call this every frame
Doesn't update the disabled widgets

Parameters:
dt - Time that has passed since the last update
*/
void wz_update( WZ_WIDGET *wgt, double dt )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, WZ_UPDATE, 0, 0 );
    wz_broadcast_event( wgt, &event );
    wz_craft_event( &event, WZ_UPDATE_POSITION, 0, 0 );
    wz_broadcast_event( wgt, &event );
}

/*
Function: wz_trigger

Triggers a widget. What the widget does depends on the widget. Generally, it will send
an event that is characteristic of the events that it usually sends. For example,
triggering a button will simulate an impression of the button, causing it to send <WZ_BUTTON_PRESSED>
event.
*/
void wz_trigger( WZ_WIDGET *wgt )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, WZ_TRIGGER, 0, 0 );
    wz_send_event( wgt, &event );
}

/*
Function: wz_draw

Draws the widget tree. Only affects the visible widgets
*/
void wz_draw( WZ_WIDGET *wgt )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, WZ_DRAW, 0, 0 );
    wz_broadcast_event( wgt, &event );
}

/*
Function: wz_set_text

Sets the text of a widget. The widget makes a local copy of the text.
*/
void wz_set_text( WZ_WIDGET *wgt, ALLEGRO_USTR *text )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, WZ_SET_TEXT, 0, (intptr_t)text );
    wz_send_event( wgt, &event );
}

/*
Function: wz_show

Shows/hides the widget tree

Parameters:
show - pass 1 to show the widget tree, or 0 to hide it
*/
void wz_show( WZ_WIDGET *wgt, int show )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, show ? WZ_SHOW : WZ_HIDE, 0, 0 );
    wz_broadcast_event( wgt, &event );
}

/*
Function: wz_destroy

Destroys the widget tree. Call it to free all of the memory used by the widget tree.
*/
void wz_destroy( WZ_WIDGET *wgt )
{
    ALLEGRO_EVENT event;
    wz_detach( wgt );
    wz_craft_event( &event, WZ_DESTROY, 0, 0 );
    wz_broadcast_event( wgt, &event );
}

/*
 Function: wz_resize
 
 Resizes the widget tree. 
 
 Parameters:
 factor: scaling factor
 */
void wz_resize( WZ_WIDGET *wgt, float factor )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, WZ_RESIZE, 0, *(intptr_t *)&factor );
    wz_broadcast_event( wgt, &event );
}

/*
Function: wz_enable

Enables/disables the widget tree

Parameters:
enable - pass 1 to enable the widget tree, or 0 to disable it
*/
void wz_enable( WZ_WIDGET *wgt, int enable )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, enable ? WZ_ENABLE : WZ_DISABLE, 0, 0 );
    wz_broadcast_event( wgt, &event );
}

/*
Function: wz_focus

Focuses/defocuses the widget tree. This function propagates the focus down the tree.

Parameters:
focus - pass 1 to focus the widget tree, or 0 to defocus it
*/
void wz_focus( WZ_WIDGET *wgt, int focus )
{
    if( focus )
    {
        wz_ask_parent_for_focus( wgt );
    }
    else
    {
        ALLEGRO_EVENT event;
        wz_craft_event( &event, WZ_LOSE_FOCUS, 0, 0 );
        wz_broadcast_event( wgt, &event );
    }
}

/*
Function: wz_set_scroll_pos

Sets the scroll position

Parameters:
max - Pass 1 to say that pos is actually the max position, 0 otherwise
*/
void wz_set_scroll_pos( WZ_WIDGET *wgt, int pos, int max )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, max ? WZ_SET_SCROLL_MAX_POS : WZ_SET_SCROLL_POS, 0, pos );
    wz_send_event( wgt, &event );
}

/*
Function: wz_set_cursor_pos

Sets the cursor position
*/
void wz_set_cursor_pos( WZ_WIDGET *wgt, int pos )
{
    ALLEGRO_EVENT event;
    wz_craft_event( &event, WZ_SET_CURSOR_POS, 0, pos );
    wz_send_event( wgt, &event );
}

/*
Function: wz_blend_colors

Blends two Allegro colors.

Parameters:
frac - Blending factor. 0 results in the output being the first color passed, 1 results in the second color passed.
Intermediate values blend between the two colors.
*/
ALLEGRO_COLOR wz_blend_colors( ALLEGRO_COLOR c1, ALLEGRO_COLOR c2, float frac )
{
    ALLEGRO_COLOR ret;
    ret.a = c1.a + ( c2.a - c1.a ) * frac;
    ret.r = c1.r + ( c2.r - c1.r ) * frac;
    ret.g = c1.g + ( c2.g - c1.g ) * frac;
    ret.b = c1.b + ( c2.b - c1.b ) * frac;
    return ret;
}

/*
Function: wz_scale_color

Tints an Allegro color by multiplying each component by a constant. Alpha is not touched.

Parameters:
scale - Scaling factor. 0 results in a black color, 1 results in no change
*/
ALLEGRO_COLOR wz_scale_color( ALLEGRO_COLOR c, float scale )
{
    ALLEGRO_COLOR ret;
    //ret.a = c.a * scale;
    ret.a = c.a;
    ret.r = c.r * scale;
    ret.g = c.g * scale;
    ret.b = c.b * scale;

    if( ret.r > 1 )
        ret.r = 1;

    if( ret.g > 1 )
        ret.g = 1;

    if( ret.b > 1 )
        ret.b = 1;

    //if(ret.a > 1)
    //	ret.a = 1;

    return ret;
}

/*
Function: wz_widget_rect_test

Tests if a point is inside the widget

Returns:
1 if the point is inside the widget, 0 otherwise
*/
int wz_widget_rect_test( WZ_WIDGET *wgt, float x, float y )
{
    return x > wgt->local_x && x < wgt->local_x + wgt->width && y > wgt->local_y && y < wgt->local_y + wgt->height;
}

/*
Function: wz_widget_rect_test_all

Like <wz_widget_rect_test> but traverses the widget tree to see if any of its branches
intersect with the passed coordinates. Hidden widgets are ignored.

Returns:
1 if the point is inside one of the widgets, 0 otherwise
*/
int wz_widget_rect_test_all( WZ_WIDGET *wgt, float x, float y )
{
    WZ_WIDGET *child;

    if( wgt->flags & WZ_STATE_HIDDEN )
        return 0;

    if( wz_widget_rect_test( wgt, x, y ) )
        return 1;

    child = wgt->first_child;

    while( child )
    {
        int ret = wz_widget_rect_test_all( child, x, y );

        if( ret )
            return 1;

        child = child->next_sib;
    }

    return 0;
}
