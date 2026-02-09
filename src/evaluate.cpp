#include "evaluate.h"
#include "position.h"
#include "bitboard.h"

#include <cstdlib>  // for std::abs
#include <cstring>  // for std::memset

namespace Boudica {

namespace Eval {

// ============================================================================
// Piece-Square Tables (PST)
// Values are from White's perspective, indexed by square (A1=0 ... H8=63)
// Separate tables for middlegame (mg) and endgame (eg)
// ============================================================================

// Pawn PST - Encourage advancement, center control, avoid edges
constexpr Score PawnTable[SQUARE_NB] = {
    // Rank 1 (impossible for pawns)
    S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),
    // Rank 2
    S(-5, 0),  S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(-5, 0),
    // Rank 3
    S(-5, 0),  S(0, 5),   S(5, 5),   S(10, 5),  S(10, 5),  S(5, 5),   S(0, 5),   S(-5, 0),
    // Rank 4
    S(-5, 5),  S(0, 10),  S(10, 10), S(25, 15), S(25, 15), S(10, 10), S(0, 10),  S(-5, 5),
    // Rank 5
    S(0, 15),  S(5, 20),  S(15, 25), S(30, 30), S(30, 30), S(15, 25), S(5, 20),  S(0, 15),
    // Rank 6
    S(5, 35),  S(10, 40), S(20, 45), S(35, 50), S(35, 50), S(20, 45), S(10, 40), S(5, 35),
    // Rank 7
    S(50, 70), S(50, 70), S(50, 70), S(50, 70), S(50, 70), S(50, 70), S(50, 70), S(50, 70),
    // Rank 8 (impossible for pawns - would be promoted)
    S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0),   S(0, 0)
};

// Knight PST - Strong center preference, avoid edges strongly
constexpr Score KnightTable[SQUARE_NB] = {
    // Rank 1
    S(-50, -30), S(-40, -20), S(-30, -10), S(-25, -5),  S(-25, -5),  S(-30, -10), S(-40, -20), S(-50, -30),
    // Rank 2
    S(-35, -20), S(-20, -10), S(-5, 0),    S(0, 5),     S(0, 5),     S(-5, 0),    S(-20, -10), S(-35, -20),
    // Rank 3
    S(-20, -10), S(0, 0),     S(15, 10),   S(20, 15),   S(20, 15),   S(15, 10),   S(0, 0),     S(-20, -10),
    // Rank 4
    S(-10, -5),  S(10, 5),    S(25, 15),   S(30, 20),   S(30, 20),   S(25, 15),   S(10, 5),    S(-10, -5),
    // Rank 5
    S(-10, -5),  S(10, 5),    S(25, 15),   S(30, 20),   S(30, 20),   S(25, 15),   S(10, 5),    S(-10, -5),
    // Rank 6
    S(-20, -10), S(5, 5),     S(20, 10),   S(25, 15),   S(25, 15),   S(20, 10),   S(5, 5),     S(-20, -10),
    // Rank 7
    S(-35, -20), S(-15, -10), S(0, 0),     S(5, 5),     S(5, 5),     S(0, 0),     S(-15, -10), S(-35, -20),
    // Rank 8
    S(-50, -30), S(-40, -20), S(-30, -10), S(-25, -5),  S(-25, -5),  S(-30, -10), S(-40, -20), S(-50, -30)
};

// Bishop PST - Long diagonals, avoid corners, center preference
constexpr Score BishopTable[SQUARE_NB] = {
    // Rank 1
    S(-20, -15), S(-10, -10), S(-15, -10), S(-10, -5),  S(-10, -5),  S(-15, -10), S(-10, -10), S(-20, -15),
    // Rank 2
    S(-10, -10), S(5, 5),     S(0, 0),     S(5, 5),     S(5, 5),     S(0, 0),     S(5, 5),     S(-10, -10),
    // Rank 3
    S(-5, -5),   S(10, 5),    S(10, 10),   S(15, 10),   S(15, 10),   S(10, 10),   S(10, 5),    S(-5, -5),
    // Rank 4
    S(0, 0),     S(5, 5),     S(15, 10),   S(20, 15),   S(20, 15),   S(15, 10),   S(5, 5),     S(0, 0),
    // Rank 5
    S(0, 0),     S(10, 10),   S(15, 10),   S(20, 15),   S(20, 15),   S(15, 10),   S(10, 10),   S(0, 0),
    // Rank 6
    S(-5, -5),   S(10, 5),    S(10, 10),   S(10, 10),   S(10, 10),   S(10, 10),   S(10, 5),    S(-5, -5),
    // Rank 7
    S(-10, -10), S(5, 0),     S(0, 0),     S(0, 5),     S(0, 5),     S(0, 0),     S(5, 0),     S(-10, -10),
    // Rank 8
    S(-20, -15), S(-10, -10), S(-15, -10), S(-10, -5),  S(-10, -5),  S(-15, -10), S(-10, -10), S(-20, -15)
};

