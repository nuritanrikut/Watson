#pragma once

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include "widgetz_nine_patch.hpp"

/*
Title: Misc
*/

/*
Struct: WZ_THEME

A struct that defines a bunch of rendering functions to render various types of widgets
*/
struct WZ_THEME
{
    virtual void draw_button( float x, float y, float width, float height, ALLEGRO_USTR *text, int style ) = 0;
    virtual void draw_box( float x, float y, float width, float height, int style ) = 0;
    virtual void draw_textbox( float x,
                               float y,
                               float width,
                               float height,
                               int halign,
                               int valign,
                               ALLEGRO_USTR *text,
                               int style ) = 0;
    virtual void
    draw_scroll( float x, float y, float width, float height, float fraction, float slider_size, int style ) = 0;
    virtual void
    draw_editbox( float x, float y, float width, float height, int cursor_pos, ALLEGRO_USTR *text, int style ) = 0;
    virtual void draw_image( float x, float y, float width, float height, ALLEGRO_BITMAP *image ) = 0;
    virtual ALLEGRO_FONT *get_font( int font_num ) = 0;
};

/*
Struct: WZ_DEF_THEME

The default theme struct


Inherits From:
<WZ_THEME>
*/
struct WZ_DEF_THEME : WZ_THEME
{
    WZ_DEF_THEME();
    WZ_DEF_THEME( ALLEGRO_COLOR color1, ALLEGRO_COLOR color2, ALLEGRO_FONT *font );
    virtual ~WZ_DEF_THEME();

    virtual void draw_button( float x, float y, float width, float height, ALLEGRO_USTR *text, int style ) override;
    virtual void draw_box( float x, float y, float width, float height, int style ) override;
    virtual void
    draw_textbox( float x, float y, float width, float height, int halign, int valign, ALLEGRO_USTR *text, int style )
        override;
    virtual void
    draw_scroll( float x, float y, float width, float height, float fraction, float slider_size, int style ) override;
    virtual void
    draw_editbox( float x, float y, float width, float height, int cursor_pos, ALLEGRO_USTR *text, int style ) override;
    virtual void draw_image( float x, float y, float width, float height, ALLEGRO_BITMAP *image ) override;
    virtual ALLEGRO_FONT *get_font( int font_num ) override;

    void draw_3d_rectangle( float x1, float y1, float x2, float y2, float border, ALLEGRO_COLOR col, bool invert );
    int find_eol( ALLEGRO_USTR *text, ALLEGRO_FONT *font, float max_width, int start, int *end );
    void draw_single_text( float x,
                           float y,
                           float width,
                           float height,
                           int halign,
                           int valign,
                           ALLEGRO_COLOR color,
                           ALLEGRO_FONT *font,
                           ALLEGRO_USTR *text );
    void draw_multi_text( float x,
                          float y,
                          float width,
                          float height,
                          int halign,
                          int valign,
                          ALLEGRO_COLOR color,
                          ALLEGRO_FONT *font,
                          ALLEGRO_USTR *text );

    /*
	Variable: color1
	The background color
	*/
    ALLEGRO_COLOR color1;
    /*
	Variable: color2
	The foreground color
	*/
    ALLEGRO_COLOR color2;

    /*
	Variable: font
	Font to use for this GUI
	*/
    ALLEGRO_FONT *font;
    int def_pad[3];
};

/*
Struct: WZ_SKIN_THEME

A simple skinnable theme struct


Inherits From:
<WZ_DEF_THEME>

See Also:
<wz_init_skin_theme>, <wz_destroy_skin_theme>
*/
struct WZ_SKIN_THEME : WZ_DEF_THEME
{
    WZ_SKIN_THEME();
    WZ_SKIN_THEME( WZ_DEF_THEME *other );
    WZ_SKIN_THEME( WZ_SKIN_THEME *other );
    ~WZ_SKIN_THEME();

    void init();

    void draw_button( float x, float y, float width, float height, ALLEGRO_USTR *text, int style ) override;
    void draw_box( float x, float y, float width, float height, int style ) override;
    void
    draw_textbox( float x, float y, float width, float height, int halign, int valign, ALLEGRO_USTR *text, int style )
        override;
    void
    draw_scroll( float x, float y, float width, float height, float fraction, float slider_size, int style ) override;
    void
    draw_editbox( float x, float y, float width, float height, int cursor_pos, ALLEGRO_USTR *text, int style ) override;
    void draw_image( float x, float y, float width, float height, ALLEGRO_BITMAP *image ) override;
    ALLEGRO_FONT *get_font( int font_num ) override;

