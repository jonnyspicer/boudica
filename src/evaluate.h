#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "eval_params.h"

// Forward declaration
class Position;

namespace Boudica {

namespace Eval {

// Main evaluation function - returns score in centipawns from side-to-move perspective
Value evaluate(const Position& pos);

// Per-position record of how the evaluation used each tunable parameter, for
// the Texel tuner. From White's point of view, for the current params:
//   v = taper(sum(coef[i] * score_param[i]) + nonlinear, phase)
//   v = (v + tempo * params.Tempo) * scale / 128
// reproduces evaluate() exactly (negated when Black is to move).
struct EvalTrace {
    int coef[NUM_SCORE_PARAMS] = {};  // White occurrences minus Black occurrences
    int tempo = 0;                    // +1 if White is to move, -1 if Black
    int phase = 0;                    // 0 (endgame) .. 256 (middlegame)
    int scale = 128;                  // Endgame scale factor out of 128
    Score nonlinear;                  // Untuned king-attack penalty, untapered
};

// Evaluate while filling a trace. Returns the same value as evaluate(), but
// bypasses the pawn hash so the pawn-structure terms are always recorded.
Value evaluate_trace(const Position& pos, EvalTrace& trace);

// Phase calculation constants
constexpr int PHASE_MIDGAME = 256;
constexpr int PHASE_ENDGAME = 0;

// Phase weights for each piece type (used to determine game phase)
constexpr int PhaseWeights[PIECE_TYPE_NB] = {
    0,    // NO_PIECE_TYPE
    0,    // PAWN (not counted in phase)
    1,    // KNIGHT
    1,    // BISHOP
    2,    // ROOK
    4,    // QUEEN
    0,    // KING (not counted in phase)
    0     // ALL_PIECES
};

// Total phase at game start: 2*1 + 2*1 + 2*2 + 1*4 = 2 + 2 + 4 + 4 = 12 per side = 24 total
constexpr int TOTAL_PHASE = 24;

// Tuned evaluation terms live in Eval::params (eval_params.h). The constants
// below are not used by the evaluation yet.

// Trapped piece penalties
constexpr Score TrappedRookPenalty = S(-50, -30);
constexpr Score TrappedBishopPenalty = S(-40, -20);

// Outpost bonuses - pieces on rank 4-6 that can't be attacked by enemy pawns
constexpr Score KnightOutpostBonus = S(25, 15);  // Knights benefit most from outposts
constexpr Score BishopOutpostBonus = S(15, 10);  // Bishops also benefit but less

// Weak squares / holes penalty
constexpr Score WeakSquarePenalty = S(-5, -3);

// Initialize evaluation tables
void init();

} // namespace Eval

} // namespace Boudica

#endif // EVALUATE_H