// Rook PST - 7th rank bonus, open files, central control
constexpr Score RookTable[SQUARE_NB] = {
    // Rank 1
    S(0, 0),     S(5, 0),     S(10, 0),    S(15, 0),    S(15, 0),    S(10, 0),    S(5, 0),     S(0, 0),
    // Rank 2
    S(-5, 0),    S(0, 0),     S(0, 0),     S(5, 0),     S(5, 0),     S(0, 0),     S(0, 0),     S(-5, 0),
    // Rank 3
    S(-5, 0),    S(0, 0),     S(0, 5),     S(5, 5),     S(5, 5),     S(0, 5),     S(0, 0),     S(-5, 0),
    // Rank 4
    S(-5, 5),    S(0, 5),     S(0, 10),    S(5, 10),    S(5, 10),    S(0, 10),    S(0, 5),     S(-5, 5),
    // Rank 5
    S(-5, 10),   S(0, 10),    S(0, 15),    S(5, 15),    S(5, 15),    S(0, 15),    S(0, 10),    S(-5, 10),
    // Rank 6
    S(-5, 15),   S(0, 15),    S(0, 20),    S(0, 20),    S(0, 20),    S(0, 20),    S(0, 15),    S(-5, 15),
    // Rank 7 - Strong bonus for rooks on 7th
    S(20, 30),   S(25, 30),   S(30, 30),   S(35, 30),   S(35, 30),   S(30, 30),   S(25, 30),   S(20, 30),
    // Rank 8
    S(10, 20),   S(15, 20),   S(20, 25),   S(25, 25),   S(25, 25),   S(20, 25),   S(15, 20),   S(10, 20)
};

// Queen PST - Central but not too early, avoid corners
constexpr Score QueenTable[SQUARE_NB] = {
    // Rank 1 - Slight penalty for early queen development
    S(-20, -15), S(-10, -10), S(-5, -5),   S(0, 0),     S(0, 0),     S(-5, -5),   S(-10, -10), S(-20, -15),
    // Rank 2
    S(-10, -10), S(0, 0),     S(5, 5),     S(5, 5),     S(5, 5),     S(5, 5),     S(0, 0),     S(-10, -10),
    // Rank 3
    S(-5, -5),   S(5, 5),     S(10, 10),   S(10, 10),   S(10, 10),   S(10, 10),   S(5, 5),     S(-5, -5),
    // Rank 4
    S(0, 0),     S(5, 5),     S(10, 10),   S(15, 15),   S(15, 15),   S(10, 10),   S(5, 5),     S(0, 0),
    // Rank 5
    S(0, 0),     S(5, 5),     S(10, 10),   S(15, 15),   S(15, 15),   S(10, 10),   S(5, 5),     S(0, 0),
    // Rank 6
    S(-5, 0),    S(5, 5),     S(10, 10),   S(10, 10),   S(10, 10),   S(10, 10),   S(5, 5),     S(-5, 0),
    // Rank 7
    S(-10, -5),  S(0, 0),     S(5, 5),     S(5, 5),     S(5, 5),     S(5, 5),     S(0, 0),     S(-10, -5),
    // Rank 8
    S(-20, -10), S(-10, -5),  S(-5, 0),    S(0, 0),     S(0, 0),     S(-5, 0),    S(-10, -5),  S(-20, -10)
};