    WZ_NINE_PATCH_PADDING
    draw_tinted_patch( WZ_NINE_PATCH_BITMAP *p9, ALLEGRO_COLOR tint, float x, float y, float width, float height );

    WZ_NINE_PATCH_BITMAP *button_up_patch;
    WZ_NINE_PATCH_BITMAP *button_down_patch;
    WZ_NINE_PATCH_BITMAP *box_patch;
    WZ_NINE_PATCH_BITMAP *editbox_patch;
    WZ_NINE_PATCH_BITMAP *scroll_track_patch;
    WZ_NINE_PATCH_BITMAP *slider_patch;

    /*
	Variable: button_up_bitmap
	Bitmap to use to draw buttons when they are unpressed
	*/
    ALLEGRO_BITMAP *button_up_bitmap;

    /*
	Variable: button_up_bitmap
	Bitmap to use to draw buttons when they are pressed
	*/
    ALLEGRO_BITMAP *button_down_bitmap;

    /*
	Variable: box_bitmap
	Bitmap to use to draw boxes
	*/
    ALLEGRO_BITMAP *box_bitmap;

    /*
	Variable: editbox_bitmap
	Bitmap to use to draw edit boxes
	*/
    ALLEGRO_BITMAP *editbox_bitmap;

    /*
	Variable: scroll_track_bitmap
	Bitmap to use to draw scrollbar tracks
	*/
    ALLEGRO_BITMAP *scroll_track_bitmap;

    /*
	Variable: slider_bitmap
	Bitmap to use to draw scrollbar sliders
	*/
    ALLEGRO_BITMAP *slider_bitmap;

    int skin_pad;
};

WZ_DEF_THEME *get_def_theme();
WZ_SKIN_THEME *get_skin_theme();

struct WZ_SHORTCUT
{
    int modifiers;
    int keycode;
};

/*
Struct: WZ_WIDGET

A base widget struct.

See Also:
<wz_create_widget>
*/
struct WZ_WIDGET
{
    WZ_WIDGET( WZ_WIDGET *parent, float x, float y, float width, float height, int id );

    void attach( WZ_WIDGET *parent );
    void attach_after( WZ_WIDGET *sib );
    void attach_before( WZ_WIDGET *sib );
    void detach();

    void set_shortcut( int keycode, int modifiers );
    void register_sources( ALLEGRO_EVENT_QUEUE *queue );

    int send_event( const ALLEGRO_EVENT *event );
    int broadcast_event( const ALLEGRO_EVENT *event );
    void update( double dt );

    void trigger();
    void draw();
    void set_text( ALLEGRO_USTR *text );

    void show( int show );
    void destroy();

    void resize( float factor );
    void enable( int enable );
    void focus( int focus );
    void set_scroll_pos( int pos, int max );
    void set_cursor_pos( int pos );

    int widget_rect_test( float x, float y );
    int widget_rect_test_all( float x, float y );

    int ask_parent_for_focus();
    void ask_parent_to_focus_next();
    void ask_parent_to_focus_prev();
    WZ_WIDGET *get_widget_dir( int dir );

    void set_theme( WZ_THEME *theme );

    virtual int proc( const ALLEGRO_EVENT *event );

    /*
	Variable: x
	x coordinate of the widget
	*/
    float x;
    /*
	Variable: y
	y coordinate of the widget
	*/
    float y;

    float local_x;
    float local_y;

    /*
	Variable: width
	Width of the widget
	*/
    float width;
    /*
	Variable: height
	Height of the widget
	*/
    float height;

    WZ_WIDGET *prev_sib;
    WZ_WIDGET *next_sib;
    WZ_WIDGET *parent;

    WZ_WIDGET *last_child;
    WZ_WIDGET *first_child;

    WZ_THEME *theme;
    ALLEGRO_EVENT_SOURCE *source;

    WZ_SHORTCUT shortcut;

    int hold_focus;

    /*
	Variable: flags
	Flags of the widget. A combination of some sort of <Widget States>
	*/
    int flags;
    /*
	Variable: id
	id of the widget
	*/
    int id;
};

struct WZ_BOX : WZ_WIDGET
{
    WZ_BOX( WZ_WIDGET *parent, float x, float y, float width, float height, int id );

