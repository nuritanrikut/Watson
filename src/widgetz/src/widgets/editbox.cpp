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

void WZ_EDITBOX::snap()
{
    ALLEGRO_FONT *font = this->theme->get_font( 0 );
    int len = al_ustr_length( this->text );
    int size = al_ustr_size( this->text );
    int scroll_offset = al_ustr_offset( this->text, this->scroll_pos );
    int cursor_offset;
    ALLEGRO_USTR_INFO info;
    const ALLEGRO_USTR *text = al_ref_ustr( &info, this->text, scroll_offset, size );
    int max_rel_cursor_pos = wz_get_text_pos( font, (ALLEGRO_USTR *)text, this->width );

    if( this->cursor_pos < this->scroll_pos )
    {
        this->scroll_pos = this->cursor_pos;
    }

    if( this->cursor_pos > this->scroll_pos + max_rel_cursor_pos )
    {
        this->scroll_pos = this->cursor_pos - max_rel_cursor_pos;
    }

    if( this->cursor_pos > 0 && this->cursor_pos - this->scroll_pos < 1 )
    {
        this->scroll_pos--;
    }

    if( this->cursor_pos > len )
    {
        this->cursor_pos = len;
    }

    if( this->cursor_pos < 0 )
    {
        this->cursor_pos = 0;
    }

    scroll_offset = al_ustr_offset( this->text, this->scroll_pos );
    cursor_offset = al_ustr_offset( this->text, this->cursor_pos );
    text = al_ref_ustr( &info, this->text, scroll_offset, cursor_offset );

    if( al_get_ustr_width( font, text ) > this->width )
    {
        this->scroll_pos++;
    }
}

/*
Title: Edit Box

Section: Internal

Function: wz_editbox_proc

See also:
<wz_widget_proc>
*/
int WZ_EDITBOX::proc( const ALLEGRO_EVENT *event )
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
                int size = al_ustr_size( this->text );
                int scroll_offset = al_ustr_offset( this->text, this->scroll_pos );
                ALLEGRO_USTR_INFO info;
                const ALLEGRO_USTR *text = al_ref_ustr( &info, this->text, scroll_offset, size );
                int pos = this->cursor_pos - this->scroll_pos;
                int flags = 0;

                if( this->flags & WZ_STATE_DISABLED )
                    flags = WZ_STYLE_DISABLED;
                else if( this->flags & WZ_STATE_HAS_FOCUS )
                    flags = WZ_STYLE_FOCUSED;

                this->theme->draw_editbox(
                    this->local_x, this->local_y, this->width, this->height, pos, (ALLEGRO_USTR *)text, flags );
            }

            break;
        }
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        {
            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else if( event->mouse.button == 1 && this->widget_rect_test( event->mouse.x, event->mouse.y ) )
            {
                int len = al_ustr_length( this->text );
                ALLEGRO_USTR_INFO info;
                const ALLEGRO_USTR *text = al_ref_ustr( &info, this->text, this->scroll_pos, len - 1 );
                ALLEGRO_FONT *font = this->theme->get_font( 0 );
                this->ask_parent_for_focus();
                this->cursor_pos =
                    wz_get_text_pos( font, (ALLEGRO_USTR *)text, event->mouse.x - this->x ) + this->scroll_pos;
            }
            else
                ret = 0;

            break;
        }
#if( ALLEGRO_SUB_VERSION > 0 )
        case ALLEGRO_EVENT_TOUCH_BEGIN:
        {
            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }
            else if( this->widget_rect_test( event->touch.x, event->touch.y ) )
            {
                int len = al_ustr_length( this->text );
                ALLEGRO_USTR_INFO info;
                const ALLEGRO_USTR *text = al_ref_ustr( &info, this->text, this->scroll_pos, len - 1 );
                ALLEGRO_FONT *font = this->theme->get_font( 0 );
                this->ask_parent_for_focus();
                this->cursor_pos =
                    wz_get_text_pos( font, (ALLEGRO_USTR *)text, event->touch.x - this->x ) + this->scroll_pos;
            }
            else
                ret = 0;

            break;
        }