// King PST (Middlegame) - Castle to safety, avoid center
constexpr Score KingTableMG[SQUARE_NB] = {
    // Rank 1 - Castled positions are good
    S(20, 0),    S(30, 0),    S(10, 0),    S(-20, 0),   S(-20, 0),   S(-10, 0),   S(30, 0),    S(20, 0),
    // Rank 2
    S(20, 0),    S(20, 0),    S(0, 0),     S(-20, 0),   S(-20, 0),   S(0, 0),     S(20, 0),    S(20, 0),
    // Rank 3
    S(-10, 0),   S(-20, 0),   S(-30, 0),   S(-40, 0),   S(-40, 0),   S(-30, 0),   S(-20, 0),   S(-10, 0),
    // Rank 4
    S(-30, 0),   S(-40, 0),   S(-50, 0),   S(-60, 0),   S(-60, 0),   S(-50, 0),   S(-40, 0),   S(-30, 0),
    // Rank 5
    S(-40, 0),   S(-50, 0),   S(-60, 0),   S(-70, 0),   S(-70, 0),   S(-60, 0),   S(-50, 0),   S(-40, 0),
    // Rank 6
    S(-50, 0),   S(-60, 0),   S(-70, 0),   S(-80, 0),   S(-80, 0),   S(-70, 0),   S(-60, 0),   S(-50, 0),
    // Rank 7
    S(-60, 0),   S(-70, 0),   S(-80, 0),   S(-90, 0),   S(-90, 0),   S(-80, 0),   S(-70, 0),   S(-60, 0),
    // Rank 8
    S(-70, 0),   S(-80, 0),   S(-90, 0),   S(-100, 0),  S(-100, 0),  S(-90, 0),   S(-80, 0),   S(-70, 0)
};

// King PST (Endgame) - Centralize the king
constexpr Score KingTableEG[SQUARE_NB] = {
    // Rank 1
    S(0, -30),   S(0, -20),   S(0, -10),   S(0, -5),    S(0, -5),    S(0, -10),   S(0, -20),   S(0, -30),
    // Rank 2
    S(0, -20),   S(0, -5),    S(0, 5),     S(0, 10),    S(0, 10),    S(0, 5),     S(0, -5),    S(0, -20),
    // Rank 3
    S(0, -10),   S(0, 5),     S(0, 15),    S(0, 20),    S(0, 20),    S(0, 15),    S(0, 5),     S(0, -10),
    // Rank 4
    S(0, -5),    S(0, 10),    S(0, 20),    S(0, 25),    S(0, 25),    S(0, 20),    S(0, 10),    S(0, -5),
    // Rank 5
    S(0, -5),    S(0, 10),    S(0, 20),    S(0, 25),    S(0, 25),    S(0, 20),    S(0, 10),    S(0, -5),
    // Rank 6
    S(0, -10),   S(0, 5),     S(0, 15),    S(0, 20),    S(0, 20),    S(0, 15),    S(0, 5),     S(0, -10),
    // Rank 7
    S(0, -20),   S(0, -5),    S(0, 5),     S(0, 10),    S(0, 10),    S(0, 5),     S(0, -5),    S(0, -20),
    // Rank 8
    S(0, -30),   S(0, -20),   S(0, -10),   S(0, -5),    S(0, -5),    S(0, -10),   S(0, -20),   S(0, -30)
};

// ============================================================================
// Utility Functions
// ============================================================================

// Get the appropriate PST score for a piece at a square
inline Score pst_score(PieceType pt, Square sq) {
    switch (pt) {
        case PAWN:   return PawnTable[sq];
        case KNIGHT: return KnightTable[sq];
        case BISHOP: return BishopTable[sq];
        case ROOK:   return RookTable[sq];
        case QUEEN:  return QueenTable[sq];
        case KING:   return KingTableMG[sq] + KingTableEG[sq];
        default:     return S(0, 0);
    }
}

// Flip square vertically for black's perspective
constexpr Square flip_square(Square sq) {
    return Square(sq ^ 56);
}

// ============================================================================
// Evaluation Helper Functions
// ============================================================================

// Calculate game phase based on remaining material (excluding pawns/kings)
// Returns 0 for endgame, 256 for full middlegame
int calculate_phase(const Position& pos) {
    int phase = 0;

    // Count phase contribution from each piece type
    for (PieceType pt = KNIGHT; pt <= QUEEN; ++pt) {
        phase += popcount(pos.pieces(WHITE, pt)) * PhaseWeights[pt];
        phase += popcount(pos.pieces(BLACK, pt)) * PhaseWeights[pt];
    }

    // Scale to 0-256 range
    phase = (phase * PHASE_MIDGAME) / TOTAL_PHASE;

    // Clamp to valid range
    if (phase > PHASE_MIDGAME) phase = PHASE_MIDGAME;
    if (phase < PHASE_ENDGAME) phase = PHASE_ENDGAME;

    return phase;
}

