#include "game.hpp"

int main( int, char ** )
{
    Game g;

    if( !g.init() )
        return EXIT_FAILURE;

    if( !g.run() )
        return EXIT_FAILURE;

    if( !g.cleanup() )
        return EXIT_FAILURE;

    return EXIT_FAILURE;
}
