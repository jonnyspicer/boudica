#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"

// Forward declaration
class Position;

namespace Boudica {

namespace Eval {

// Main evaluation function - returns score in centipawns from side-to-move perspective
Value evaluate(const Position& pos);

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

// Material values (middlegame, endgame) - TUNED
constexpr Score PieceValue[PIECE_TYPE_NB] = {
    S(0, 0),       // NO_PIECE_TYPE
    S(97, 119),    // PAWN - tuned
    S(317, 308),   // KNIGHT - tuned
    S(327, 337),   // BISHOP - tuned
    S(496, 519),   // ROOK - tuned
    S(895, 903),   // QUEEN - tuned
    S(0, 0),       // KING
    S(0, 0)        // ALL_PIECES
};

// Bishop pair bonus - TUNED
constexpr Score BishopPairBonus = S(33, 54);

// Pawn structure bonuses/penalties - TUNED
constexpr Score DoubledPawnPenalty = S(-14, -17);
constexpr Score IsolatedPawnPenalty = S(-13, -7);
constexpr Score BackwardPawnPenalty = S(-10, -8);
constexpr Score ConnectedPawnBonus = S(2, 5);

// Passed pawn bonuses by rank (from perspective of advancing color) - TUNED
constexpr Score PassedPawnBonus[RANK_NB] = {
    S(0, 0),       // RANK_1 (impossible)
    S(2, 7),       // RANK_2 - tuned
    S(7, 17),      // RANK_3 - tuned
    S(17, 37),     // RANK_4 - tuned
    S(38, 73),     // RANK_5 - tuned
    S(63, 123),    // RANK_6 - tuned
    S(103, 203),   // RANK_7 - tuned
    S(0, 0)        // RANK_8 (would be promoted)
};

// King safety - TUNED
constexpr Score PawnShieldBonus = S(7, -3);
constexpr Score OpenFileNearKingPenalty = S(-23, -8);
constexpr Score SemiOpenFileNearKingPenalty = S(-7, 1);

// Mobility bonuses per square controlled - TUNED
constexpr Score KnightMobility = S(1, 2);
constexpr Score BishopMobility = S(2, 2);
constexpr Score RookMobility = S(5, 6);
constexpr Score QueenMobility = S(4, 6);

// Rook bonuses - TUNED
constexpr Score RookOnOpenFile = S(22, 17);
constexpr Score RookOnSemiOpenFile = S(13, 10);
constexpr Score RookOn7thRank = S(33, 43);

// Trapped piece penalties
constexpr Score TrappedRookPenalty = S(-50, -30);
constexpr Score TrappedBishopPenalty = S(-40, -20);

// Outpost bonuses - pieces on rank 4-6 that can't be attacked by enemy pawns
constexpr Score KnightOutpostBonus = S(25, 15);  // Knights benefit most from outposts
constexpr Score BishopOutpostBonus = S(15, 10);  // Bishops also benefit but less

// Rook behind passed pawn
constexpr Score RookBehindPasserBonus = S(15, 25);

// Weak squares / holes penalty
constexpr Score WeakSquarePenalty = S(-5, -3);

// Initialize evaluation tables
void init();

} // namespace Eval

} // namespace Boudica

#endif // EVALUATE_H
