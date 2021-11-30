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
//void get_clue( int column, int row, Clue *clue );
//int filter_clues( GameData *game_data );

auto is_vclue( RELATION rel ) -> int
{
    return ( ( rel == TOGETHER_2 ) || ( rel == TOGETHER_3 ) || ( rel == NOT_TOGETHER ) || ( rel == TOGETHER_NOT_MIDDLE )
             || ( rel == TOGETHER_FIRST_WITH_ONLY_ONE ) );
}

void reset_rel_params()
{
    for( int i = 0; i < NUMBER_OF_RELATIONS; i++ )
    {
        REL_PERCENT[i] = DEFAULT_REL_PERCENT[i];
    }
}

void GameData::create_puzzle()
{
    int permutation[8];

    guessed = 0;
    for( int i = 0; i < 8; i++ )
    {
        permutation[i] = i;
    }

    for( int row = 0; row < column_height; row++ )
    {
        shuffle( permutation, number_of_columns );
        for( int column = 0; column < number_of_columns; column++ )
        {
            puzzle[column][row] = permutation[column];
            where[row][permutation[column]] = column;
        }
    }
}

auto rand_int( int n ) -> int
{ // make static
    int limit = RAND_MAX - RAND_MAX % n;
    int rnd;

    do
    {
        rnd = rand();
    } while( rnd >= limit );
    return rnd % n;
}

static auto rand_sign() -> int
{
    return ( rand() % 2 == 0 ) ? -1 : 1;
}

void shuffle( int p[], int n )
{
    for( int i = n - 1; i > 0; i-- )
    {
        int j = rand_int( i + 1 );
        std::swap( p[i], p[j] );
    }
}

void GameData::remove_clue( int i )
{
    // swap clue[i] with last clue, reduce clue_n
    clue_n--;
    clues[i] = clues[clue_n];
}

auto GameData::check_this_clue_reveal( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];

    if( this->guess[tile0.column][tile0.row] < 0 )
    {
        tile = tile0;
        tile.valid = true;
        guess_tile( tile0 );
    }
    return tile;
}

auto GameData::check_this_clue_one_side( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( tiles[column][tile1.row][tile1.cell] )
        {
            tile = { column, tile1.row, tile1.cell };
            hide_tile_and_check( tile );
        }
        if( tiles[column][tile0.row][tile0.cell] )
        {
            break;
        }
    }
    for( int column = number_of_columns - 1; column >= 0; column-- )
    {
        if( tiles[column][tile0.row][tile0.cell] )
        {
            tile = { column, tile0.row, tile0.cell };
            hide_tile_and_check( tile );
        }
        if( tiles[column][tile1.row][tile1.cell] )
        {
            break;
        }
    }
    return tile;
}

