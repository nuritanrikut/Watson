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
void get_clue( GameData *game_data, int i, int j, int rel, Clue *clue );
int filter_clues( GameData *game_data );

void create_puzzle( GameData *game_data )
{
    int i, j;
    int permutation[8];

    game_data->guessed = 0;
    for( i = 0; i < 8; i++ )
    {
        permutation[i] = i;
    }

    for( i = 0; i < game_data->column_height; i++ )
    {
        shuffle( permutation, game_data->number_of_columns );
        for( j = 0; j < game_data->number_of_columns; j++ )
        {
            game_data->puzzle[j][i] = permutation[j];
            game_data->where[i][permutation[j]] = j;
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
    int i, j, tmp;

    for( i = n - 1; i > 0; i-- )
    {
        j = rand_int( i + 1 );
        tmp = p[j];
        p[j] = p[i];
        p[i] = tmp;
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
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    if( game_data->guess[clue->column[0]][j0] < 0 )
    {
        guess_tile( game_data, clue->column[0], j0, k0 );
        tile.valid = true;
        tile.column = clue->column[0];
        tile.row = j0;
        tile.cell = k0;
    }
    return tile;
}

TileAddress check_this_clue_one_side( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( game_data->tile[i][j1][k1] )
        {
            hide_tile_and_check( game_data, i, j1, k1 );
            tile = { i, j1, k1 };
        }
        if( game_data->tile[i][j0][k0] )
            break;
    }
    for( int i = game_data->number_of_columns - 1; i >= 0; i-- )
    {
        if( game_data->tile[i][j0][k0] )
        {
            hide_tile_and_check( game_data, i, j0, k0 );
            tile = { i, j0, k0 };
        }
        if( game_data->tile[i][j1][k1] )
            break;
    }
    return tile;
}

TileAddress check_this_clue_together_2( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( !game_data->tile[i][j0][k0] || !game_data->tile[i][j1][k1] )
        {
            if( game_data->tile[i][j0][k0] )
            {
                hide_tile_and_check( game_data, i, j0, k0 );
                tile = { i, j0, k0 };
            }
            if( game_data->tile[i][j1][k1] )
            {
                hide_tile_and_check( game_data, i, j1, k1 );
                tile = { i, j1, k1 };
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_together_3( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    int j2 = clue->row[2];
    int k2 = clue->cell[2];
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( !game_data->tile[i][j0][k0] || !game_data->tile[i][j1][k1] || !game_data->tile[i][j2][k2] )
        { // if one exists but one doesn't
            if( game_data->tile[i][j0][k0] )
            {
                hide_tile_and_check( game_data, i, j0, k0 );
                tile = { i, j0, k0 };
            }
            if( game_data->tile[i][j1][k1] )
            {
                hide_tile_and_check( game_data, i, j1, k1 );
                tile = { i, j1, k1 };
            }
            if( game_data->tile[i][j2][k2] )
            {
                hide_tile_and_check( game_data, i, j2, k2 );
                tile = { i, j2, k2 };
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_together_not_middle( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    int j2 = clue->row[2];
    int k2 = clue->cell[2];
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( ( game_data->guess[i][j0] == k0 ) || ( game_data->guess[i][j2] == k2 ) )
        {
            if( game_data->tile[i][j1][k1] )
            {
                hide_tile_and_check( game_data, i, j1, k1 );
                tile = { i, j1, k1 };
            }
        }
        if( ( !game_data->tile[i][j0][k0] ) || ( game_data->guess[i][j1] == k1 ) || !game_data->tile[i][j2][k2] )
        {
            if( game_data->tile[i][j0][k0] )
            {
                hide_tile_and_check( game_data, i, j0, k0 );
                tile = { i, j0, k0 };
            }
            if( game_data->tile[i][j2][k2] )
            {
                hide_tile_and_check( game_data, i, j2, k2 );
                tile = { i, j2, k2 };
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_not_together( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( ( game_data->guess[i][j0] == k0 ) && game_data->tile[i][j1][k1] )
        {
            hide_tile_and_check( game_data, i, j1, k1 );
            tile = { i, j1, k1 };
        }
        if( ( game_data->guess[i][j1] == k1 ) && game_data->tile[i][j0][k0] )
        {
            hide_tile_and_check( game_data, i, j0, k0 );
            tile = { i, j0, k0 };
        }
    }
    return tile;
}

TileAddress check_this_clue_next_to( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    if( !game_data->tile[1][j0][k0] && game_data->tile[0][j1][k1] )
    {
        hide_tile_and_check( game_data, 0, j1, k1 );
        tile = { 0, j1, k1 };
    }
    if( !game_data->tile[1][j1][k1] && game_data->tile[0][j0][k0] )
    {
        hide_tile_and_check( game_data, 0, j0, k0 );
        tile = { 0, j0, k0 };
    }
    if( !game_data->tile[game_data->number_of_columns - 2][j0][k0]
        && game_data->tile[game_data->number_of_columns - 1][j1][k1] )
    {
        hide_tile_and_check( game_data, game_data->number_of_columns - 1, j1, k1 );
        tile = { game_data->number_of_columns - 1, j1, k1 };
    }
    if( !game_data->tile[game_data->number_of_columns - 2][j1][k1]
        && game_data->tile[game_data->number_of_columns - 1][j0][k0] )
    {
        hide_tile_and_check( game_data, game_data->number_of_columns - 1, j0, k0 );
        tile = { game_data->number_of_columns - 1, j0, k0 };
    }

    for( int i = 1; i < game_data->number_of_columns - 1; i++ )
    {
        if( !game_data->tile[i - 1][j0][k0] && !game_data->tile[i + 1][j0][k0] )
        {
            if( game_data->tile[i][j1][k1] )
            {
                hide_tile_and_check( game_data, i, j1, k1 );
                tile = { i, j1, k1 };
            }
        }
        if( !game_data->tile[i - 1][j1][k1] && !game_data->tile[i + 1][j1][k1] )
        {
            if( game_data->tile[i][j0][k0] )
            {
                hide_tile_and_check( game_data, i, j0, k0 );
                tile = { i, j0, k0 };
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_not_next_to( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( i < game_data->number_of_columns - 1 )
        {
            if( ( game_data->guess[i][j0] == k0 ) && game_data->tile[i + 1][j1][k1] )
            {
                hide_tile_and_check( game_data, i + 1, j1, k1 );
                tile = { i + 1, j1, k1 };
            }

            if( ( game_data->guess[i][j1] == k1 ) && game_data->tile[i + 1][j0][k0] )
            {
                hide_tile_and_check( game_data, i + 1, j0, k0 );
                tile = { i + 1, j0, k0 };
            }
        }
        if( i > 0 )
        {
            if( ( game_data->guess[i][j0] == k0 ) && game_data->tile[i - 1][j1][k1] )
            {
                hide_tile_and_check( game_data, i - 1, j1, k1 );
                tile = { i - 1, j1, k1 };
            }

            if( ( game_data->guess[i][j1] == k1 ) && game_data->tile[i - 1][j0][k0] )
            {
                hide_tile_and_check( game_data, i - 1, j0, k0 );
                tile = { i - 1, j0, k0 };
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_consecutive( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    int j2 = clue->row[2];
    int k2 = clue->cell[2];
    int hide_first = 0;
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        for( int m = 0; m < 2; m++ )
        {
            if( game_data->tile[i][j0][k0] )
            {
                if( ( i < game_data->number_of_columns - 2 ) && ( i < 2 ) )
                {
                    if( ( !game_data->tile[i + 1][j1][k1] ) || ( !game_data->tile[i + 2][j2][k2] ) )
                    {
                        hide_first = 1;
                    }
                }
                if( ( i >= 2 ) && ( i >= game_data->number_of_columns - 2 ) )
                {
                    if( ( !game_data->tile[i - 2][j2][k2] ) || ( !game_data->tile[i - 1][j1][k1] ) )
                    {
                        hide_first = 1;
                    }
                }

                if( ( i >= 2 ) && ( i < game_data->number_of_columns - 2 ) )
                {
                    if( ( ( !game_data->tile[i + 1][j1][k1] ) || ( !game_data->tile[i + 2][j2][k2] ) )
                        && ( ( !game_data->tile[i - 2][j2][k2] ) || ( !game_data->tile[i - 1][j1][k1] ) ) )
                    {
                        hide_first = 1;
                    }
                }
                if( hide_first )
                {
                    hide_first = 0;
                    hide_tile_and_check( game_data, i, j0, k0 );
                    tile = { i, j0, k0 };
                }
            }
            std::swap( j0, j2 );
            std::swap( k0, k2 );
        }

        if( game_data->tile[i][j1][k1] )
        {
            if( ( i == 0 ) || ( i == game_data->number_of_columns - 1 ) )
            {
                hide_first = 1;
            }
            else
            {
                if( ( ( !game_data->tile[i - 1][j0][k0] ) && !( game_data->tile[i + 1][j0][k0] ) )
                    || ( ( !game_data->tile[i - 1][j2][k2] ) && !( game_data->tile[i + 1][j2][k2] ) ) )
                { // error here! incorrect check!
                    hide_first = 1;
                }
            }
            if( hide_first )
            {
                hide_first = 0;
                hide_tile_and_check( game_data, i, j1, k1 );
                tile = { i, j1, k1 };
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_not_middle( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    int j2 = clue->row[2];
    int k2 = clue->cell[2];
    int hide_first = 0;
    for( int i = 0; i < game_data->number_of_columns; i++ )
    { // apply mask
        for( int m = 0; m < 2; m++ )
        {
            if( game_data->tile[i][j0][k0] )
            {
                if( ( i < game_data->number_of_columns - 2 ) && ( i < 2 ) )
                {
                    if( ( game_data->guess[i + 1][j1] == k1 ) || ( !game_data->tile[i + 2][j2][k2] ) )
                    {
                        hide_first = 1;
                    }
                }
                if( ( i >= 2 ) && ( i >= game_data->number_of_columns - 2 ) )
                {
                    if( ( game_data->guess[i - 1][j1] == k1 ) || ( !game_data->tile[i - 2][j2][k2] ) )
                    {
                        hide_first = 1;
                    }
                }

                if( ( i >= 2 ) && ( i < game_data->number_of_columns - 2 ) )
                {
                    if( ( ( game_data->guess[i + 1][j1] == k1 ) || ( !game_data->tile[i + 2][j2][k2] ) )
                        && ( ( !game_data->tile[i - 2][j2][k2] ) || ( game_data->guess[i - 1][j1] == k1 ) ) )
                    {
                        hide_first = 1;
                    }
                }
                if( hide_first )
                {
                    hide_first = 0;
                    tile = { i, j0, k0 };
                    hide_tile_and_check( game_data, i, j0, k0 );
                }
            }
            std::swap( j0, j2 );
            std::swap( k0, k2 );
        }
        if( ( i >= 1 ) && ( i <= game_data->number_of_columns - 2 ) )
        {
            if( ( ( game_data->guess[i - 1][j0] == k0 ) && ( game_data->guess[i + 1][j2] == k2 ) )
                || ( ( game_data->guess[i - 1][j2] == k2 ) && ( game_data->guess[i + 1][j0] == k0 ) ) )
            {
                if( game_data->tile[i][j1][k1] )
                {
                    hide_tile_and_check( game_data, i, j1, k1 );
                    tile = { i, j1, k1 };
                }
            }
        }
    }
    return tile;
}

TileAddress check_this_clue_together_first_with_only_one( GameData *game_data, Clue *clue )
{
    TileAddress tile;
    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    int j2 = clue->row[2];
    int k2 = clue->cell[2];
    // xxx todo: check this
    for( int i = 0; i < game_data->number_of_columns; i++ )
    {
        if( !game_data->tile[i][j1][k1] && !game_data->tile[i][j2][k2] )
        {
            if( game_data->tile[i][j1][k1] )
            {
                hide_tile_and_check( game_data, i, j0, k0 );
                tile = { i, j0, k0 };
            }
        }
        else if( game_data->guess[i][j1] == k1 )
        {
            if( game_data->tile[i][j2][k2] )
            {
                hide_tile_and_check( game_data, i, j2, k2 );
                tile = { i, j2, k2 };
            }
        }
        else if( game_data->guess[i][j2] == k2 )
        {
            if( game_data->tile[i][j1][k1] )
            {
                hide_tile_and_check( game_data, i, j1, k1 );
                tile = { i, j1, k1 };
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
    int i, j;
    for( i = 0; i < game_data->number_of_columns; i++ )
        for( j = 0; j < game_data->column_height; j++ )
            if( !game_data->tile[i][j][game_data->puzzle[i][j]] )
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
    int info, m;

    for( int column = 0; column < game_data->number_of_columns; column++ )
    {
        for( int row = 0; row < game_data->column_height; row++ )
        {
            for( int cell = 0; cell < game_data->number_of_columns; cell++ )
            {
                if( game_data->tile[column][row][cell] )
                {
                    switch_game( game_data, 0 ); // save state
                    guess_tile( game_data, column, row, cell );
                    do
                    { // repeat until no more information remains in clues
                        info = 0;
                        for( m = 0; m < game_data->clue_n; m++ )
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
                        hide_tile_and_check( game_data, column, row, cell );
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
    int info, m, ret;

    ret = 0;
    do
    { // repeat until no more information remains in clues
        info = 0;
        for( m = 0; m < game_data->clue_n; m++ )
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
    int i;

    init_game( game_data );
    create_puzzle( game_data );

    game_data->clue_n = 0;
    for( i = 0; i < 100; i++ )
    { // xxx todo add a check to see if we have found
        // solution or not after 100
        game_data->clue_n++;
        do
        {
            get_clue( game_data,
                      rand_int( game_data->number_of_columns ),
                      rand_int( game_data->column_height ),
                      -1,
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
    for( i = 0; i < game_data->clue_n; i++ )
    {
        if( game_data->clue[i].rel == REVEAL )
        {
            guess_tile( game_data, game_data->clue[i].column[0], game_data->clue[i].row[0], game_data->clue[i].cell[0] );
            remove_clue( game_data, i );
            i--;
        }
    }

    // mark clues unhidden
    for( i = 0; i < game_data->clue_n; i++ )
        game_data->clue[i].hidden = 0;
}

// checks if clue is compatible with current panel (not necessarily with
// solution)
int is_clue_compatible( GameData *game_data, Clue *clue )
{
    int i, j, ret = 0;
    int j0, k0, j1, k1, j2, k2;

    j0 = clue->row[0];
    k0 = clue->cell[0];
    j1 = clue->row[1];
    k1 = clue->cell[1];
    j2 = clue->row[2];
    k2 = clue->cell[2];

    ret = 0;

    switch( clue->rel )
    {
        case REVEAL:
            if( game_data->tile[clue->column[0]][j0][k0] )
                ret = 1;
            break;
        case ONE_SIDE:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                if( game_data->tile[i][j1][k1] && ( ret == -1 ) )
                {
                    ret = 1;
                    break; // loop
                }

                if( game_data->tile[i][j0][k0] )
                {
                    ret = -1;
                }
            }
            if( ret != 1 )
                ret = 0;
            break; // switch

        case TOGETHER_2:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                if( game_data->tile[i][j0][k0] && game_data->tile[i][j1][k1] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case TOGETHER_3:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                if( game_data->tile[i][j0][k0] && game_data->tile[i][j1][k1] && game_data->tile[i][j2][k2] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case TOGETHER_NOT_MIDDLE:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                if( ( game_data->tile[i][j0][k0] ) && ( game_data->guess[i][j1] != k1 ) && game_data->tile[i][j2][k2] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_TOGETHER:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                if( ( game_data->guess[i][j0] != k0 ) && game_data->tile[i][j1][k1] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NEXT_TO:
            for( i = 0; i < game_data->number_of_columns - 1; i++ )
            {
                if( ( game_data->tile[i][j0][k0] && game_data->tile[i + 1][j1][k1] )
                    || ( game_data->tile[i][j1][k1] && game_data->tile[i + 1][j0][k0] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_NEXT_TO:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                for( j = 0; j < game_data->number_of_columns; j++ )
                {
                    if( game_data->tile[i][j0][k0] && game_data->tile[j][j1][k1] )
                    {
                        if( ( i - j != 1 ) && ( j - i ) != 1 )
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
            for( i = 0; i < game_data->number_of_columns - 2; i++ )
            {
                if( ( game_data->tile[i][j0][k0] && game_data->tile[i + 1][j1][k1] && game_data->tile[i + 2][j2][k2] )
                    || ( game_data->tile[i][j2][k2] && game_data->tile[i + 1][j1][k1]
                         && game_data->tile[i + 2][j0][k0] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_MIDDLE:
            for( i = 0; i < game_data->number_of_columns - 2; i++ )
            {
                if( ( game_data->tile[i][j0][k0] && ( game_data->guess[i + 1][j1] != k1 )
                      && game_data->tile[i + 2][j2][k2] )
                    || ( game_data->tile[i][j2][k2] && ( game_data->guess[i + 1][j1] != k1 )
                         && game_data->tile[i + 2][j0][k0] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            // xxx todo: check this:
            for( i = 0; i < game_data->number_of_columns; i++ )
            {
                if( game_data->tile[i][j0][k0] && ( game_data->tile[i][j1][k1] || game_data->tile[i][j2][k2] )
                    && !( ( game_data->guess[i][j1] == k1 ) && ( game_data->guess[i][j2] == k2 ) ) )
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
    int m;
    for( m = 0; m < game_data->clue_n; m++ )
    {
        if( !is_clue_compatible( game_data, &game_data->clue[m] ) )
            return 0;
    }
    return 1;
}

int check_panel_correctness( GameData *game_data )
{
    int i, j;
    for( i = 0; i < game_data->number_of_columns; i++ )
    {
        for( j = 0; j < game_data->column_height; j++ )
        {
            if( !game_data->tile[i][j][game_data->puzzle[i][j]] )
            {
                return 0;
            }
        }
    }
    return 1;
}

int check_clues_for_solution( GameData *game_data )
{
    int m, info;

    init_game( game_data );

    do
    { // repeat until no more information remains in clues
        info = 0;
        for( m = 0; m < game_data->clue_n; m++ )
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
    int m, i, j, ret = 0;

    for( m = 0; m < game_data->clue_n; m++ )
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
    for( i = game_data->clue_n - 1; i > 0; i-- )
    {
        if( game_data->clue[i].rel == TOGETHER_2 || game_data->clue[i].rel == NOT_TOGETHER )
        {
            for( j = i - 1; j >= 0; j-- )
            {
                if( ( game_data->clue[j].rel == TOGETHER_2 || game_data->clue[j].rel == NOT_TOGETHER )
                    && game_data->clue[i].rel == TOGETHER_2 )
                {
                    if( ( ( game_data->clue[j].row[0] == game_data->clue[i].row[0] )
                          && ( game_data->clue[j].cell[0] == game_data->clue[i].cell[0] ) )
                        || ( ( game_data->clue[j].row[1] == game_data->clue[i].row[0] )
                             && ( game_data->clue[j].cell[1] == game_data->clue[i].cell[0] ) ) )
                    {
                        game_data->clue[j].row[2] = game_data->clue[i].row[1];
                        game_data->clue[j].cell[2] = game_data->clue[i].cell[1];
                        game_data->clue[j].rel =
                            game_data->clue[j].rel == TOGETHER_2 ? TOGETHER_3 : TOGETHER_NOT_MIDDLE;
                        remove_clue( game_data, i );
                        break;
                    }
                    else if( game_data->clue[j].rel == TOGETHER_2 )
                    {
                        if( ( ( game_data->clue[j].row[0] == game_data->clue[i].row[1] )
                              && ( game_data->clue[j].cell[0] == game_data->clue[i].cell[1] ) )
                            || ( ( game_data->clue[j].row[1] == game_data->clue[i].row[1] )
                                 && ( game_data->clue[j].cell[1] == game_data->clue[i].cell[1] ) ) )
                        {
                            game_data->clue[j].row[2] = game_data->clue[i].row[0];
                            game_data->clue[j].cell[2] = game_data->clue[i].cell[0];
                            game_data->clue[j].rel = TOGETHER_3;
                            remove_clue( game_data, i );
                            break;
                        }
                    }
                }
                else if( game_data->clue[j].rel == TOGETHER_2 && game_data->clue[i].rel == NOT_TOGETHER )
                {
                    if( ( game_data->clue[j].row[0] == game_data->clue[i].row[0] )
                        && ( game_data->clue[j].cell[0] == game_data->clue[i].cell[0] ) )
                    {
                        game_data->clue[i].row[2] = game_data->clue[j].row[1];
                        game_data->clue[i].cell[2] = game_data->clue[j].cell[1];
                        game_data->clue[i].rel = TOGETHER_NOT_MIDDLE;
                        game_data->clue[j] = game_data->clue[i];
                        remove_clue( game_data, i );
                        break;
                    }
                    else if( ( game_data->clue[j].row[1] == game_data->clue[i].row[0] )
                             && ( game_data->clue[j].cell[1] == game_data->clue[i].cell[0] ) )
                    {
                        game_data->clue[i].row[2] = game_data->clue[j].row[0];
                        game_data->clue[i].cell[2] = game_data->clue[j].cell[0];
                        game_data->clue[i].rel = TOGETHER_NOT_MIDDLE;
                        game_data->clue[j] = game_data->clue[i];
                        remove_clue( game_data, i );
                        break;
                    }
                }
            }
        }
    }

    // sort clues
    for( i = 0; i < game_data->clue_n; i++ )
    {
        switch( game_data->clue[i].rel )
        {
            case TOGETHER_2:
                if( game_data->clue[i].row[0] > game_data->clue[i].row[1] )
                {
                    std::swap( game_data->clue[i].column[0], game_data->clue[i].column[1] );
                    std::swap( game_data->clue[i].row[0], game_data->clue[i].row[1] );
                    std::swap( game_data->clue[i].cell[0], game_data->clue[i].cell[1] );
                }
                break;
            case TOGETHER_3:
                if( game_data->clue[i].row[0] > game_data->clue[i].row[2] )
                {
                    std::swap( game_data->clue[i].column[0], game_data->clue[i].column[2] );
                    std::swap( game_data->clue[i].row[0], game_data->clue[i].row[2] );
                    std::swap( game_data->clue[i].cell[0], game_data->clue[i].cell[2] );
                }
                if( game_data->clue[i].row[0] > game_data->clue[i].row[1] )
                {
                    std::swap( game_data->clue[i].column[0], game_data->clue[i].column[1] );
                    std::swap( game_data->clue[i].row[0], game_data->clue[i].row[1] );
                    std::swap( game_data->clue[i].cell[0], game_data->clue[i].cell[1] );
                }
                if( game_data->clue[i].row[1] > game_data->clue[i].row[2] )
                {
                    std::swap( game_data->clue[i].column[1], game_data->clue[i].column[2] );
                    std::swap( game_data->clue[i].row[1], game_data->clue[i].row[2] );
                    std::swap( game_data->clue[i].cell[1], game_data->clue[i].cell[2] );
                }

                break;
            case TOGETHER_NOT_MIDDLE:
                if( game_data->clue[i].row[0] > game_data->clue[i].row[2] )
                {
                    std::swap( game_data->clue[i].column[0], game_data->clue[i].column[2] );
                    std::swap( game_data->clue[i].row[0], game_data->clue[i].row[2] );
                    std::swap( game_data->clue[i].cell[0], game_data->clue[i].cell[2] );
                }
                break;
            default:
                break;
        }
    }

    return ret;
}

int get_random_tile( GameData *game_data, int i, int *j, int *k )
{ // random item in column i
    int m, jj, kk = 0;

    m = 0;
    for( jj = 0; jj < game_data->column_height; jj++ )
        for( kk = 0; kk < game_data->number_of_columns; kk++ )
            if( game_data->guess[i][jj] < 0 )
                if( game_data->tile[i][jj][kk] )
                    m++;

    if( m == 0 )
        return 0;
    m = rand_int( m );
    for( jj = 0; jj < game_data->column_height; jj++ )
    {
        for( kk = 0; kk < game_data->number_of_columns; kk++ )
        {
            if( game_data->tile[i][jj][kk] && ( game_data->guess[i][jj] < 0 ) )
            {
                m--;
                if( m < 0 )
                    break;
            }
        }
        if( m < 0 )
            break;
    }
    *j = jj;
    *k = kk;
    return 1;
};

int random_relation( void )
{
    int m, s, i;
    int rel;

    rel = -1;
    m = rand_int( REL_PERCENT_MAX );
    s = 0;
    for( i = 0; i < NUMBER_OF_RELATIONS; i++ )
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

// get a new solved item at column ii
void get_random_item_col( GameData *game_data, int i, int *j, int *k )
{
    *j = rand_int( game_data->column_height );
    *k = game_data->puzzle[i][*j];
};

// get a new solved item not in rows ej1 or ej2
void get_random_item_col_except( GameData *game_data, int i, int *j, int *k, int ej1, int ej2 )
{
    int m = ( ej1 == ej2 ) ? 1 : 2;
    *j = rand_int( game_data->column_height - m );

    for( m = 0; m < 2; m++ ) // skip ej1 and ej2
        if( ( *j == ej1 ) || ( *j == ej2 ) )
            *j = ( *j + 1 ) % game_data->column_height;
    *k = game_data->puzzle[i][*j];
};

void get_clue( GameData *game_data, int i, int j, int rel, Clue *clue )
{
    int k, ii, jj, kk, jjj, kkk, m, s;

    if( rel < 0 )
    {
        do
        { // to avoid problem when game_data->number_of_columns is too small in
            // the NOT_MIDDLE case
            rel = random_relation();
        } while( ( rel == NOT_MIDDLE ) && ( i < 2 ) && ( i > game_data->number_of_columns - 3 ) );
    }
    clue->rel = (RELATION)rel;

    k = game_data->puzzle[i][j];

    switch( rel )
    {
        case CONSECUTIVE:
            s = rand_int( 3 );
            // check that i-s, i-s+1, i-s+2 are all in range
            if( i - s + 2 > game_data->number_of_columns - 1 )
                s += ( i - s + 2 ) - ( game_data->number_of_columns - 1 );
            else if( i - s < 0 )
                s = i;

            for( m = 0; m < 3; m++ )
            {
                ii = i - s + m;
                if( ii != i )
                    get_random_item_col( game_data,
                                         ii,
                                         &jj,
                                         &kk ); // get a new solved item at column ii (solve if necessary)
                else
                {
                    jj = j;
                    kk = k;
                }
                clue->column[m] = ii;
                clue->row[m] = jj;
                clue->cell[m] = kk;
            }
            if( rand_int( 2 ) )
            { // random swap of outer elements
                std::swap( clue->column[0], clue->column[2] );
                std::swap( clue->row[0], clue->row[2] );
                std::swap( clue->cell[0], clue->cell[2] );
            }
            break;
        case ONE_SIDE:
            s = rand_int( game_data->number_of_columns - 1 );
            ii = ( i + s + 1 ) % game_data->number_of_columns;
            get_random_item_col( game_data, ii, &jj, &kk );
            if( ii < i )
            {
                std::swap( i, ii );
                std::swap( j, jj );
                std::swap( k, kk );
            }
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = ii;
            clue->row[1] = jj;
            clue->cell[1] = kk;
            clue->column[2] = ii;
            clue->row[2] = jj;
            clue->cell[2] = kk; // filler
            break;
        case NEXT_TO:
            ii = i + rand_sign();
            if( ii >= game_data->number_of_columns )
                ii = i - 1;
            else if( ii < 0 )
                ii = i + 1;
            get_random_item_col( game_data, ii, &jj, &kk );
            if( rand_int( 2 ) )
            {
                std::swap( i, ii );
                std::swap( j, jj );
                std::swap( k, kk );
            }
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = ii;
            clue->row[1] = jj;
            clue->cell[1] = kk;
            clue->column[2] = i;
            clue->row[2] = j;
            clue->cell[2] = k;
            break;
        case NOT_NEXT_TO:
            if( i >= game_data->number_of_columns - 1 )
                s = -1;
            else if( i <= 0 )
                s = +1;
            else
                s = rand_sign();
            ii = i + s;
            get_random_item_col( game_data, ii, &jj, &kk );
            kk = ( kk + 1 ) % game_data->number_of_columns; // get an item that is NOT the neighbor one
            // avoid same item
            if( ( kk == game_data->puzzle[i][j] ) && ( j == jj ) )
                kk = ( kk + 1 ) % game_data->number_of_columns;
            if( ( i - s >= 0 ) && ( i - s < game_data->number_of_columns ) )
                if( game_data->puzzle[i - s][jj] == kk ) // avoid the neighbor from the other side
                    kk = ( kk + 1 ) % game_data->number_of_columns;

            if( rand_int( 2 ) )
            {
                std::swap( i, ii );
                std::swap( j, jj );
                std::swap( k, kk );
            }
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = ii;
            clue->row[1] = jj;
            clue->cell[1] = kk;
            clue->column[2] = i;
            clue->row[2] = j;
            clue->cell[2] = k;
            break;
        case NOT_MIDDLE:
            if( i > game_data->number_of_columns - 3 )
                s = -1;
            else if( i < 2 )
                s = 1;
            else
                s = rand_sign();
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            ii = i + s;
            get_random_item_col( game_data, ii, &jj, &kk );
            clue->column[1] = ii;
            clue->row[1] = jj;
            clue->cell[1] = ( kk + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;
            ii = i + 2 * s;
            get_random_item_col( game_data, ii, &jj, &kk );
            clue->column[2] = ii;
            clue->row[2] = jj;
            clue->cell[2] = kk;
            if( rand_int( 2 ) )
            { // random swap of outer elements
                std::swap( clue->column[0], clue->column[2] );
                std::swap( clue->row[0], clue->row[2] );
                std::swap( clue->cell[0], clue->cell[2] );
            }
            break;
        case TOGETHER_2:
            get_random_item_col_except( game_data, i, &jj, &kk, j, j ); // except row j (and j)
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = i;
            clue->row[1] = jj;
            clue->cell[1] = kk;
            clue->column[2] = i;
            clue->row[2] = jj;
            clue->cell[2] = kk; // filler
            break;
        case TOGETHER_3:
            get_random_item_col_except( game_data, i, &jj, &kk, j, j );    // except row j (and j)
            get_random_item_col_except( game_data, i, &jjj, &kkk, j, jj ); // except row j and jj
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = i;
            clue->row[1] = jj;
            clue->cell[1] = kk;
            clue->column[2] = i;
            clue->row[2] = jjj;
            clue->cell[2] = kkk;
            break;
        case NOT_TOGETHER:
            get_random_item_col_except( game_data, i, &jj, &kk, j, j ); // except row j (and j)
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = i;
            clue->row[1] = jj;
            clue->cell[1] = ( kk + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;
            clue->column[2] = i;
            clue->row[2] = j;
            clue->cell[2] = k; // filler
            break;
        case TOGETHER_NOT_MIDDLE:
            get_random_item_col_except( game_data, i, &jj, &kk, j, j );    // except row j (and j)
            get_random_item_col_except( game_data, i, &jjj, &kkk, j, jj ); // except row j and jj
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            clue->column[1] = i;
            clue->row[1] = jj;
            clue->cell[1] = ( kk + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;
            clue->column[2] = i;
            clue->row[2] = jjj;
            clue->cell[2] = kkk;
            break;
        case REVEAL:
            ii = rand_int( game_data->number_of_columns );
            get_random_item_col( game_data, ii, &jj, &kk );
            for( m = 0; m < 3; m++ )
            {
                clue->column[m] = ii;
                clue->row[m] = jj;
                clue->cell[m] = kk;
            }
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            // xxx todo: check this
            get_random_item_col_except( game_data, i, &jj, &kk, j, j );    // except row j (and j)
            get_random_item_col_except( game_data, i, &jjj, &kkk, j, jj ); // except row j and jj
            clue->column[0] = i;
            clue->row[0] = j;
            clue->cell[0] = k;
            // same as together_not_middle but sorted (so we don't know which is
            // middle)
            if( jj < jjj )
            {
                clue->column[1] = i;
                clue->row[1] = jj;
                clue->cell[1] = ( kk + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;
                clue->column[2] = i;
                clue->row[2] = jjj;
                clue->cell[2] = kkk;
            }
            else
            {
                clue->column[1] = i;
                clue->row[1] = jjj;
                clue->cell[1] = kkk;
                clue->column[2] = i;
                clue->row[2] = jj;
                clue->cell[2] = ( kk + 1 + rand_int( game_data->number_of_columns - 1 ) ) % game_data->number_of_columns;
            }
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
    int i, j, k;

    // if REL_PERCENT is not set, use defaults
    if( REL_PERCENT[NEXT_TO] == -1 )
        reset_rel_params();

    // initialize REL_PERCENT_MAX (total sum) for random relation creation
    REL_PERCENT_MAX = 0;
    for( i = 0; i < NUMBER_OF_RELATIONS; i++ )
    {
        REL_PERCENT_MAX += REL_PERCENT[i];
    }

    for( i = 0; i < game_data->number_of_columns; i++ )
    {
        for( j = 0; j < game_data->column_height; j++ )
        {
            game_data->guess[i][j] = -1;
            game_data->tile_col[j][i] = -1;
            for( k = 0; k < game_data->number_of_columns; k++ )
            {
                game_data->tile[i][j][k] = 1;
            }
        }
    }
    game_data->guessed = 0;
}

// return -1 if there is more than one tile left in block
// last tile number otherwise
int last_tile_in_block( GameData *game_data, int i, int j )
{
    int k, m = -1, count = 0;

    if( game_data->guess[i][j] >= 0 )
        return -1;

    for( k = 0; k < game_data->number_of_columns; k++ )
    { // find if there is only 1 tile left
        if( game_data->tile[i][j][k] )
        {
            m = k;
            count++;
        }
        if( count > 1 )
            return -1;
    }
    return m;
};

// return -1 if there is more than one block with given tile
// last block number (column) otherwise
int last_tile_in_row( GameData *game_data, int j, int k )
{
    int i, m = -1, count = 0;

    for( i = 0; i < game_data->number_of_columns; i++ )
    { // find if there is only 1 tile left
        if( game_data->guess[i][j] == k )
            return -1;
        if( game_data->tile[i][j][k] )
        {
            m = i;
            count++;
        }
        if( count > 1 )
            return -1;
    }
    return m;
};

// check any obviously guessable clues in row
int check_row( GameData *game_data, int j )
{
    int i, m, k;

    for( i = 0; i < game_data->number_of_columns; i++ )
    { // find if there is only 1 tile left
        m = last_tile_in_block( game_data, i, j );
        if( m >= 0 )
        {
            guess_tile( game_data, i, j, m );
            return 1;
        }
    }

    for( k = 0; k < game_data->number_of_columns; k++ )
    {
        m = last_tile_in_row( game_data, j, k ); // check if there is only 1 left of this tile
        if( m >= 0 )
        {
            guess_tile( game_data, m, j, k );
            return 1;
        }
    }
    return 0;
};

void hide_tile_and_check( GameData *game_data, int i, int j, int k )
{
    game_data->tile[i][j][k] = 0;
    check_row( game_data, j );
};

void guess_tile( GameData *game_data, int column, int row, int cell )
{
    int m;

    game_data->guess[column][row] = cell;
    game_data->guessed++;
    for( m = 0; m < game_data->number_of_columns; m++ )
        if( m != cell )
            game_data->tile[column][row][m] = 0; // hide all tiles from this block

    for( m = 0; m < game_data->number_of_columns; m++ )
    {
        if( m != column )
            game_data->tile[m][row][cell] = 0; // hide this tile in all blocks
    }

    check_row( game_data, row );
};

int is_guessed( GameData *game_data, int j, int k )
{
    int i;

    for( i = 0; i < game_data->number_of_columns; i++ )
    {
        if( game_data->guess[i][j] == k )
            return 1;
    }

    return 0;
};

void unguess_tile( GameData *game_data, int i, int j )
{
    int m, k;

    k = game_data->guess[i][j];
    game_data->guess[i][j] = -1;
    game_data->guessed--;

    for( m = 0; m < game_data->number_of_columns; m++ )
    {
        if( !is_guessed( game_data, j, m ) )
            game_data->tile[i][j][m] = 1;
        if( game_data->guess[m][j] < 0 )
            game_data->tile[m][j][k] = 1;
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

    int j0 = clue->row[0];
    int k0 = clue->cell[0];
    int j1 = clue->row[1];
    int k1 = clue->cell[1];
    int j2 = clue->row[2];
    int k2 = clue->cell[2];
    int i0 = game_data->where[j0][k0];
    int i1 = game_data->where[j1][k1];
    int i2 = game_data->where[j2][k2];

    ret = 1;

    switch( clue->rel )
    {
        case ONE_SIDE:
            if( game_data->where[j0][k0] >= game_data->where[j1][k1] )
                ret = 0;
            break;
        case TOGETHER_2:
            if( game_data->where[j0][k0] != game_data->where[j1][k1] )
                ret = 0;
            break;
        case TOGETHER_3:
            if( ( game_data->where[j0][k0] != game_data->where[j1][k1] )
                || ( game_data->where[j0][k0] != game_data->where[j2][k2] ) )
                ret = 0;
            break;
        case TOGETHER_NOT_MIDDLE:
            if( ( game_data->where[j0][k0] == game_data->where[j1][k1] )
                || ( game_data->where[j0][k0] != game_data->where[j2][k2] ) )
                ret = 0;
            break;
        case NOT_TOGETHER:
            if( ( game_data->where[j0][k0] == game_data->where[j1][k1] )
                || ( game_data->where[j1][k1] == game_data->where[j2][k2] ) )
                ret = 0;
            break;
        case NEXT_TO:
            if( ( game_data->where[j0][k0] - game_data->where[j1][k1] != 1 )
                && ( game_data->where[j0][k0] - game_data->where[j1][k1] != -1 ) )
                ret = 0;
            if( ( game_data->where[j2][k2] - game_data->where[j1][k1] != 1 )
                && ( game_data->where[j2][k2] - game_data->where[j1][k1] != -1 ) )
                ret = 0;
            break;

        case NOT_NEXT_TO:
            if( ( game_data->where[j0][k0] - game_data->where[j1][k1] == 1 )
                || ( game_data->where[j0][k0] - game_data->where[j1][k1] == -1 ) )
                ret = 0;
            if( ( game_data->where[j2][k2] - game_data->where[j1][k1] == 1 )
                || ( game_data->where[j2][k2] - game_data->where[j1][k1] == -1 ) )
                ret = 0;
            break;

        case CONSECUTIVE:
            if( !( ( i1 == i0 + 1 ) && ( i2 == i0 + 2 ) ) && !( ( i1 == i2 + 1 ) && ( i0 == i2 + 2 ) ) )
                ret = 0;
            break;

        case NOT_MIDDLE:
            if( i0 - i2 == 2 )
            {
                if( i0 - i1 == 1 )
                    ret = 0;
            }
            else if( i2 - i0 == 2 )
            {
                if( i1 - i0 == 1 )
                    ret = 0;
            }
            else
                ret = 0;
            break;

        case TOGETHER_FIRST_WITH_ONLY_ONE:
            if( ( game_data->where[j0][k0] == game_data->where[j1][k1] )
                == ( game_data->where[j0][k0] == game_data->where[j2][k2] ) )
                ret = 0;
            break;
        default:
            break;
    }
    return ret;
};
