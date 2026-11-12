#ifndef EVAL_PARAMS_H
#define EVAL_PARAMS_H

#include "types.h"

#include <cstddef>  // for offsetof

namespace Boudica {

namespace Eval {

// ============================================================================
// Tunable Evaluation Parameters
// ============================================================================
//
// Every linear term of the handcrafted evaluation lives here so the Texel
// tuner (src/tuner/) can change it at runtime; the engine reads Eval::params,
// which starts from these defaults. Each Score is added once per occurrence of
// its feature, so the evaluation is linear in these values apart from the
// king-attack penalty, the endgame scale factors and integer rounding.
//
// The tuner writes this struct body in exactly this format: replace everything
// between the BEGIN/END markers with its output. Piece-square tables are
// indexed A1..H8 from White's point of view and are added on top of material.

struct EvalParams {
    // BEGIN TUNED PARAMETERS
    // Material
    Score PieceValue[PIECE_TYPE_NB] = { S(0, 0), S(97, 119), S(317, 308), S(327, 337), S(496, 519), S(895, 903), S(0, 0), S(0, 0) };
    Score BishopPairBonus = S(33, 54);

    // Pawn structure
    Score DoubledPawnPenalty = S(-14, -17);
    Score IsolatedPawnPenalty = S(-13, -7);
    Score BackwardPawnPenalty = S(-10, -8);
    Score ConnectedPawnBonus = S(2, 5);

    // Passed pawns, indexed by relative rank
    Score PassedPawnBonus[RANK_NB] = { S(0, 0), S(2, 7), S(7, 17), S(17, 37), S(38, 73), S(63, 123), S(103, 203), S(0, 0) };
    Score BlockedPasserPenalty[RANK_NB] = { S(0, 0), S(-1, -3), S(-3, -8), S(-8, -18), S(-19, -36), S(-31, -61), S(-51, -101), S(0, 0) };
    Score ProtectedPasserBonus = S(10, 15);
    Score PasserEnemyKingDistance = S(0, 3);
    Score PasserOwnKingProximity = S(0, 2);
    Score RookBehindPasserBonus = S(15, 25);

    // King safety
    Score PawnShieldBonus = S(7, -3);
    Score PawnShield2Bonus = S(5, 0);
    Score OpenFileNearKingPenalty = S(-23, -8);
    Score SemiOpenFileNearKingPenalty = S(-7, 1);

    // Mobility, per square not attacked by enemy pawns
    Score KnightMobility = S(1, 2);
    Score BishopMobility = S(2, 2);
    Score RookMobility = S(5, 6);
    Score QueenMobility = S(4, 6);

    // Rooks
    Score RookOnOpenFile = S(22, 17);
    Score RookOnSemiOpenFile = S(13, 10);
    Score RookOn7thRank = S(33, 43);

    // Piece-square tables
    Score PawnTable[SQUARE_NB] = {
        // Rank 1
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
        // Rank 2
        S(-5, 0),     S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(-5, 0),
        // Rank 3
        S(-5, 0),     S(0, 5),      S(5, 5),      S(10, 5),     S(10, 5),     S(5, 5),      S(0, 5),      S(-5, 0),
        // Rank 4
        S(-5, 5),     S(0, 10),     S(10, 10),    S(25, 15),    S(25, 15),    S(10, 10),    S(0, 10),     S(-5, 5),
        // Rank 5
        S(0, 15),     S(5, 20),     S(15, 25),    S(30, 30),    S(30, 30),    S(15, 25),    S(5, 20),     S(0, 15),
        // Rank 6
        S(5, 35),     S(10, 40),    S(20, 45),    S(35, 50),    S(35, 50),    S(20, 45),    S(10, 40),    S(5, 35),
        // Rank 7
        S(50, 70),    S(50, 70),    S(50, 70),    S(50, 70),    S(50, 70),    S(50, 70),    S(50, 70),    S(50, 70),
        // Rank 8
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
    };

