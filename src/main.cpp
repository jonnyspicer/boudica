#include <iostream>
#include <string>

#include "types.h"
#include "bitboard.h"
#include "position.h"
#include "search.h"
#include "tt.h"
#include "evaluate.h"
#include "uci.h"
#include "book.h"

// ============================================================================
// Boudica Chess Engine - Main Entry Point
// ============================================================================
//
// Boudica is a UCI-compliant chess engine named after the Celtic warrior queen
// who led a major uprising against Roman occupation of Britain in 60-61 AD.
//
// Build: g++ -O3 -march=native -std=c++17 -o boudica src/*.cpp
// Run: ./boudica
//
// ============================================================================
// Initialization Functions
// ============================================================================

void init_all() {
    // Initialize bitboard attack tables (magic bitboards, etc.)
    Boudica::init_bitboards();

    // Initialize Zobrist hash keys for position hashing
    Boudica::Zobrist::init();

    // Initialize transposition table with default size (64 MB)
    TT.resize(64);

    // Initialize LMR reduction tables
    Boudica::init_lmr_table();

    // Initialize evaluation tables (PST, etc.)
    Boudica::Eval::init();

    // Initialize opening book
    Boudica::Book::init();
}

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char* argv[]) {
    // Disable stdout buffering for UCI communication
    // This ensures immediate output which is critical for UCI protocol
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.setf(std::ios::unitbuf);

    // Initialize all engine components
    init_all();

    // Check for command line arguments
    if (argc > 1) {
        std::string arg = argv[1];

        if (arg == "bench" || arg == "--bench") {
            // Run benchmark for testing/profiling
            std::cout << "Benchmark not yet implemented" << std::endl;
            // TODO: Implement benchmark with standard positions
            // bench();
            return 0;
        }
        else if (arg == "version" || arg == "--version" || arg == "-v") {
            std::cout << UCI::ENGINE_NAME << " " << UCI::ENGINE_VERSION
                      << " by " << UCI::ENGINE_AUTHOR << std::endl;
            return 0;
        }
        else if (arg == "help" || arg == "--help" || arg == "-h") {
            std::cout << UCI::ENGINE_NAME << " " << UCI::ENGINE_VERSION
                      << " - UCI Chess Engine" << std::endl;
            std::cout << "Author: " << UCI::ENGINE_AUTHOR << std::endl;
            std::cout << std::endl;
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --help, -h       Show this help message" << std::endl;
            std::cout << "  --version, -v    Show version information" << std::endl;
            std::cout << "  bench            Run built-in benchmark" << std::endl;
            std::cout << std::endl;
            std::cout << "The engine communicates using the UCI (Universal Chess Interface)" << std::endl;
            std::cout << "protocol. Start the engine and type 'uci' to begin." << std::endl;
            std::cout << std::endl;
            std::cout << "For more information about UCI, see:" << std::endl;
            std::cout << "  https://backscattering.de/chess/uci/" << std::endl;
            return 0;
        }
    }

    // Create position with initial state info
    Boudica::Position position;
    Boudica::StateInfo si;
    position.set(Boudica::Position::StartFEN, &si);

    // Create UCI handler and enter main loop
    UCI::Handler uci(position, TT);
    uci.loop();

    return 0;
}