    virtual int proc( const ALLEGRO_EVENT *event );
};

/*
Struct: WZ_BUTTON

A button. Sends <WZ_BUTTON_PRESSED> when pressed.

Inherits From:
<WZ_WIDGET>

See Also:
<wz_create_button>
*/
struct WZ_BUTTON : WZ_BOX
{
    WZ_BUTTON( WZ_WIDGET *parent, float x, float y, float width, float height, ALLEGRO_USTR *text, int own, int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    ALLEGRO_USTR *text;
    int own;
    int down;
};

/*
Struct: WZ_IMAGE_BUTTON

An image button. Draws itself using four images, one for each state. The text is
not drawn.

Inherits From:
<WZ_BUTTON>

See Also:
<wz_create_image_button>
*/
struct WZ_IMAGE_BUTTON : WZ_BUTTON
{
    WZ_IMAGE_BUTTON( WZ_WIDGET *parent,
                     float x,
                     float y,
                     float width,
                     float height,
                     ALLEGRO_BITMAP *normal,
                     ALLEGRO_BITMAP *down,
                     ALLEGRO_BITMAP *focused,
                     ALLEGRO_BITMAP *disabled,
                     int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    ALLEGRO_BITMAP *normal;
    ALLEGRO_BITMAP *down;
    ALLEGRO_BITMAP *focused;
    ALLEGRO_BITMAP *disabled;
};

/*
Struct: WZ_FILL_LAYOUT

A meta-widget that controls the layout of its sibling widgets. The algorithm is simple, it takes all widgets
that go after it (stopping as soon as it hits another layout widget), and arranges them in order, in a grid,
respecting the horizontal and vertical spacings. It wraps the widgets to try to fit in this layout widget's
width.

Inherits From:
<WZ_WIDGET>

See Also:
<wz_create_fill_layout>
*/
struct WZ_FILL_LAYOUT : WZ_BOX
{
    WZ_FILL_LAYOUT( WZ_WIDGET *parent,
                    float x,
                    float y,
                    float width,
                    float height,
                    float hspace,
                    float vspace,
                    int halign,
                    int valign,
                    int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    float h_spacing;
    float v_spacing;
    int h_align;
    int v_align;
};

/*
Struct: WZ_TEXTBOX

A texbox that will wrap the text inside if needed.

Inherits From:
<WZ_WIDGET>

See Also:
<wz_create_textbox>
*/
struct WZ_TEXTBOX : WZ_WIDGET
{
    WZ_TEXTBOX( WZ_WIDGET *parent,
                float x,
                float y,
                float width,
                float height,
                int halign,
                int valign,
                ALLEGRO_USTR *text,
                int own,
                int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    ALLEGRO_USTR *text;
    int own;
    int h_align;
    int v_align;
};

/*
Struct: WZ_SCROLL

A scroll bar. If it's higher than it is wide, it is a vertical scroll bar. Otherwise it is a horizontal scroll bar.
Sends <WZ_SCROLL_POS_CHANGED> when it's slider is moved.

Inherits From:
<WZ_WIDGET>

See Also:
<wz_create_scroll>
*/
struct WZ_SCROLL : WZ_WIDGET
{
    WZ_SCROLL( WZ_WIDGET *parent, float x, float y, float width, float height, int max_pos, int slider_size, int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    int set_scroll_pos( float x, float y );

    int max_pos;
    int cur_pos;
    int down;
    int slider_size;
};

/*
Struct: WZ_TOGGLE

A toggle button. Creates a toggle button. A toggle button stays pressed when you press it, requiring you to click a second time
to depress it. If group does not equal to -1, then all toggle button siblings sharing the same group number will
force only one button in this group to be pressed. Sends the same events as <WZ_BUTTON>.

Inherits From:
<WZ_BUTTON>

See Also:
<wz_create_toggle_button>
*/
struct WZ_TOGGLE : WZ_BUTTON
{
    WZ_TOGGLE( WZ_WIDGET *parent,
               float x,
               float y,
               float width,
               float height,
               ALLEGRO_USTR *text,
               int own,
               int group,
               int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    int group;
};

/*
Struct: WZ_EDITBOX

An single line edit box. You can type text in it, until the buffer inside of it is filled.
Sends the <WZ_TEXT_CHANGED> event.

Inherits From:
<WZ_WIDGET>

See Also:
<wz_create_editbox>
*/
struct WZ_EDITBOX : WZ_WIDGET
{
    WZ_EDITBOX( WZ_WIDGET *parent, float x, float y, float width, float height, ALLEGRO_USTR *text, int own, int id );