// Blend middlegame and endgame scores based on phase
Value taper_score(Score score, int phase) {
    return Value((score.mg * phase + score.eg * (PHASE_MIDGAME - phase)) / PHASE_MIDGAME);
}

// ============================================================================
// Material Evaluation
// ============================================================================

Score evaluate_material(const Position& pos, Color us) {
    Score score = S(0, 0);
    Color them = ~us;

    // Count material for each piece type
    for (PieceType pt = PAWN; pt <= QUEEN; ++pt) {
        int our_count = popcount(pos.pieces(us, pt));
        int their_count = popcount(pos.pieces(them, pt));
        score += PieceValue[pt] * (our_count - their_count);
    }

    // Bishop pair bonus
    if (popcount(pos.pieces(us, BISHOP)) >= 2) {
        score += BishopPairBonus;
    }
    if (popcount(pos.pieces(them, BISHOP)) >= 2) {
        score -= BishopPairBonus;
    }

    return score;
}

// ============================================================================
// Piece-Square Table Evaluation
// ============================================================================

Score evaluate_pst(const Position& pos, Color us) {
    Score score = S(0, 0);
    Color them = ~us;

    // Evaluate our pieces
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        Bitboard bb = pos.pieces(us, pt);
        while (bb) {
            Square sq = pop_lsb(bb);
            // Use square as-is for white, flip for black
            Square eval_sq = (us == WHITE) ? sq : flip_square(sq);
            score += pst_score(pt, eval_sq);
        }
    }

    // Evaluate their pieces (subtract from our score)
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        Bitboard bb = pos.pieces(them, pt);
        while (bb) {
            Square sq = pop_lsb(bb);
            // Use flipped square for black pieces when evaluating from white's view
            Square eval_sq = (them == WHITE) ? sq : flip_square(sq);
            score -= pst_score(pt, eval_sq);
        }
    }

    return score;
}

// ============================================================================
// Pawn Structure Evaluation
// ============================================================================

// ============================================================================
// Pawn Hash Table
// ============================================================================

struct PawnHashEntry {
    Key key;
    Score scores[COLOR_NB];
};

constexpr int PAWN_HASH_SIZE = 16384;  // Must be power of 2
static PawnHashEntry pawn_hash_table[PAWN_HASH_SIZE];

// ============================================================================
// Pawn Structure - Basic (Cacheable: depends only on pawn positions)
// ============================================================================

Score evaluate_pawn_structure_basic(const Position& pos, Color us) {
    Score score = S(0, 0);
    Color them = ~us;

    Bitboard our_pawns = pos.pieces(us, PAWN);
    Bitboard their_pawns = pos.pieces(them, PAWN);

    // Compute enemy pawn control for backward pawn detection
    Bitboard enemy_pawn_control = (us == WHITE) ?
        pawn_attacks_bb<BLACK>(their_pawns) :
        pawn_attacks_bb<WHITE>(their_pawns);

    Bitboard pawns = our_pawns;
    while (pawns) {
        Square sq = pop_lsb(pawns);
        Rank r = rank_of(sq);

        // Doubled pawns - penalize if there's a friendly pawn ahead (avoids double-counting)
        if (our_pawns & forward_file_bb(us, sq)) {
            score += DoubledPawnPenalty;
        }

        // Isolated pawns - no friendly pawns on adjacent files
        Bitboard adjacent_files = adjacent_files_bb(sq);

        if (!(our_pawns & adjacent_files)) {
            score += IsolatedPawnPenalty;
        }

        // Connected pawns - friendly pawn on adjacent file and same/adjacent rank
        Bitboard adjacent_ranks = rank_bb(r);
        if (r > RANK_1) adjacent_ranks |= rank_bb(Rank(r - 1));
        if (r < RANK_8) adjacent_ranks |= rank_bb(Rank(r + 1));

        if (our_pawns & adjacent_files & adjacent_ranks) {
            score += ConnectedPawnBonus;
        }

        // Backward pawns - no adjacent friendly pawn at same rank or behind,
        // and stop square is controlled by enemy pawn
        if (our_pawns & adjacent_files) {  // Not isolated (already penalized)
            Bitboard at_or_behind = ~ForwardRanksBB[us][r];
            if (!(our_pawns & adjacent_files & at_or_behind)) {
                Square stop = sq + pawn_push(us);
                if (stop >= SQ_A1 && stop <= SQ_H8) {
                    if (enemy_pawn_control & square_bb(stop)) {
                        score += BackwardPawnPenalty;
                    }
                }
            }
        }

        // Passed pawns - base bonus (pawn-only: identification + rank bonus + protected)
        Bitboard blocking_zone = passed_pawn_span(us, sq);

        if (!(their_pawns & blocking_zone)) {
            Rank rel_rank = relative_rank(us, sq);
            score += PassedPawnBonus[rel_rank];

            // Protected passed pawn (defended by another pawn)
            Bitboard pawn_defenders = (us == WHITE)
                ? pawn_attacks_bb<BLACK>(square_bb(sq))
                : pawn_attacks_bb<WHITE>(square_bb(sq));
            if (our_pawns & pawn_defenders) {
                score += S(10, 15);
            }
        }
    }

    return score;
}

