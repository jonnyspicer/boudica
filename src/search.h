#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <cstring>

// Forward declarations (implemented in other modules)
class Position;
class TranspositionTable;

namespace Boudica {

// ============================================================================
// Search Limits - Controls when to stop searching
// ============================================================================
struct SearchLimits {
    int depth = 0;              // Maximum depth to search (0 = unlimited)
    uint64_t nodes = 0;         // Maximum nodes to search (0 = unlimited)
    int time[COLOR_NB] = {};    // Time remaining in ms for each side
    int inc[COLOR_NB] = {};     // Increment per move in ms
    int movetime = 0;           // Exact time to search in ms
    int movestogo = 0;          // Moves until next time control
    bool infinite = false;      // Search until "stop" command
    Move searchmoves[MAX_MOVES] = {}; // Restrict search to these moves
    int searchmoves_count = 0;
};

// ============================================================================
// Search Info - Statistics and results from search
// ============================================================================
struct SearchInfo {
    uint64_t nodes = 0;         // Total nodes searched
    int depth = 0;              // Current/completed depth
    int seldepth = 0;           // Selective (max) depth reached
    Value score = VALUE_NONE;   // Best score found
    Move pv[MAX_PLY] = {};      // Principal variation
    int pv_length = 0;          // Length of PV
    uint64_t time_ms = 0;       // Time spent in ms

    void clear() {
        nodes = 0;
        depth = 0;
        seldepth = 0;
        score = VALUE_NONE;
        pv_length = 0;
        time_ms = 0;
    }
};

// ============================================================================
// Move Ordering - Killer moves and history heuristic
// ============================================================================
constexpr int KILLERS_PER_PLY = 2;

struct MoveOrderingTables {
    // Killer moves: quiet moves that caused beta cutoffs
    Move killers[MAX_PLY][KILLERS_PER_PLY];

    // History heuristic: indexed by [color][from][to]
    int history[COLOR_NB][SQUARE_NB][SQUARE_NB];

    // Counter move heuristic: indexed by [piece][to_square]
    Move countermoves[PIECE_NB][SQUARE_NB];

    // Continuation history: indexed by [prev_piece][prev_to][curr_piece][curr_to]
    // Tracks piece-to-square correlations across consecutive moves
    int continuation_history[PIECE_NB][SQUARE_NB][PIECE_NB][SQUARE_NB];

    void clear() {
        std::memset(killers, 0, sizeof(killers));
        std::memset(history, 0, sizeof(history));
        std::memset(countermoves, 0, sizeof(countermoves));
        std::memset(continuation_history, 0, sizeof(continuation_history));
    }

    // Age the tables between searches - decay history by 50%, clear killers
    // This preserves learned patterns while allowing adaptation
    void age() {
        // Clear killers - they're ply-specific and don't transfer well
        std::memset(killers, 0, sizeof(killers));

        // Decay history scores by 50% (right shift by 1)
        // This keeps valuable patterns while allowing new patterns to emerge
        for (int c = 0; c < COLOR_NB; ++c) {
            for (int from = 0; from < SQUARE_NB; ++from) {
                for (int to = 0; to < SQUARE_NB; ++to) {
                    history[c][from][to] /= 2;
                }
            }
        }

        // Decay continuation history by 50%
        for (int p1 = 0; p1 < PIECE_NB; ++p1) {
            for (int s1 = 0; s1 < SQUARE_NB; ++s1) {
                for (int p2 = 0; p2 < PIECE_NB; ++p2) {
                    for (int s2 = 0; s2 < SQUARE_NB; ++s2) {
                        continuation_history[p1][s1][p2][s2] /= 2;
                    }
                }
            }
        }

        // Keep countermoves - they're generally position-independent
        // and valuable across moves
    }

    void update_killers(Move m, int ply) {
        if (ply < 0 || ply >= MAX_PLY) return;  // Bounds check
        if (killers[ply][0] != m) {
            killers[ply][1] = killers[ply][0];
            killers[ply][0] = m;
        }
    }

