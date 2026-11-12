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
    Score PieceValue[PIECE_TYPE_NB] = { S(0, 0), S(70, 126), S(404, 365), S(416, 386), S(576, 644), S(1586, 1036), S(0, 0), S(0, 0) };
    Score BishopPairBonus = S(24, 66);

    // Pawn structure
    Score DoubledPawnPenalty = S(-5, -34);
    Score IsolatedPawnPenalty = S(-12, -8);
    Score BackwardPawnPenalty = S(-12, -9);
    Score ConnectedPawnBonus = S(14, 5);

    // Passed pawns, indexed by relative rank
    Score PassedPawnBonus[RANK_NB] = { S(0, 0), S(44, -80), S(21, -49), S(5, -10), S(52, 22), S(119, 59), S(279, 114), S(0, 0) };
    Score BlockedPasserPenalty[RANK_NB] = { S(0, 0), S(38, -6), S(-16, 15), S(-31, 3), S(-35, -5), S(-96, -19), S(-188, -98), S(0, 0) };
    Score ProtectedPasserBonus = S(30, -12);
    Score PasserEnemyKingDistance = S(-3, 9);
    Score PasserOwnKingProximity = S(-4, 8);
    Score RookBehindPasserBonus = S(22, 13);

    // King safety
    Score PawnShieldBonus = S(17, -14);
    Score PawnShield2Bonus = S(9, -6);
    Score OpenFileNearKingPenalty = S(-19, -14);
    Score SemiOpenFileNearKingPenalty = S(-5, 7);

    // Mobility, per square not attacked by enemy pawns
    Score KnightMobility = S(6, -1);
    Score BishopMobility = S(6, 4);
    Score RookMobility = S(2, 5);
    Score QueenMobility = S(4, -3);

    // Rooks
    Score RookOnOpenFile = S(38, -1);
    Score RookOnSemiOpenFile = S(16, 21);
    Score RookOn7thRank = S(18, 31);

    // Piece-square tables
    Score PawnTable[SQUARE_NB] = {
        // Rank 1
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
        // Rank 2
        S(-21, 19),   S(-28, 18),   S(-14, 13),   S(-18, -17),  S(-3, 17),    S(15, 23),    S(8, 22),     S(-3, 19),
        // Rank 3
        S(-15, 16),   S(-22, 18),   S(-11, 2),    S(1, 1),      S(-4, 11),    S(5, 22),     S(1, 14),     S(7, 13),
        // Rank 4
        S(-5, 26),    S(-15, 16),   S(3, 2),      S(19, -9),    S(23, 7),     S(10, 25),    S(-3, 22),    S(11, 17),
        // Rank 5
        S(-2, 37),    S(-6, 25),    S(17, 9),     S(23, -10),   S(30, 5),     S(22, 16),    S(-10, 21),   S(15, 16),
        // Rank 6
        S(10, 40),    S(2, 47),     S(56, 36),    S(34, 14),    S(108, -15),  S(90, 12),    S(79, 36),    S(63, 25),
        // Rank 7
        S(90, 40),    S(80, 46),    S(95, 54),    S(45, 36),    S(89, 56),    S(122, 6),    S(34, 74),    S(426, -95),
        // Rank 8
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
    };

    Score KnightTable[SQUARE_NB] = {
        // Rank 1
        S(-16, -65),  S(-9, -27),   S(2, -20),    S(11, -16),   S(-9, 5),     S(17, -24),   S(-14, -77),  S(-52, -147),
        // Rank 2
        S(-41, -31),  S(-21, -2),   S(-34, 7),    S(6, 6),      S(11, 5),     S(14, 10),    S(-10, 30),   S(-19, 20),
        // Rank 3
        S(-24, -43),  S(-9, 20),    S(-1, 20),    S(11, 30),    S(36, 26),    S(7, 22),     S(1, 7),      S(-3, 1),
        // Rank 4
        S(2, 15),     S(-15, 41),   S(31, 23),    S(14, 51),    S(17, 41),    S(29, 29),    S(63, 4),     S(4, 6),
        // Rank 5
        S(6, -21),    S(7, 26),     S(39, 45),    S(56, 41),    S(20, 54),    S(58, 21),    S(1, 32),     S(22, 22),
        // Rank 6
        S(55, 7),     S(39, 15),    S(72, 23),    S(47, 33),    S(122, 9),    S(147, 14),   S(92, 0),     S(76, 1),
        // Rank 7
        S(11, -28),   S(38, -1),    S(25, 25),    S(53, 23),    S(61, 8),     S(71, -14),   S(52, -26),   S(35, -78),
        // Rank 8
        S(-93, -84),  S(34, 30),    S(26, -14),   S(-71, 23),   S(99, -32),   S(209, -40),  S(394, -94),  S(-124, -143),
    };

    Score BishopTable[SQUARE_NB] = {
        // Rank 1
        S(-2, -61),   S(13, -57),   S(-4, -7),    S(-1, -15),   S(-22, 2),    S(-4, -8),    S(19, -59),   S(94, -146),
        // Rank 2
        S(44, -5),    S(11, 2),     S(29, 3),     S(5, 16),     S(18, 13),    S(7, 16),     S(27, -4),    S(23, -72),
        // Rank 3
        S(12, -16),   S(17, 19),    S(17, 26),    S(21, 10),    S(6, 24),     S(19, 10),    S(23, -1),    S(-3, 32),
        // Rank 4
        S(20, -7),    S(1, 8),      S(14, 36),    S(40, 30),    S(30, -4),    S(-5, 24),    S(-6, 24),    S(21, -31),
        // Rank 5
        S(9, 2),      S(-1, 36),    S(6, 18),     S(61, 4),     S(32, 22),    S(44, 14),    S(-5, 24),    S(-3, 10),
        // Rank 6
        S(17, 3),     S(24, 43),    S(-24, 42),   S(40, 6),     S(46, 15),    S(65, 30),    S(97, 7),     S(-8, 30),
        // Rank 7
        S(-15, 21),   S(-31, 30),   S(2, 34),     S(-40, 9),    S(-5, 18),    S(-48, 9),    S(-23, 27),   S(-2, -37),
        // Rank 8
        S(18, -14),   S(-28, 33),   S(-115, 63),  S(53, 0),     S(12, -12),   S(-115, 5),   S(82, 18),    S(-101, 66),
    };

    Score RookTable[SQUARE_NB] = {
        // Rank 1
        S(9, 6),      S(11, 6),     S(13, 8),     S(20, -6),    S(20, -14),   S(16, 5),     S(8, -6),     S(-2, -16),
        // Rank 2
        S(-5, -10),   S(-8, 7),     S(-11, 18),   S(0, 10),     S(-22, -1),   S(8, 5),      S(-38, 10),   S(-58, 7),
        // Rank 3
        S(-16, 14),   S(11, 5),     S(13, 17),    S(-24, 24),   S(-40, 28),   S(-32, 26),   S(-28, 26),   S(-4, 14),
        // Rank 4
        S(-34, 46),   S(-29, 61),   S(-7, 45),    S(-3, 35),    S(-22, 39),   S(-19, 38),   S(-6, 51),    S(-18, 42),
        // Rank 5
        S(3, 48),     S(-24, 60),   S(5, 50),     S(62, 20),    S(13, 38),    S(-8, 59),    S(27, 29),    S(28, 29),
        // Rank 6
        S(6, 42),     S(71, 41),    S(18, 40),    S(39, 32),    S(83, 19),    S(125, 27),   S(128, 28),   S(84, 32),
        // Rank 7
        S(-25, 28),   S(-29, 32),   S(69, 7),     S(54, 15),    S(32, 21),    S(14, 18),    S(9, 22),     S(7, 10),
        // Rank 8
        S(0, 21),     S(34, 37),    S(15, 28),    S(81, 12),    S(36, 11),    S(89, 19),    S(185, -16),  S(-26, 61),
    };

    Score QueenTable[SQUARE_NB] = {
        // Rank 1
        S(-3, 10),    S(37, -46),   S(10, -27),   S(26, -3),    S(54, -65),   S(3, -22),    S(-42, 6),    S(142, -124),
        // Rank 2
        S(0, 12),     S(0, 0),      S(29, -34),   S(31, -16),   S(41, -21),   S(51, -66),   S(32, -42),   S(-74, 124),
        // Rank 3
        S(18, -41),   S(19, -12),   S(-7, 26),    S(18, 11),    S(26, 7),     S(27, 5),     S(47, -29),   S(21, -41),
        // Rank 4
        S(12, 21),    S(15, 44),    S(16, 16),    S(19, 30),    S(13, 38),    S(13, 52),    S(28, 57),    S(9, 46),
        // Rank 5
        S(-27, 60),   S(9, 76),     S(12, 62),    S(1, 61),     S(20, 55),    S(31, 84),    S(18, 105),   S(36, 41),
        // Rank 6
        S(13, 23),    S(-7, 91),    S(6, 74),     S(5, 77),     S(80, 15),    S(56, 46),    S(123, 13),   S(23, 61),
        // Rank 7
        S(-34, 62),   S(-34, 89),   S(10, 76),    S(7, 116),    S(27, 75),    S(29, 96),    S(47, 15),    S(109, -51),
        // Rank 8
        S(-29, 38),   S(-80, 85),   S(-25, 91),   S(56, 51),    S(147, -36),  S(82, 43),    S(187, -32),  S(-36, 66),
    };

    Score KingTable[SQUARE_NB] = {
        // Rank 1
        S(-14, -46),  S(0, -36),    S(0, -18),    S(-75, -19),  S(4, -40),    S(-66, -15),  S(11, -37),   S(5, -64),
        // Rank 2
        S(21, -71),   S(-34, 8),    S(-16, -2),   S(-67, 9),    S(-36, 2),    S(-24, -2),   S(-3, -12),   S(7, -33),
        // Rank 3
        S(-71, -27),  S(53, -28),   S(-13, 8),    S(-47, 32),   S(-6, 19),    S(-37, 10),   S(-2, -13),   S(12, -45),
        // Rank 4
        S(55, -51),   S(46, -18),   S(-33, 27),   S(-185, 70),  S(-139, 63),  S(-7, 7),     S(65, -29),   S(12, -29),
        // Rank 5
        S(184, -58),  S(138, -6),   S(-52, 47),   S(-258, 113), S(-206, 99),  S(5, 38),     S(163, -3),   S(45, -28),
        // Rank 6
        S(231, -17),  S(5, 50),     S(-126, 89),  S(-205, 90),  S(-53, 77),   S(17, 66),    S(131, 54),   S(84, -5),
        // Rank 7
        S(327, -44),  S(44, 68),    S(-123, 75),  S(-93, 13),   S(22, 42),    S(-6, 62),    S(-98, 92),   S(101, -23),
        // Rank 8
        S(251, -80),  S(450, -134), S(-87, 14),   S(71, -62),   S(185, -26),  S(34, -16),   S(382, -141), S(1099, -244),
    };

    // Side to move bonus, added after tapering
    int Tempo = 0;
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
