#pragma once

#include <allegro5/allegro_font.h>

int yes_no_dialog( const char *text );
void draw_center_text_box( ALLEGRO_FONT *font,
                           ALLEGRO_COLOR text_color,
                           ALLEGRO_COLOR bg_color,
                           ALLEGRO_COLOR bd_color,
                           float width_factor,
                           const char *text );