auto GameData::check_this_clue_together_2( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( !tiles[column][tile0.row][tile0.cell] || !tiles[column][tile1.row][tile1.cell] )
        {
            if( tiles[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
            if( tiles[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_together_3( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( !tiles[column][tile0.row][tile0.cell] || !tiles[column][tile1.row][tile1.cell]
            || !tiles[column][tile2.row][tile2.cell] )
        { // if one exists but one doesn't
            if( tiles[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
            if( tiles[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }
            if( tiles[column][tile2.row][tile2.cell] )
            {
                tile = { column, tile2.row, tile2.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_together_not_middle( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( ( this->guess[column][tile0.row] == tile0.cell ) || ( this->guess[column][tile2.row] == tile2.cell ) )
        {
            if( tiles[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }
        }
        if( ( !tiles[column][tile0.row][tile0.cell] ) || ( this->guess[column][tile1.row] == tile1.cell )
            || !tiles[column][tile2.row][tile2.cell] )
        {
            if( tiles[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
            if( tiles[column][tile2.row][tile2.cell] )
            {
                tile = { column, tile2.row, tile2.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_not_together( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( ( this->guess[column][tile0.row] == tile0.cell ) && tiles[column][tile1.row][tile1.cell] )
        {
            tile = { column, tile1.row, tile1.cell };
            hide_tile_and_check( tile );
        }
        if( ( this->guess[column][tile1.row] == tile1.cell ) && tiles[column][tile0.row][tile0.cell] )
        {
            tile = { column, tile0.row, tile0.cell };
            hide_tile_and_check( tile );
        }
    }
    return tile;
}

auto GameData::check_this_clue_next_to( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    if( !tiles[1][tile0.row][tile0.cell] && tiles[0][tile1.row][tile1.cell] )
    {
        tile = { 0, tile1.row, tile1.cell };
        hide_tile_and_check( tile );
    }
    if( !tiles[1][tile1.row][tile1.cell] && tiles[0][tile0.row][tile0.cell] )
    {
        tile = { 0, tile0.row, tile0.cell };
        hide_tile_and_check( tile );
    }
    if( !tiles[number_of_columns - 2][tile0.row][tile0.cell] && tiles[number_of_columns - 1][tile1.row][tile1.cell] )
    {
        tile = { number_of_columns - 1, tile1.row, tile1.cell };
        hide_tile_and_check( tile );
    }
    if( !tiles[number_of_columns - 2][tile1.row][tile1.cell] && tiles[number_of_columns - 1][tile0.row][tile0.cell] )
    {
        tile = { number_of_columns - 1, tile0.row, tile0.cell };
        hide_tile_and_check( tile );
    }

    for( int column = 1; column < number_of_columns - 1; column++ )
    {
        if( !tiles[column - 1][tile0.row][tile0.cell] && !tiles[column + 1][tile0.row][tile0.cell] )
        {
            if( tiles[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }
        }
        if( !tiles[column - 1][tile1.row][tile1.cell] && !tiles[column + 1][tile1.row][tile1.cell] )
        {
            if( tiles[column][tile0.row][tile0.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_not_next_to( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( column < number_of_columns - 1 )
        {
            if( ( this->guess[column][tile0.row] == tile0.cell ) && tiles[column + 1][tile1.row][tile1.cell] )
            {
                tile = { column + 1, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }

            if( ( this->guess[column][tile1.row] == tile1.cell ) && tiles[column + 1][tile0.row][tile0.cell] )
            {
                tile = { column + 1, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
        }
        if( column > 0 )
        {
            if( ( this->guess[column][tile0.row] == tile0.cell ) && tiles[column - 1][tile1.row][tile1.cell] )
            {
                tile = { column - 1, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }

            if( ( this->guess[column][tile1.row] == tile1.cell ) && tiles[column - 1][tile0.row][tile0.cell] )
            {
                tile = { column - 1, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_consecutive( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    int hide_first = 0;
    for( int column = 0; column < number_of_columns; column++ )
    {
        for( int m = 0; m < 2; m++ )
        {
            if( tiles[column][tile0.row][tile0.cell] )
            {
                if( ( column < number_of_columns - 2 ) && ( column < 2 ) )
                {
                    if( ( !tiles[column + 1][tile1.row][tile1.cell] ) || ( !tiles[column + 2][tile2.row][tile2.cell] ) )
                    {
                        hide_first = 1;
                    }
                }
                if( ( column >= 2 ) && ( column >= number_of_columns - 2 ) )
                {
                    if( ( !tiles[column - 2][tile2.row][tile2.cell] ) || ( !tiles[column - 1][tile1.row][tile1.cell] ) )
                    {
                        hide_first = 1;
                    }
                }

                if( ( column >= 2 ) && ( column < number_of_columns - 2 ) )
                {
                    if( ( ( !tiles[column + 1][tile1.row][tile1.cell] )
                          || ( !tiles[column + 2][tile2.row][tile2.cell] ) )
                        && ( ( !tiles[column - 2][tile2.row][tile2.cell] )
                             || ( !tiles[column - 1][tile1.row][tile1.cell] ) ) )
                    {
                        hide_first = 1;
                    }
                }
                if( hide_first )
                {
                    hide_first = 0;
                    tile = { column, tile0.row, tile0.cell };
                    hide_tile_and_check( tile );
                }
            }
            std::swap( tile0.row, tile2.row );
            std::swap( tile0.cell, tile2.cell );
        }

        if( tiles[column][tile1.row][tile1.cell] )
        {
            if( ( column == 0 ) || ( column == number_of_columns - 1 ) )
            {
                hide_first = 1;
            }
            else
            {
                if( ( ( !tiles[column - 1][tile0.row][tile0.cell] ) && !( tiles[column + 1][tile0.row][tile0.cell] ) )
                    || ( ( !tiles[column - 1][tile2.row][tile2.cell] )
                         && !( tiles[column + 1][tile2.row][tile2.cell] ) ) )
                { // error here! incorrect check!
                    hide_first = 1;
                }
            }
            if( hide_first )
            {
                hide_first = 0;
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_not_middle( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    int hide_first = 0;
    for( int column = 0; column < number_of_columns; column++ )
    { // apply mask
        for( int m = 0; m < 2; m++ )
        {
            if( tiles[column][tile0.row][tile0.cell] )
            {
                if( ( column < number_of_columns - 2 ) && ( column < 2 ) )
                {
                    if( ( this->guess[column + 1][tile1.row] == tile1.cell )
                        || ( !tiles[column + 2][tile2.row][tile2.cell] ) )
                    {
                        hide_first = 1;
                    }
                }
                if( ( column >= 2 ) && ( column >= number_of_columns - 2 ) )
                {
                    if( ( this->guess[column - 1][tile1.row] == tile1.cell )
                        || ( !tiles[column - 2][tile2.row][tile2.cell] ) )
                    {
                        hide_first = 1;
                    }
                }

                if( ( column >= 2 ) && ( column < number_of_columns - 2 ) )
                {
                    if( ( ( this->guess[column + 1][tile1.row] == tile1.cell )
                          || ( !tiles[column + 2][tile2.row][tile2.cell] ) )
                        && ( ( !tiles[column - 2][tile2.row][tile2.cell] )
                             || ( this->guess[column - 1][tile1.row] == tile1.cell ) ) )
                    {
                        hide_first = 1;
                    }
                }
                if( hide_first )
                {
                    hide_first = 0;
                    tile = { column, tile0.row, tile0.cell };
                    hide_tile_and_check( tile );
                }
            }
            std::swap( tile0.row, tile2.row );
            std::swap( tile0.cell, tile2.cell );
        }
        if( ( column >= 1 ) && ( column <= number_of_columns - 2 ) )
        {
            if( ( ( this->guess[column - 1][tile0.row] == tile0.cell )
                  && ( this->guess[column + 1][tile2.row] == tile2.cell ) )
                || ( ( this->guess[column - 1][tile2.row] == tile2.cell )
                     && ( this->guess[column + 1][tile0.row] == tile0.cell ) ) )
            {
                if( tiles[column][tile1.row][tile1.cell] )
                {
                    tile = { column, tile1.row, tile1.cell };
                    hide_tile_and_check( tile );
                }
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue_together_first_with_only_one( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    // xxx todo: check this
    for( int column = 0; column < number_of_columns; column++ )
    {
        if( !tiles[column][tile1.row][tile1.cell] && !tiles[column][tile2.row][tile2.cell] )
        {
            if( tiles[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile0.row, tile0.cell };
                hide_tile_and_check( tile );
            }
        }
        else if( this->guess[column][tile1.row] == tile1.cell )
        {
            if( tiles[column][tile2.row][tile2.cell] )
            {
                tile = { column, tile2.row, tile2.cell };
                hide_tile_and_check( tile );
            }
        }
        else if( this->guess[column][tile2.row] == tile2.cell )
        {
            if( tiles[column][tile1.row][tile1.cell] )
            {
                tile = { column, tile1.row, tile1.cell };
                hide_tile_and_check( tile );
            }
        }
    }
    return tile;
}

auto GameData::check_this_clue( Clue *clue ) -> TileAddress
{
    TileAddress tile;

    switch( clue->rel )
    {
        case REVEAL:
            tile = check_this_clue_reveal( clue );
            break;
        case ONE_SIDE:
            tile = check_this_clue_one_side( clue );
            break;

        case TOGETHER_2:
            tile = check_this_clue_together_2( clue );
            break;
        case TOGETHER_3:
            tile = check_this_clue_together_3( clue );
            break;

        case TOGETHER_NOT_MIDDLE:
            tile = check_this_clue_together_not_middle( clue );
            break;

        case NOT_TOGETHER:
            tile = check_this_clue_not_together( clue );
            break;

        case NEXT_TO:
            tile = check_this_clue_next_to( clue );
            break;

        case NOT_NEXT_TO:
            tile = check_this_clue_not_next_to( clue );
            break;

        case CONSECUTIVE:
            tile = check_this_clue_consecutive( clue );
            break;

        case NOT_MIDDLE:
            tile = check_this_clue_not_middle( clue );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            tile = check_this_clue_together_first_with_only_one( clue );
            break;
        default:
            break;
    }
    return tile;
}

auto GameData::check_solution() -> int
{
    for( int column = 0; column < number_of_columns; column++ )
    {
        for( int row = 0; row < column_height; row++ )
        {
            if( !tiles[column][row][puzzle[column][row]] )
            {
                return 0;
            }
        }
    }

    return 1;
}

// save a backup copy of game data if type==0, restore the data if type==1
void GameData::switch_game( int type )
{
    static int _tiles[8][8][8];
    static int _guess[8][8];
    static int _guessed;

    if( type == 0 )
    {
        memcpy( &_tiles, &tiles, sizeof( _tiles ) );
        memcpy( &_guess, &this->guess, sizeof( guess ) );
        _guessed = guessed;
    }

    std::swap( tiles, _tiles );
    std::swap( this->guess, _guess );
    std::swap( guessed, _guessed );
}

// returns a hint that contains a clue and a tile that can be ruled out with this clue
auto GameData::get_hint() -> Hint
{ // still not working properly
    int clue_number = 0;
    TileAddress tile_to_rule_out;

    switch_game( 0 ); // store game_data
    for( int i = 0; i < clue_n; i++ )
    {
        tile_to_rule_out = check_this_clue( &clues[i] );
        if( tile_to_rule_out.valid )
        {
            clue_number = i;
            break;
        }
    }
    switch_game( 1 ); // restore game
    Hint hint;
    hint.valid = tile_to_rule_out.valid;
    hint.clue_number = clue_number;
    hint.tile = tile_to_rule_out;
    return hint;
}
auto GameData::advanced_check_clues() -> int
{
    int info;

    for( int column = 0; column < number_of_columns; column++ )
    {
        for( int row = 0; row < column_height; row++ )
        {
            for( int cell = 0; cell < number_of_columns; cell++ )
            {
                if( tiles[column][row][cell] )
                {
                    switch_game( 0 ); // save state
                    guess_tile( { column, row, cell } );
                    do
                    { // repeat until no more information remains in clues
                        info = 0;
                        for( int m = 0; m < clue_n; m++ )
                        {
                            if( check_this_clue( &clues[m] ).valid )
                            {
                                info = 1;
                            }
                        }
                    } while( info );
                    if( !check_panel_consistency() )
                    {
                        switch_game( 1 ); // restore
                        hide_tile_and_check( { column, row, cell } );
                        return 1;
                    }
                    else
                    {
                        switch_game( 1 ); // restore state
                    }
                }
            }
        }
    }

    return 0;
}

auto GameData::check_clues() -> int
{
    // check whether the clues add new info (within reason -- this can be tuned)
    // for now it does not combine clues (analyze each one separately)
    // if so, discover the info in tiles
    // return 1 if new info was found, 0 if not
    int info;

    int ret = 0;
    do
    { // repeat until no more information remains in clues
        info = 0;
        for( int m = 0; m < clue_n; m++ )
        {
            if( check_this_clue( &clues[m] ).valid )
            {
                ret = 1;
                info = 1;
            }
        }
        if( !info && this->advanced )
        { // check "what if" depth 1
            while( advanced_check_clues() )
            {
                info = 1;
            }
        }
    } while( info );

    return ret;
}

void GameData::create_game_with_clues()
{
    init_game();
    create_puzzle();

    clue_n = 0;
    for( int i = 0; i < 100; i++ )
    { // xxx todo add a check to see if we have found
        // solution or not after 100
        clue_n++;
        do
        {
            get_clue( rand_int( number_of_columns ), rand_int( column_height ), &clues[clue_n - 1] );
        } while( !check_this_clue( &clues[clue_n - 1] ).valid ); // should be while
                                                                 // !check_clues?
        check_clues();
        if( guessed == number_of_columns * column_height )
        {
            break;
        }
    }

    if( !check_solution() )
    { // debug
        SPDLOG_ERROR( "ERROR: SOLUTION DOESN'T MATCH CLUES" );
    }

    filter_clues();
    SPDLOG_INFO( "{}x{} game created with {} clues", number_of_columns, column_height, clue_n );

    // clean guesses and tiles
    init_game();

    // reveal reveal clues and remove them from clue list
    for( int i = 0; i < clue_n; i++ )
    {
        auto &clue = clues[i];
        if( clue.rel == REVEAL )
        {
            guess_tile( clue.tile[0] );
            remove_clue( i );
            i--;
        }
    }

    // mark clues unhidden
    for( int i = 0; i < clue_n; i++ )
    {
        clues[i].hidden = false;
    }
}

auto GameData::is_clue_compatible_reveal( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    if( tiles[clue->tile[0].column][tile0.row][tile0.cell] )
    {
        return 1;
    }

    return 0;
}

auto GameData::is_clue_compatible_one_side( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    int ret = 0;

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( tiles[column][tile1.row][tile1.cell] && ( ret == -1 ) )
        {
            ret = 1;
            break; // loop
        }

        if( tiles[column][tile0.row][tile0.cell] )
        {
            ret = -1;
        }
    }
    if( ret != 1 )
    {
        ret = 0;
    }

    return ret;
}

auto GameData::is_clue_compatible_together_2( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( tiles[column][tile0.row][tile0.cell] && tiles[column][tile1.row][tile1.cell] )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_together_3( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( tiles[column][tile0.row][tile0.cell] && tiles[column][tile1.row][tile1.cell]
            && tiles[column][tile2.row][tile2.cell] )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_together_not_middle( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( ( tiles[column][tile0.row][tile0.cell] ) && ( this->guess[column][tile1.row] != tile1.cell )
            && tiles[column][tile2.row][tile2.cell] )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_not_together( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        if( ( this->guess[column][tile0.row] != tile0.cell ) && tiles[column][tile1.row][tile1.cell] )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_next_to( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns - 1; column++ )
    {
        if( ( tiles[column][tile0.row][tile0.cell] && tiles[column + 1][tile1.row][tile1.cell] )
            || ( tiles[column][tile1.row][tile1.cell] && tiles[column + 1][tile0.row][tile0.cell] ) )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_not_next_to( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns; column++ )
    {
        for( int row = 0; row < number_of_columns; row++ )
        {
            if( tiles[column][tile0.row][tile0.cell] && tiles[row][tile1.row][tile1.cell] )
            {
                if( ( column - row != 1 ) && ( row - column ) != 1 )
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_consecutive( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns - 2; column++ )
    {
        if( ( tiles[column][tile0.row][tile0.cell] && tiles[column + 1][tile1.row][tile1.cell]
              && tiles[column + 2][tile2.row][tile2.cell] )
            || ( tiles[column][tile2.row][tile2.cell] && tiles[column + 1][tile1.row][tile1.cell]
                 && tiles[column + 2][tile0.row][tile0.cell] ) )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_not_middle( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    for( int column = 0; column < number_of_columns - 2; column++ )
    {
        if( ( tiles[column][tile0.row][tile0.cell] && ( this->guess[column + 1][tile1.row] != tile1.cell )
              && tiles[column + 2][tile2.row][tile2.cell] )
            || ( tiles[column][tile2.row][tile2.cell] && ( this->guess[column + 1][tile1.row] != tile1.cell )
                 && tiles[column + 2][tile0.row][tile0.cell] ) )
        {
            return 1;
        }
    }

    return 0;
}

auto GameData::is_clue_compatible_together_first_with_only_one( Clue *clue ) -> int
{
    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    // xxx todo: check this:
    for( int column = 0; column < number_of_columns; column++ )
    {
        if( tiles[column][tile0.row][tile0.cell]
            && ( tiles[column][tile1.row][tile1.cell] || tiles[column][tile2.row][tile2.cell] )
            && !( ( this->guess[column][tile1.row] == tile1.cell )
                  && ( this->guess[column][tile2.row] == tile2.cell ) ) )
        {
            return 1;
        }
    }

    return 0;
}

// checks if clue is compatible with current panel (not necessarily with solution)
auto GameData::is_clue_compatible( Clue *clue ) -> int
{
    switch( clue->rel )
    {
        case REVEAL:
            return is_clue_compatible_reveal( clue );
            break;
        case ONE_SIDE:
            return is_clue_compatible_one_side( clue );
            break; // switch

        case TOGETHER_2:
            return is_clue_compatible_together_2( clue );
            break;

        case TOGETHER_3:
            return is_clue_compatible_together_3( clue );
            break;

        case TOGETHER_NOT_MIDDLE:
            return is_clue_compatible_together_not_middle( clue );
            break;

        case NOT_TOGETHER:
            return is_clue_compatible_not_together( clue );
            break;

        case NEXT_TO:
            return is_clue_compatible_next_to( clue );
            break;

        case NOT_NEXT_TO:
            return is_clue_compatible_not_next_to( clue );
            break;

        case CONSECUTIVE:
            return is_clue_compatible_consecutive( clue );
            break;

        case NOT_MIDDLE:
            return is_clue_compatible_not_middle( clue );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            return is_clue_compatible_together_first_with_only_one( clue );

            break;
        default:
            break;
    }
    return 0;
}

auto GameData::check_panel_consistency() -> int
{
    for( int m = 0; m < clue_n; m++ )
    {
        if( !is_clue_compatible( &clues[m] ) )
        {
            return 0;
        }
    }
    return 1;
}

auto GameData::check_panel_correctness() -> int
{
    for( int column = 0; column < number_of_columns; column++ )
    {
        for( int row = 0; row < column_height; row++ )
        {
            if( !tiles[column][row][puzzle[column][row]] )
            {
                return 0;
            }
        }
    }
    return 1;
}

auto GameData::check_clues_for_solution() -> int
{
    int info;

    init_game();

    do
    { // repeat until no more information remains in clues
        info = 0;
        for( int m = 0; m < clue_n; m++ )
        {
            if( check_this_clue( &clues[m] ).valid )
            {
                info = 1;
            }
        }
        if( !info && this->advanced )
        { // check "what if" depth 1
            while( advanced_check_clues() )
            {
                info = 1;
            }
        }
    } while( info );

    if( guessed == number_of_columns * column_height )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void GameData::join_clues()
{
    // xxx todo: check this
    for( int i = clue_n - 1; i > 0; i-- )
    {
        auto &clue = clues[i];
        if( clue.rel == TOGETHER_2 || clue.rel == NOT_TOGETHER )
        {
            for( int j = i - 1; j >= 0; j-- )
            {
                auto &clue2 = clues[j];
                if( ( clue2.rel == TOGETHER_2 || clue2.rel == NOT_TOGETHER ) && clue.rel == TOGETHER_2 )
                {
                    if( ( ( clue2.tile[0].row == clue.tile[0].row ) && ( clue2.tile[0].cell == clue.tile[0].cell ) )
                        || ( ( clue2.tile[1].row == clue.tile[0].row )
                             && ( clue2.tile[1].cell == clue.tile[0].cell ) ) )
                    {
                        clue2.tile[2].row = clue.tile[1].row;
                        clue2.tile[2].cell = clue.tile[1].cell;
                        clue2.rel = clue2.rel == TOGETHER_2 ? TOGETHER_3 : TOGETHER_NOT_MIDDLE;
                        remove_clue( i );
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
                            remove_clue( i );
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
                        remove_clue( i );
                        break;
                    }
                    else if( ( clue2.tile[1].row == clue.tile[0].row ) && ( clue2.tile[1].cell == clue.tile[0].cell ) )
                    {
                        clue.tile[2].row = clue2.tile[0].row;
                        clue.tile[2].cell = clue2.tile[0].cell;
                        clue.rel = TOGETHER_NOT_MIDDLE;
                        clue2 = clue;
                        remove_clue( i );
                        break;
                    }
                }
            }
        }
    }
}

void GameData::sort_clues()
{
    for( int i = 0; i < clue_n; i++ )
    {
        auto &clue = clues[i];
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
}

auto GameData::filter_clues() -> int
{
    int ret = 0;

    for( int m = 0; m < clue_n; m++ )
    { // test reduction
        std::swap( clues[clue_n - 1], clues[m] );
        init_game();
        clue_n--;
        if( check_clues_for_solution() )
        {
            ret = 1;
        }
        else
        {
            clue_n++;
        }
    }

    join_clues();
    sort_clues();

    return ret;
}

auto GameData::get_random_tile( int column, int *row, int *cell ) -> int
{ // random item in column
    int row2;
    int cell2 = 0;

    int m = 0;
    for( row2 = 0; row2 < column_height; row2++ )
    {
        for( cell2 = 0; cell2 < number_of_columns; cell2++ )
        {
            if( this->guess[column][row2] < 0 )
            {
                if( tiles[column][row2][cell2] )
                {
                    m++;
                }
            }
        }
    }

    if( m == 0 )
    {
        return 0;
    }
    m = rand_int( m );
    for( row2 = 0; row2 < column_height; row2++ )
    {
        for( cell2 = 0; cell2 < number_of_columns; cell2++ )
        {
            if( tiles[column][row2][cell2] && ( this->guess[column][row2] < 0 ) )
            {
                m--;
                if( m < 0 )
                {
                    break;
                }
            }
        }
        if( m < 0 )
        {
            break;
        }
    }
    *row = row2;
    *cell = cell2;
    return 1;
}

auto GameData::random_relation() -> int
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
    {
        return random_relation();
    }
    return rel;
}

// get a new solved item at column
void GameData::get_random_item_col( int column, int *row, int *cell )
{
    *row = rand_int( column_height );
    *cell = puzzle[column][*row];
}

// get a new solved item not in rows ej1 or ej2
void GameData::get_random_item_col_except( int column, int *row, int *cell, int ej1, int ej2 )
{
    int m = ( ej1 == ej2 ) ? 1 : 2;
    *row = rand_int( column_height - m );

    for( m = 0; m < 2; m++ )
    { // skip ej1 and ej2
        if( ( *row == ej1 ) || ( *row == ej2 ) )
        {
            *row = ( *row + 1 ) % column_height;
        }
    }
    *cell = puzzle[column][*row];
}

void GameData::get_clue_consecutive( int column, int row, int cell, Clue *clue )
{
    int s = rand_int( 3 );
    // check that column-s, column-s+1, column-s+2 are all in range
    if( column - s + 2 > number_of_columns - 1 )
    {
        s += ( column - s + 2 ) - ( number_of_columns - 1 );
    }
    else if( column - s < 0 )
    {
        s = column;
    }

    for( int m = 0; m < 3; m++ )
    {
        int column2 = column - s + m;
        int row2;
        int cell2;
        if( column2 != column )
        {
            get_random_item_col( column2,
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

void GameData::get_clue_one_side( int column, int row, int cell, Clue *clue )
{
    int s = rand_int( number_of_columns - 1 );
    int column2 = ( column + s + 1 ) % number_of_columns;
    int row2;
    int cell2;
    get_random_item_col( column2, &row2, &cell2 );
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

void GameData::get_clue_next_to( int column, int row, int cell, Clue *clue )
{
    int column2 = column + rand_sign();
    if( column2 >= number_of_columns )
    {
        column2 = column - 1;
    }
    else if( column2 < 0 )
    {
        column2 = column + 1;
    }
    int row2;
    int cell2;
    get_random_item_col( column2, &row2, &cell2 );
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

void GameData::get_clue_not_next_to( int column, int row, int cell, Clue *clue )
{
    int s;
    if( column >= number_of_columns - 1 )
    {
        s = -1;
    }
    else if( column <= 0 )
    {
        s = +1;
    }
    else
    {
        s = rand_sign();
    }
    int column2 = column + s;
    int row2;
    int cell2;
    get_random_item_col( column2, &row2, &cell2 );

    cell2 = ( cell2 + 1 ) % number_of_columns; // get an item that is NOT the neighbor one
    // avoid same item
    if( ( cell2 == puzzle[column][row] ) && ( row == row2 ) )
    {
        cell2 = ( cell2 + 1 ) % number_of_columns;
    }
    if( ( column - s >= 0 ) && ( column - s < number_of_columns ) )
    {
        if( puzzle[column - s][row2] == cell2 ) // avoid the neighbor from the other side
        {
            cell2 = ( cell2 + 1 ) % number_of_columns;
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

void GameData::get_clue_not_middle( int column, int row, int cell, Clue *clue )
{
    int s;
    if( column > number_of_columns - 3 )
    {
        s = -1;
    }
    else if( column < 2 )
    {
        s = 1;
    }
    else
    {
        s = rand_sign();
    }

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    int column2 = column + s;
    int row2;
    int cell2;
    get_random_item_col( column2, &row2, &cell2 );
    clue->tile[1].column = column2;
    clue->tile[1].row = row2;
    clue->tile[1].cell = ( cell2 + 1 + rand_int( number_of_columns - 1 ) ) % number_of_columns;

    column2 = column + 2 * s;
    get_random_item_col( column2, &row2, &cell2 );
    clue->tile[2].column = column2;
    clue->tile[2].row = row2;
    clue->tile[2].cell = cell2;

    if( rand_int( 2 ) )
    { // random swap of outer elements
        std::swap( clue->tile[0], clue->tile[2] );
    }
}

void GameData::get_clue_together_2( int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( column, &row2, &cell2, row, row ); // except row row (and row)

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

void GameData::get_clue_together_3( int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( column, &row2, &cell2, row, row ); // except row row (and row)
    int row3;
    int cell3;
    get_random_item_col_except( column, &row3, &cell3, row, row2 ); // except row row and row2

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

void GameData::get_clue_not_together( int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( column, &row2, &cell2, row, row ); // except row row (and row)

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    clue->tile[1].column = column;
    clue->tile[1].row = row2;
    clue->tile[1].cell = ( cell2 + 1 + rand_int( number_of_columns - 1 ) ) % number_of_columns;

    clue->tile[2].column = column;
    clue->tile[2].row = row;
    clue->tile[2].cell = cell; // filler
}

void GameData::get_clue_together_not_middle( int column, int row, int cell, Clue *clue )
{
    int row2;
    int cell2;
    get_random_item_col_except( column, &row2, &cell2, row, row ); // except row row (and row)
    int row3;
    int cell3;
    get_random_item_col_except( column, &row3, &cell3, row, row2 ); // except row row and row2

    auto &tile0 = clue->tile[0];
    tile0.column = column;
    tile0.row = row;
    tile0.cell = cell;

    auto &tile1 = clue->tile[1];
    tile1.column = column;
    tile1.row = row2;
    tile1.cell = ( cell2 + 1 + rand_int( number_of_columns - 1 ) ) % number_of_columns;

    auto &tile2 = clue->tile[2];
    tile2.column = column;
    tile2.row = row3;
    tile2.cell = cell3;
}

void GameData::get_clue_reveal( int column, int row, int cell, Clue *clue )
{
    int column2 = rand_int( number_of_columns );
    int row2;
    int cell2;
    get_random_item_col( column2, &row2, &cell2 );
    for( int m = 0; m < 3; m++ )
    {
        auto &tile = clue->tile[m];
        tile.column = column2;
        tile.row = row2;
        tile.cell = cell2;
    }
}

void GameData::get_clue_together_first_with_only_one( int column, int row, int cell, Clue *clue )
{
    // xxx todo: check this
    int row2;
    int cell2;
    get_random_item_col_except( column, &row2, &cell2, row, row ); // except row row (and row)
    int row3;
    int cell3;
    get_random_item_col_except( column, &row3, &cell3, row, row2 ); // except row row and row2

    clue->tile[0].column = column;
    clue->tile[0].row = row;
    clue->tile[0].cell = cell;

    // same as together_not_middle but sorted (so we don't know which is middle)
    if( row2 < row3 )
    {
        clue->tile[1].column = column;
        clue->tile[1].row = row2;
        clue->tile[1].cell = ( cell2 + 1 + rand_int( number_of_columns - 1 ) ) % number_of_columns;

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
        clue->tile[2].cell = ( cell2 + 1 + rand_int( number_of_columns - 1 ) ) % number_of_columns;
    }
}

void GameData::get_clue( int column, int row, Clue *clue )
{
    int rel = -1;

    if( rel < 0 )
    {
        do
        { // to avoid problem when number_of_columns is too small in
            // the NOT_MIDDLE case
            rel = random_relation();
        } while( ( rel == NOT_MIDDLE ) && ( column < 2 ) && ( column > number_of_columns - 3 ) );
    }
    clue->rel = (RELATION)rel;

    int cell = puzzle[column][row];

    switch( rel )
    {
        case CONSECUTIVE:
            get_clue_consecutive( column, row, cell, clue );
            break;
        case ONE_SIDE:
            get_clue_one_side( column, row, cell, clue );
            break;
        case NEXT_TO:
            get_clue_next_to( column, row, cell, clue );
            break;
        case NOT_NEXT_TO:
            get_clue_not_next_to( column, row, cell, clue );
            break;
        case NOT_MIDDLE:
            get_clue_not_middle( column, row, cell, clue );
            break;
        case TOGETHER_2:
            get_clue_together_2( column, row, cell, clue );
            break;
        case TOGETHER_3:
            get_clue_together_3( column, row, cell, clue );
            break;
        case NOT_TOGETHER:
            get_clue_not_together( column, row, cell, clue );
            break;
        case TOGETHER_NOT_MIDDLE:
            get_clue_together_not_middle( column, row, cell, clue );
            break;
        case REVEAL:
            get_clue_reveal( column, row, cell, clue );
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            get_clue_together_first_with_only_one( column, row, cell, clue );
            break;
    }
}

void GameData::init_game()
{
    // if REL_PERCENT is not set, use defaults
    if( REL_PERCENT[NEXT_TO] == -1 )
    {
        reset_rel_params();
    }

    // initialize REL_PERCENT_MAX (total sum) for random relation creation
    REL_PERCENT_MAX = 0;
    for( int i = 0; i < NUMBER_OF_RELATIONS; i++ )
    {
        REL_PERCENT_MAX += REL_PERCENT[i];
    }

    for( int column = 0; column < number_of_columns; column++ )
    {
        for( int row = 0; row < column_height; row++ )
        {
            this->guess[column][row] = -1;
            this->tile_col[row][column] = -1;
            for( int cell = 0; cell < number_of_columns; cell++ )
            {
                tiles[column][row][cell] = 1;
            }
        }
    }
    guessed = 0;
}

// return -1 if there is more than one tile left in block
// last tile number otherwise
auto GameData::last_tile_in_block( int column, int row ) -> int
{
    int m = -1;
    int count = 0;

    if( this->guess[column][row] >= 0 )
    {
        return -1;
    }

    for( int cell = 0; cell < number_of_columns; cell++ )
    { // find if there is only 1 tile left
        if( tiles[column][row][cell] )
        {
            m = cell;
            count++;
        }
        if( count > 1 )
        {
            return -1;
        }
    }
    return m;
}

// return -1 if there is more than one block with given tile
// last block number (column) otherwise
auto GameData::last_tile_in_row( int row, int cell ) -> int
{
    int m = -1;
    int count = 0;

    for( int column = 0; column < number_of_columns; column++ )
    { // find if there is only 1 tile left
        if( this->guess[column][row] == cell )
        {
            return -1;
        }
        if( tiles[column][row][cell] )
        {
            m = column;
            count++;
        }
        if( count > 1 )
        {
            return -1;
        }
    }
    return m;
}

// check any obviously guessable clues in row
auto GameData::check_row( int row ) -> int
{
    int m;

    for( int column = 0; column < number_of_columns; column++ )
    { // find if there is only 1 tile left
        m = last_tile_in_block( column, row );
        if( m >= 0 )
        {
            guess_tile( { column, row, m } );
            return 1;
        }
    }

    for( int cell = 0; cell < number_of_columns; cell++ )
    {
        m = last_tile_in_row( row, cell ); // check if there is only 1 left of this tile
        if( m >= 0 )
        {
            guess_tile( { m, row, cell } );
            return 1;
        }
    }
    return 0;
}

void GameData::hide_tile_and_check( TileAddress tile )
{
    tiles[tile.column][tile.row][tile.cell] = 0;
    check_row( tile.row );
}

void GameData::guess_tile( TileAddress tile )
{
    this->guess[tile.column][tile.row] = tile.cell;
    guessed++;
    for( int m = 0; m < number_of_columns; m++ )
    {
        if( m != tile.cell )
        {
            tiles[tile.column][tile.row][m] = 0; // hide all tiles from this block
        }
    }

    for( int m = 0; m < number_of_columns; m++ )
    {
        if( m != tile.column )
        {
            tiles[m][tile.row][tile.cell] = 0; // hide this tile in all blocks
        }
    }

    check_row( tile.row );
}

auto GameData::is_guessed( int row, int cell ) -> int
{
    for( int column = 0; column < number_of_columns; column++ )
    {
        if( this->guess[column][row] == cell )
        {
            return 1;
        }
    }

    return 0;
}

void GameData::unguess_tile( int column, int row )
{
    int cell = this->guess[column][row];
    this->guess[column][row] = -1;
    guessed--;

    for( int m = 0; m < number_of_columns; m++ )
    {
        if( !is_guessed( row, m ) )
        {
            tiles[column][row][m] = 1;
        }
        if( this->guess[m][row] < 0 )
        {
            tiles[m][row][cell] = 1;
        }
    }
}

// only for debug puprposes. Check if clue is compatible with solution
auto GameData::is_clue_valid( Clue *clue ) -> int
{
    int ret = 0;

    auto &tile0 = clue->tile[0];
    auto &tile1 = clue->tile[1];
    auto &tile2 = clue->tile[2];

    int column0 = where[tile0.row][tile0.cell];
    int column1 = where[tile1.row][tile1.cell];
    int column2 = where[tile2.row][tile2.cell];

    ret = 1;

    switch( clue->rel )
    {
        case ONE_SIDE:
            if( where[tile0.row][tile0.cell] >= where[tile1.row][tile1.cell] )
            {
                ret = 0;
            }
            break;
        case TOGETHER_2:
            if( where[tile0.row][tile0.cell] != where[tile1.row][tile1.cell] )
            {
                ret = 0;
            }
            break;
        case TOGETHER_3:
            if( ( where[tile0.row][tile0.cell] != where[tile1.row][tile1.cell] )
                || ( where[tile0.row][tile0.cell] != where[tile2.row][tile2.cell] ) )
            {
                ret = 0;
            }
            break;
        case TOGETHER_NOT_MIDDLE:
            if( ( where[tile0.row][tile0.cell] == where[tile1.row][tile1.cell] )
                || ( where[tile0.row][tile0.cell] != where[tile2.row][tile2.cell] ) )
            {
                ret = 0;
            }
            break;
        case NOT_TOGETHER:
            if( ( where[tile0.row][tile0.cell] == where[tile1.row][tile1.cell] )
                || ( where[tile1.row][tile1.cell] == where[tile2.row][tile2.cell] ) )
            {
                ret = 0;
            }
            break;
        case NEXT_TO:
            if( ( where[tile0.row][tile0.cell] - where[tile1.row][tile1.cell] != 1 )
                && ( where[tile0.row][tile0.cell] - where[tile1.row][tile1.cell] != -1 ) )
            {
                ret = 0;
            }
            if( ( where[tile2.row][tile2.cell] - where[tile1.row][tile1.cell] != 1 )
                && ( where[tile2.row][tile2.cell] - where[tile1.row][tile1.cell] != -1 ) )
            {
                ret = 0;
            }
            break;

        case NOT_NEXT_TO:
            if( ( where[tile0.row][tile0.cell] - where[tile1.row][tile1.cell] == 1 )
                || ( where[tile0.row][tile0.cell] - where[tile1.row][tile1.cell] == -1 ) )
            {
                ret = 0;
            }
            if( ( where[tile2.row][tile2.cell] - where[tile1.row][tile1.cell] == 1 )
                || ( where[tile2.row][tile2.cell] - where[tile1.row][tile1.cell] == -1 ) )
            {
                ret = 0;
            }
            break;

        case CONSECUTIVE:
            if( !( ( column1 == column0 + 1 ) && ( column2 == column0 + 2 ) )
                && !( ( column1 == column2 + 1 ) && ( column0 == column2 + 2 ) ) )
            {
                ret = 0;
            }
            break;

        case NOT_MIDDLE:
            if( column0 - column2 == 2 )
            {
                if( column0 - column1 == 1 )
                {
                    ret = 0;
                }
            }
            else if( column2 - column0 == 2 )
            {
                if( column1 - column0 == 1 )
                {
                    ret = 0;
                }
            }
            else
            {
                ret = 0;
            }
            break;

        case TOGETHER_FIRST_WITH_ONLY_ONE:
            if( ( where[tile0.row][tile0.cell] == where[tile1.row][tile1.cell] )
                == ( where[tile0.row][tile0.cell] == where[tile2.row][tile2.cell] ) )
            {
                ret = 0;
            }
            break;
        default:
            break;
    }
    return ret;
}
