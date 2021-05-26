#pragma once

struct Settings
{
    Settings()
    {
        number_of_columns = 6;
        column_height = 6;
        advanced = 0;
        sound_mute = 0;
        type_of_tiles = 0;
        fat_fingers = 0;
        restart = 0;
        saved = 0;
    }

    int number_of_columns;
    int column_height;
    int advanced;
    int sound_mute;
    int type_of_tiles;
    int fat_fingers; // todo: implement zoom of tiledblocks for small screens
    int restart;
    int saved; // is there a saved game?
};