// ============================================================================
// Passed Pawn Piece-Dependent Bonuses (NOT cacheable)
// ============================================================================

Score evaluate_passed_pawn_pieces(const Position& pos, Color us) {
    Score score = S(0, 0);
    Color them = ~us;

    Bitboard our_pawns = pos.pieces(us, PAWN);
    Bitboard their_pawns = pos.pieces(them, PAWN);

    Bitboard pawns = our_pawns;
    while (pawns) {
        Square sq = pop_lsb(pawns);
        File f = file_of(sq);
        Rank r = rank_of(sq);

        // Only process passed pawns
        Bitboard blocking_zone = passed_pawn_span(us, sq);
        if (their_pawns & blocking_zone) continue;

        Rank rel_rank = relative_rank(us, sq);

        // Check if the pawn is blocked (piece directly in front)
        Square front_sq = sq + pawn_push(us);
        if (front_sq >= SQ_A1 && front_sq <= SQ_H8) {
            if (pos.piece_on(front_sq) != NO_PIECE) {
                // Subtract half the base bonus (cached function added full bonus)
                Score base = PassedPawnBonus[rel_rank];
                score -= Score(base.mg / 2, base.eg / 2);
            }

            // King proximity bonus in endgame - enemy king far is good
            Square enemy_king = pos.king_square(them);
            int enemy_king_dist = std::abs(file_of(sq) - file_of(enemy_king)) +
                                  std::abs(rank_of(front_sq) - rank_of(enemy_king));
            score += S(0, enemy_king_dist * 3);

            // Our king close to passed pawn in endgame is good
            Square our_king = pos.king_square(us);
            int our_king_dist = std::abs(file_of(sq) - file_of(our_king)) +
                                std::abs(rank_of(sq) - rank_of(our_king));
            score += S(0, (8 - our_king_dist) * 2);
        }

        // Rook behind passed pawn bonus
        Bitboard file_mask = file_bb(f);
        Bitboard our_rooks = pos.pieces(us, ROOK);

        if (our_rooks & file_mask) {
            // Check if rook is behind the pawn
            Bitboard behind_span = ~ForwardRanksBB[us][r] & ~rank_bb(r) & file_mask;

            if (our_rooks & behind_span) {
                score += RookBehindPasserBonus;
            }
        }
    }

    return score;
}

// ============================================================================
// King Safety Evaluation
// ============================================================================

// Attack weights for different piece types
constexpr int ATTACK_WEIGHT_KNIGHT = 3;
constexpr int ATTACK_WEIGHT_BISHOP = 3;
constexpr int ATTACK_WEIGHT_ROOK = 5;
constexpr int ATTACK_WEIGHT_QUEEN = 8;

