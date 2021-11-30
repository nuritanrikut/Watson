#pragma once

#include "tiled_block.hpp"
#include "macros.hpp"

// Structures
enum class HOLD_CLICK_CHECK
{
    RELEASED = 0,
    DRAGGING,
    HOLDING
};

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

struct TileAddress
{
    bool valid;
    int column;
    int row;
    int cell;

    TileAddress() : valid( false ), column( 0 ), row( 0 ), cell( 0 ) { }
    TileAddress( bool valid_, int i, int j, int k ) : valid( valid_ ), column( i ), row( j ), cell( k ) { }
    TileAddress( int i, int j, int k ) : valid( true ), column( i ), row( j ), cell( k ) { }
};

struct Hint
{
    bool valid;
    int clue_number;
    TileAddress tile;

    Hint() : valid( false ), clue_number( 0 ), tile() { }
    Hint( bool valid_, int cn, TileAddress t ) : valid( valid_ ), clue_number( cn ), tile( t ) { }
};

struct Clue
{
    // the three tiles from the clue are j[m], k[m] for m=0,1,2
    // if clue uses only 1 or 2 tiles, use the first and repeat them arbitrarily
    // i coordinate points to the column in the solution where the item appears (not shown to user)
    TileAddress tile[3];
    RELATION rel; // how they relate
    int index;
    bool hidden;
};

struct GameData
{
    int guess[8][8];    // guessed value for guess[column][row] = cell;
    int puzzle[8][8];   // [col][block] = [tile]
    int tiles[8][8][8]; // [col][block][tile]
    Clue clues[MAX_CLUES];
    int clue_n;
    int number_of_columns; // number of columns
    int column_height;     // column height
    double time;
    int guessed;
    int tile_col[8][8]; // column where puzzle tile [row][tile] is located (in solution);
    int where[8][8];
    int advanced;

    void init_game(); // clean board and guesses xxx todo: add clues?
    void switch_game( int type );
    auto advanced_check_clues() -> int;
    void create_game_with_clues();
    void create_puzzle();
    auto get_hint() -> Hint;
    auto check_solution() -> int;
    auto check_panel_consistency() -> int;
    auto check_panel_correctness() -> int;
    void guess_tile( TileAddress tile );
    void hide_tile_and_check( TileAddress tile );
    void unguess_tile( int i, int j );
    auto is_guessed( int j, int k ) -> int; // is the value k on row j guessed?
    void get_clue( int i, int j, Clue *clue );

    void remove_clue( int i );
    void join_clues();
    auto filter_clues() -> int;
    void sort_clues();

    auto get_random_tile( int column, int *row, int *cell ) -> int;
    auto random_relation() -> int;
    void get_random_item_col( int column, int *row, int *cell );
    void get_random_item_col_except( int column, int *row, int *cell, int ej1, int ej2 );

    void get_clue_consecutive( int column, int row, int cell, Clue *clue );
    void get_clue_one_side( int column, int row, int cell, Clue *clue );
    void get_clue_next_to( int column, int row, int cell, Clue *clue );
    void get_clue_not_next_to( int column, int row, int cell, Clue *clue );
    void get_clue_not_middle( int column, int row, int cell, Clue *clue );
    void get_clue_together_2( int column, int row, int cell, Clue *clue );
    void get_clue_together_3( int column, int row, int cell, Clue *clue );
    void get_clue_not_together( int column, int row, int cell, Clue *clue );
    void get_clue_together_not_middle( int column, int row, int cell, Clue *clue );
    void get_clue_reveal( int column, int row, int cell, Clue *clue );
    void get_clue_together_first_with_only_one( int column, int row, int cell, Clue *clue );

    auto check_clues() -> int;
    auto check_clues_for_solution() -> int;
    auto check_this_clue( Clue *clue ) -> TileAddress;
    auto check_this_clue_reveal( Clue *clue ) -> TileAddress;
    auto check_this_clue_one_side( Clue *clue ) -> TileAddress;
    auto check_this_clue_together_2( Clue *clue ) -> TileAddress;
    auto check_this_clue_together_3( Clue *clue ) -> TileAddress;
    auto check_this_clue_together_not_middle( Clue *clue ) -> TileAddress;
    auto check_this_clue_not_together( Clue *clue ) -> TileAddress;
    auto check_this_clue_next_to( Clue *clue ) -> TileAddress;
    auto check_this_clue_not_next_to( Clue *clue ) -> TileAddress;
    auto check_this_clue_consecutive( Clue *clue ) -> TileAddress;
    auto check_this_clue_not_middle( Clue *clue ) -> TileAddress;
    auto check_this_clue_together_first_with_only_one( Clue *clue ) -> TileAddress;

    auto last_tile_in_block( int column, int row ) -> int;
    auto last_tile_in_row( int row, int cell ) -> int;
    auto check_row( int row ) -> int;

    // debug
    auto is_clue_valid( Clue *clue ) -> int;

    auto is_clue_compatible( Clue *clue ) -> int;
    auto is_clue_compatible_reveal( Clue *clue ) -> int;
    auto is_clue_compatible_one_side( Clue *clue ) -> int;
    auto is_clue_compatible_together_2( Clue *clue ) -> int;
    auto is_clue_compatible_together_3( Clue *clue ) -> int;
    auto is_clue_compatible_together_not_middle( Clue *clue ) -> int;
    auto is_clue_compatible_not_together( Clue *clue ) -> int;
    auto is_clue_compatible_next_to( Clue *clue ) -> int;
    auto is_clue_compatible_not_next_to( Clue *clue ) -> int;
    auto is_clue_compatible_consecutive( Clue *clue ) -> int;
    auto is_clue_compatible_not_middle( Clue *clue ) -> int;
    auto is_clue_compatible_together_first_with_only_one( Clue *clue ) -> int;
};

void shuffle( int p[], int n );
auto is_vclue( RELATION rel ) -> int; // is this relation a vertical clue?
void reset_rel_params();

// globals
extern int REL_PERCENT[NUMBER_OF_RELATIONS];
