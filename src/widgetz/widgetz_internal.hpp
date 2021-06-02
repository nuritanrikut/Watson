#pragma once

#include "widgetz.hpp"

void wz_craft_event( ALLEGRO_EVENT *event, int type, WZ_WIDGET *source, intptr_t data );

ALLEGRO_COLOR wz_blend_colors( ALLEGRO_COLOR c1, ALLEGRO_COLOR c2, float frac );
int wz_get_text_pos( ALLEGRO_FONT *font, ALLEGRO_USTR *text, float x );