    Score KnightTable[SQUARE_NB] = {
        // Rank 1
        S(-50, -30),  S(-40, -20),  S(-30, -10),  S(-25, -5),   S(-25, -5),   S(-30, -10),  S(-40, -20),  S(-50, -30),
        // Rank 2
        S(-35, -20),  S(-20, -10),  S(-5, 0),     S(0, 5),      S(0, 5),      S(-5, 0),     S(-20, -10),  S(-35, -20),
        // Rank 3
        S(-20, -10),  S(0, 0),      S(15, 10),    S(20, 15),    S(20, 15),    S(15, 10),    S(0, 0),      S(-20, -10),
        // Rank 4
        S(-10, -5),   S(10, 5),     S(25, 15),    S(30, 20),    S(30, 20),    S(25, 15),    S(10, 5),     S(-10, -5),
        // Rank 5
        S(-10, -5),   S(10, 5),     S(25, 15),    S(30, 20),    S(30, 20),    S(25, 15),    S(10, 5),     S(-10, -5),
        // Rank 6
        S(-20, -10),  S(5, 5),      S(20, 10),    S(25, 15),    S(25, 15),    S(20, 10),    S(5, 5),      S(-20, -10),
        // Rank 7
        S(-35, -20),  S(-15, -10),  S(0, 0),      S(5, 5),      S(5, 5),      S(0, 0),      S(-15, -10),  S(-35, -20),
        // Rank 8
        S(-50, -30),  S(-40, -20),  S(-30, -10),  S(-25, -5),   S(-25, -5),   S(-30, -10),  S(-40, -20),  S(-50, -30),
    };

    Score BishopTable[SQUARE_NB] = {
        // Rank 1
        S(-20, -15),  S(-10, -10),  S(-15, -10),  S(-10, -5),   S(-10, -5),   S(-15, -10),  S(-10, -10),  S(-20, -15),
        // Rank 2
        S(-10, -10),  S(5, 5),      S(0, 0),      S(5, 5),      S(5, 5),      S(0, 0),      S(5, 5),      S(-10, -10),
        // Rank 3
        S(-5, -5),    S(10, 5),     S(10, 10),    S(15, 10),    S(15, 10),    S(10, 10),    S(10, 5),     S(-5, -5),
        // Rank 4
        S(0, 0),      S(5, 5),      S(15, 10),    S(20, 15),    S(20, 15),    S(15, 10),    S(5, 5),      S(0, 0),
        // Rank 5
        S(0, 0),      S(10, 10),    S(15, 10),    S(20, 15),    S(20, 15),    S(15, 10),    S(10, 10),    S(0, 0),
        // Rank 6
        S(-5, -5),    S(10, 5),     S(10, 10),    S(10, 10),    S(10, 10),    S(10, 10),    S(10, 5),     S(-5, -5),
        // Rank 7
        S(-10, -10),  S(5, 0),      S(0, 0),      S(0, 5),      S(0, 5),      S(0, 0),      S(5, 0),      S(-10, -10),
        // Rank 8
        S(-20, -15),  S(-10, -10),  S(-15, -10),  S(-10, -5),   S(-10, -5),   S(-15, -10),  S(-10, -10),  S(-20, -15),
    };

    Score RookTable[SQUARE_NB] = {
        // Rank 1
        S(0, 0),      S(5, 0),      S(10, 0),     S(15, 0),     S(15, 0),     S(10, 0),     S(5, 0),      S(0, 0),
        // Rank 2
        S(-5, 0),     S(0, 0),      S(0, 0),      S(5, 0),      S(5, 0),      S(0, 0),      S(0, 0),      S(-5, 0),
        // Rank 3
        S(-5, 0),     S(0, 0),      S(0, 5),      S(5, 5),      S(5, 5),      S(0, 5),      S(0, 0),      S(-5, 0),
        // Rank 4
        S(-5, 5),     S(0, 5),      S(0, 10),     S(5, 10),     S(5, 10),     S(0, 10),     S(0, 5),      S(-5, 5),
        // Rank 5
        S(-5, 10),    S(0, 10),     S(0, 15),     S(5, 15),     S(5, 15),     S(0, 15),     S(0, 10),     S(-5, 10),
        // Rank 6
        S(-5, 15),    S(0, 15),     S(0, 20),     S(0, 20),     S(0, 20),     S(0, 20),     S(0, 15),     S(-5, 15),
        // Rank 7
        S(20, 30),    S(25, 30),    S(30, 30),    S(35, 30),    S(35, 30),    S(30, 30),    S(25, 30),    S(20, 30),
        // Rank 8
        S(10, 20),    S(15, 20),    S(20, 25),    S(25, 25),    S(25, 25),    S(20, 25),    S(15, 20),    S(10, 20),
    };

