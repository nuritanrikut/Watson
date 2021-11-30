#pragma once

struct Settings
{
    Settings()
    {
        number_of_columns = 5;
        column_height = 5;
        advanced = false;
        sound_mute = false;
        type_of_tiles = 0;
        fat_fingers = false;
        restart = 0;
        saved = false;
    }

    int number_of_columns;
    int column_height;
    bool advanced;
    bool sound_mute;
    int type_of_tiles;
    bool fat_fingers; // todo: implement zoom of tiledblocks for small screens
    int restart;
    bool saved; // is there a saved game?
};