// Safety table - indexed by attack units, gives penalty
// Non-linear scaling: multiple attackers are much more dangerous
constexpr int SAFETY_TABLE[100] = {
    0,   0,   1,   2,   3,   5,   7,  10,  13,  16,
   20,  25,  30,  36,  42,  49,  56,  64,  72,  81,
   90, 100, 110, 121, 132, 144, 156, 169, 182, 196,
  210, 225, 240, 256, 272, 289, 306, 324, 342, 361,
  380, 400, 420, 441, 462, 484, 506, 529, 552, 576,
  600, 625, 650, 676, 702, 729, 756, 784, 812, 841,
  870, 900, 900, 900, 900, 900, 900, 900, 900, 900,
  900, 900, 900, 900, 900, 900, 900, 900, 900, 900,
  900, 900, 900, 900, 900, 900, 900, 900, 900, 900,
  900, 900, 900, 900, 900, 900, 900, 900, 900, 900
};

Score evaluate_king_safety(const Position& pos, Color us) {
    Score score = S(0, 0);
    Color them = ~us;

    Square king_sq = pos.king_square(us);
    File king_file = file_of(king_sq);
    Rank king_rank = rank_of(king_sq);

    Bitboard our_pawns = pos.pieces(us, PAWN);
    Bitboard their_pawns = pos.pieces(them, PAWN);

    // Pawn shield - check for pawns in front of king
    Bitboard shield_zone = Bitboard(0);
    Rank shield_rank = (us == WHITE) ? Rank(king_rank + 1) : Rank(king_rank - 1);

    if (shield_rank >= RANK_1 && shield_rank <= RANK_8) {
        // Files around the king
        if (king_file > FILE_A) shield_zone |= square_bb(make_square(File(king_file - 1), shield_rank));
        shield_zone |= square_bb(make_square(king_file, shield_rank));
        if (king_file < FILE_H) shield_zone |= square_bb(make_square(File(king_file + 1), shield_rank));

        int shield_pawns = popcount(our_pawns & shield_zone);
        score += PawnShieldBonus * shield_pawns;

        // Extra bonus for second rank shield (more protective)
        Rank shield_rank2 = (us == WHITE) ? Rank(king_rank + 2) : Rank(king_rank - 2);
        if (shield_rank2 >= RANK_1 && shield_rank2 <= RANK_8) {
            Bitboard shield_zone2 = Bitboard(0);
            if (king_file > FILE_A) shield_zone2 |= square_bb(make_square(File(king_file - 1), shield_rank2));
            shield_zone2 |= square_bb(make_square(king_file, shield_rank2));
            if (king_file < FILE_H) shield_zone2 |= square_bb(make_square(File(king_file + 1), shield_rank2));
            int shield_pawns2 = popcount(our_pawns & shield_zone2);
            score += S(5, 0) * shield_pawns2;  // Smaller bonus for 2nd row
        }
    }

    // Open files near king - penalize if no friendly pawn on file
    for (int df = -1; df <= 1; ++df) {
        File f = File(king_file + df);
        if (f < FILE_A || f > FILE_H) continue;

        Bitboard file_mask = file_bb(f);
        bool our_pawn_on_file = (our_pawns & file_mask) != 0;
        bool their_pawn_on_file = (their_pawns & file_mask) != 0;

        if (!our_pawn_on_file && !their_pawn_on_file) {
            score += OpenFileNearKingPenalty;
        } else if (!our_pawn_on_file) {
            score += SemiOpenFileNearKingPenalty;
        }
    }

    // Enemy piece attacks on king zone
    Bitboard king_zone = attacks_bb<KING>(king_sq);
    Bitboard occupied = pos.pieces();
    int attack_units = 0;
    int attacker_count = 0;

    // Knight attacks
    Bitboard enemy_knights = pos.pieces(them, KNIGHT);
    while (enemy_knights) {
        Square sq = pop_lsb(enemy_knights);
        Bitboard attacks = attacks_bb<KNIGHT>(sq);

        // Direct attack on king zone
        if (attacks & king_zone) {
            attack_units += ATTACK_WEIGHT_KNIGHT;
            attacker_count++;
        }
    }

    // Bishop attacks
    Bitboard enemy_bishops = pos.pieces(them, BISHOP);
    while (enemy_bishops) {
        Square sq = pop_lsb(enemy_bishops);
        Bitboard attacks = attacks_bb<BISHOP>(sq, occupied);

        if (attacks & king_zone) {
            attack_units += ATTACK_WEIGHT_BISHOP;
            attacker_count++;
        }
    }

    // Rook attacks
    Bitboard enemy_rooks = pos.pieces(them, ROOK);
    while (enemy_rooks) {
        Square sq = pop_lsb(enemy_rooks);
        Bitboard attacks = attacks_bb<ROOK>(sq, occupied);

        if (attacks & king_zone) {
            attack_units += ATTACK_WEIGHT_ROOK;
            attacker_count++;
        }
    }

    // Queen attacks
    Bitboard enemy_queens = pos.pieces(them, QUEEN);
    while (enemy_queens) {
        Square sq = pop_lsb(enemy_queens);
        Bitboard attacks = attacks_bb<QUEEN>(sq, occupied);

        if (attacks & king_zone) {
            attack_units += ATTACK_WEIGHT_QUEEN;
            attacker_count++;
        }
    }

    // Apply non-linear safety penalty based on total attack units
    // Multiple attackers scale up the danger dramatically
    if (attacker_count >= 2) {
        attack_units = attack_units * attacker_count / 2;  // Scale up for multiple attackers
    }

    if (attack_units > 0 && attack_units < 100) {
        score -= S(SAFETY_TABLE[attack_units], SAFETY_TABLE[attack_units] / 4);
    } else if (attack_units >= 100) {
        score -= S(900, 225);  // Maximum penalty
    }

    return score;
}