    void update_history(Color c, Move m, int bonus) {
        Square from = from_sq(m);
        Square to = to_sq(m);

        // Gravity: reduce magnitude over time to prevent overflow
        int clamped_bonus = std::min(std::max(bonus, -16384), 16384);
        history[c][from][to] += clamped_bonus - history[c][from][to] * std::abs(clamped_bonus) / 16384;
    }

    bool is_killer(Move m, int ply) const {
        if (ply < 0 || ply >= MAX_PLY) return false;  // Bounds check
        return m == killers[ply][0] || m == killers[ply][1];
    }

    int get_history(Color c, Move m) const {
        return history[c][from_sq(m)][to_sq(m)];
    }

    // Get continuation history score for a move given previous move's piece/square
    int get_continuation_history(Piece prev_piece, Square prev_to, Piece curr_piece, Square curr_to) const {
        if (prev_piece == NO_PIECE || curr_piece == NO_PIECE) return 0;
        return continuation_history[prev_piece][prev_to][curr_piece][curr_to];
    }

    // Update continuation history
    void update_continuation_history(Piece prev_piece, Square prev_to, Piece curr_piece, Square curr_to, int bonus) {
        if (prev_piece == NO_PIECE || curr_piece == NO_PIECE) return;
        int& entry = continuation_history[prev_piece][prev_to][curr_piece][curr_to];
        int clamped_bonus = std::min(std::max(bonus, -16384), 16384);
        entry += clamped_bonus - entry * std::abs(clamped_bonus) / 16384;
    }
};

// ============================================================================
// Move Scoring Constants for Ordering
// ============================================================================
constexpr int SCORE_TT_MOVE      = 10000000;  // TT move first
constexpr int SCORE_GOOD_CAPTURE = 8000000;   // Winning captures (MVV-LVA)
constexpr int SCORE_KILLER_1     = 6000000;   // First killer move
constexpr int SCORE_KILLER_2     = 5000000;   // Second killer move
constexpr int SCORE_COUNTERMOVE  = 4000000;   // Counter move heuristic
constexpr int SCORE_QUIET_BASE   = 0;         // Quiet moves (history added)
constexpr int SCORE_BAD_CAPTURE  = -2000000;  // Losing captures

// MVV-LVA (Most Valuable Victim - Least Valuable Attacker) table
// Index: [victim][attacker], higher score = search first
constexpr int MVV_LVA[PIECE_TYPE_NB][PIECE_TYPE_NB] = {
    // victim:   x  P   N   B   R   Q   K
    /* x    */ { 0, 0,  0,  0,  0,  0,  0  },
    /* Pawn */ { 0, 15, 14, 13, 12, 11, 10 },
    /* Knight */{ 0, 25, 24, 23, 22, 21, 20 },
    /* Bishop */{ 0, 35, 34, 33, 32, 31, 30 },
    /* Rook */ { 0, 45, 44, 43, 42, 41, 40 },
    /* Queen */{ 0, 55, 54, 53, 52, 51, 50 },
    /* King */ { 0, 0,  0,  0,  0,  0,  0  }
};

// ============================================================================
// Search Stack Entry - Per-ply search state
// ============================================================================
struct SearchStack {
    Move current_move = MOVE_NONE;
    Move excluded_move = MOVE_NONE;
    Move killers[2] = {MOVE_NONE, MOVE_NONE};
    int ply = 0;
    int static_eval = VALUE_NONE;
    int move_count = 0;
    bool in_check = false;
    bool tt_hit = false;
    Move tt_move = MOVE_NONE;
    Piece moved_piece = NO_PIECE;    // For continuation history
    Square to_square = SQ_NONE;      // For continuation history
    int* cont_history = nullptr;     // Pointer to continuation history entry
    Move pv[MAX_PLY];               // Triangular PV table
    int pv_length = 0;
};

// ============================================================================
// Scored Move - Move with assigned priority score
// ============================================================================
struct ScoredMove {
    Move move;
    int score;

    bool operator<(const ScoredMove& other) const {
        return score > other.score;  // Higher scores first
    }
};

// ============================================================================
// Time Manager - Controls search time allocation
// ============================================================================
class TimeManager {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    void init(const SearchLimits& limits, Color us);
    bool should_stop(uint64_t nodes) const;
    uint64_t elapsed() const;

