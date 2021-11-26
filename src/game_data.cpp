#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "game_data.hpp"

#include <algorithm> // for std::swap

#include <spdlog/spdlog.h>

// xxx todo: in clue creation - check that the clue includes one non-guessed
// block xxx todo: add TOGETHER_FIRST_WITH_ONLY_ONE logic xxx todo: improve
// composite clue checking (or what-ifs up to a given level) xxx todo: check for
// difficulty counting the number of positive clue checks (even for repeated
// clues) xxx todo: pass to binary and bitwise operations

const char *clue_description[NUMBER_OF_RELATIONS] = {
    // Horizontal
    "Neighbors",
    "Middle not next to other",
    "Second to the right of first",
    "Middle has other two one on each side",
    "First and third two away, middle not in between",
    // Vertical
    "First and second on same column",
    "All three on same column",
    "Second not on same column as first",
    "First and third on same column, second not",
    "Unused",
    // Positional
    "First on given column" };

const int DEFAULT_REL_PERCENT[NUMBER_OF_RELATIONS] = { 20, 5, 2, 3, 5, 25, 1, 20, 1, 5, 1 };

int REL_PERCENT[NUMBER_OF_RELATIONS] = { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// xxx todo: fix rel_percent_max when rel_percent changes!!!
int REL_PERCENT_MAX;

// Prototypes
void get_clue( GameData *game_data, int column, int row, Clue *clue );
int filter_clues( GameData *game_data );

void create_puzzle( GameData *game_data )
{
    int permutation[8];

    game_data->guessed = 0;
    for( int i = 0; i < 8; i++ )
    {
        permutation[i] = i;
    }

    for( int row = 0; row < game_data->column_height; row++ )
    {
        shuffle( permutation, game_data->number_of_columns );
        for( int column = 0; column < game_data->number_of_columns; column++ )
        {
            game_data->puzzle[column][row] = permutation[column];
            game_data->where[row][permutation[column]] = column;
        }
    }
};

int rand_int( int n )
{ // make static
    int limit = RAND_MAX - RAND_MAX % n;
    int rnd;

    do
    {
        rnd = rand();
    } while( rnd >= limit );
    return rnd % n;
};

static int rand_sign( void )
{
    return ( rand() % 2 == 0 ) ? -1 : 1;
};

void shuffle( int p[], int n )
{
    for( int i = n - 1; i > 0; i-- )
    {
        int j = rand_int( i + 1 );
        std::swap( p[i], p[j] );
    }
};

void remove_clue( GameData *game_data, int i )
{
    // swap clue[i] with last clue, reduce clue_n
    game_data->clue_n--;
    game_data->clue[i] = game_data->clue[game_data->clue_n];
};

TileAddress check_this_clue_reveal( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];

    if( game_data->guess[tile0.column][tile0.row] < 0 )
    {
        tile = tile0;
        tile.valid = true;
        guess_tile( game_data, tile0 );
    }
    return tile;
}

TileAddress check_this_clue_one_side( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( game_data->tile[column][tile1.row][tile1.cell] )
        {
            tile = { column, tile1.row, tile1.cell };
            hide_tile_and_check( game_data, tile );
        }
        if( game_data->tile[column][tile0.row][tile0.cell] )
            break;
    }
    for( int column = game_data->number_of_columns - 1; column >= 0; column-- )
    {
        if( game_data->tile[column][tile0.row][tile0.cell] )
        {
            tile = { column, tile0.row, tile0.cell };
            hide_tile_and_check( game_data, tile );
        }
        if( game_data->tile[column][tile1.row][tile1.cell] )
            break;
    }
    return tile;
}