// ============================================================================
// Mobility Evaluation
// ============================================================================

Score evaluate_mobility(const Position& pos, Color us) {
    Score score = S(0, 0);
    Color them = ~us;

    Bitboard occupied = pos.pieces();
    Bitboard our_pieces = pos.pieces(us);
    Bitboard their_pawns = pos.pieces(them, PAWN);

    // Areas controlled by enemy pawns (dangerous for our pieces)
    Bitboard enemy_pawn_attacks = (us == WHITE)
        ? pawn_attacks_bb<BLACK>(pos.pieces(BLACK, PAWN))
        : pawn_attacks_bb<WHITE>(pos.pieces(WHITE, PAWN));

    // Safe squares for our pieces (not attacked by enemy pawns)
    Bitboard safe_squares = ~enemy_pawn_attacks;

    // Knight mobility
    Bitboard knights = pos.pieces(us, KNIGHT);
    while (knights) {
        Square sq = pop_lsb(knights);
        Bitboard attacks = attacks_bb<KNIGHT>(sq) & safe_squares & ~our_pieces;
        score += KnightMobility * popcount(attacks);
    }

    // Bishop mobility
    Bitboard bishops = pos.pieces(us, BISHOP);
    while (bishops) {
        Square sq = pop_lsb(bishops);
        Bitboard attacks = attacks_bb<BISHOP>(sq, occupied) & safe_squares & ~our_pieces;
        score += BishopMobility * popcount(attacks);
    }

    // Rook mobility and file evaluation
    Bitboard rooks = pos.pieces(us, ROOK);
    Bitboard our_pawns = pos.pieces(us, PAWN);

    while (rooks) {
        Square sq = pop_lsb(rooks);
        Bitboard attacks = attacks_bb<ROOK>(sq, occupied) & ~our_pieces;
        int mob = popcount(attacks & safe_squares);
        score += RookMobility * mob;

        // Rook on open/semi-open file
        File f = file_of(sq);
        Bitboard file_mask = file_bb(f);

        if (!(our_pawns & file_mask)) {
            if (!(their_pawns & file_mask)) {
                score += RookOnOpenFile;
            } else {
                score += RookOnSemiOpenFile;
            }
        }

        // Rook on 7th rank
        Rank rel_rank = relative_rank(us, sq);
        if (rel_rank == RANK_7) {
            score += RookOn7thRank;
        }
    }

    // Queen mobility
    Bitboard queens = pos.pieces(us, QUEEN);
    while (queens) {
        Square sq = pop_lsb(queens);
        Bitboard attacks = attacks_bb<QUEEN>(sq, occupied) & safe_squares & ~our_pieces;
        score += QueenMobility * popcount(attacks);
    }

    return score;
}

// ============================================================================
// Main Evaluation Function
// ============================================================================

