#pragma once

#include "tiled_block.hpp"
#include "macros.hpp"

// Structures
enum class RESTART_STATE
{
    NO_RESTART = 0,
    NEW_GAME,
    LOADED_GAME
};

enum GAME_STATE
{
    GAME_NULL = 0,
    GAME_INTRO = 1,
    GAME_PLAYING,
    GAME_WRONG,
    GAME_OVER = 3,
    GAME_SETTINGS,
    NUMBER_OF_STATES
};

enum RELATION
{
    // Horizontal
    NEXT_TO,
    NOT_NEXT_TO,
    ONE_SIDE,
    CONSECUTIVE,
    NOT_MIDDLE,
    // Vertical
    TOGETHER_2,
    TOGETHER_3,
    NOT_TOGETHER,
    TOGETHER_NOT_MIDDLE,
    TOGETHER_FIRST_WITH_ONLY_ONE,
    // Positional
    REVEAL,
    NUMBER_OF_RELATIONS
};

struct Clue
{
    // the three tiles from the clue are j[m], k[m] for m=0,1,2
    // if clue uses only 1 or 2 tiles, use the first and repeat them arbitrarily
    // i coordinate points to the column in the solution where the item appears (not shown to user)
    int i[3], j[3], k[3];
    RELATION rel; // how they relate
    int index;
    bool hidden;
};

struct GameData
{
    int guess[8][8];   // guessed value for guess[column][row] = cell;
    int puzzle[8][8];  // [col][block] = [tile]
    int tile[8][8][8]; // [col][block][tile]
    Clue clue[MAX_CLUES];
    int clue_n;
    int number_of_columns; // number of columns
    int column_height;     // column height
    double time;
    int guessed;
    int tile_col[8][8]; // column where puzzle tile [row][tile] is located (in solution);
    int where[8][8];
    int advanced;
};

struct Pair
{
    int i;
    int j;
};

// Prototypes

int rand_int( int n );
void init_game( GameData *game_data ); // clean board and guesses xxx todo: add clues?
void create_game_with_clues( GameData *game_data );
void create_puzzle( GameData *game_data );
int get_hint( GameData *game_data );
int check_solution( GameData *game_data );
int check_panel_consistency( GameData *game_data );
int check_panel_correctness( GameData *game_data );
void shuffle( int p[], int n );
void guess_tile( GameData *game_data, int column, int row, int cell );
void hide_tile_and_check( GameData *game_data, int i, int j, int k );
void unguess_tile( GameData *game_data, int i, int j );
int is_guessed( GameData *game_data, int j, int k ); // is the value k on row j guessed?
void get_clue( GameData *game_data, int i, int j, int rel, Clue *clue );
int is_vclue( RELATION rel ); // is this relation a vertical clue?
void reset_rel_params( void );

// debug
int is_clue_valid( GameData *game_data, Clue *clue );

// globals
extern int REL_PERCENT[NUMBER_OF_RELATIONS];
