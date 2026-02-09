#include "uci.h"
#include "position.h"
#include "search.h"
#include "tt.h"
#include "movegen.h"
#include "book.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cstring>

namespace UCI {

// ============================================================================
// Global State
// ============================================================================

// Stop flag - search thread checks this periodically
std::atomic<bool> stopSearch(false);

// Search thread handle
static std::thread searchThread;

// ============================================================================
// UCI Handler Implementation
// ============================================================================

Handler::Handler(Boudica::Position& pos, TranspositionTable& tt)
    : position(pos), transpositionTable(tt), stateInfoIdx(0)
{
    initOptions();
    std::memset(stateInfoStack, 0, sizeof(stateInfoStack));
}

Handler::~Handler() {
    // Ensure search thread is stopped and joined
    stopSearch = true;
    if (searchThread.joinable()) {
        searchThread.join();
    }
}

void Handler::initOptions() {
    // Hash table size in MB (default 64, range 1-1024)
    options.emplace_back("Hash", 64, 1, 1024);

    // Clear Hash button
    options.emplace_back("Clear Hash", OPT_BUTTON);
}

// ============================================================================
// Main UCI Loop
// ============================================================================

void Handler::loop() {
    std::string line;
    std::string token;

    while (std::getline(std::cin, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        std::istringstream is(line);
        is >> std::skipws >> token;

        if (token == "quit") {
            cmdQuit();
            break;
        }
        else if (token == "uci")         cmdUCI();
        else if (token == "isready")     cmdIsReady();
        else if (token == "ucinewgame")  cmdUCINewGame();
        else if (token == "position")    cmdPosition(is);
        else if (token == "go")          cmdGo(is);
        else if (token == "stop")        cmdStop();
        else if (token == "setoption")   cmdSetOption(is);
        // Debug commands
        else if (token == "d" || token == "display") cmdDisplay();
        else if (token == "perft")       cmdPerft(is);
        else if (token == "bench")       cmdBench();
        // Ignore unknown commands silently (UCI protocol requirement)
    }
}

void Handler::processCommand(const std::string& command) {
    std::istringstream is(command);
    std::string token;
    is >> std::skipws >> token;

    if (token == "quit")             cmdQuit();
    else if (token == "uci")         cmdUCI();
    else if (token == "isready")     cmdIsReady();
    else if (token == "ucinewgame")  cmdUCINewGame();
    else if (token == "position")    cmdPosition(is);
    else if (token == "go")          cmdGo(is);
    else if (token == "stop")        cmdStop();
    else if (token == "setoption")   cmdSetOption(is);
    else if (token == "d")           cmdDisplay();
    else if (token == "perft")       cmdPerft(is);
    else if (token == "bench")       cmdBench();
}

// ============================================================================
// UCI Command Handlers
// ============================================================================

void Handler::cmdUCI() {
    std::cout << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << std::endl;
    std::cout << "id author " << ENGINE_AUTHOR << std::endl;
    std::cout << std::endl;

    // Output available options
    for (const auto& opt : options) {
        std::cout << "option name " << opt.name << " type ";
        switch (opt.type) {
            case OPT_SPIN:
                std::cout << "spin default " << opt.defaultValue
                          << " min " << opt.min
                          << " max " << opt.max;
                break;
            case OPT_CHECK:
                std::cout << "check default " << opt.defaultValue;
                break;
            case OPT_COMBO:
                std::cout << "combo default " << opt.defaultValue;
                for (const auto& v : opt.comboValues) {
                    std::cout << " var " << v;
                }
                break;
            case OPT_BUTTON:
                std::cout << "button";
                break;
            case OPT_STRING:
                std::cout << "string default " << opt.defaultValue;
                break;
        }
        std::cout << std::endl;
    }

    std::cout << "uciok" << std::endl;
}

void Handler::cmdIsReady() {
    // Ensure any pending initialization is complete
    std::cout << "readyok" << std::endl;
}

void Handler::cmdUCINewGame() {
    // Wait for any running search to complete
    cmdStop();

    // Clear the transposition table
    transpositionTable.clear();

    // Reset state info stack
    stateInfoIdx = 0;
    std::memset(stateInfoStack, 0, sizeof(stateInfoStack));

    // Reset position to starting position
    position.set(Boudica::Position::StartFEN, &stateInfoStack[stateInfoIdx]);

    // Clear search state (history tables, killers, etc.)
    Boudica::clear_search();
}

void Handler::cmdPosition(std::istringstream& is) {
    std::string token;
    is >> token;

    // Reset state info index
    stateInfoIdx = 0;

    if (token == "startpos") {
        // Set up starting position
        position.set(Boudica::Position::StartFEN, &stateInfoStack[stateInfoIdx]);

        // Check for moves
        is >> token;  // Should be "moves" if present
    }
    else if (token == "fen") {
        // Parse FEN string (up to 6 parts)
        std::string fen;

        while (is >> token && token != "moves") {
            if (!fen.empty()) fen += " ";
            fen += token;
        }

        position.set(fen, &stateInfoStack[stateInfoIdx]);

        // token is now either "moves" or we've reached end
        if (token != "moves") {
            return;  // No moves to apply
        }
    }
    else {
        // Unknown position format
        return;
    }

    // Apply moves if present
    if (token == "moves") {
        applyMoves(is);
    }
}

void Handler::applyMoves(std::istringstream& is) {
    std::string moveStr;

    while (is >> moveStr) {
        Move m = parseMove(moveStr);
        if (m != MOVE_NONE) {
            // Advance to next state info slot
            stateInfoIdx++;
            position.do_move(m, stateInfoStack[stateInfoIdx]);
        }
    }
}

Move Handler::parseMove(const std::string& moveStr) const {
    if (moveStr.length() < 4) return MOVE_NONE;

    // Parse source and destination squares
    File fromFile = File(moveStr[0] - 'a');
    Rank fromRank = Rank(moveStr[1] - '1');
    File toFile = File(moveStr[2] - 'a');
    Rank toRank = Rank(moveStr[3] - '1');

    // Validate square bounds
    if (fromFile < FILE_A || fromFile > FILE_H ||
        fromRank < RANK_1 || fromRank > RANK_8 ||
        toFile < FILE_A || toFile > FILE_H ||
        toRank < RANK_1 || toRank > RANK_8) {
        return MOVE_NONE;
    }

    Square from = make_square(fromFile, fromRank);
    Square to = make_square(toFile, toRank);

    // Check for promotion
    PieceType promotion = NO_PIECE_TYPE;
    if (moveStr.length() >= 5) {
        switch (moveStr[4]) {
            case 'q': promotion = QUEEN; break;
            case 'r': promotion = ROOK; break;
            case 'b': promotion = BISHOP; break;
            case 'n': promotion = KNIGHT; break;
            default: break;
        }
    }

    // Generate legal moves and find the matching one
    // This handles castling, en passant, and promotion correctly
    Boudica::MoveList<Boudica::LEGAL> moves(position);

    for (const auto& em : moves) {
        Move m = em.move;
        if (from_sq(m) == from && to_sq(m) == to) {
            // Check promotion type if this is a promotion
            if (type_of(m) == PROMOTION) {
                if (promotion_type(m) == promotion) {
                    return m;
                }
            }
            else if (promotion == NO_PIECE_TYPE) {
                return m;
            }
        }
    }

    // No legal move found - return MOVE_NONE
    // (Invalid moves should be rejected, not constructed)
    return MOVE_NONE;
}

void Handler::cmdGo(std::istringstream& is) {
    // Wait for any previous search to finish
    if (searchThread.joinable()) {
        stopSearch = true;
        searchThread.join();
    }

    // Parse search limits
    Boudica::SearchLimits limits;
    std::string token;

    while (is >> token) {
        if (token == "wtime")          is >> limits.time[WHITE];
        else if (token == "btime")     is >> limits.time[BLACK];
        else if (token == "winc")      is >> limits.inc[WHITE];
        else if (token == "binc")      is >> limits.inc[BLACK];
        else if (token == "movestogo") is >> limits.movestogo;
        else if (token == "depth")     is >> limits.depth;
        else if (token == "nodes")     is >> limits.nodes;
        else if (token == "movetime")  is >> limits.movetime;
        else if (token == "infinite")  limits.infinite = true;
        else if (token == "searchmoves") {
            // Parse list of moves to search
            std::string moveStr;
            while (is >> moveStr) {
                // Check if this is actually another go parameter
                if (moveStr == "wtime" || moveStr == "btime" ||
                    moveStr == "winc" || moveStr == "binc" ||
                    moveStr == "movestogo" || moveStr == "depth" ||
                    moveStr == "nodes" || moveStr == "movetime" ||
                    moveStr == "infinite" || moveStr == "ponder") {
                    // Put it back and break
                    // Note: Can't put back, so we'd need to handle differently
                    // For now, just break
                    break;
                }
                Move m = parseMove(moveStr);
                if (m != MOVE_NONE && limits.searchmoves_count < MAX_MOVES) {
                    limits.searchmoves[limits.searchmoves_count++] = m;
                }
            }
        }
        else if (token == "ponder") {
            // Ponder mode - search until stop
            limits.infinite = true;
        }
    }

    // Reset stop flag and start search in new thread
    stopSearch = false;
    Boudica::stop_search = false;

    searchThread = std::thread([this, limits]() {
        Move bestMove = MOVE_NONE;
        Move ponderMove = MOVE_NONE;

        {
            // Call the iterative deepening search
            bestMove = Boudica::iterative_deepening(
                const_cast<Boudica::Position&>(position),
                const_cast<Boudica::SearchLimits&>(limits)
            );

            // Output best move (ponder move would come from PV[1] if available)
            if (Boudica::search_info.pv_length > 1) {
                ponderMove = Boudica::search_info.pv[1];
            }
        }
        sendBestMove(bestMove, ponderMove);
    });
}

void Handler::cmdStop() {
    // Signal search to stop
    stopSearch = true;
    Boudica::stop_search = true;

    // Wait for search thread to finish
    if (searchThread.joinable()) {
        searchThread.join();
    }
}

void Handler::cmdSetOption(std::istringstream& is) {
    std::string token, name, value;

    // Parse "name <name> [value <value>]"
    is >> token;  // Should be "name"
    if (token != "name") return;

    // Read option name (may contain spaces)
    while (is >> token && token != "value") {
        if (!name.empty()) name += " ";
        name += token;
    }

    // Read value if present
    if (token == "value") {
        std::getline(is, value);
        // Trim leading whitespace
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos) {
            value = value.substr(start);
        }
    }

    // Find and update the option
    Option* opt = findOption(name);
    if (!opt) return;

    // Handle button type (no value needed)
    if (opt->type == OPT_BUTTON) {
        if (name == "Clear Hash") {
            transpositionTable.clear();
        }
        return;
    }

    // Validate and set value
    if (opt->type == OPT_SPIN) {
        int val = std::stoi(value);
        val = std::max(opt->min, std::min(opt->max, val));
        opt->currentValue = std::to_string(val);

        // Apply hash size change
        if (name == "Hash") {
            transpositionTable.resize(static_cast<size_t>(val));
        }
    }
    else if (opt->type == OPT_CHECK) {
        opt->currentValue = (value == "true") ? "true" : "false";
    }
    else {
        opt->currentValue = value;
    }
}

void Handler::cmdQuit() {
    cmdStop();
    // Exit will happen after loop() returns
}

// ============================================================================
// Debug Commands
// ============================================================================

void Handler::cmdDisplay() {
    position.print();
    std::cout << "Fen: " << position.fen() << std::endl;
    std::cout << "Key: " << std::hex << position.key() << std::dec << std::endl;
}

uint64_t perft(Boudica::Position& pos, int depth) {
    using namespace Boudica;

    if (depth == 0) return 1;

    uint64_t nodes = 0;
    MoveList<LEGAL> moves(pos);

    for (const auto& m : moves) {
        StateInfo st;
        pos.do_move(m.move, st);
        nodes += perft(pos, depth - 1);
        pos.undo_move(m.move);
    }
    return nodes;
}

void Handler::cmdPerft(std::istringstream& is) {
    using namespace Boudica;

    int depth = 6;
    is >> depth;

    std::cout << "Perft " << depth << std::endl;

    auto start = std::chrono::steady_clock::now();

    uint64_t total = 0;
    MoveList<LEGAL> moves(position);

    for (const auto& m : moves) {
        StateInfo st;
        position.do_move(m.move, st);
        uint64_t cnt = perft(position, depth - 1);
        position.undo_move(m.move);
        std::cout << moveToUCI(m.move) << ": " << cnt << std::endl;
        total += cnt;
    }

    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << std::endl;
    std::cout << "Nodes: " << total << std::endl;
    std::cout << "Time: " << ms << " ms" << std::endl;
    if (ms > 0) std::cout << "NPS: " << (total * 1000 / ms) << std::endl;
}

void Handler::cmdBench() {
    std::cout << "Benchmark not yet implemented" << std::endl;
}

// ============================================================================
// Helper Methods
// ============================================================================

Option* Handler::findOption(const std::string& name) {
    for (auto& opt : options) {
        if (opt.name == name) return &opt;
    }
    return nullptr;
}

const Option* Handler::findOption(const std::string& name) const {
    for (const auto& opt : options) {
        if (opt.name == name) return &opt;
    }
    return nullptr;
}

int Handler::getOptionInt(const std::string& name) const {
    const Option* opt = findOption(name);
    return opt ? opt->intValue() : 0;
}

bool Handler::getOptionBool(const std::string& name) const {
    const Option* opt = findOption(name);
    return opt ? opt->boolValue() : false;
}

std::string Handler::getOptionString(const std::string& name) const {
    const Option* opt = findOption(name);
    return opt ? opt->currentValue : "";
}

// ============================================================================
// Static Output Functions
// ============================================================================

void Handler::sendInfo(const SearchInfo& info) {
    std::cout << "info";
    std::cout << " depth " << info.depth;

    if (info.seldepth > 0) {
        std::cout << " seldepth " << info.seldepth;
    }

    if (info.isMate) {
        std::cout << " score mate " << info.mateIn;
    }
    else {
        std::cout << " score cp " << info.score;
    }

    if (info.nodes > 0) {
        std::cout << " nodes " << info.nodes;
    }

    if (info.nps > 0) {
        std::cout << " nps " << info.nps;
    }

    if (info.time > 0) {
        std::cout << " time " << info.time;
    }

    if (info.hashfull > 0) {
        std::cout << " hashfull " << info.hashfull;
    }

    // Output PV
    if (!info.pv.empty()) {
        std::cout << " pv";
        for (Move m : info.pv) {
            std::cout << " " << moveToUCI(m);
        }
    }

    std::cout << std::endl;
}

void Handler::sendBestMove(Move bestMove, Move ponderMove) {
    std::cout << "bestmove " << moveToUCI(bestMove);

    if (ponderMove != MOVE_NONE) {
        std::cout << " ponder " << moveToUCI(ponderMove);
    }

    std::cout << std::endl;
}

void Handler::sendString(const std::string& str) {
    std::cout << "info string " << str << std::endl;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string moveToUCI(Move m) {
    if (m == MOVE_NONE) return "0000";
    if (m == MOVE_NULL) return "0000";

    Square from = from_sq(m);
    Square to = to_sq(m);

    std::string result = square_to_string(from) + square_to_string(to);

    if (type_of(m) == PROMOTION) {
        switch (promotion_type(m)) {
            case QUEEN:  result += 'q'; break;
            case ROOK:   result += 'r'; break;
            case BISHOP: result += 'b'; break;
            case KNIGHT: result += 'n'; break;
            default: break;
        }
    }

    return result;
}

Move uciToMove(const Boudica::Position& pos, const std::string& str) {
    if (str.length() < 4) return MOVE_NONE;

    File fromFile = File(str[0] - 'a');
    Rank fromRank = Rank(str[1] - '1');
    File toFile = File(str[2] - 'a');
    Rank toRank = Rank(str[3] - '1');

    // Validate bounds
    if (fromFile < FILE_A || fromFile > FILE_H ||
        fromRank < RANK_1 || fromRank > RANK_8 ||
        toFile < FILE_A || toFile > FILE_H ||
        toRank < RANK_1 || toRank > RANK_8) {
        return MOVE_NONE;
    }

    Square from = make_square(fromFile, fromRank);
    Square to = make_square(toFile, toRank);

    PieceType promotion = NO_PIECE_TYPE;
    if (str.length() >= 5) {
        switch (str[4]) {
            case 'q': promotion = QUEEN; break;
            case 'r': promotion = ROOK; break;
            case 'b': promotion = BISHOP; break;
            case 'n': promotion = KNIGHT; break;
            default: break;
        }
    }

    // Generate legal moves and find matching one
    Boudica::MoveList<Boudica::LEGAL> moves(pos);

    for (const auto& em : moves) {
        Move m = em.move;
        if (from_sq(m) == from && to_sq(m) == to) {
            if (type_of(m) == PROMOTION) {
                if (promotion_type(m) == promotion) return m;
            }
            else if (promotion == NO_PIECE_TYPE) {
                return m;
            }
        }
    }

    return MOVE_NONE;
}

std::string scoreToUCI(Value score) {
    std::ostringstream ss;

    if (std::abs(score) >= VALUE_MATE_IN_MAX_PLY) {
        int mateIn = (score > 0)
            ? (VALUE_MATE - score + 1) / 2
            : -(VALUE_MATE + score) / 2;
        ss << "mate " << mateIn;
    }
    else {
        ss << "cp " << score;
    }

    return ss.str();
}

std::string pvToUCI(const Move* pv, int length) {
    std::ostringstream ss;
    for (int i = 0; i < length; ++i) {
        if (i > 0) ss << " ";
        ss << moveToUCI(pv[i]);
    }
    return ss.str();
}

std::string pvToUCI(const std::vector<Move>& pv) {
    std::ostringstream ss;
    for (size_t i = 0; i < pv.size(); ++i) {
        if (i > 0) ss << " ";
        ss << moveToUCI(pv[i]);
    }
    return ss.str();
}

} // namespace UCI