Value evaluate(const Position& pos) {
    Score score = S(0, 0);
    Color us = pos.side_to_move();
    Color them = ~us;

    // Check for insufficient material draws (K vs K, K+B vs K, K+N vs K)
    if (pos.count<ALL_PIECES>() <= 3 && !pos.pieces(PAWN) && !pos.pieces(ROOK) && !pos.pieces(QUEEN)) {
        return VALUE_DRAW;
    }

    // Calculate game phase for tapered evaluation
    int phase = calculate_phase(pos);

    // Material evaluation
    score += evaluate_material(pos, us);

    // Piece-square table evaluation
    score += evaluate_pst(pos, us);

    // Pawn structure (with hash table for basic pawn-only features)
    Key pk = pos.pawn_key();
    int pawn_idx = pk & (PAWN_HASH_SIZE - 1);
    PawnHashEntry& pe = pawn_hash_table[pawn_idx];

    Score pawn_white, pawn_black;
    if (pe.key == pk) {
        pawn_white = pe.scores[WHITE];
        pawn_black = pe.scores[BLACK];
    } else {
        pawn_white = evaluate_pawn_structure_basic(pos, WHITE);
        pawn_black = evaluate_pawn_structure_basic(pos, BLACK);
        pe.key = pk;
        pe.scores[WHITE] = pawn_white;
        pe.scores[BLACK] = pawn_black;
    }

    if (us == WHITE)
        score += pawn_white - pawn_black;
    else
        score += pawn_black - pawn_white;

    // Piece-dependent passed pawn adjustments (not cached)
    score += evaluate_passed_pawn_pieces(pos, us);
    score -= evaluate_passed_pawn_pieces(pos, them);

    // King safety (weighted more heavily in middlegame)
    Score king_safety_us = evaluate_king_safety(pos, us);
    Score king_safety_them = evaluate_king_safety(pos, them);
    score += king_safety_us;
    score -= king_safety_them;

    // Mobility
    score += evaluate_mobility(pos, us);
    score -= evaluate_mobility(pos, them);

    // Taper the score based on game phase
    Value tapered = taper_score(score, phase);

    // Tempo bonus for side to move - TUNED
    tapered += 13;

    // Endgame scale factors for drawish positions
    if (std::abs(tapered) > 0) {
        int scale = 128;  // Normal = 128 (no scaling)

        int white_pawns = popcount(pos.pieces(WHITE, PAWN));
        int black_pawns = popcount(pos.pieces(BLACK, PAWN));
        int total_pawns = white_pawns + black_pawns;

        // Opposite-colored bishops endgame (very drawish)
        if (popcount(pos.pieces(WHITE, BISHOP)) == 1 &&
            popcount(pos.pieces(BLACK, BISHOP)) == 1 &&
            !pos.pieces(KNIGHT) && !pos.pieces(ROOK) && !pos.pieces(QUEEN)) {
            Bitboard wb = pos.pieces(WHITE, BISHOP);
            Bitboard bb = pos.pieces(BLACK, BISHOP);
            // Check if on opposite colors
            bool white_on_light = (wb & LightSquares) != 0;
            bool black_on_light = (bb & LightSquares) != 0;
            if (white_on_light != black_on_light) {
                scale = total_pawns <= 2 ? 32 : 64;  // Very drawish
            }
        }

        // No pawns - very hard to win with minor material advantage
        if (total_pawns == 0) {
            // Side with advantage needs enough material to force mate
            Value stronger_npm = tapered > 0 ?
                pos.non_pawn_material(us) : pos.non_pawn_material(~us);
            if (stronger_npm <= BishopValue) {
                scale = 0;  // Can't force mate with just a minor piece
            } else if (stronger_npm <= 2 * KnightValue) {
                scale = 16;  // Very difficult to mate
            }
        }

        if (scale != 128) {
            tapered = Value(tapered * scale / 128);
        }
    }

    return tapered;
}

// ============================================================================
// Initialization
// ============================================================================

void init() {
    // Clear pawn hash table
    for (int i = 0; i < PAWN_HASH_SIZE; ++i) {
        pawn_hash_table[i].key = 0;
        pawn_hash_table[i].scores[WHITE] = S(0, 0);
        pawn_hash_table[i].scores[BLACK] = S(0, 0);
    }
}

} // namespace Eval

} // namespace Boudica
