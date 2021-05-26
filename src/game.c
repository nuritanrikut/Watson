#include "game.h"
//xxx todo: in clue creation - check that the clue includes one non-guessed block
//xxx todo: add TOGETHER_FIRST_WITH_ONLY_ONE logic
//xxx todo: improve composite clue checking (or what-ifs up to a given level)
//xxx todo: check for difficulty counting the number of positive clue checks (even for repeated clues)
//xxx todo: pass to binary and bitwise operations

char *clue_description[NUMBER_OF_RELATIONS] = {
    // Horizontal
    [NEXT_TO] = "Neighbors",
    [NOT_NEXT_TO] = "Middle not next to other",
    [ONE_SIDE] = "Second to the right of first",
    [CONSECUTIVE] = "Middle has other two one on each side",
    [NOT_MIDDLE] = "First and third two away, middle not in between",
    // Vertical
    [TOGETHER_2] = "First and second on same column",
    [TOGETHER_3] = "All three on same column",
    [NOT_TOGETHER] = "Second not on same column as first",
    [TOGETHER_NOT_MIDDLE] = "First and third on same column, second not",
    [TOGETHER_FIRST_WITH_ONLY_ONE] = "Unused",
    // Positional
    [REVEAL] = "First on given column" };

int DEFAULT_REL_PERCENT[NUMBER_OF_RELATIONS] = { [NEXT_TO] = 20,
                                                 [NOT_NEXT_TO] = 5,
                                                 [ONE_SIDE] = 2,
                                                 [CONSECUTIVE] = 3,
                                                 [NOT_MIDDLE] = 5,
                                                 [TOGETHER_2] = 25,
                                                 [TOGETHER_3] = 1,
                                                 [NOT_TOGETHER] = 20,
                                                 [TOGETHER_NOT_MIDDLE] = 1,
                                                 [TOGETHER_FIRST_WITH_ONLY_ONE] = 5,
                                                 [REVEAL] = 1 };

int REL_PERCENT[NUMBER_OF_RELATIONS] = { [NEXT_TO] = -1 };

//xxx todo: fix rel_percent_max when rel_percent changes!!!
int REL_PERCENT_MAX;

// Prototypes
void get_clue( Game *game, int i, int j, int rel, Clue *clue );
int filter_clues( Game *game );

