#pragma once

#include <cstdlib>

#include "macros.hpp"

enum SOUND
{
    SOUND_HIDE_TILE,
    SOUND_GUESS_TILE,
    SOUND_WIN,
    SOUND_WRONG,
    SOUND_UNHIDE_TILE,
    SOUND_CLICK,
    SOUND_STONE,
    NUMBER_OF_SOUNDS
};

/* Prototypes */
int init_sound( void );
void play_sound( SOUND s );
void destroy_sound( void );