#endif
        case WZ_HANDLE_SHORTCUT:
        {
            this->ask_parent_for_focus();
            break;
        }
        case WZ_DESTROY:
        {
            if( this->own )
                al_ustr_free( this->text );

            ret = 0;
            break;
        }
        case ALLEGRO_EVENT_KEY_CHAR:
        {
            int len;

            if( this->flags & WZ_STATE_DISABLED || !( this->flags & WZ_STATE_HAS_FOCUS ) )
            {
                ret = 0;
                break;
            }
            else if( event->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL || event->keyboard.modifiers & ALLEGRO_KEYMOD_ALT )
            {
                ret = 0;
            }

            len = al_ustr_length( this->text );

            if( (int)( event->keyboard.unichar ) > 31 && (int)( event->keyboard.unichar ) != 127 )
            {
                al_ustr_insert_chr(
                    this->text, al_ustr_offset( this->text, this->cursor_pos ), event->keyboard.unichar );
                this->cursor_pos++;
            }
            else
            {
                switch( event->keyboard.keycode )
                {
                    case ALLEGRO_KEY_BACKSPACE:
                    {
                        if( len > 0 && this->cursor_pos > 0 )
                        {
                            al_ustr_remove_chr( this->text, al_ustr_offset( this->text, this->cursor_pos - 1 ) );
                            this->cursor_pos--;
                        }

                        break;
                    }
                    case ALLEGRO_KEY_DELETE:
                    {
                        if( len > 0 && this->cursor_pos < len )
                        {
                            al_ustr_remove_chr( this->text, al_ustr_offset( this->text, this->cursor_pos ) );
                        }

                        break;
                    }
                    case ALLEGRO_KEY_LEFT:
                    {
                        if( this->cursor_pos > 0 )
                        {
                            this->cursor_pos--;
                        }
                        else
                            ret = 0;

                        break;
                    }
                    case ALLEGRO_KEY_RIGHT:
                    {
                        if( this->cursor_pos < len )
                        {
                            this->cursor_pos++;
                        }
                        else
                            ret = 0;

                        break;
                    }
                    case ALLEGRO_KEY_HOME:
                    {
                        this->cursor_pos = 0;
                        break;
                    }
                    case ALLEGRO_KEY_END:
                    {
                        len = al_ustr_length( this->text );
                        this->cursor_pos = len;
                        break;
                    }
                    case ALLEGRO_KEY_ENTER:
                    {
                        this->trigger();
                        break;
                    }
                    default:
                        ret = 0;
                }
            }

            snap();
            break;
        }
        case WZ_SET_CURSOR_POS:
        {
            this->cursor_pos = event->user.data3;
            snap();
        }
        case WZ_SET_TEXT:
        {
            if( this->own )
            {
                al_ustr_assign( this->text, (ALLEGRO_USTR *)event->user.data3 );
            }
            else
                this->text = (ALLEGRO_USTR *)event->user.data3;

            snap();
            break;
        }
        case WZ_TRIGGER:
        {
            ALLEGRO_EVENT ev;
            wz_craft_event( &ev, WZ_TEXT_CHANGED, this, 0 );
            al_emit_user_event( this->source, &ev, 0 );
            break;
        }
        case ALLEGRO_EVENT_MOUSE_AXES:
        {
            if( this->flags & WZ_STATE_DISABLED )
            {
                ret = 0;
            }

            if( this->widget_rect_test( event->mouse.x, event->mouse.y ) )
            {
                this->ask_parent_for_focus();
            }

            return WZ_WIDGET::proc( event );
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

Function: wz_create_editbox

Creates an edit this.

Parameters:

own - Set to 1 if you want the widget to own the text

Inherits From:
<WZ_WIDGET>

See Also:
<WZ_EDITBOX>

<wz_create_widget>
*/
WZ_EDITBOX::WZ_EDITBOX( WZ_WIDGET *parent,
                        float x,
                        float y,
                        float width,
                        float height,
                        ALLEGRO_USTR *text,
                        int own,
                        int id )
    : WZ_WIDGET( parent, x, y, width, height, id )
{
    this->own = own;

    if( !text )
    {
        this->text = al_ustr_new( "" );
        al_ustr_assign( this->text, text );
        this->own = 1;
    }
    else
    {
        this->text = text;
    }

    this->cursor_pos = 0;
    this->scroll_pos = 0;
}