TileAddress check_this_clue_together_2( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( !game_data->tile[column][tile0.row][tile0.cell] || !game_data->tile[column][tile1.row][tile1.cell] )
        {
            if( game_data->tile[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
            if( game_data->tile[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_together_3( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( !game_data->tile[column][tile0.row][tile0.cell] || !game_data->tile[column][tile1.row][tile1.cell]
            || !game_data->tile[column][tile2.row][tile2.cell] )
        { // if one exists but one doesn't
            if( game_data->tile[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
            if( game_data->tile[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }
            if( game_data->tile[column][tile2.row][tile2.cell] )
            {
                tile = { column, tile2.row, tile2.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_together_not_middle( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( ( game_data->guess[column][tile0.row] == tile0.cell )
            || ( game_data->guess[column][tile2.row] == tile2.cell ) )
        {
            if( game_data->tile[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
        if( ( !game_data->tile[column][tile0.row][tile0.cell] ) || ( game_data->guess[column][tile1.row] == tile1.cell )
            || !game_data->tile[column][tile2.row][tile2.cell] )
        {
            if( game_data->tile[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
            if( game_data->tile[column][tile2.row][tile2.cell] )
            {
                tile = { column, tile2.row, tile2.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_not_together( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( ( game_data->guess[column][tile0.row] == tile0.cell ) && game_data->tile[column][tile1.row][tile1.cell] )
        {
            tile = { column, tile1.row, tile1.cell };
            hide_tile_and_check( game_data, tile );
        }
        if( ( game_data->guess[column][tile1.row] == tile1.cell ) && game_data->tile[column][tile0.row][tile0.cell] )
        {
            tile = { column, tile0.row, tile0.cell };
            hide_tile_and_check( game_data, tile );
        }
    }
    return tile;
}

TileAddress check_this_clue_next_to( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    if( !game_data->tile[1][tile0.row][tile0.cell] && game_data->tile[0][tile1.row][tile1.cell] )
    {
        tile = { 0, tile1.row, tile1.cell };
        hide_tile_and_check( game_data, tile );
    }
    if( !game_data->tile[1][tile1.row][tile1.cell] && game_data->tile[0][tile0.row][tile0.cell] )
    {
        tile = { 0, tile0.row, tile0.cell };
        hide_tile_and_check( game_data, tile );
    }
    if( !game_data->tile[game_data->number_of_columns - 2][tile0.row][tile0.cell]
        && game_data->tile[game_data->number_of_columns - 1][tile1.row][tile1.cell] )
    {
        tile = { game_data->number_of_columns - 1, tile1.row, tile1.cell };
        hide_tile_and_check( game_data, tile );
    }
    if( !game_data->tile[game_data->number_of_columns - 2][tile1.row][tile1.cell]
        && game_data->tile[game_data->number_of_columns - 1][tile0.row][tile0.cell] )
    {
        tile = { game_data->number_of_columns - 1, tile0.row, tile0.cell };
        hide_tile_and_check( game_data, tile );
    }

    for( int column = 1; column < game_data->number_of_columns - 1; column++ )
    {
        if( !game_data->tile[column - 1][tile0.row][tile0.cell] && !game_data->tile[column + 1][tile0.row][tile0.cell] )
        {
            if( game_data->tile[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
        if( !game_data->tile[column - 1][tile1.row][tile1.cell] && !game_data->tile[column + 1][tile1.row][tile1.cell] )
        {
            if( game_data->tile[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_not_next_to( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( column < game_data->number_of_columns - 1 )
        {
            if( ( game_data->guess[column][tile0.row] == tile0.cell ) && game_data->tile[column + 1][tile1.row][tile1.cell] )
            {
                tile = { column + 1, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }

            if( ( game_data->guess[column][tile1.row] == tile1.cell ) && game_data->tile[column + 1][tile0.row][tile0.cell] )
            {
                tile = { column + 1, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
        if( column > 0 )
        {
            if( ( game_data->guess[column][tile0.row] == tile0.cell ) && game_data->tile[column - 1][tile1.row][tile1.cell] )
            {
                tile = { column - 1, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }

            if( ( game_data->guess[column][tile1.row] == tile1.cell ) && game_data->tile[column - 1][tile0.row][tile0.cell] )
            {
                tile = { column - 1, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_consecutive( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    int hide_first = 0;
    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        for( int m = 0; m < 2; m++ )
        {
            if( game_data->tile[column][tile0.row][tile0.cell] )
            {
                if( ( column < game_data->number_of_columns - 2 ) && ( column < 2 ) )
                {
                    if( ( !game_data->tile[column + 1][tile1.row][tile1.cell] )
                        || ( !game_data->tile[column + 2][tile2.row][tile2.cell] ) )
                    {
                        hide_first = 1;
                    }
                }
                if( ( column >= 2 ) && ( column >= game_data->number_of_columns - 2 ) )
                {
                    if( ( !game_data->tile[column - 2][tile2.row][tile2.cell] )
                        || ( !game_data->tile[column - 1][tile1.row][tile1.cell] ) )
                    {
                        hide_first = 1;
                    }
                }

                if( ( column >= 2 ) && ( column < game_data->number_of_columns - 2 ) )
                {
                    if( ( ( !game_data->tile[column + 1][tile1.row][tile1.cell] )
                          || ( !game_data->tile[column + 2][tile2.row][tile2.cell] ) )
                        && ( ( !game_data->tile[column - 2][tile2.row][tile2.cell] )
                             || ( !game_data->tile[column - 1][tile1.row][tile1.cell] ) ) )
                    {
                        hide_first = 1;
                    }
                }
                if( hide_first )
                {
                    hide_first = 0;
                    tile = { column, tile0.row, tile0.cell };
                    hide_tile_and_check( game_data, tile );
                }
            }
            std::swap( tile0.row, tile2.row );
            std::swap( tile0.cell, tile2.cell );
        }

        if( game_data->tile[column][tile1.row][tile1.cell] )
        {
            if( ( column == 0 ) || ( column == game_data->number_of_columns - 1 ) )
            {
                hide_first = 1;
            }
            else
            {
                if( ( ( !game_data->tile[column - 1][tile0.row][tile0.cell] )
                      && !( game_data->tile[column + 1][tile0.row][tile0.cell] ) )
                    || ( ( !game_data->tile[column - 1][tile2.row][tile2.cell] )
                         && !( game_data->tile[column + 1][tile2.row][tile2.cell] ) ) )
                { // error here! incorrect check!
                    hide_first = 1;
                }
            }
            if( hide_first )
            {
                hide_first = 0;
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_not_middle( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    int hide_first = 0;
    for( int column = 0; column < game_data->number_of_columns; column++ )
    { // apply mask
        for( int m = 0; m < 2; m++ )
        {
            if( game_data->tile[column][tile0.row][tile0.cell] )
            {
                if( ( column < game_data->number_of_columns - 2 ) && ( column < 2 ) )
                {
                    if( ( game_data->guess[column + 1][tile1.row] == tile1.cell )
                        || ( !game_data->tile[column + 2][tile2.row][tile2.cell] ) )
                    {
                        hide_first = 1;
                    }
                }
                if( ( column >= 2 ) && ( column >= game_data->number_of_columns - 2 ) )
                {
                    if( ( game_data->guess[column - 1][tile1.row] == tile1.cell )
                        || ( !game_data->tile[column - 2][tile2.row][tile2.cell] ) )
                    {
                        hide_first = 1;
                    }
                }

                if( ( column >= 2 ) && ( column < game_data->number_of_columns - 2 ) )
                {
                    if( ( ( game_data->guess[column + 1][tile1.row] == tile1.cell )
                          || ( !game_data->tile[column + 2][tile2.row][tile2.cell] ) )
                        && ( ( !game_data->tile[column - 2][tile2.row][tile2.cell] )
                             || ( game_data->guess[column - 1][tile1.row] == tile1.cell ) ) )
                    {
                        hide_first = 1;
                    }
                }
                if( hide_first )
                {
                    hide_first = 0;
                    tile = { column, tile0.row, tile0.cell };
                    hide_tile_and_check( game_data, tile );
                }
            }
            std::swap( tile0.row, tile2.row );
            std::swap( tile0.cell, tile2.cell );
        }
        if( ( column >= 1 ) && ( column <= game_data->number_of_columns - 2 ) )
        {
            if( ( ( game_data->guess[column - 1][tile0.row] == tile0.cell )
                  && ( game_data->guess[column + 1][tile2.row] == tile2.cell ) )
                || ( ( game_data->guess[column - 1][tile2.row] == tile2.cell )
                     && ( game_data->guess[column + 1][tile0.row] == tile0.cell ) ) )
            {
                if( game_data->tile[column][tile1.row][tile1.cell] )
                {
                    tile = { column, tile1.row, tile1.cell };
                    hide_tile_and_check( game_data, tile );
                }
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_together_first_with_only_one( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    // xxx todo: check this
    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( !game_data->tile[column][tile1.row][tile1.cell] && !game_data->tile[column][tile2.row][tile2.cell] )
        {
            if( game_data->tile[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
        else if( game_data->guess[column][tile1.row] == tile1.cell )
        {
            if( game_data->tile[column][tile2.row][tile2.cell] )
            {
                tile = { column, tile2.row, tile2.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
        else if( game_data->guess[column][tile2.row] == tile2.cell )
        {
            if( game_data->tile[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( game_data, tile );
            }
        }
    }
    return tile;
}

TileAddress check_this_clue( GameData *game_data, Clue *clue )
{
    TileAddress tile;

    switch( clue->rel )
    {
        case REVEAL:
            tile = check_this_clue_reveal( game_data, clue );
            break;
        case ONE_SIDE:
            tile = check_this_clue_one_side( game_data, clue );
            break;

        case TOGETHER_2:
            tile = check_this_clue_together_2( game_data, clue );
            break;
        case TOGETHER_3:
            tile = check_this_clue_together_3( game_data, clue );
            break;

        case TOGETHER_NOT_MIDDLE:
            tile = check_this_clue_together_not_middle( game_data, clue );
            break;

        case NOT_TOGETHER:
            tile = check_this_clue_not_together( game_data, clue );
            break;

        case NEXT_TO:
            tile = check_this_clue_next_to( game_data, clue );
            break;

        case NOT_NEXT_TO:
            tile = check_this_clue_not_next_to( game_data, clue );
            break;

        case CONSECUTIVE:
            tile = check_this_clue_consecutive( game_data, clue );
            break;

        case NOT_MIDDLE:
            tile = check_this_clue_not_middle( game_data, clue );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            tile = check_this_clue_together_first_with_only_one( game_data, clue );
            break;
        default:
            break;
    }
    return tile;
}

int check_solution( GameData *game_data )
{
    for( int column = 0; column < game_data->number_of_columns; column++ )
        for( int row = 0; row < game_data->column_height; row++ )
            if( !game_data->tile[column][row][game_data->puzzle[column][row]] )
                return 0;

    return 1;
}

// save a backup copy of game data if type==0, restore the data if type==1
void switch_game( GameData *game_data, int type )
{
    static int tile[8][8][8];
    static int guess[8][8];
    static int guessed;

    if( type == 0 )
    {
        memcpy( &tile, &game_data->tile, sizeof( tile ) );
        memcpy( &guess, &game_data->guess, sizeof( guess ) );
        guessed = game_data->guessed;
    }

    std::swap( game_data->tile, tile );
    std::swap( game_data->guess, guess );
    std::swap( game_data->guessed, guessed );
}

// returns a hint that contains a clue and a tile that can be ruled out with this clue
Hint get_hint( GameData *game_data )
{ // still not working properly
    int clue_number = 0;
    TileAddress tile_to_rule_out;

    switch_game( game_data, 0 ); // store game_data
    for( int i = 0; i < game_data->clue_n; i++ )
    {
        tile_to_rule_out = check_this_clue( game_data, &game_data->clue[i] );
        if( tile_to_rule_out.valid )
        {
            clue_number = i;
            break;
        }
    }
    switch_game( game_data, 1 ); // restore game
    Hint hint;
    hint.valid = tile_to_rule_out.valid;
    hint.clue_number = clue_number;
    hint.tile = tile_to_rule_out;
    return hint;
}
int advanced_check_clues( GameData *game_data )
{
    int info;

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        for( int row = 0; row < game_data->column_height; row++ )
        {
            for( int cell = 0; cell < game_data->number_of_columns; cell++ )
            {
                if( game_data->tile[column][row][cell] )
                {
                    switch_game( game_data, 0 ); // save state
                    guess_tile( game_data, { column, row, cell } );
                    do
                    { // repeat until no more information remains in clues
                        info = 0;
                        for( int m = 0; m < game_data->clue_n; m++ )
                        {
                            if( check_this_clue( game_data, &game_data->clue[m] ).valid )
                            {
                                info = 1;
                            }
                        }
                    } while( info );
                    if( !check_panel_consistency( game_data ) )
                    {
                        switch_game( game_data, 1 ); // restore
                        hide_tile_and_check( game_data, { column, row, cell } );
                        return 1;
                    }
                    else
                    {
                        switch_game( game_data, 1 ); // restore state
                    }
                }
            }
        }
    }

    return 0;
}

int check_clues( GameData *game_data )
{
    // check whether the clues add new info (within reason -- this can be tuned)
    // for now it does not combine clues (analyze each one separately)
    // if so, discover the info in game_data->tile
    // return 1 if new info was found, 0 if not
    int info;

    int ret = 0;
    do
    { // repeat until no more information remains in clues
        info = 0;
        for( int m = 0; m < game_data->clue_n; m++ )
        {
            if( check_this_clue( game_data, &game_data->clue[m] ).valid )
            {
                ret = 1;
                info = 1;
            }
        }
        if( !info && game_data->advanced )
        { // check "what if" depth 1
            while( advanced_check_clues( game_data ) )
            {
                info = 1;
            }
        }
    } while( info );

    return ret;
}

void create_game_with_clues( GameData *game_data )
{
    init_game( game_data );
    create_puzzle( game_data );

    game_data->clue_n = 0;
    for( int i = 0; i < 100; i++ )
    { // xxx todo add a check to see if we have found
        // solution or not after 100
        game_data->clue_n++;
        do
        {
            get_clue( game_data,
                      rand_int( game_data->number_of_columns ),
                      rand_int( game_data->column_height ),
                      &game_data->clue[game_data->clue_n - 1] );
        } while( !check_this_clue( game_data, &game_data->clue[game_data->clue_n - 1] ).valid ); // should be while
                                                                                                 // !check_clues?
        check_clues( game_data );
        if( game_data->guessed == game_data->number_of_columns * game_data->column_height )
            break;
    }

    if( !check_solution( game_data ) ) // debug
        SPDLOG_ERROR( "ERROR: SOLUTION DOESN'T MATCH CLUES" );

    filter_clues( game_data );
    SPDLOG_INFO(
        "{}x{} game created with {} clues", game_data->number_of_columns, game_data->column_height, game_data->clue_n );

    // clean guesses and tiles
    init_game( game_data );

    // reveal reveal clues and remove them from clue list
    for( int i = 0; i < game_data->clue_n; i++ )
    {
        auto &clue = game_data->clue[i];
        if( clue.rel == REVEAL )
        {
            guess_tile( game_data, clue.tile[0] );
            remove_clue( game_data, i );
            i--;
        }
    }

    // mark clues unhidden
    for( int i = 0; i < game_data->clue_n; i++ )
    {
        game_data->clue[i].hidden = 0;
    }
}

// checks if clue is compatible with current panel (not necessarily with
// solution)
int is_clue_compatible( GameData *game_data, Clue *clue )
{
    int column, row, ret = 0;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    ret = 0;

    switch( clue->rel )
    {
        case REVEAL:
            if( game_data->tile[clue->tile[0].column][tile0.row][tile0.cell] )
                ret = 1;
            break;
        case ONE_SIDE:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                if( game_data->tile[column][tile1.row][tile1.cell] && ( ret == -1 ) )
                {
                    ret = 1;
                    break; // loop
                }

                if( game_data->tile[column][tile0.row][tile0.cell] )
                {
                    ret = -1;
                }
            }
            if( ret != 1 )
                ret = 0;
            break; // switch

        case TOGETHER_2:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                if( game_data->tile[column][tile0.row][tile0.cell] && game_data->tile[column][tile1.row][tile1.cell] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case TOGETHER_3:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                if( game_data->tile[column][tile0.row][tile0.cell] && game_data->tile[column][tile1.row][tile1.cell]
                    && game_data->tile[column][tile2.row][tile2.cell] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case TOGETHER_NOT_MIDDLE:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                if( ( game_data->tile[column][tile0.row][tile0.cell] ) && ( game_data->guess[column][tile1.row] != tile1.cell )
                    && game_data->tile[column][tile2.row][tile2.cell] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_TOGETHER:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                if( ( game_data->guess[column][tile0.row] != tile0.cell ) && game_data->tile[column][tile1.row][tile1.cell] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NEXT_TO:
            for( column = 0; column < game_data->number_of_columns - 1; column++ )
            {
                if( ( game_data->tile[column][tile0.row][tile0.cell] && game_data->tile[column + 1][tile1.row][tile1.cell] )
                    || ( game_data->tile[column][tile1.row][tile1.cell] && game_data->tile[column + 1][tile0.row][tile0.cell] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_NEXT_TO:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                for( row = 0; row < game_data->number_of_columns; row++ )
                {
                    if( game_data->tile[column][tile0.row][tile0.cell] && game_data->tile[row][tile1.row][tile1.cell] )
                    {
                        if( ( column - row != 1 ) && ( row - column ) != 1 )
                        {
                            ret = 1;
                            break;
                        }
                    }
                }
                if( ret )
                    break;
            }
            break;

        case CONSECUTIVE:
            for( column = 0; column < game_data->number_of_columns - 2; column++ )
            {
                if( ( game_data->tile[column][tile0.row][tile0.cell] && game_data->tile[column + 1][tile1.row][tile1.cell]
                      && game_data->tile[column + 2][tile2.row][tile2.cell] )
                    || ( game_data->tile[column][tile2.row][tile2.cell] && game_data->tile[column + 1][tile1.row][tile1.cell]
                         && game_data->tile[column + 2][tile0.row][tile0.cell] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_MIDDLE:
            for( column = 0; column < game_data->number_of_columns - 2; column++ )
            {
                if( ( game_data->tile[column][tile0.row][tile0.cell] && ( game_data->guess[column + 1][tile1.row] != tile1.cell )
                      && game_data->tile[column + 2][tile2.row][tile2.cell] )
                    || ( game_data->tile[column][tile2.row][tile2.cell] && ( game_data->guess[column + 1][tile1.row] != tile1.cell )
                         && game_data->tile[column + 2][tile0.row][tile0.cell] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            // xxx todo: check this:
            for( column = 0; column < game_data->number_of_columns; column++ )
            {
                if( game_data->tile[column][tile0.row][tile0.cell]
                    && ( game_data->tile[column][tile1.row][tile1.cell] || game_data->tile[column][tile2.row][tile2.cell] )
                    && !( ( game_data->guess[column][tile1.row] == tile1.cell )
                          && ( game_data->guess[column][tile2.row] == tile2.cell ) ) )
                    ret = 1;
            }

            break;
        default:
            break;
    }
    return ret;
}

int check_panel_consistency( GameData *game_data )
{
    for( int m = 0; m < game_data->clue_n; m++ )
    {
        if( !is_clue_compatible( game_data, &game_data->clue[m] ) )
            return 0;
    }
    return 1;
}

int check_panel_correctness( GameData *game_data )
{
    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        for( int row = 0; row < game_data->column_height; row++ )
        {
            if( !game_data->tile[column][row][game_data->puzzle[column][row]] )
            {
                return 0;
            }
        }
    }
    return 1;
}

int check_clues_for_solution( GameData *game_data )
{
    int info;

    init_game( game_data );

    do
    { // repeat until no more information remains in clues
        info = 0;
        for( int m = 0; m < game_data->clue_n; m++ )
        {
            if( check_this_clue( game_data, &game_data->clue[m] ).valid )
            {
                info = 1;
            }
        }
        if( !info && game_data->advanced )
        { // check "what if" depth 1
            while( advanced_check_clues( game_data ) )
            {
                info = 1;
            }
        }
    } while( info );

    if( game_data->guessed == game_data->number_of_columns * game_data->column_height )
        return 1;
    else
        return 0;
}

int filter_clues( GameData *game_data )
{
    int ret = 0;

    for( int m = 0; m < game_data->clue_n; m++ )
    { // test reduction
        std::swap( game_data->clue[game_data->clue_n - 1], game_data->clue[m] );
        init_game( game_data );
        game_data->clue_n--;
        if( check_clues_for_solution( game_data ) )
        {
            ret = 1;
        }
        else
        {
            game_data->clue_n++;
        }
    }

    // join clues if possible
    // xxx todo: check this
    for( int i = game_data->clue_n - 1; i > 0; i-- )
    {
        auto &clue = game_data->clue[i];
        if( clue.rel == TOGETHER_2 || clue.rel == NOT_TOGETHER )
        {
            for( int j = i - 1; j >= 0; j-- )
            {
                auto &clue2 = game_data->clue[j];
                if( ( clue2.rel == TOGETHER_2 || clue2.rel == NOT_TOGETHER ) && clue.rel == TOGETHER_2 )
                {
                    if( ( ( clue2.tile[0].row == clue.tile[0].row ) && ( clue2.tile[0].cell == clue.tile[0].cell ) )
                        || ( ( clue2.tile[1].row == clue.tile[0].row )
                             && ( clue2.tile[1].cell == clue.tile[0].cell ) ) )
                    {
                        clue2.tile[2].row = clue.tile[1].row;
                        clue2.tile[2].cell = clue.tile[1].cell;
                        clue2.rel = clue2.rel == TOGETHER_2 ? TOGETHER_3 : TOGETHER_NOT_MIDDLE;
                        remove_clue( game_data, i );
                        break;
                    }
                    else if( clue2.rel == TOGETHER_2 )
                    {
                        if( ( ( clue2.tile[0].row == clue.tile[1].row ) && ( clue2.tile[0].cell == clue.tile[1].cell ) )
                            || ( ( clue2.tile[1].row == clue.tile[1].row )
                                 && ( clue2.tile[1].cell == clue.tile[1].cell ) ) )
                        {
                            clue2.tile[2].row = clue.tile[0].row;
                            clue2.tile[2].cell = clue.tile[0].cell;
                            clue2.rel = TOGETHER_3;
                            remove_clue( game_data, i );
                            break;
                        }
                    }
                }
                else if( clue2.rel == TOGETHER_2 && clue.rel == NOT_TOGETHER )
                {
                    if( ( clue2.tile[0].row == clue.tile[0].row ) && ( clue2.tile[0].cell == clue.tile[0].cell ) )
                    {
                        clue.tile[2].row = clue2.tile[1].row;
                        clue.tile[2].cell = clue2.tile[1].cell;
                        clue.rel = TOGETHER_NOT_MIDDLE;
                        clue2 = clue;
                        remove_clue( game_data, i );
                        break;
                    }
                    else if( ( clue2.tile[1].row == clue.tile[0].row ) && ( clue2.tile[1].cell == clue.tile[0].cell ) )
                    {
                        clue.tile[2].row = clue2.tile[0].row;
                        clue.tile[2].cell = clue2.tile[0].cell;
                        clue.rel = TOGETHER_NOT_MIDDLE;
                        clue2 = clue;
                        remove_clue( game_data, i );
                        break;
                    }
                }
            }
        }
    }

    // sort clues
    for( int i = 0; i < game_data->clue_n; i++ )
    {
        auto &clue = game_data->clue[i];
        switch( clue.rel )
        {
            case TOGETHER_2:
                if( clue.tile[0].row > clue.tile[1].row )
                {
                    std::swap( clue.tile[0], clue.tile[1] );
                }
                break;
            case TOGETHER_3:
                if( clue.tile[0].row > clue.tile[2].row )
                {
                    std::swap( clue.tile[0], clue.tile[2] );
                }
                if( clue.tile[0].row > clue.tile[1].row )
                {
                    std::swap( clue.tile[0], clue.tile[1] );
                }
                if( clue.tile[1].row > clue.tile[2].row )
                {
                    std::swap( clue.tile[1], clue.tile[2] );
                }

                break;
            case TOGETHER_NOT_MIDDLE:
                if( clue.tile[0].row > clue.tile[2].row )
                {
                    std::swap( clue.tile[0], clue.tile[2] );
                }
                break;
            default:
                break;
        }
    }

    return ret;
}

int get_random_tile( GameData *game_data, int column, int *row, int *cell )
{ // random item in column
    int row2, cell2 = 0;

    int m = 0;
    for( row2 = 0; row2 < game_data->column_height; row2++ )
    {
        for( cell2 = 0; cell2 < game_data->number_of_columns; cell2++ )
        {
            if( game_data->guess[column][row2] < 0 )
            {
                if( game_data->tile[column][row2][cell2] )
                {
                    m++;
                }
            }
        }
    }

    if( m == 0 )
        return 0;
    m = rand_int( m );
    for( row2 = 0; row2 < game_data->column_height; row2++ )
    {
        for( cell2 = 0; cell2 < game_data->number_of_columns; cell2++ )
        {
            if( game_data->tile[column][row2][cell2] && ( game_data->guess[column][row2] < 0 ) )
            {
                m--;
                if( m < 0 )
                    break;
            }
        }
        if( m < 0 )
            break;
    }
    *row = row2;
    *cell = cell2;
    return 1;
};

int random_relation( void )
{
    int rel = -1;
    int m = rand_int( REL_PERCENT_MAX );
    int s = 0;
    for( int i = 0; i < NUMBER_OF_RELATIONS; i++ )
    {
        s += REL_PERCENT[i];
        if( m < s )
        {
            rel = i;
            break;
        }
    }
    if( rel < 0 )
        return random_relation();
    return rel;
};

// get a new solved item at column
void get_random_item_col( GameData *game_data, int column, int *row, int *cell )
{
    *row = rand_int( game_data->column_height );
    *cell = game_data->puzzle[column][*row];
};

// get a new solved item not in rows ej1 or ej2
void get_random_item_col_except( GameData *game_data, int column, int *row, int *cell, int ej1, int ej2 )
{
    int m = ( ej1 == ej2 ) ? 1 : 2;
    *row = rand_int( game_data->column_height - m );

    for( m = 0; m < 2; m++ ) // skip ej1 and ej2
        if( ( *row == ej1 ) || ( *row == ej2 ) )
            *row = ( *row + 1 ) % game_data->column_height;
    *cell = game_data->puzzle[column][*row];
};

void get_clue_consecutive( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int s = rand_int( 3 );
    // check that column-s, column-s+1, column-s+2 are all in range
    if( column - s + 2 > game_data->number_of_columns - 1 )
        s += ( column - s + 2 ) - ( game_data->number_of_columns - 1 );
    else if( column - s < 0 )
        s = column;

    for( int m = 0; m < 3; m++ )
    {
        int column2 = column - s + m;
        int row2;
        int cell2;
        if( column2 != column )
        {
            get_random_item_col( game_data,
                                 column2,
                                 &row2,
                                 &cell2 ); // get a new solved item at column column2 (solve if necessary)
        }
        else
        {
            row2 = row;
            cell2 = cell;
        }
        auto &tile = clue->tile[m];
        tile.column = column2;
        tile.row = row2;
        tile.cell = cell2;
    }
    if( rand_int( 2 ) )
    { // random swap of outer elements
        std::swap( clue->tile[0], clue->tile[2] );
    }
}

void get_clue_one_side( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int s = rand_int( game_data->number_of_columns - 1 );
    int column2 = ( column + s + 1 ) % game_data->number_of_columns;
    int row2;
    int cell2;
    get_random_item_col( game_data, column2, &row2, &cell2 );
    if( column2 < column )
    {
        std::swap( column, column2 );
        std::swap( row, row2 );
        std::swap( cell, cell2 );
    }
    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column2;
    clue->tile[1].row = row2;
    clue->tile[1].cell = cell2;

    clue->tile[2].column = column2;
    clue->tile[2].row = row2;
    clue->tile[2].cell = cell2; // filler
}

void get_clue_next_to( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int column2 = column + rand_sign();
    if( column2 >= game_data->number_of_columns )
        column2 = column - 1;
    else if( column2 < 0 )
        column2 = column + 1;
    int row2;
    int cell2;
    get_random_item_col( game_data, column2, &row2, &cell2 );
    if( rand_int( 2 ) )
    {
        std::swap( column, column2 );
        std::swap( row, row2 );
        std::swap( cell, cell2 );
    }
    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column2;
    clue->tile[1].row = row2;
    clue->tile[1].cell = cell2;

    clue->tile[2].column = column;
    clue->tile[2].row = row;
    clue->tile[2].cell = cell;
}

void get_clue_not_next_to( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int s;
    if( column >= game_data->number_of_columns - 1 )
        s = -1;
    else if( column <= 0 )
        s = +1;
    else
        s = rand_sign();
    int column2 = column + s;
    int row2;
    int cell2;
    get_random_item_col( game_data, column2, &row2, &cell2 );

    cell2 = ( cell2 + 1 ) % game_data->number_of_columns; // get an item that is NOT the neighbor one
    // avoid same item
    if( ( cell2 == game_data->puzzle[column][row] ) && ( row == row2 ) )
    {
        cell2 = ( cell2 + 1 ) % game_data->number_of_columns;
    }
    if( ( column - s >= 0 ) && ( column - s < game_data->number_of_columns ) )
    {
        if( game_data->puzzle[column - s][row2] == cell2 ) // avoid the neighbor from the other side
        {
            cell2 = ( cell2 + 1 ) % game_data->number_of_columns;
        }
    }

    if( rand_int( 2 ) )
    {
        std::swap( column, column2 );
        std::swap( row, row2 );
        std::swap( cell, cell2 );
    }
    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column2;
    clue->tile[1].row = row2;
    clue->tile[1].cell = cell2;

    clue->tile[2].column = column;
    clue->tile[2].row = row;
    clue->tile[2].cell = cell;
}

void get_clue_not_middle( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int s;
    if( column > game_data->number_of_columns - 3 )
        s = -1;
    else if( column < 2 )
        s = 1;
    else
        s = rand_sign();

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    int column2 = column + s;
    int row2;
    int cell2;
    get_random_item_col( game_data, column2, &row2, &cell2 );
    clue->tile[1].column = column2;
    clue->tile[1].row = row2;
    clue->tile[1].cell = ( cell2 + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;

    column2 = column + 2 * s;
    get_random_item_col( game_data, column2, &row2, &cell2 );
    clue->tile[2].column = column2;
    clue->tile[2].row = row2;
    clue->tile[2].cell = cell2;

    if( rand_int( 2 ) )
    { // random swap of outer elements
        std::swap( clue->tile[0], clue->tile[2] );
    }
}

void get_clue_together_2( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( game_data, column, &row2, &cell2, row, row ); // except row row (and row)

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column;
    clue->tile[1].row = row2;
    clue->tile[1].cell = cell2;

    clue->tile[2].column = column;
    clue->tile[2].row = row2;
    clue->tile[2].cell = cell2; // filler
}

void get_clue_together_3( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( game_data, column, &row2, &cell2, row, row ); // except row row (and row)
    int row3;
    int cell3;
    get_random_item_col_except( game_data, column, &row3, &cell3, row, row2 ); // except row row and row2

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column;
    clue->tile[1].row = row2;
    clue->tile[1].cell = cell2;

    clue->tile[2].column = column;
    clue->tile[2].row = row3;
    clue->tile[2].cell = cell3;
}

void get_clue_not_together( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( game_data, column, &row2, &cell2, row, row ); // except row row (and row)

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column;
    clue->tile[1].row = row2;
    clue->tile[1].cell = ( cell2 + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;

    clue->tile[2].column = column;
    clue->tile[2].row = row;
    clue->tile[2].cell = cell; // filler
}

void get_clue_together_not_middle( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( game_data, column, &row2, &cell2, row, row ); // except row row (and row)
    int row3;
    int cell3;
    get_random_item_col_except( game_data, column, &row3, &cell3, row, row2 ); // except row row and row2

    auto &tile0 = clue->tile[0];
    tile0.column = column;
    tile0.row = row;
    tile0.cell = cell;

    auto &tile1 = clue->tile[1];
    tile1.column = column;
    tile1.row = row2;
    tile1.cell = ( cell2 + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;

    auto &tile2 = clue->tile[2];
    tile2.column = column;
    tile2.row = row3;
    tile2.cell = cell3;
}

void get_clue_reveal( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    int column2 = rand_int( game_data->number_of_columns );
    int row2;
    int cell2;
    get_random_item_col( game_data, column2, &row2, &cell2 );
    for( int m = 0; m < 3; m++ )
    {
        auto &tile = clue->tile[m];
        tile.column = column2;
        tile.row = row2;
        tile.cell = cell2;
    }
}

void get_clue_together_first_with_only_one( GameData *game_data, int column, int row, int cell, Clue *clue )
{
    // xxx todo: check this
    int row2;
    int cell2;
    get_random_item_col_except( game_data, column, &row2, &cell2, row, row ); // except row row (and row)
    int row3;
    int cell3;
    get_random_item_col_except( game_data, column, &row3, &cell3, row, row2 ); // except row row and row2

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    // same as together_not_middle but sorted (so we don't know which is middle)
    if( row2 < row3 )
    {
        clue->tile[1].column = column;
        clue->tile[1].row = row2;
        clue->tile[1].cell =
            ( cell2 + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;

        clue->tile[2].column = column;
        clue->tile[2].row = row3;
        clue->tile[2].cell = cell3;
    }
    else
    {
        clue->tile[1].column = column;
        clue->tile[1].row = row3;
        clue->tile[1].cell = cell3;

        clue->tile[2].column = column;
        clue->tile[2].row = row2;
        clue->tile[2].cell =
            ( cell2 + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;
    }
}

void get_clue( GameData *game_data, int column, int row, Clue *clue )
{
    int rel = -1;

    if( rel < 0 )
    {
        do
        { // to avoid problem when game_data->number_of_columns is too small in
            // the NOT_MIDDLE case
            rel = random_relation();
        } while( ( rel == NOT_MIDDLE ) && ( column < 2 ) && ( column > game_data->number_of_columns - 3 ) );
    }
    clue->rel = (RELATION)rel;

    int cell = game_data->puzzle[column][row];

    switch( rel )
    {
        case CONSECUTIVE:
            get_clue_consecutive( game_data, column, row, cell, clue );
            break;
        case ONE_SIDE:
            get_clue_one_side( game_data, column, row, cell, clue );
            break;
        case NEXT_TO:
            get_clue_next_to( game_data, column, row, cell, clue );
            break;
        case NOT_NEXT_TO:
            get_clue_not_next_to( game_data, column, row, cell, clue );
            break;
        case NOT_MIDDLE:
            get_clue_not_middle( game_data, column, row, cell, clue );
            break;
        case TOGETHER_2:
            get_clue_together_2( game_data, column, row, cell, clue );
            break;
        case TOGETHER_3:
            get_clue_together_3( game_data, column, row, cell, clue );
            break;
        case NOT_TOGETHER:
            get_clue_not_together( game_data, column, row, cell, clue );
            break;
        case TOGETHER_NOT_MIDDLE:
            get_clue_together_not_middle( game_data, column, row, cell, clue );
            break;
        case REVEAL:
            get_clue_reveal( game_data, column, row, cell, clue );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            get_clue_together_first_with_only_one( game_data, column, row, cell, clue );
            break;
    }
};

void reset_rel_params( void )
{
    int i;

    for( i = 0; i < NUMBER_OF_RELATIONS; i++ )
        REL_PERCENT[i] = DEFAULT_REL_PERCENT[i];
}

void init_game( GameData *game_data )
{
    // if REL_PERCENT is not set, use defaults
    if( REL_PERCENT[NEXT_TO] == -1 )
        reset_rel_params();

    // initialize REL_PERCENT_MAX (total sum) for random relation creation
    REL_PERCENT_MAX = 0;
    for( int i = 0; i < NUMBER_OF_RELATIONS; i++ )
    {
        REL_PERCENT_MAX += REL_PERCENT[i];
    }

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        for( int row = 0; row < game_data->column_height; row++ )
        {
            game_data->guess[column][row] = -1;
            game_data->tile_col[row][column] = -1;
            for( int cell = 0; cell < game_data->number_of_columns; cell++ )
            {
                game_data->tile[column][row][cell] = 1;
            }
        }
    }
    game_data->guessed = 0;
}

// return -1 if there is more than one tile left in block
// last tile number otherwise
int last_tile_in_block( GameData *game_data, int column, int row )
{
    int m = -1, count = 0;

    if( game_data->guess[column][row] >= 0 )
        return -1;

    for( int cell = 0; cell < game_data->number_of_columns; cell++ )
    { // find if there is only 1 tile left
        if( game_data->tile[column][row][cell] )
        {
            m = cell;
            count++;
        }
        if( count > 1 )
            return -1;
    }
    return m;
};

// return -1 if there is more than one block with given tile
// last block number (column) otherwise
int last_tile_in_row( GameData *game_data, int row, int cell )
{
    int m = -1, count = 0;

    for( int column = 0; column < game_data->number_of_columns; column++ )
    { // find if there is only 1 tile left
        if( game_data->guess[column][row] == cell )
            return -1;
        if( game_data->tile[column][row][cell] )
        {
            m = column;
            count++;
        }
        if( count > 1 )
            return -1;
    }
    return m;
};

// check any obviously guessable clues in row
int check_row( GameData *game_data, int row )
{
    int m;

    for( int column = 0; column < game_data->number_of_columns; column++ )
    { // find if there is only 1 tile left
        m = last_tile_in_block( game_data, column, row );
        if( m >= 0 )
        {
            guess_tile( game_data, { column, row, m } );
            return 1;
        }
    }

    for( int cell = 0; cell < game_data->number_of_columns; cell++ )
    {
        m = last_tile_in_row( game_data, row, cell ); // check if there is only 1 left of this tile
        if( m >= 0 )
        {
            guess_tile( game_data, { m, row, cell } );
            return 1;
        }
    }
    return 0;
};

void hide_tile_and_check( GameData *game_data, TileAddress tile )
{
    game_data->tile[tile.column][tile.row][tile.cell] = 0;
    check_row( game_data, tile.row );
};

void guess_tile( GameData *game_data, TileAddress tile )
{
    game_data->guess[tile.column][tile.row] = tile.cell;
    game_data->guessed++;
    for( int m = 0; m < game_data->number_of_columns; m++ )
        if( m != tile.cell )
            game_data->tile[tile.column][tile.row][m] = 0; // hide all tiles from this block

    for( int m = 0; m < game_data->number_of_columns; m++ )
    {
        if( m != tile.column )
            game_data->tile[m][tile.row][tile.cell] = 0; // hide this tile in all blocks
    }

    check_row( game_data, tile.row );
};

int is_guessed( GameData *game_data, int row, int cell )
{
    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        if( game_data->guess[column][row] == cell )
            return 1;
    }

    return 0;
};

void unguess_tile( GameData *game_data, int column, int row )
{
    int cell = game_data->guess[column][row];
    game_data->guess[column][row] = -1;
    game_data->guessed--;

    for( int m = 0; m < game_data->number_of_columns; m++ )
    {
        if( !is_guessed( game_data, row, m ) )
            game_data->tile[column][row][m] = 1;
        if( game_data->guess[m][row] < 0 )
            game_data->tile[m][row][cell] = 1;
    }
};

int is_vclue( RELATION rel )
{
    return ( ( rel == TOGETHER_2 ) || ( rel == TOGETHER_3 ) || ( rel == NOT_TOGETHER ) || ( rel == TOGETHER_NOT_MIDDLE )
             || ( rel == TOGETHER_FIRST_WITH_ONLY_ONE ) );
}

// only for debug puprposes. Check if clue is compatible with solution
int is_clue_valid( GameData *game_data, Clue *clue )
{
    int ret = 0;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];
    
    int column0 = game_data->where[tile0.row][tile0.cell];
    int column1 = game_data->where[tile1.row][tile1.cell];
    int column2 = game_data->where[tile2.row][tile2.cell];

    ret = 1;

    switch( clue->rel )
    {
        case ONE_SIDE:
            if( game_data->where[tile0.row][tile0.cell] >= game_data->where[tile1.row][tile1.cell] )
                ret = 0;
            break;
        case TOGETHER_2:
            if( game_data->where[tile0.row][tile0.cell] != game_data->where[tile1.row][tile1.cell] )
                ret = 0;
            break;
        case TOGETHER_3:
            if( ( game_data->where[tile0.row][tile0.cell] != game_data->where[tile1.row][tile1.cell] )
                || ( game_data->where[tile0.row][tile0.cell] != game_data->where[tile2.row][tile2.cell] ) )
                ret = 0;
            break;
        case TOGETHER_NOT_MIDDLE:
            if( ( game_data->where[tile0.row][tile0.cell] == game_data->where[tile1.row][tile1.cell] )
                || ( game_data->where[tile0.row][tile0.cell] != game_data->where[tile2.row][tile2.cell] ) )
                ret = 0;
            break;
        case NOT_TOGETHER:
            if( ( game_data->where[tile0.row][tile0.cell] == game_data->where[tile1.row][tile1.cell] )
                || ( game_data->where[tile1.row][tile1.cell] == game_data->where[tile2.row][tile2.cell] ) )
                ret = 0;
            break;
        case NEXT_TO:
            if( ( game_data->where[tile0.row][tile0.cell] - game_data->where[tile1.row][tile1.cell] != 1 )
                && ( game_data->where[tile0.row][tile0.cell] - game_data->where[tile1.row][tile1.cell] != -1 ) )
                ret = 0;
            if( ( game_data->where[tile2.row][tile2.cell] - game_data->where[tile1.row][tile1.cell] != 1 )
                && ( game_data->where[tile2.row][tile2.cell] - game_data->where[tile1.row][tile1.cell] != -1 ) )
                ret = 0;
            break;

        case NOT_NEXT_TO:
            if( ( game_data->where[tile0.row][tile0.cell] - game_data->where[tile1.row][tile1.cell] == 1 )
                || ( game_data->where[tile0.row][tile0.cell] - game_data->where[tile1.row][tile1.cell] == -1 ) )
                ret = 0;
            if( ( game_data->where[tile2.row][tile2.cell] - game_data->where[tile1.row][tile1.cell] == 1 )
                || ( game_data->where[tile2.row][tile2.cell] - game_data->where[tile1.row][tile1.cell] == -1 ) )
                ret = 0;
            break;

        case CONSECUTIVE:
            if( !( ( column1 == column0 + 1 ) && ( column2 == column0 + 2 ) )
                && !( ( column1 == column2 + 1 ) && ( column0 == column2 + 2 ) ) )
                ret = 0;
            break;

        case NOT_MIDDLE:
            if( column0 - column2 == 2 )
            {
                if( column0 - column1 == 1 )
                    ret = 0;
            }
            else if( column2 - column0 == 2 )
            {
                if( column1 - column0 == 1 )
                    ret = 0;
            }
            else
                ret = 0;
            break;

        case TOGETHER_FIRST_WITH_ONLY_ONE:
            if( ( game_data->where[tile0.row][tile0.cell] == game_data->where[tile1.row][tile1.cell] )
                == ( game_data->where[tile0.row][tile0.cell] == game_data->where[tile2.row][tile2.cell] ) )
                ret = 0;
            break;
        default:
            break;
    }
    return ret;
};