    Score QueenTable[SQUARE_NB] = {
        // Rank 1
        S(-20, -15),  S(-10, -10),  S(-5, -5),    S(0, 0),      S(0, 0),      S(-5, -5),    S(-10, -10),  S(-20, -15),
        // Rank 2
        S(-10, -10),  S(0, 0),      S(5, 5),      S(5, 5),      S(5, 5),      S(5, 5),      S(0, 0),      S(-10, -10),
        // Rank 3
        S(-5, -5),    S(5, 5),      S(10, 10),    S(10, 10),    S(10, 10),    S(10, 10),    S(5, 5),      S(-5, -5),
        // Rank 4
        S(0, 0),      S(5, 5),      S(10, 10),    S(15, 15),    S(15, 15),    S(10, 10),    S(5, 5),      S(0, 0),
        // Rank 5
        S(0, 0),      S(5, 5),      S(10, 10),    S(15, 15),    S(15, 15),    S(10, 10),    S(5, 5),      S(0, 0),
        // Rank 6
        S(-5, 0),     S(5, 5),      S(10, 10),    S(10, 10),    S(10, 10),    S(10, 10),    S(5, 5),      S(-5, 0),
        // Rank 7
        S(-10, -5),   S(0, 0),      S(5, 5),      S(5, 5),      S(5, 5),      S(5, 5),      S(0, 0),      S(-10, -5),
        // Rank 8
        S(-20, -10),  S(-10, -5),   S(-5, 0),     S(0, 0),      S(0, 0),      S(-5, 0),     S(-10, -5),   S(-20, -10),
    };

    Score KingTable[SQUARE_NB] = {
        // Rank 1
        S(20, -30),   S(30, -20),   S(10, -10),   S(-20, -5),   S(-20, -5),   S(-10, -10),  S(30, -20),   S(20, -30),
        // Rank 2
        S(20, -20),   S(20, -5),    S(0, 5),      S(-20, 10),   S(-20, 10),   S(0, 5),      S(20, -5),    S(20, -20),
        // Rank 3
        S(-10, -10),  S(-20, 5),    S(-30, 15),   S(-40, 20),   S(-40, 20),   S(-30, 15),   S(-20, 5),    S(-10, -10),
        // Rank 4
        S(-30, -5),   S(-40, 10),   S(-50, 20),   S(-60, 25),   S(-60, 25),   S(-50, 20),   S(-40, 10),   S(-30, -5),
        // Rank 5
        S(-40, -5),   S(-50, 10),   S(-60, 20),   S(-70, 25),   S(-70, 25),   S(-60, 20),   S(-50, 10),   S(-40, -5),
        // Rank 6
        S(-50, -10),  S(-60, 5),    S(-70, 15),   S(-80, 20),   S(-80, 20),   S(-70, 15),   S(-60, 5),    S(-50, -10),
        // Rank 7
        S(-60, -20),  S(-70, -5),   S(-80, 5),    S(-90, 10),   S(-90, 10),   S(-80, 5),    S(-70, -5),   S(-60, -20),
        // Rank 8
        S(-70, -30),  S(-80, -20),  S(-90, -10),  S(-100, -5),  S(-100, -5),  S(-90, -10),  S(-80, -20),  S(-70, -30),
    };

    // Side to move bonus, added after tapering
    int Tempo = 13;
    // END TUNED PARAMETERS
};

extern EvalParams params;

// Flat index of a Score parameter, for the evaluation trace: EvalParams is
// laid out as consecutive Scores followed by Tempo
#define EVAL_PARAM_INDEX(member) int(offsetof(Boudica::Eval::EvalParams, member) / sizeof(::Score))

constexpr int NUM_SCORE_PARAMS = EVAL_PARAM_INDEX(Tempo);

static_assert(sizeof(Score) == 2 * sizeof(int), "Score must be two packed ints");
static_assert(offsetof(EvalParams, Tempo) == NUM_SCORE_PARAMS * sizeof(Score),
              "EvalParams must be an unpadded run of Scores before Tempo");

} // namespace Eval

} // namespace Boudica

#endif // EVAL_PARAMS_H
