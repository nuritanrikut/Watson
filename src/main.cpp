#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "game.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main( int, char ** )
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern( "[%H:%M:%S.%e] %v" );
    console_sink->set_level( spdlog::level::trace );
    auto logger = std::make_shared<spdlog::logger>( "logger", console_sink );
    logger->set_level( spdlog::level::trace );
    spdlog::set_default_logger( logger );
    spdlog::flush_every( std::chrono::seconds( 3 ) );

    Game g;

    if( !g.init() )
        return EXIT_FAILURE;

    if( !g.run() )
        return EXIT_FAILURE;

    if( !g.cleanup() )
        return EXIT_FAILURE;

    return EXIT_FAILURE;
}