void create_puzzle( Game *game )
{
    int i, j;
    int permutation[8];

    game->guessed = 0;
    for( i = 0; i < 8; i++ )
    {
        permutation[i] = i;
    }

    for( i = 0; i < game->column_height; i++ )
    {
        shuffle( permutation, game->number_of_columns );
        for( j = 0; j < game->number_of_columns; j++ )
        {
            game->puzzle[j][i] = permutation[j];
            game->where[i][permutation[j]] = j;
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

void remove_clue( Game *game, int i )
{
    // swap clue[i] with last clue, reduce clue_n
    game->clue_n--;
    game->clue[i] = game->clue[game->clue_n];
};

int check_this_clue( Game *game, Clue *clue )
{
    int i, m, ret = 0, hide_first;
    int j0, k0, j1, k1, j2, k2;

    j0 = clue->j[0];
    k0 = clue->k[0];
    j1 = clue->j[1];
    k1 = clue->k[1];
    j2 = clue->j[2];
    k2 = clue->k[2];

    ret = 0;
    hide_first = 0;

    switch( clue->rel )
    {
        case REVEAL:
            if( game->guess[clue->i[0]][j0] < 0 )
            {
                guess_tile( game, clue->i[0], j0, k0 );
                ret = 1 | k0 << 1 | j0 << 4 | clue->i[0] << 7 | 1 << 10;
            }
            break;
        case ONE_SIDE:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( game->tile[i][j1][k1] )
                {
                    hide_tile_and_check( game, i, j1, k1 );
                    ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                }
                if( game->tile[i][j0][k0] )
                    break;
            }
            for( i = game->number_of_columns - 1; i >= 0; i-- )
            {
                if( game->tile[i][j0][k0] )
                {
                    hide_tile_and_check( game, i, j0, k0 );
                    ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                }
                if( game->tile[i][j1][k1] )
                    break;
            }
            break;

        case TOGETHER_2:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( !game->tile[i][j0][k0] || !game->tile[i][j1][k1] )
                {
                    if( game->tile[i][j0][k0] )
                    {
                        hide_tile_and_check( game, i, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                    }
                    if( game->tile[i][j1][k1] )
                    {
                        hide_tile_and_check( game, i, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                        ;
                    }
                }
            }
            break;
        case TOGETHER_3:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( !game->tile[i][j0][k0] || !game->tile[i][j1][k1] || !game->tile[i][j2][k2] )
                { // if one exists but one doesn't
                    if( game->tile[i][j0][k0] )
                    {
                        hide_tile_and_check( game, i, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                    }
                    if( game->tile[i][j1][k1] )
                    {
                        hide_tile_and_check( game, i, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                        ;
                    }
                    if( game->tile[i][j2][k2] )
                    {
                        hide_tile_and_check( game, i, j2, k2 );
                        ret = 1 | k2 << 1 | j2 << 4 | i << 7;
                        ;
                    }
                }
            }
            break;

        case TOGETHER_NOT_MIDDLE:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( ( game->guess[i][j0] == k0 ) || ( game->guess[i][j2] == k2 ) )
                {
                    if( game->tile[i][j1][k1] )
                    {
                        hide_tile_and_check( game, i, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                    }
                }
                if( ( !game->tile[i][j0][k0] ) || ( game->guess[i][j1] == k1 ) || !game->tile[i][j2][k2] )
                {
                    if( game->tile[i][j0][k0] )
                    {
                        hide_tile_and_check( game, i, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                    }
                    if( game->tile[i][j2][k2] )
                    {
                        hide_tile_and_check( game, i, j2, k2 );
                        ret = 1 | k2 << 1 | j2 << 4 | i << 7;
                        ;
                    }
                }
            }
            break;

        case NOT_TOGETHER:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( ( game->guess[i][j0] == k0 ) && game->tile[i][j1][k1] )
                {
                    hide_tile_and_check( game, i, j1, k1 );
                    ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                }
                if( ( game->guess[i][j1] == k1 ) && game->tile[i][j0][k0] )
                {
                    hide_tile_and_check( game, i, j0, k0 );
                    ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                    ;
                }
            }
            break;

        case NEXT_TO:
            if( !game->tile[1][j0][k0] && game->tile[0][j1][k1] )
            {
                hide_tile_and_check( game, 0, j1, k1 );
                ret = 1 | k1 << 1 | j1 << 4 | 0 << 7;
            }
            if( !game->tile[1][j1][k1] && game->tile[0][j0][k0] )
            {
                hide_tile_and_check( game, 0, j0, k0 );
                ret = 1 | k0 << 1 | j0 << 4 | 0 << 7;
            }
            if( !game->tile[game->number_of_columns - 2][j0][k0] && game->tile[game->number_of_columns - 1][j1][k1] )
            {
                hide_tile_and_check( game, game->number_of_columns - 1, j1, k1 );
                ret = 1 | k1 << 1 | j1 << 4 | ( game->number_of_columns - 1 ) << 7;
            }
            if( !game->tile[game->number_of_columns - 2][j1][k1] && game->tile[game->number_of_columns - 1][j0][k0] )
            {
                hide_tile_and_check( game, game->number_of_columns - 1, j0, k0 );
                ret = 1 | k0 << 1 | j0 << 4 | ( game->number_of_columns - 1 ) << 7;
                ;
            }

            for( i = 1; i < game->number_of_columns - 1; i++ )
            {
                if( !game->tile[i - 1][j0][k0] && !game->tile[i + 1][j0][k0] )
                {
                    if( game->tile[i][j1][k1] )
                    {
                        hide_tile_and_check( game, i, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                        ;
                    }
                }
                if( !game->tile[i - 1][j1][k1] && !game->tile[i + 1][j1][k1] )
                {
                    if( game->tile[i][j0][k0] )
                    {
                        hide_tile_and_check( game, i, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                        ;
                    }
                }
            }
            break;

        case NOT_NEXT_TO:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( i < game->number_of_columns - 1 )
                {
                    if( ( game->guess[i][j0] == k0 ) && game->tile[i + 1][j1][k1] )
                    {
                        hide_tile_and_check( game, i + 1, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | ( i + 1 ) << 7;
                        ;
                    }

                    if( ( game->guess[i][j1] == k1 ) && game->tile[i + 1][j0][k0] )
                    {
                        hide_tile_and_check( game, i + 1, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | ( i + 1 ) << 7;
                        ;
                    }
                }
                if( i > 0 )
                {
                    if( ( game->guess[i][j0] == k0 ) && game->tile[i - 1][j1][k1] )
                    {
                        hide_tile_and_check( game, i - 1, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | ( i - 1 ) << 7;
                        ;
                    }

                    if( ( game->guess[i][j1] == k1 ) && game->tile[i - 1][j0][k0] )
                    {
                        hide_tile_and_check( game, i - 1, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | ( i - 1 ) << 7;
                        ;
                    }
                }
            }
            break;

        case CONSECUTIVE:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                for( m = 0; m < 2; m++ )
                {
                    if( game->tile[i][j0][k0] )
                    {
                        if( ( i < game->number_of_columns - 2 ) && ( i < 2 ) )
                        {
                            if( ( !game->tile[i + 1][j1][k1] ) || ( !game->tile[i + 2][j2][k2] ) )
                            {
                                hide_first = 1;
                            }
                        }
                        if( ( i >= 2 ) && ( i >= game->number_of_columns - 2 ) )
                        {
                            if( ( !game->tile[i - 2][j2][k2] ) || ( !game->tile[i - 1][j1][k1] ) )
                            {
                                hide_first = 1;
                            }
                        }

                        if( ( i >= 2 ) && ( i < game->number_of_columns - 2 ) )
                        {
                            if( ( ( !game->tile[i + 1][j1][k1] ) || ( !game->tile[i + 2][j2][k2] ) )
                                && ( ( !game->tile[i - 2][j2][k2] ) || ( !game->tile[i - 1][j1][k1] ) ) )
                            {
                                hide_first = 1;
                            }
                        }
                        if( hide_first )
                        {
                            hide_first = 0;
                            hide_tile_and_check( game, i, j0, k0 );
                            ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                        }
                    }
                    SWAP( j0, j2 );
                    SWAP( k0, k2 );
                }

                if( game->tile[i][j1][k1] )
                {
                    if( ( i == 0 ) || ( i == game->number_of_columns - 1 ) )
                    {
                        hide_first = 1;
                    }
                    else
                    {
                        if( ( ( !game->tile[i - 1][j0][k0] ) && !( game->tile[i + 1][j0][k0] ) )
                            || ( ( !game->tile[i - 1][j2][k2] ) && !( game->tile[i + 1][j2][k2] ) ) )
                        { // error here! incorrect check!
                            hide_first = 1;
                        }
                    }
                    if( hide_first )
                    {
                        hide_first = 0;
                        hide_tile_and_check( game, i, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                        ;
                    }
                }
            }
            break;

        case NOT_MIDDLE:
            for( i = 0; i < game->number_of_columns; i++ )
            { // apply mask
                for( m = 0; m < 2; m++ )
                {
                    if( game->tile[i][j0][k0] )
                    {
                        if( ( i < game->number_of_columns - 2 ) && ( i < 2 ) )
                        {
                            if( ( game->guess[i + 1][j1] == k1 ) || ( !game->tile[i + 2][j2][k2] ) )
                            {
                                hide_first = 1;
                            }
                        }
                        if( ( i >= 2 ) && ( i >= game->number_of_columns - 2 ) )
                        {
                            if( ( game->guess[i - 1][j1] == k1 ) || ( !game->tile[i - 2][j2][k2] ) )
                            {
                                hide_first = 1;
                            }
                        }

                        if( ( i >= 2 ) && ( i < game->number_of_columns - 2 ) )
                        {
                            if( ( ( game->guess[i + 1][j1] == k1 ) || ( !game->tile[i + 2][j2][k2] ) )
                                && ( ( !game->tile[i - 2][j2][k2] ) || ( game->guess[i - 1][j1] == k1 ) ) )
                            {
                                hide_first = 1;
                            }
                        }
                        if( hide_first )
                        {
                            hide_first = 0;
                            ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                            ;
                            hide_tile_and_check( game, i, j0, k0 );
                        }
                    }
                    SWAP( j0, j2 );
                    SWAP( k0, k2 );
                }
                if( ( i >= 1 ) && ( i <= game->number_of_columns - 2 ) )
                {
                    if( ( ( game->guess[i - 1][j0] == k0 ) && ( game->guess[i + 1][j2] == k2 ) )
                        || ( ( game->guess[i - 1][j2] == k2 ) && ( game->guess[i + 1][j0] == k0 ) ) )
                    {
                        if( game->tile[i][j1][k1] )
                        {
                            hide_tile_and_check( game, i, j1, k1 );
                            ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                        }
                    }
                }
            }
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            //xxx todo: check this
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( !game->tile[i][j1][k1] && !game->tile[i][j2][k2] )
                {
                    if( game->tile[i][j1][k1] )
                    {
                        hide_tile_and_check( game, i, j0, k0 );
                        ret = 1 | k0 << 1 | j0 << 4 | i << 7;
                    }
                }
                else if( game->guess[i][j1] == k1 )
                {
                    if( game->tile[i][j2][k2] )
                    {
                        hide_tile_and_check( game, i, j2, k2 );
                        ret = 1 | k2 << 1 | j2 << 4 | i << 7;
                    }
                }
                else if( game->guess[i][j2] == k2 )
                {
                    if( game->tile[i][j1][k1] )
                    {
                        hide_tile_and_check( game, i, j1, k1 );
                        ret = 1 | k1 << 1 | j1 << 4 | i << 7;
                    }
                }
            }
            break;
        default:
            break;
    }
    return ret;
}

int check_solution( Game *game )
{
    int i, j;
    for( i = 0; i < game->number_of_columns; i++ )
        for( j = 0; j < game->column_height; j++ )
            if( !game->tile[i][j][game->puzzle[i][j]] )
                return 0;

    return 1;
}

// save a backup copy of game data if type==0, restore the data if type==1
void switch_game( Game *game, int type )
{
    static int tile[8][8][8];
    static int guess[8][8];
    static int guessed;

    if( type == 0 )
    {
        memcpy( &tile, &game->tile, sizeof( tile ) );
        memcpy( &guess, &game->guess, sizeof( guess ) );
        guessed = game->guessed;
    }
    SWAP( game->tile, tile );
    SWAP( game->guess, guess );
    SWAP( game->guessed, guessed );
}

// returns: clue number | 1<<8 | k<<9 | j<<12 | k<<15
// where i,j,k is a tile that can be ruled out with this clue
int get_hint( Game *game )
{ // still not working properly
    int i, ret = 0, tro = 0;

    switch_game( game, 0 ); // store game
    for( i = 0; i < game->clue_n; i++ )
    {
        if( ( tro = check_this_clue( game, &game->clue[i] ) ) )
        {
            ret = i;
            break;
        }
    }
    switch_game( game, 1 ); // restore game
    return ret | tro << 8;
}
int advanced_check_clues( Game *game )
{
    int info, m;
    int i, j, k;

    for( i = 0; i < game->number_of_columns; i++ )
    {
        for( j = 0; j < game->column_height; j++ )
        {
            for( k = 0; k < game->number_of_columns; k++ )
            {
                if( game->tile[i][j][k] )
                {
                    switch_game( game, 0 ); // save state
                    guess_tile( game, i, j, k );
                    do
                    { // repeat until no more information remains in clues
                        info = 0;
                        for( m = 0; m < game->clue_n; m++ )
                        {
                            if( check_this_clue( game, &game->clue[m] ) )
                            {
                                info = 1;
                            }
                        }
                    } while( info );
                    if( !check_panel_consistency( game ) )
                    {
                        switch_game( game, 1 ); // restore
                        hide_tile_and_check( game, i, j, k );
                        return 1;
                    }
                    else
                    {
                        switch_game( game, 1 ); // restore state
                    }
                }
            }
        }
    }

    return 0;
}

int check_clues( Game *game )
{
    // check whether the clues add new info (within reason -- this can be tuned)
    // for now it does not combine clues (analyze each one separately)
    // if so, discover the info in game->tile
    // return 1 if new info was found, 0 if not
    int info, m, ret;

    ret = 0;
    do
    { // repeat until no more information remains in clues
        info = 0;
        for( m = 0; m < game->clue_n; m++ )
        {
            if( check_this_clue( game, &game->clue[m] ) )
            {
                ret = 1;
                info = 1;
            }
        }
        if( !info && game->advanced )
        { // check "what if" depth 1
            while( advanced_check_clues( game ) )
            {
                info = 1;
            }
        }
    } while( info );

    return ret;
}

void create_game_with_clues( Game *game )
{
    int i;

    init_game( game );
    create_puzzle( game );

    game->clue_n = 0;
    for( i = 0; i < 100; i++ )
    { // xxx todo add a check to see if we have found solution or not after 100
        game->clue_n++;
        do
        {
            get_clue( game,
                      rand_int( game->number_of_columns ),
                      rand_int( game->column_height ),
                      -1,
                      &game->clue[game->clue_n - 1] );
        } while( !check_this_clue( game, &game->clue[game->clue_n - 1] ) ); // should be while !check_clues?
        check_clues( game );
        if( game->guessed == game->number_of_columns * game->column_height )
            break;
    }

    if( !check_solution( game ) ) // debug
        fprintf( stderr, "ERROR: SOLUTION DOESN'T MATCH CLUES\n" );

    filter_clues( game );
    fprintf(
        stdout, "%dx%d game created with %d clues.\n", game->number_of_columns, game->column_height, game->clue_n );

    //clean guesses and tiles
    init_game( game );

    // reveal reveal clues and remove them from clue list
    for( i = 0; i < game->clue_n; i++ )
    {
        if( game->clue[i].rel == REVEAL )
        {
            guess_tile( game, game->clue[i].i[0], game->clue[i].j[0], game->clue[i].k[0] );
            remove_clue( game, i );
            i--;
        }
    }

    // mark clues unhidden
    for( i = 0; i < game->clue_n; i++ )
        game->clue[i].hidden = 0;
}

// checks if clue is compatible with current panel (not necessarily with solution)
int is_clue_compatible( Game *game, Clue *clue )
{
    int i, j, ret = 0;
    int j0, k0, j1, k1, j2, k2;

    j0 = clue->j[0];
    k0 = clue->k[0];
    j1 = clue->j[1];
    k1 = clue->k[1];
    j2 = clue->j[2];
    k2 = clue->k[2];

    ret = 0;

    switch( clue->rel )
    {
        case REVEAL:
            if( game->tile[clue->i[0]][j0][k0] )
                ret = 1;
            break;
        case ONE_SIDE:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( game->tile[i][j1][k1] && ( ret == -1 ) )
                {
                    ret = 1;
                    break; //loop
                }

                if( game->tile[i][j0][k0] )
                {
                    ret = -1;
                }
            }
            if( ret != 1 )
                ret = 0;
            break; //switch

        case TOGETHER_2:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( game->tile[i][j0][k0] && game->tile[i][j1][k1] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case TOGETHER_3:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( game->tile[i][j0][k0] && game->tile[i][j1][k1] && game->tile[i][j2][k2] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case TOGETHER_NOT_MIDDLE:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( ( game->tile[i][j0][k0] ) && ( game->guess[i][j1] != k1 ) && game->tile[i][j2][k2] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_TOGETHER:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( ( game->guess[i][j0] != k0 ) && game->tile[i][j1][k1] )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NEXT_TO:
            for( i = 0; i < game->number_of_columns - 1; i++ )
            {
                if( ( game->tile[i][j0][k0] && game->tile[i + 1][j1][k1] )
                    || ( game->tile[i][j1][k1] && game->tile[i + 1][j0][k0] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_NEXT_TO:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                for( j = 0; j < game->number_of_columns; j++ )
                {
                    if( game->tile[i][j0][k0] && game->tile[j][j1][k1] )
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
            for( i = 0; i < game->number_of_columns - 2; i++ )
            {
                if( ( game->tile[i][j0][k0] && game->tile[i + 1][j1][k1] && game->tile[i + 2][j2][k2] )
                    || ( game->tile[i][j2][k2] && game->tile[i + 1][j1][k1] && game->tile[i + 2][j0][k0] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;

        case NOT_MIDDLE:
            for( i = 0; i < game->number_of_columns - 2; i++ )
            {
                if( ( game->tile[i][j0][k0] && ( game->guess[i + 1][j1] != k1 ) && game->tile[i + 2][j2][k2] )
                    || ( game->tile[i][j2][k2] && ( game->guess[i + 1][j1] != k1 ) && game->tile[i + 2][j0][k0] ) )
                {
                    ret = 1;
                    break;
                }
            }
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            //xxx todo: check this:
            for( i = 0; i < game->number_of_columns; i++ )
            {
                if( game->tile[i][j0][k0] && ( game->tile[i][j1][k1] || game->tile[i][j2][k2] )
                    && !( ( game->guess[i][j1] == k1 ) && ( game->guess[i][j2] == k2 ) ) )
                    ret = 1;
            }

            break;
        default:
            break;
    }
    return ret;
}

int check_panel_consistency( Game *game )
{
    int m;
    for( m = 0; m < game->clue_n; m++ )
    {
        if( !is_clue_compatible( game, &game->clue[m] ) )
            return 0;
    }
    return 1;
}

int check_panel_correctness( Game *game )
{
    int i, j;
    for( i = 0; i < game->number_of_columns; i++ )
    {
        for( j = 0; j < game->column_height; j++ )
        {
            if( !game->tile[i][j][game->puzzle[i][j]] )
            {
                return 0;
            }
        }
    }
    return 1;
}

int check_clues_for_solution( Game *game )
{
    int m, info;

    init_game( game );

    do
    { // repeat until no more information remains in clues
        info = 0;
        for( m = 0; m < game->clue_n; m++ )
        {
            if( check_this_clue( game, &game->clue[m] ) )
            {
                info = 1;
            }
        }
        if( !info && game->advanced )
        { // check "what if" depth 1
            while( advanced_check_clues( game ) )
            {
                info = 1;
            }
        }
    } while( info );

    if( game->guessed == game->number_of_columns * game->column_height )
        return 1;
    else
        return 0;
}

int filter_clues( Game *game )
{
    int m, i, j, ret = 0;
    // revert clue order
    //    for(i=0; i<game->clue_n/2; i++){
    //        SWAP(game->clue[i], game->clue[game->clue_n-i-1]);
    //    }
    //
    for( m = 0; m < game->clue_n; m++ )
    { // test reduction
        SWAP( game->clue[game->clue_n - 1], game->clue[m] );
        init_game( game );
        game->clue_n--;
        if( check_clues_for_solution( game ) )
        {
            ret = 1;
        }
        else
        {
            game->clue_n++;
        }
    }

    // join clues if possible
    //xxx todo: check this
    for( i = game->clue_n - 1; i > 0; i-- )
    {
        if( game->clue[i].rel == TOGETHER_2 || game->clue[i].rel == NOT_TOGETHER )
        {
            for( j = i - 1; j >= 0; j-- )
            {
                if( ( game->clue[j].rel == TOGETHER_2 || game->clue[j].rel == NOT_TOGETHER )
                    && game->clue[i].rel == TOGETHER_2 )
                {
                    if( ( ( game->clue[j].j[0] == game->clue[i].j[0] ) && ( game->clue[j].k[0] == game->clue[i].k[0] ) )
                        || ( ( game->clue[j].j[1] == game->clue[i].j[0] )
                             && ( game->clue[j].k[1] == game->clue[i].k[0] ) ) )
                    {
                        game->clue[j].j[2] = game->clue[i].j[1];
                        game->clue[j].k[2] = game->clue[i].k[1];
                        game->clue[j].rel = game->clue[j].rel == TOGETHER_2 ? TOGETHER_3 : TOGETHER_NOT_MIDDLE;
                        remove_clue( game, i );
                        break;
                    }
                    else if( game->clue[j].rel == TOGETHER_2 )
                    {
                        if( ( ( game->clue[j].j[0] == game->clue[i].j[1] )
                              && ( game->clue[j].k[0] == game->clue[i].k[1] ) )
                            || ( ( game->clue[j].j[1] == game->clue[i].j[1] )
                                 && ( game->clue[j].k[1] == game->clue[i].k[1] ) ) )
                        {
                            game->clue[j].j[2] = game->clue[i].j[0];
                            game->clue[j].k[2] = game->clue[i].k[0];
                            game->clue[j].rel = TOGETHER_3;
                            remove_clue( game, i );
                            break;
                        }
                    }
                }
                else if( game->clue[j].rel == TOGETHER_2 && game->clue[i].rel == NOT_TOGETHER )
                {
                    if( ( game->clue[j].j[0] == game->clue[i].j[0] ) && ( game->clue[j].k[0] == game->clue[i].k[0] ) )
                    {
                        game->clue[i].j[2] = game->clue[j].j[1];
                        game->clue[i].k[2] = game->clue[j].k[1];
                        game->clue[i].rel = TOGETHER_NOT_MIDDLE;
                        game->clue[j] = game->clue[i];
                        remove_clue( game, i );
                        break;
                    }
                    else if( ( game->clue[j].j[1] == game->clue[i].j[0] )
                             && ( game->clue[j].k[1] == game->clue[i].k[0] ) )
                    {
                        game->clue[i].j[2] = game->clue[j].j[0];
                        game->clue[i].k[2] = game->clue[j].k[0];
                        game->clue[i].rel = TOGETHER_NOT_MIDDLE;
                        game->clue[j] = game->clue[i];
                        remove_clue( game, i );
                        break;
                    }
                }
            }
        }
    }

    //sort clues
    for( i = 0; i < game->clue_n; i++ )
    {
        switch( game->clue[i].rel )
        {
            case TOGETHER_2:
                if( game->clue[i].j[0] > game->clue[i].j[1] )
                {
                    SWAP( game->clue[i].i[0], game->clue[i].i[1] );
                    SWAP( game->clue[i].j[0], game->clue[i].j[1] );
                    SWAP( game->clue[i].k[0], game->clue[i].k[1] );
                }
                break;
            case TOGETHER_3:
                if( game->clue[i].j[0] > game->clue[i].j[2] )
                {
                    SWAP( game->clue[i].i[0], game->clue[i].i[2] );
                    SWAP( game->clue[i].j[0], game->clue[i].j[2] );
                    SWAP( game->clue[i].k[0], game->clue[i].k[2] );
                }
                if( game->clue[i].j[0] > game->clue[i].j[1] )
                {
                    SWAP( game->clue[i].i[0], game->clue[i].i[1] );
                    SWAP( game->clue[i].j[0], game->clue[i].j[1] );
                    SWAP( game->clue[i].k[0], game->clue[i].k[1] );
                }
                if( game->clue[i].j[1] > game->clue[i].j[2] )
                {
                    SWAP( game->clue[i].i[1], game->clue[i].i[2] );
                    SWAP( game->clue[i].j[1], game->clue[i].j[2] );
                    SWAP( game->clue[i].k[1], game->clue[i].k[2] );
                }

                break;
            case TOGETHER_NOT_MIDDLE:
                if( game->clue[i].j[0] > game->clue[i].j[2] )
                {
                    SWAP( game->clue[i].i[0], game->clue[i].i[2] );
                    SWAP( game->clue[i].j[0], game->clue[i].j[2] );
                    SWAP( game->clue[i].k[0], game->clue[i].k[2] );
                }
                break;
            default:
                break;
        }
    }

    //simplify clues that are redundant with one another
    //    for(i=0;i<game->clue_n;i++){
    //        for(j=i+1; j<game->clue_n+j++){
    //            if((game->clue[i].rel = TOGETHER_2) && (game->clue[j].rel =)
    //        }
    //    }

    return ret;
}

int get_random_tile( Game *game, int i, int *j, int *k )
{ // random item in column i
    int m, jj, kk = 0;

    m = 0;
    for( jj = 0; jj < game->column_height; jj++ )
        for( kk = 0; kk < game->number_of_columns; kk++ )
            if( game->guess[i][jj] < 0 )
                if( game->tile[i][jj][kk] )
                    m++;

    if( m == 0 )
        return 0;
    m = rand_int( m );
    for( jj = 0; jj < game->column_height; jj++ )
    {
        for( kk = 0; kk < game->number_of_columns; kk++ )
        {
            if( game->tile[i][jj][kk] && ( game->guess[i][jj] < 0 ) )
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
void get_random_item_col( Game *game, int i, int *j, int *k )
{
    *j = rand_int( game->column_height );
    *k = game->puzzle[i][*j];
};

// get a new solved item not in rows ej1 or ej2
void get_random_item_col_except( Game *game, int i, int *j, int *k, int ej1, int ej2 )
{
    int m = ( ej1 == ej2 ) ? 1 : 2;
    *j = rand_int( game->column_height - m );

    for( m = 0; m < 2; m++ ) // skip ej1 and ej2
        if( ( *j == ej1 ) || ( *j == ej2 ) )
            *j = ( *j + 1 ) % game->column_height;
    *k = game->puzzle[i][*j];
};

void get_clue( Game *game, int i, int j, int rel, Clue *clue )
{
    int k, ii, jj, kk, jjj, kkk, m, s;

    if( rel < 0 )
    {
        do
        { // to avoid problem when game->number_of_columns is too small in the NOT_MIDDLE case
            rel = random_relation();
        } while( ( rel == NOT_MIDDLE ) && ( i < 2 ) && ( i > game->number_of_columns - 3 ) );
    }
    clue->rel = rel;

    k = game->puzzle[i][j];

    switch( rel )
    {
        case CONSECUTIVE:
            s = rand_int( 3 );
            // check that i-s, i-s+1, i-s+2 are all in range
            if( i - s + 2 > game->number_of_columns - 1 )
                s += ( i - s + 2 ) - ( game->number_of_columns - 1 );
            else if( i - s < 0 )
                s = i;

            for( m = 0; m < 3; m++ )
            {
                ii = i - s + m;
                if( ii != i )
                    get_random_item_col(
                        game, ii, &jj, &kk ); // get a new solved item at column ii (solve if necessary)
                else
                {
                    jj = j;
                    kk = k;
                }
                clue->i[m] = ii;
                clue->j[m] = jj;
                clue->k[m] = kk;
            }
            if( rand_int( 2 ) )
            { // random swap of outer elements
                SWAP( clue->i[0], clue->i[2] );
                SWAP( clue->j[0], clue->j[2] );
                SWAP( clue->k[0], clue->k[2] );
            }
            break;
        case ONE_SIDE:
            s = rand_int( game->number_of_columns - 1 );
            ii = ( i + s + 1 ) % game->number_of_columns;
            get_random_item_col( game, ii, &jj, &kk );
            if( ii < i )
            {
                SWAP( i, ii );
                SWAP( j, jj );
                SWAP( k, kk );
            }
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = ii;
            clue->j[1] = jj;
            clue->k[1] = kk;
            clue->i[2] = ii;
            clue->j[2] = jj;
            clue->k[2] = kk; // filler
            break;
        case NEXT_TO:
            ii = i + rand_sign();
            if( ii >= game->number_of_columns )
                ii = i - 1;
            else if( ii < 0 )
                ii = i + 1;
            get_random_item_col( game, ii, &jj, &kk );
            if( rand_int( 2 ) )
            {
                SWAP( i, ii );
                SWAP( j, jj );
                SWAP( k, kk );
            }
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = ii;
            clue->j[1] = jj;
            clue->k[1] = kk;
            clue->i[2] = i;
            clue->j[2] = j;
            clue->k[2] = k;
            break;
        case NOT_NEXT_TO:
            if( i >= game->number_of_columns - 1 )
                s = -1;
            else if( i <= 0 )
                s = +1;
            else
                s = rand_sign();
            ii = i + s;
            get_random_item_col( game, ii, &jj, &kk );
            kk = ( kk + 1 ) % game->number_of_columns; // get an item that is NOT the neighbor one
            // avoid same item
            if( ( kk == game->puzzle[i][j] ) && ( j == jj ) )
                kk = ( kk + 1 ) % game->number_of_columns;
            if( ( i - s >= 0 ) && ( i - s < game->number_of_columns ) )
                if( game->puzzle[i - s][jj] == kk ) // avoid the neighbor from the other side
                    kk = ( kk + 1 ) % game->number_of_columns;

            if( rand_int( 2 ) )
            {
                SWAP( i, ii );
                SWAP( j, jj );
                SWAP( k, kk );
            }
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = ii;
            clue->j[1] = jj;
            clue->k[1] = kk;
            clue->i[2] = i;
            clue->j[2] = j;
            clue->k[2] = k;
            break;
        case NOT_MIDDLE:
            if( i > game->number_of_columns - 3 )
                s = -1;
            else if( i < 2 )
                s = 1;
            else
                s = rand_sign();
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            ii = i + s;
            get_random_item_col( game, ii, &jj, &kk );
            clue->i[1] = ii;
            clue->j[1] = jj;
            clue->k[1] = ( kk + 1 + rand_int( game->number_of_columns - 1 ) ) % game->number_of_columns;
            ii = i + 2 * s;
            get_random_item_col( game, ii, &jj, &kk );
            clue->i[2] = ii;
            clue->j[2] = jj;
            clue->k[2] = kk;
            if( rand_int( 2 ) )
            { // random swap of outer elements
                SWAP( clue->i[0], clue->i[2] );
                SWAP( clue->j[0], clue->j[2] );
                SWAP( clue->k[0], clue->k[2] );
            }
            break;
        case TOGETHER_2:
            get_random_item_col_except( game, i, &jj, &kk, j, j ); // except row j (and j)
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = i;
            clue->j[1] = jj;
            clue->k[1] = kk;
            clue->i[2] = i;
            clue->j[2] = jj;
            clue->k[2] = kk; // filler
            break;
        case TOGETHER_3:
            get_random_item_col_except( game, i, &jj, &kk, j, j );    // except row j (and j)
            get_random_item_col_except( game, i, &jjj, &kkk, j, jj ); // except row j and jj
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = i;
            clue->j[1] = jj;
            clue->k[1] = kk;
            clue->i[2] = i;
            clue->j[2] = jjj;
            clue->k[2] = kkk;
            break;
        case NOT_TOGETHER:
            get_random_item_col_except( game, i, &jj, &kk, j, j ); // except row j (and j)
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = i;
            clue->j[1] = jj;
            clue->k[1] = ( kk + 1 + rand_int( game->number_of_columns - 1 ) ) % game->number_of_columns;
            clue->i[2] = i;
            clue->j[2] = j;
            clue->k[2] = k; // filler
            break;
        case TOGETHER_NOT_MIDDLE:
            get_random_item_col_except( game, i, &jj, &kk, j, j );    // except row j (and j)
            get_random_item_col_except( game, i, &jjj, &kkk, j, jj ); // except row j and jj
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            clue->i[1] = i;
            clue->j[1] = jj;
            clue->k[1] = ( kk + 1 + rand_int( game->number_of_columns - 1 ) ) % game->number_of_columns;
            clue->i[2] = i;
            clue->j[2] = jjj;
            clue->k[2] = kkk;
            break;
        case REVEAL:
            ii = rand_int( game->number_of_columns );
            get_random_item_col( game, ii, &jj, &kk );
            for( m = 0; m < 3; m++ )
            {
                clue->i[m] = ii;
                clue->j[m] = jj;
                clue->k[m] = kk;
            }
            break;
        case TOGETHER_FIRST_WITH_ONLY_ONE:
            // xxx todo: check this
            get_random_item_col_except( game, i, &jj, &kk, j, j );    // except row j (and j)
            get_random_item_col_except( game, i, &jjj, &kkk, j, jj ); // except row j and jj
            clue->i[0] = i;
            clue->j[0] = j;
            clue->k[0] = k;
            // same as together_not_middle but sorted (so we don't know which is middle)
            if( jj < jjj )
            {
                clue->i[1] = i;
                clue->j[1] = jj;
                clue->k[1] = ( kk + 1 + rand_int( game->number_of_columns - 1 ) ) % game->number_of_columns;
                clue->i[2] = i;
                clue->j[2] = jjj;
                clue->k[2] = kkk;
            }
            else
            {
                clue->i[1] = i;
                clue->j[1] = jjj;
                clue->k[1] = kkk;
                clue->i[2] = i;
                clue->j[2] = jj;
                clue->k[2] = ( kk + 1 + rand_int( game->number_of_columns - 1 ) ) % game->number_of_columns;
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

void init_game( Game *game )
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

    for( i = 0; i < game->number_of_columns; i++ )
    {
        for( j = 0; j < game->column_height; j++ )
        {
            game->guess[i][j] = -1;
            game->tile_col[j][i] = -1;
            for( k = 0; k < game->number_of_columns; k++ )
            {
                game->tile[i][j][k] = 1;
            }
        }
    }
    game->guessed = 0;
}

// return -1 if there is more than one tile left in block
// last tile number otherwise
int last_tile_in_block( Game *game, int i, int j )
{
    int k, m = -1, count = 0;

    if( game->guess[i][j] >= 0 )
        return -1;

    for( k = 0; k < game->number_of_columns; k++ )
    { // find if there is only 1 tile left
        if( game->tile[i][j][k] )
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
int last_tile_in_row( Game *game, int j, int k )
{
    int i, m = -1, count = 0;

    for( i = 0; i < game->number_of_columns; i++ )
    { // find if there is only 1 tile left
        if( game->guess[i][j] == k )
            return -1;
        if( game->tile[i][j][k] )
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
int check_row( Game *game, int j )
{
    int i, m, k;

    for( i = 0; i < game->number_of_columns; i++ )
    { // find if there is only 1 tile left
        m = last_tile_in_block( game, i, j );
        if( m >= 0 )
        {
            guess_tile( game, i, j, m );
            return 1;
        }
    }

    for( k = 0; k < game->number_of_columns; k++ )
    {
        m = last_tile_in_row( game, j, k ); // check if there is only 1 left of this tile
        if( m >= 0 )
        {
            guess_tile( game, m, j, k );
            return 1;
        }
    }
    return 0;
};

void hide_tile_and_check( Game *game, int i, int j, int k )
{
    game->tile[i][j][k] = 0;
    check_row( game, j );
};

void guess_tile( Game *game, int i, int j, int k )
{
    int m;

    game->guess[i][j] = k;
    game->guessed++;
    for( m = 0; m < game->number_of_columns; m++ )
        if( m != k )
            game->tile[i][j][m] = 0; // hide all tiles from this block

    for( m = 0; m < game->number_of_columns; m++ )
    {
        if( m != i )
            game->tile[m][j][k] = 0; // hide this tile in all blocks
    }

    check_row( game, j );
};

int is_guessed( Game *game, int j, int k )
{
    int i;

    for( i = 0; i < game->number_of_columns; i++ )
    {
        if( game->guess[i][j] == k )
            return 1;
    }

    return 0;
};

void unguess_tile( Game *game, int i, int j )
{
    int m, k;

    k = game->guess[i][j];
    game->guess[i][j] = -1;
    game->guessed--;

    for( m = 0; m < game->number_of_columns; m++ )
    {
        if( !is_guessed( game, j, m ) )
            game->tile[i][j][m] = 1;
        if( game->guess[m][j] < 0 )
            game->tile[m][j][k] = 1;
    }
};

int is_vclue( RELATION rel )
{
    return ( ( rel == TOGETHER_2 ) || ( rel == TOGETHER_3 ) || ( rel == NOT_TOGETHER ) || ( rel == TOGETHER_NOT_MIDDLE )
             || ( rel == TOGETHER_FIRST_WITH_ONLY_ONE ) );
}

// only for debug puprposes. Check if clue is compatible with solution
int is_clue_valid( Game *game, Clue *clue )
{
    int ret = 0;
    int i0, i1, i2, j0, k0, j1, k1, j2, k2;

    j0 = clue->j[0];
    k0 = clue->k[0];
    j1 = clue->j[1];
    k1 = clue->k[1];
    j2 = clue->j[2];
    k2 = clue->k[2];
    i0 = game->where[j0][k0];
    i1 = game->where[j1][k1];
    i2 = game->where[j2][k2];

    ret = 1;

    switch( clue->rel )
    {
        case ONE_SIDE:
            if( game->where[j0][k0] >= game->where[j1][k1] )
                ret = 0;
            break;
        case TOGETHER_2:
            if( game->where[j0][k0] != game->where[j1][k1] )
                ret = 0;
            break;
        case TOGETHER_3:
            if( ( game->where[j0][k0] != game->where[j1][k1] ) || ( game->where[j0][k0] != game->where[j2][k2] ) )
                ret = 0;
            break;
        case TOGETHER_NOT_MIDDLE:
            if( ( game->where[j0][k0] == game->where[j1][k1] ) || ( game->where[j0][k0] != game->where[j2][k2] ) )
                ret = 0;
            break;
        case NOT_TOGETHER:
            if( ( game->where[j0][k0] == game->where[j1][k1] ) || ( game->where[j1][k1] == game->where[j2][k2] ) )
                ret = 0;
            break;
        case NEXT_TO:
            if( ( game->where[j0][k0] - game->where[j1][k1] != 1 )
                && ( game->where[j0][k0] - game->where[j1][k1] != -1 ) )
                ret = 0;
            if( ( game->where[j2][k2] - game->where[j1][k1] != 1 )
                && ( game->where[j2][k2] - game->where[j1][k1] != -1 ) )
                ret = 0;
            break;

        case NOT_NEXT_TO:
            if( ( game->where[j0][k0] - game->where[j1][k1] == 1 )
                || ( game->where[j0][k0] - game->where[j1][k1] == -1 ) )
                ret = 0;
            if( ( game->where[j2][k2] - game->where[j1][k1] == 1 )
                || ( game->where[j2][k2] - game->where[j1][k1] == -1 ) )
                ret = 0;
            break;

        case CONSECUTIVE:
            if( !( ( i1 == i0 + 1 ) && ( i2 == i0 + 2 ) ) && !( ( i1 == i2 + 1 ) && ( i0 = i2 + 2 ) ) )
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
            if( ( game->where[j0][k0] == game->where[j1][k1] ) == ( game->where[j0][k0] == game->where[j2][k2] ) )
                ret = 0;
            break;
        default:
            break;
    }
    return ret;
};
