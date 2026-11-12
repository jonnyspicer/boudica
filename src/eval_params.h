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
    Score PieceValue[PIECE_TYPE_NB] = { S(0, 0), S(67, 131), S(394, 375), S(402, 394), S(543, 666), S(1690, 988), S(0, 0), S(0, 0) };
    Score BishopPairBonus = S(22, 69);

    // Pawn structure
    Score DoubledPawnPenalty = S(-4, -38);
    Score IsolatedPawnPenalty = S(-11, -5);
    Score BackwardPawnPenalty = S(-12, -10);
    Score ConnectedPawnBonus = S(13, 5);

    // Passed pawns, indexed by relative rank
    Score PassedPawnBonus[RANK_NB] = { S(0, 0), S(83, -94), S(65, -66), S(38, -21), S(73, 13), S(121, 52), S(231, 132), S(0, 0) };
    Score BlockedPasserPenalty[RANK_NB] = { S(0, 0), S(7, 3), S(-12, 9), S(-14, -2), S(-30, -17), S(-44, -30), S(-170, -97), S(0, 0) };
    Score ProtectedPasserBonus = S(12, -6);
    Score PasserEnemyKingDistance = S(-6, 12);
    Score PasserOwnKingProximity = S(-9, 10);
    Score RookBehindPasserBonus = S(13, 21);

    // King safety
    Score PawnShieldBonus = S(12, -15);
    Score PawnShield2Bonus = S(8, -9);
    Score OpenFileNearKingPenalty = S(-29, -18);
    Score SemiOpenFileNearKingPenalty = S(-12, 8);

    // Mobility, per square not attacked by enemy pawns
    Score KnightMobility = S(6, 0);
    Score BishopMobility = S(6, 3);
    Score RookMobility = S(3, 6);
    Score QueenMobility = S(5, -1);

    // Rooks
    Score RookOnOpenFile = S(38, -3);
    Score RookOnSemiOpenFile = S(12, 19);
    Score RookOn7thRank = S(23, 32);

    // Piece-square tables
    Score PawnTable[SQUARE_NB] = {
        // Rank 1
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
        // Rank 2
        S(-22, 21),   S(-24, 20),   S(-9, 12),    S(-8, -22),   S(2, 28),     S(21, 25),    S(20, 20),    S(-3, 11),
        // Rank 3
        S(-17, 19),   S(-25, 16),   S(-9, 1),     S(3, 0),      S(2, 13),     S(0, 22),     S(9, 20),     S(4, 10),
        // Rank 4
        S(-4, 24),    S(-17, 13),   S(4, -7),     S(14, -9),    S(15, 6),     S(10, 17),    S(-16, 21),   S(-1, 14),
        // Rank 5
        S(-13, 41),   S(-3, 22),    S(2, 11),     S(17, -5),    S(26, 2),     S(20, 13),    S(-11, 22),   S(0, 18),
        // Rank 6
        S(12, 54),    S(14, 54),    S(27, 43),    S(21, 20),    S(70, -24),   S(59, 28),    S(42, 34),    S(43, 45),
        // Rank 7
        S(112, 16),   S(142, 47),   S(88, 53),    S(126, 4),    S(93, 25),    S(2, 50),     S(14, 71),    S(139, -3),
        // Rank 8
        S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),      S(0, 0),
    };

    Score KnightTable[SQUARE_NB] = {
        // Rank 1
        S(-99, -34),  S(-14, -27),  S(-5, -5),    S(22, -27),   S(-8, -5),    S(-2, -17),   S(-19, -70),  S(-106, -146),
        // Rank 2
        S(-21, -26),  S(-32, 2),    S(-23, -3),   S(13, 4),     S(9, 12),     S(11, 17),    S(-3, 40),    S(10, 17),
        // Rank 3
        S(-24, -42),  S(-5, 12),    S(-4, 14),    S(19, 35),    S(33, 34),    S(13, 17),    S(14, 6),     S(-16, -13),
        // Rank 4
        S(-8, 19),    S(-8, 28),    S(23, 25),    S(13, 54),    S(30, 38),    S(27, 43),    S(26, 4),     S(5, 10),
        // Rank 5
        S(-7, -12),   S(13, 37),    S(49, 37),    S(54, 57),    S(25, 60),    S(65, 29),    S(16, 40),    S(25, 1),
        // Rank 6
        S(-10, 6),    S(16, 24),    S(30, 44),    S(61, 30),    S(153, -4),   S(102, 16),   S(59, 7),     S(5, 0),
        // Rank 7
        S(11, -18),   S(34, -4),    S(26, 24),    S(85, 11),    S(79, 20),    S(69, -5),    S(43, -26),   S(51, -77),
        // Rank 8
        S(-100, -62), S(-44, 11),   S(41, -24),   S(4, -21),    S(142, -32),  S(87, -41),   S(26, -29),   S(-25, -151),
    };

    Score BishopTable[SQUARE_NB] = {
        // Rank 1
        S(-11, -41),  S(34, -46),   S(0, -3),     S(2, -1),     S(-1, -3),    S(-3, -2),    S(26, -40),   S(17, -69),
        // Rank 2
        S(24, -4),    S(19, 4),     S(26, 9),     S(9, 14),     S(18, 8),     S(39, 2),     S(36, -2),    S(48, -83),
        // Rank 3
        S(14, -21),   S(15, 22),    S(27, 18),    S(22, 17),    S(22, 27),    S(27, 13),    S(22, 1),     S(-10, 15),
        // Rank 4
        S(27, -18),   S(19, 6),     S(12, 28),    S(45, 24),    S(38, 6),     S(5, 24),     S(2, 30),     S(30, -35),
        // Rank 5
        S(10, 2),     S(3, 35),     S(30, 9),     S(59, 13),    S(34, 20),    S(63, 13),    S(2, 16),     S(6, 22),
        // Rank 6
        S(15, 11),    S(25, 30),    S(-6, 40),    S(33, 11),    S(54, 10),    S(78, 33),    S(80, -4),    S(6, 41),
        // Rank 7
        S(9, 5),      S(-21, 40),   S(9, 25),     S(-25, 16),   S(0, 17),     S(-30, 17),   S(34, 11),    S(-30, -41),
        // Rank 8
        S(39, -18),   S(-19, 30),   S(-68, 48),   S(86, 0),     S(-14, 2),    S(-61, 1),    S(24, 11),    S(-20, 0),
    };

    Score RookTable[SQUARE_NB] = {
        // Rank 1
        S(-1, 8),     S(7, -4),     S(8, 8),      S(15, -3),    S(21, -2),    S(13, 13),    S(1, 6),      S(14, -27),
        // Rank 2
        S(-14, -11),  S(-10, 8),    S(-9, 9),     S(2, 9),      S(-6, -5),    S(3, 16),     S(-8, 8),     S(-27, 8),
        // Rank 3
        S(-34, 38),   S(-6, 13),    S(-23, 30),   S(-26, 29),   S(-33, 18),   S(-29, 29),   S(3, 6),      S(-39, 20),
        // Rank 4
        S(-22, 34),   S(-15, 53),   S(-7, 40),    S(-13, 40),   S(-6, 43),    S(-20, 34),   S(-20, 49),   S(3, 43),
        // Rank 5
        S(-10, 64),   S(-14, 60),   S(12, 50),    S(46, 35),    S(17, 42),    S(30, 48),    S(5, 44),     S(33, 23),
        // Rank 6
        S(-2, 54),    S(30, 46),    S(17, 42),    S(52, 33),    S(63, 32),    S(80, 25),    S(113, 20),   S(32, 34),
        // Rank 7
        S(-16, 22),   S(-15, 26),   S(23, 13),    S(60, 6),     S(32, 24),    S(14, 28),    S(18, 25),    S(37, 9),
        // Rank 8
        S(29, 19),    S(59, 29),    S(66, 30),    S(62, 21),    S(61, 18),    S(86, 27),    S(132, 11),   S(-5, 40),
    };

    Score QueenTable[SQUARE_NB] = {
        // Rank 1
        S(46, -69),   S(42, -67),   S(11, -20),   S(36, -11),   S(41, -40),   S(-19, 1),    S(-59, -19),  S(31, -70),
        // Rank 2
        S(-2, -21),   S(22, -3),    S(33, -28),   S(35, -36),   S(35, -23),   S(52, -76),   S(16, -46),   S(-32, 26),
        // Rank 3
        S(30, -32),   S(24, 2),     S(16, 42),    S(21, 14),    S(33, 2),     S(26, 18),    S(51, -41),   S(21, -2),
        // Rank 4
        S(18, 11),    S(8, 61),     S(11, 49),    S(13, 58),    S(22, 31),    S(16, 52),    S(23, 70),    S(17, 30),
        // Rank 5
        S(-3, 27),    S(32, 39),    S(17, 79),    S(10, 67),    S(25, 68),    S(25, 77),    S(29, 65),    S(11, 66),
        // Rank 6
        S(26, 9),     S(24, 55),    S(1, 56),     S(15, 79),    S(85, 42),    S(101, 32),   S(121, 25),   S(29, 52),
        // Rank 7
        S(-21, 45),   S(-39, 86),   S(16, 77),    S(-1, 106),   S(31, 61),    S(90, 47),    S(46, 11),    S(83, -34),
        // Rank 8
        S(-12, 35),   S(7, 46),     S(-41, 104),  S(8, 58),     S(91, 28),    S(95, 39),    S(144, 10),   S(50, 31),
    };

    Score KingTable[SQUARE_NB] = {
        // Rank 1
        S(-25, -49),  S(2, -39),    S(-10, -14),  S(-83, -10),  S(-7, -57),   S(-79, -11),  S(23, -40),   S(13, -64),
        // Rank 2
        S(44, -82),   S(-37, 1),    S(-14, -4),   S(-57, 13),   S(-50, 11),   S(-32, 5),    S(6, -14),    S(19, -38),
        // Rank 3
        S(-20, -37),  S(44, -28),   S(-17, 9),    S(-52, 29),   S(-3, 21),    S(-39, 13),   S(3, -11),    S(-14, -42),
        // Rank 4
        S(15, -35),   S(41, -17),   S(-38, 28),   S(-134, 63),  S(-99, 54),   S(-16, 15),   S(43, -26),   S(-44, -16),
        // Rank 5
        S(156, -51),  S(112, -1),   S(-84, 62),   S(-253, 111), S(-201, 91),  S(-6, 42),    S(133, 4),    S(-24, -5),
        // Rank 6
        S(121, -18),  S(13, 63),    S(-93, 64),   S(-203, 85),  S(-158, 77),  S(-18, 77),   S(-92, 62),   S(116, -5),
        // Rank 7
        S(206, -43),  S(21, 30),    S(-32, 63),   S(79, 0),     S(14, 41),    S(30, 39),    S(0, 58),     S(308, -49),
        // Rank 8
        S(671, -248), S(590, -129), S(175, -18),  S(161, -83),  S(196, -43),  S(284, -30),  S(806, -151), S(45, -219),
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