    virtual int proc( const ALLEGRO_EVENT *event );

    void snap();

    /*
	Variable: text
	Holds the current text that is entered inside the textbox.
	*/
    ALLEGRO_USTR *text;
    int own;
    int cursor_pos;
    int scroll_pos;
};

/*
Enum: Events

For events, data1 is the issuer widget id and data2 points to the
actual widget, data3 varies per event

WZ_DRAW - Draw the widget
WZ_DESTROY - Destroy widget
WZ_UPDATE - Update widget. data3 is the time delta
WZ_UPDATE_POSITION - Tell widget to update its local position
WZ_HIDE - Hide the widget
WZ_SHOW - Show the widget
WZ_DISABLE - Disable the widget
WZ_ENABLE - Enabled the widget
WZ_TRIGGER - Trigger the widget
WZ_BUTTON_PRESSED - Indicates that a button was pressed
WZ_TEXT_CHANGED - Indicates that text was changed
WZ_SET_TEXT - Tells the widget to set its text. data3 is the new text.
WZ_WANT_FOCUS - Tells the widget that a widget wants focus
WZ_TAKE_FOCUS - Tells the widget that it has obtained focus
WZ_LOSE_FOCUS - Tells the widget that it has lost focus
WZ_HANDLE_SHORTCUT - Called by self when own shortcut is pressed
WZ_SET_SCROLL_POS - Tells the widget to move its scroll position somewhere. data3 is the new scroll position.
WZ_SET_SCROLL_MAX_POS - Tells the widget to alter its max position. data3 is the new max scroll position.
WZ_SCROLL_POS_CHANGED - Indicates that the scroll position changed. data3 is the new scroll position.
WZ_TOGGLE_GROUP - Tells something to a toggle group. data3 is the group number
WZ_SET_CURSOR_POS - Sets the cursor position. data3 is the new cursor position
WZ_EVENT_COUNT - A dummy event, useful if you need to add more events without conflicts
*/
enum
{
    WZ_DRAW = 9000,
    WZ_DESTROY,
    WZ_UPDATE, //3rd pos is time delta
    WZ_UPDATE_POSITION,
    WZ_HIDE,
    WZ_SHOW,
    WZ_DISABLE,
    WZ_ENABLE,
    WZ_TRIGGER,
    WZ_BUTTON_PRESSED,
    WZ_TEXT_CHANGED,
    WZ_SET_TEXT, //3rd pos is new text
    WZ_WANT_FOCUS,
    WZ_TAKE_FOCUS,
    WZ_LOSE_FOCUS,
    WZ_HANDLE_SHORTCUT,    //called by self when own shortcut is pressed
    WZ_SET_SCROLL_POS,     //3rd pos is the new position for the scroll
    WZ_SET_SCROLL_MAX_POS, //3rd pos is the new maximum position for the scroll
    WZ_SCROLL_POS_CHANGED, //3rd pos is the new scroll position
    WZ_TOGGLE_GROUP,       //3rd pos is the group number
    WZ_SET_CURSOR_POS,     //sets the new cursor position
    WZ_RESIZE,             // data1 should be resize factor (float)
    WZ_EVENT_COUNT
};

/*
Enum: Alignments

WZ_ALIGN_CENTRE - Centre alignment
WZ_ALIGN_LEFT - Left alignment
WZ_ALIGN_RIGHT - Right alignment
WZ_ALIGN_TOP - Top alignment
WZ_ALIGN_BOTTOM - Buttom alignment
*/
enum
{
    WZ_ALIGN_CENTRE = 0,
    WZ_ALIGN_LEFT,
    WZ_ALIGN_RIGHT,
    WZ_ALIGN_TOP,
    WZ_ALIGN_BOTTOM
};

extern const WZ_DEF_THEME wz_def_theme;
extern const WZ_SKIN_THEME wz_skin_theme;

ALLEGRO_COLOR wz_blend_colors( ALLEGRO_COLOR c1, ALLEGRO_COLOR c2, float frac );
ALLEGRO_COLOR wz_scale_color( ALLEGRO_COLOR c, float scale );
