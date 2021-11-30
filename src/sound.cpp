#include "sound.hpp"

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include <spdlog/spdlog.h>

ALLEGRO_SAMPLE *sound_sample[NUMBER_OF_SOUNDS];

const char *sound_sample_filename[] = { "sounds/click-hide.wav",
                                        "sounds/click-guess.wav",
                                        "sounds/win.wav",
                                        "sounds/wrong.wav",
                                        "sounds/click-unhide.wav",
                                        "sounds/click-sound.wav",
                                        "sounds/stone.wav" };

auto init_sound() -> int
{
    int i;
    int err = 0;

    al_init_acodec_addon();

    if( !al_install_audio() )
    {
        SPDLOG_ERROR( "failed to initialize the audio!" );
        return -1;
    }

    SPDLOG_DEBUG( "initialized audio addon" );
    for( i = 0; i < NUMBER_OF_SOUNDS; i++ )
    {
        if( !( sound_sample[i] = al_load_sample( sound_sample_filename[i] ) ) )
        {
            SPDLOG_ERROR( "Error loading sample %s\n", sound_sample_filename[i] );
            err = 1;
        }
    }

    SPDLOG_DEBUG( "loaded samples" );

    al_reserve_samples( RESERVED_SAMPLES );
    if( err )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

void play_sound( SOUND s )
{
    if( sound_sample[s] )
    {
        al_play_sample( sound_sample[s], 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, nullptr );
    }
}

void destroy_sound()
{
    for( int i = 0; i < NUMBER_OF_SOUNDS; i++ )
    {
        if( sound_sample[i] )
        {
            al_destroy_sample( sound_sample[i] );
        }
    }
}