    void set_start_time() { start_time = Clock::now(); }
    uint64_t get_allocated_time() const { return allocated_time; }

private:
    TimePoint start_time;
    uint64_t allocated_time = 0;     // Time allocated for this move (ms)
    uint64_t max_time = 0;           // Maximum time (hard limit)
    uint64_t check_interval = 512;  // Check time every N nodes (more frequent for short TC)
    mutable uint64_t last_check = 0;
};

// ============================================================================
// Global Search State
// ============================================================================
extern std::atomic<bool> stop_search;  // Set by UCI "stop" command
extern MoveOrderingTables move_ordering;
extern SearchInfo search_info;

// ============================================================================
// Main Search Functions
// ============================================================================

// Main entry point - called from UCI "go" command
// Returns the best move found
Move iterative_deepening(Position& pos, SearchLimits& limits);

// Recursive alpha-beta search with fail-soft
// Returns exact score (not just bound)
Value search(Position& pos, SearchStack* ss, int depth, Value alpha, Value beta, bool cut_node);

// Quiescence search - extends tactical sequences until quiet
Value qsearch(Position& pos, SearchStack* ss, Value alpha, Value beta);

// ============================================================================
// Helper Functions
// ============================================================================

// Initialize search state before new search
void init_search();

// Clear all search tables (call on "ucinewgame")
void clear_search();

// Score moves for ordering
void score_moves(Position& pos, ScoredMove* moves, int move_count,
                 Move tt_move, int ply, Move prev_move);

// Pick the next best move from the list
Move pick_next_move(ScoredMove* moves, int move_count, int current);

// Print UCI info string
void print_uci_info(const Position& pos, int depth, Value score,
                    const Move* pv, int pv_length, uint64_t nodes, uint64_t time_ms);

// Convert PV to UCI string format
std::string pv_to_string(const Move* pv, int length);

// Check if position is a draw (50-move, repetition, insufficient material)
// ply parameter is required for correct repetition detection within search tree
bool is_draw(const Position& pos, int ply);

// Mate distance pruning helper
Value mated_in(int ply);
Value mate_in(int ply);

// ============================================================================
// Null Move Pruning Parameters
// ============================================================================
constexpr int NULL_MOVE_REDUCTION = 3;  // R value for null move pruning (more aggressive)
constexpr int NULL_MOVE_MIN_DEPTH = 3;  // Minimum depth to try null move

// ============================================================================
// Late Move Reductions Parameters
// ============================================================================
constexpr int LMR_MIN_DEPTH = 3;        // Minimum depth for LMR
constexpr int LMR_FULL_MOVES = 4;       // Search first N moves at full depth

// LMR reduction table (depth, move_number)
extern int LMR_TABLE[MAX_PLY][MAX_MOVES];
void init_lmr_table();

// ============================================================================
// Delta Pruning (for quiescence search)
// ============================================================================
constexpr Value DELTA_MARGIN = Value(200);  // ~2 pawns margin
constexpr int QSEARCH_MAX_PLY = 8;          // Limit qsearch depth to prevent explosions

// ============================================================================
// Aspiration Window Parameters
// ============================================================================
constexpr Value ASPIRATION_WINDOW = Value(25);
constexpr Value ASPIRATION_MAX_DELTA = Value(500);

// ============================================================================
// Futility Pruning - More aggressive at deeper depths
// ============================================================================
constexpr int FUTILITY_MAX_DEPTH = 4;
constexpr Value FUTILITY_MARGIN[5] = { Value(0), Value(150), Value(250), Value(350), Value(450) };

// ============================================================================
// Late Move Pruning - Prune quiet moves after searching N moves
// ============================================================================
constexpr int LMP_MAX_DEPTH = 5;
constexpr int LMP_MOVE_COUNT[6] = { 0, 8, 12, 16, 20, 24 };

// ============================================================================
// Razoring - Drop into qsearch when static eval is far below alpha
// ============================================================================
constexpr int RAZOR_MAX_DEPTH = 3;
constexpr Value RAZOR_MARGIN_BASE = Value(200);
constexpr Value RAZOR_MARGIN_PER_DEPTH = Value(200);

} // namespace Boudica

#endif // SEARCH_H
