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
    Score PieceValue[PIECE_TYPE_NB] = { S(0, 0), S(69, 136), S(407, 376), S(415, 398), S(558, 678), S(1782, 971), S(0, 0), S(0, 0) };
    Score BishopPairBonus = S(17, 77);

    // Pawn structure
    Score DoubledPawnPenalty = S(0, -39);
    Score IsolatedPawnPenalty = S(-9, -8);
    Score BackwardPawnPenalty = S(-18, -17);
    Score ConnectedPawnBonus = S(13, 3);

    // Passed pawns, indexed by relative rank
    Score PassedPawnBonus[RANK_NB] = { S(0, 0), S(93, -104), S(76, -74), S(57, -32), S(87, 8), S(132, 48), S(243, 128), S(0, 0) };
    Score BlockedPasserPenalty[RANK_NB] = { S(0, 0), S(8, -8), S(-19, 7), S(-20, -3), S(-27, -14), S(-47, -29), S(-159, -90), S(0, 0) };
    Score ProtectedPasserBonus = S(18, -7);
    Score PasserEnemyKingDistance = S(-7, 13);
    Score PasserOwnKingProximity = S(-10, 11);
    Score RookBehindPasserBonus = S(14, 28);

    // King safety
    Score PawnShieldBonus = S(9, -15);
    Score PawnShield2Bonus = S(4, -10);
    Score OpenFileNearKingPenalty = S(-26, -17);
    Score SemiOpenFileNearKingPenalty = S(-13, 6);

    // Mobility, per square not attacked by enemy pawns
    Score KnightMobility = S(6, 1);
    Score BishopMobility = S(5, 3);
    Score RookMobility = S(2, 7);
    Score QueenMobility = S(3, -1);

    // Rooks
    Score RookOnOpenFile = S(36, -3);
    Score RookOnSemiOpenFile = S(10, 28);
    Score RookOn7thRank = S(20, 30);

    // Piece-square tables
    Score PawnTable[SQUARE_NB] = {
        // Rank 1
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
        // Rank 2
        S(-18, 17),   S(-29, 28),   S(-13, 13),   S(-9, -12),   S(-8, 30),    S(28, 32),    S(32, 14),    S(-2, 3),
        // Rank 3
        S(-20, 18),   S(-23, 24),   S(-15, 1),    S(-5, 4),     S(1, 11),     S(3, 31),     S(17, 10),    S(3, 4),
        // Rank 4
        S(-7, 28),    S(-17, 19),   S(0, 0),      S(12, -7),    S(12, 8),     S(7, 20),     S(-3, 21),    S(-8, 10),
        // Rank 5
        S(-2, 43),    S(-4, 20),    S(-3, 15),    S(9, -12),    S(19, 4),     S(12, 7),     S(-6, 21),    S(3, 14),
        // Rank 6
        S(12, 60),    S(22, 60),    S(30, 46),    S(33, 7),     S(68, -12),   S(59, 19),    S(37, 22),    S(37, 38),
        // Rank 7
        S(151, 11),   S(157, 49),   S(88, 41),    S(135, 2),    S(95, 15),    S(-27, 54),   S(2, 73),     S(106, 2),
        // Rank 8
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
    };

    Score KnightTable[SQUARE_NB] = {
        // Rank 1
        S(-106, -35), S(-13, -36),  S(-19, -5),   S(0, -14),    S(9, -3),     S(9, -21),    S(-6, -74),   S(-98, -112),
        // Rank 2
        S(-21, -19),  S(-18, -2),   S(-9, 5),     S(12, 4),     S(8, 5),      S(18, 14),    S(11, 14),    S(-3, 22),
        // Rank 3
        S(-18, -44),  S(3, 6),      S(-2, 15),    S(22, 40),    S(34, 28),    S(14, 24),    S(25, 17),    S(-15, -24),
        // Rank 4
        S(-4, 16),    S(-17, 28),   S(13, 33),    S(12, 47),    S(28, 49),    S(30, 41),    S(46, 9),     S(14, 14),
        // Rank 5
        S(-4, -11),   S(16, 30),    S(34, 41),    S(52, 45),    S(28, 60),    S(65, 28),    S(18, 32),    S(40, 15),
        // Rank 6
        S(-20, 0),    S(15, 21),    S(17, 46),    S(71, 30),    S(134, 11),   S(111, 11),   S(76, 20),    S(24, -15),
        // Rank 7
        S(-21, 4),    S(19, 1),     S(28, 21),    S(101, 12),   S(57, 25),    S(75, -15),   S(30, -33),   S(55, -65),
        // Rank 8
        S(-100, -56), S(-105, 10),  S(27, -12),   S(36, -6),    S(136, -36),  S(63, -27),   S(59, -38),   S(-93, -126),
    };

    Score BishopTable[SQUARE_NB] = {
        // Rank 1
        S(-4, -41),   S(42, -60),   S(3, -9),     S(-2, -10),   S(-21, 5),    S(-14, 0),    S(47, -42),   S(22, -61),
        // Rank 2
        S(21, -7),    S(21, -14),   S(25, 13),    S(9, 8),      S(20, 7),     S(36, -5),    S(37, -3),    S(49, -78),
        // Rank 3
        S(14, -15),   S(25, 11),    S(28, 19),    S(19, 19),    S(22, 27),    S(33, 15),    S(31, -6),    S(14, 5),
        // Rank 4
        S(21, -5),    S(11, 7),     S(16, 29),    S(36, 25),    S(34, 13),    S(9, 20),     S(2, 32),     S(34, -37),
        // Rank 5
        S(-6, 4),     S(11, 31),    S(26, 18),    S(53, 15),    S(46, 21),    S(54, 11),    S(8, 18),     S(3, 20),
        // Rank 6
        S(6, 17),     S(11, 46),    S(2, 32),     S(33, 20),    S(44, 22),    S(99, 28),    S(69, 14),    S(17, 42),
        // Rank 7
        S(-3, 4),     S(-17, 34),   S(-2, 20),    S(-3, 22),    S(-9, 13),    S(-35, 12),   S(31, 3),     S(-23, -40),
        // Rank 8
        S(38, -14),   S(-10, 25),   S(-37, 35),   S(33, 9),     S(-13, 4),    S(-58, 2),    S(70, 4),     S(-34, 8),
    };

    Score RookTable[SQUARE_NB] = {
        // Rank 1
        S(3, 6),      S(10, -10),   S(7, 9),      S(16, 3),     S(18, -4),    S(10, 4),     S(4, 10),     S(12, -25),
        // Rank 2
        S(-22, -9),   S(-17, 7),    S(0, 15),     S(2, 13),     S(1, -1),     S(9, 10),     S(-3, 6),     S(-49, 20),
        // Rank 3
        S(-45, 43),   S(-15, 25),   S(-20, 31),   S(-20, 24),   S(-16, 26),   S(-9, 17),    S(-1, 10),    S(-30, 21),
        // Rank 4
        S(-22, 38),   S(-17, 48),   S(-11, 37),   S(-8, 42),    S(-4, 35),    S(-23, 49),   S(-10, 39),   S(-18, 42),
        // Rank 5
        S(-8, 62),    S(6, 55),     S(5, 54),     S(36, 38),    S(28, 46),    S(48, 39),    S(21, 50),    S(14, 37),
        // Rank 6
        S(-5, 57),    S(38, 42),    S(33, 48),    S(51, 32),    S(71, 21),    S(84, 38),    S(112, 31),   S(30, 38),
        // Rank 7
        S(-9, 23),    S(-16, 28),   S(17, 19),    S(54, 5),     S(15, 21),    S(30, 16),    S(11, 19),    S(28, 16),
        // Rank 8
        S(34, 17),    S(27, 33),    S(58, 27),    S(64, 26),    S(54, 27),    S(89, 28),    S(130, 11),   S(21, 37),
    };

    Score QueenTable[SQUARE_NB] = {
        // Rank 1
        S(20, -61),   S(39, -57),   S(19, -36),   S(34, -28),   S(38, -55),   S(-23, 4),    S(-33, -38),  S(36, -94),
        // Rank 2
        S(5, -18),    S(26, -22),   S(32, -20),   S(38, -32),   S(42, -21),   S(59, -69),   S(24, -51),   S(-26, -8),
        // Rank 3
        S(13, -27),   S(18, -2),    S(11, 30),    S(25, 12),    S(34, 18),    S(28, 45),    S(45, -22),   S(8, -15),
        // Rank 4
        S(10, 11),    S(2, 53),     S(20, 42),    S(21, 48),    S(29, 34),    S(23, 62),    S(10, 70),    S(17, 45),
        // Rank 5
        S(-8, 30),    S(17, 50),    S(6, 73),     S(2, 56),     S(37, 77),    S(31, 72),    S(26, 50),    S(12, 69),
        // Rank 6
        S(25, -8),    S(21, 40),    S(1, 69),     S(32, 61),    S(92, 33),    S(133, 22),   S(121, 3),    S(30, 47),
        // Rank 7
        S(-27, 40),   S(-29, 78),   S(17, 79),    S(3, 84),     S(22, 85),    S(104, 58),   S(40, 12),    S(94, -25),
        // Rank 8
        S(12, 17),    S(-6, 60),    S(-32, 115),  S(26, 57),    S(85, 29),    S(111, 53),   S(145, 26),   S(85, 46),
    };

    Score KingTable[SQUARE_NB] = {
        // Rank 1
        S(-22, -41),  S(14, -38),   S(14, -15),   S(-78, -3),   S(0, -57),    S(-76, -17),  S(29, -45),   S(21, -71),
        // Rank 2
        S(4, -66),    S(-24, -3),   S(-1, -8),    S(-48, 12),   S(-54, 2),    S(-21, 1),    S(14, -16),   S(18, -39),
        // Rank 3
        S(-57, -36),  S(52, -27),   S(-15, 6),    S(-50, 25),   S(-35, 25),   S(-42, 11),   S(-12, -14),  S(-10, -46),
        // Rank 4
        S(13, -32),   S(20, -19),   S(-25, 30),   S(-114, 54),  S(-94, 47),   S(-45, 13),   S(21, -16),   S(-39, -37),
        // Rank 5
        S(165, -52),  S(74, 6),     S(-88, 59),   S(-271, 111), S(-203, 94),  S(-21, 50),   S(65, 13),    S(-31, -15),
        // Rank 6
        S(37, -8),    S(-36, 50),   S(-50, 75),   S(-203, 98),  S(-192, 83),  S(-54, 79),   S(-96, 68),   S(-58, 15),
        // Rank 7
        S(177, -39),  S(10, 30),    S(-60, 62),   S(78, 9),     S(76, 46),    S(28, 51),    S(8, 68),     S(142, -31),
        // Rank 8
        S(630, -240), S(495, -127), S(262, -27),  S(281, -85),  S(351, -66),  S(309, -33),  S(409, -122), S(-3, -198),
    };

    // Side to move bonus, added after tapering
    int Tempo = -1;
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
