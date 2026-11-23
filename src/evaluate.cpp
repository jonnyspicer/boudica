#include "evaluate.h"
#include "position.h"
#include "bitboard.h"

#include <cstdlib>  // for std::abs
#include <cstring>  // for std::memset

namespace Boudica {

namespace Eval {

// Tunable parameters, initialised to the defaults in eval_params.h
EvalParams params;

// ============================================================================
// Evaluation Trace
// ============================================================================
//
// Every evaluation function is a template on Trace. With Trace == false (the
// engine's path) the TRACE statements are compiled out; with Trace == true each
// parameter use also adds its occurrence count to trace->coef, signed so that
// White's occurrences count +1 and Black's -1.

#define TRACE(param_index, count) \
    do { if constexpr (Trace) trace->coef[param_index] += (count); } while (0)

#define IDX(member) EVAL_PARAM_INDEX(member)

// First trace index of each piece type's PST
constexpr int PstIndex[PIECE_TYPE_NB] = {
    0, IDX(PawnTable), IDX(KnightTable), IDX(BishopTable),
    IDX(RookTable), IDX(QueenTable), IDX(KingTable), 0
};

// ============================================================================
// Utility Functions
// ============================================================================

// Get the appropriate PST score for a piece at a square
inline Score pst_score(PieceType pt, Square sq) {
    switch (pt) {
        case PAWN:   return params.PawnTable[sq];
        case KNIGHT: return params.KnightTable[sq];
        case BISHOP: return params.BishopTable[sq];
        case ROOK:   return params.RookTable[sq];
        case QUEEN:  return params.QueenTable[sq];
        case KING:   return params.KingTable[sq];
        default:     return S(0, 0);
    }
}

// Flip square vertically for black's perspective
constexpr Square flip_square(Square sq) {
    return Square(sq ^ 56);
}

// +1 for White, -1 for Black: the sign of a color's terms in the trace
constexpr int trace_sign(Color c) {
    return c == WHITE ? 1 : -1;
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

template<bool Trace>
Score evaluate_material(const Position& pos, Color us, EvalTrace* trace) {
    Score score = S(0, 0);
    Color them = ~us;
    const int sign = trace_sign(us);

    // Count material for each piece type
    for (PieceType pt = PAWN; pt <= QUEEN; ++pt) {
        int our_count = popcount(pos.pieces(us, pt));
        int their_count = popcount(pos.pieces(them, pt));
        score += params.PieceValue[pt] * (our_count - their_count);
        TRACE(IDX(PieceValue) + pt, sign * (our_count - their_count));
    }

    // Bishop pair bonus
    if (popcount(pos.pieces(us, BISHOP)) >= 2) {
        score += params.BishopPairBonus;
        TRACE(IDX(BishopPairBonus), sign);
    }
    if (popcount(pos.pieces(them, BISHOP)) >= 2) {
        score -= params.BishopPairBonus;
        TRACE(IDX(BishopPairBonus), -sign);
    }

    return score;
}

// ============================================================================
// Piece-Square Table Evaluation
// ============================================================================

template<bool Trace>
Score evaluate_pst(const Position& pos, Color us, EvalTrace* trace) {
    Score score = S(0, 0);
    Color them = ~us;
    const int sign = trace_sign(us);

    // Evaluate our pieces
    for (PieceType pt = PAWN; pt <= KING; ++pt) {
        Bitboard bb = pos.pieces(us, pt);
        while (bb) {
            Square sq = pop_lsb(bb);
            // Use square as-is for white, flip for black
            Square eval_sq = (us == WHITE) ? sq : flip_square(sq);
            score += pst_score(pt, eval_sq);
            TRACE(PstIndex[pt] + eval_sq, sign);
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
            TRACE(PstIndex[pt] + eval_sq, -sign);
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

template<bool Trace>
Score evaluate_pawn_structure_basic(const Position& pos, Color us, EvalTrace* trace) {
    Score score = S(0, 0);
    Color them = ~us;
    const int sign = trace_sign(us);

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
            score += params.DoubledPawnPenalty;
            TRACE(IDX(DoubledPawnPenalty), sign);
        }

        // Isolated pawns - no friendly pawns on adjacent files
        Bitboard adjacent_files = adjacent_files_bb(sq);

        if (!(our_pawns & adjacent_files)) {
            score += params.IsolatedPawnPenalty;
            TRACE(IDX(IsolatedPawnPenalty), sign);
        }

        // Connected pawns - friendly pawn on adjacent file and same/adjacent rank
        Bitboard adjacent_ranks = rank_bb(r);
        if (r > RANK_1) adjacent_ranks |= rank_bb(Rank(r - 1));
        if (r < RANK_8) adjacent_ranks |= rank_bb(Rank(r + 1));

        if (our_pawns & adjacent_files & adjacent_ranks) {
            score += params.ConnectedPawnBonus;
            TRACE(IDX(ConnectedPawnBonus), sign);
        }

        // Backward pawns - no adjacent friendly pawn at same rank or behind,
        // and stop square is controlled by enemy pawn
        if (our_pawns & adjacent_files) {  // Not isolated (already penalized)
            Bitboard at_or_behind = ~ForwardRanksBB[us][r];
            if (!(our_pawns & adjacent_files & at_or_behind)) {
                Square stop = sq + pawn_push(us);
                if (stop >= SQ_A1 && stop <= SQ_H8) {
                    if (enemy_pawn_control & square_bb(stop)) {
                        score += params.BackwardPawnPenalty;
                        TRACE(IDX(BackwardPawnPenalty), sign);
                    }
                }
            }
        }

        // Passed pawns - base bonus (pawn-only: identification + rank bonus + protected)
        Bitboard blocking_zone = passed_pawn_span(us, sq);

        if (!(their_pawns & blocking_zone)) {
            Rank rel_rank = relative_rank(us, sq);
            score += params.PassedPawnBonus[rel_rank];
            TRACE(IDX(PassedPawnBonus) + rel_rank, sign);

            // Protected passed pawn (defended by another pawn)
            Bitboard pawn_defenders = (us == WHITE)
                ? pawn_attacks_bb<BLACK>(square_bb(sq))
                : pawn_attacks_bb<WHITE>(square_bb(sq));
            if (our_pawns & pawn_defenders) {
                score += params.ProtectedPasserBonus;
                TRACE(IDX(ProtectedPasserBonus), sign);
            }
        }
    }

    return score;
}

// ============================================================================
// Passed Pawn Piece-Dependent Bonuses (NOT cacheable)
// ============================================================================

template<bool Trace>
Score evaluate_passed_pawn_pieces(const Position& pos, Color us, EvalTrace* trace) {
    Score score = S(0, 0);
    Color them = ~us;
    const int sign = trace_sign(us);

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
                // Takes back part of the base bonus the cached function added
                score += params.BlockedPasserPenalty[rel_rank];
                TRACE(IDX(BlockedPasserPenalty) + rel_rank, sign);
            }

            // King proximity bonus in endgame - enemy king far is good
            Square enemy_king = pos.king_square(them);
            int enemy_king_dist = std::abs(file_of(sq) - file_of(enemy_king)) +
                                  std::abs(rank_of(front_sq) - rank_of(enemy_king));
            score += params.PasserEnemyKingDistance * enemy_king_dist;
            TRACE(IDX(PasserEnemyKingDistance), sign * enemy_king_dist);

            // Our king close to passed pawn in endgame is good
            Square our_king = pos.king_square(us);
            int our_king_dist = std::abs(file_of(sq) - file_of(our_king)) +
                                std::abs(rank_of(sq) - rank_of(our_king));
            score += params.PasserOwnKingProximity * (8 - our_king_dist);
            TRACE(IDX(PasserOwnKingProximity), sign * (8 - our_king_dist));
        }

        // Rook behind passed pawn bonus
        Bitboard file_mask = file_bb(f);
        Bitboard our_rooks = pos.pieces(us, ROOK);

        if (our_rooks & file_mask) {
            // Check if rook is behind the pawn
            Bitboard behind_span = ~ForwardRanksBB[us][r] & ~rank_bb(r) & file_mask;

            if (our_rooks & behind_span) {
                score += params.RookBehindPasserBonus;
                TRACE(IDX(RookBehindPasserBonus), sign);
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

template<bool Trace>
Score evaluate_king_safety(const Position& pos, Color us, EvalTrace* trace) {
    Score score = S(0, 0);
    Color them = ~us;
    const int sign = trace_sign(us);

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
        score += params.PawnShieldBonus * shield_pawns;
        TRACE(IDX(PawnShieldBonus), sign * shield_pawns);

        // Extra bonus for second rank shield (more protective)
        Rank shield_rank2 = (us == WHITE) ? Rank(king_rank + 2) : Rank(king_rank - 2);
        if (shield_rank2 >= RANK_1 && shield_rank2 <= RANK_8) {
            Bitboard shield_zone2 = Bitboard(0);
            if (king_file > FILE_A) shield_zone2 |= square_bb(make_square(File(king_file - 1), shield_rank2));
            shield_zone2 |= square_bb(make_square(king_file, shield_rank2));
            if (king_file < FILE_H) shield_zone2 |= square_bb(make_square(File(king_file + 1), shield_rank2));
            int shield_pawns2 = popcount(our_pawns & shield_zone2);
            score += params.PawnShield2Bonus * shield_pawns2;  // Smaller bonus for 2nd row
            TRACE(IDX(PawnShield2Bonus), sign * shield_pawns2);
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
            score += params.OpenFileNearKingPenalty;
            TRACE(IDX(OpenFileNearKingPenalty), sign);
        } else if (!our_pawn_on_file) {
            score += params.SemiOpenFileNearKingPenalty;
            TRACE(IDX(SemiOpenFileNearKingPenalty), sign);
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

    // Not tuned: the trace records this as its fixed nonlinear part
    Score attack_penalty = S(0, 0);
    if (attack_units > 0 && attack_units < 100) {
        attack_penalty = S(SAFETY_TABLE[attack_units], SAFETY_TABLE[attack_units] / 4);
    } else if (attack_units >= 100) {
        attack_penalty = S(900, 225);  // Maximum penalty
    }
    score -= attack_penalty;
    if constexpr (Trace) trace->nonlinear -= attack_penalty * sign;

    return score;
}

// ============================================================================
// Mobility Evaluation
// ============================================================================

template<bool Trace>
Score evaluate_mobility(const Position& pos, Color us, EvalTrace* trace) {
    Score score = S(0, 0);
    Color them = ~us;
    const int sign = trace_sign(us);

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
        int mob = popcount(attacks);
        score += params.KnightMobility * mob;
        TRACE(IDX(KnightMobility), sign * mob);
    }

    // Bishop mobility
    Bitboard bishops = pos.pieces(us, BISHOP);
    while (bishops) {
        Square sq = pop_lsb(bishops);
        Bitboard attacks = attacks_bb<BISHOP>(sq, occupied) & safe_squares & ~our_pieces;
        int mob = popcount(attacks);
        score += params.BishopMobility * mob;
        TRACE(IDX(BishopMobility), sign * mob);
    }

    // Rook mobility and file evaluation
    Bitboard rooks = pos.pieces(us, ROOK);
    Bitboard our_pawns = pos.pieces(us, PAWN);

    while (rooks) {
        Square sq = pop_lsb(rooks);
        Bitboard attacks = attacks_bb<ROOK>(sq, occupied) & ~our_pieces;
        int mob = popcount(attacks & safe_squares);
        score += params.RookMobility * mob;
        TRACE(IDX(RookMobility), sign * mob);

        // Rook on open/semi-open file
        File f = file_of(sq);
        Bitboard file_mask = file_bb(f);

        if (!(our_pawns & file_mask)) {
            if (!(their_pawns & file_mask)) {
                score += params.RookOnOpenFile;
                TRACE(IDX(RookOnOpenFile), sign);
            } else {
                score += params.RookOnSemiOpenFile;
                TRACE(IDX(RookOnSemiOpenFile), sign);
            }
        }

        // Rook on 7th rank
        Rank rel_rank = relative_rank(us, sq);
        if (rel_rank == RANK_7) {
            score += params.RookOn7thRank;
            TRACE(IDX(RookOn7thRank), sign);
        }
    }

    // Queen mobility
    Bitboard queens = pos.pieces(us, QUEEN);
    while (queens) {
        Square sq = pop_lsb(queens);
        Bitboard attacks = attacks_bb<QUEEN>(sq, occupied) & safe_squares & ~our_pieces;
        int mob = popcount(attacks);
        score += params.QueenMobility * mob;
        TRACE(IDX(QueenMobility), sign * mob);
    }

    return score;
}

// ============================================================================
// Main Evaluation Function
// ============================================================================

template<bool Trace>
Value evaluate_impl(const Position& pos, EvalTrace* trace) {
    Score score = S(0, 0);
    Color us = pos.side_to_move();
    Color them = ~us;

    // Check for insufficient material draws (K vs K, K+B vs K, K+N vs K)
    if (pos.count<ALL_PIECES>() <= 3 && !pos.pieces(PAWN) && !pos.pieces(ROOK) && !pos.pieces(QUEEN)) {
        if constexpr (Trace) trace->scale = 0;
        return VALUE_DRAW;
    }

    // Calculate game phase for tapered evaluation
    int phase = calculate_phase(pos);

    // Material evaluation
    score += evaluate_material<Trace>(pos, us, trace);

    // Piece-square table evaluation
    score += evaluate_pst<Trace>(pos, us, trace);

    // Pawn structure (with hash table for basic pawn-only features). The trace
    // bypasses the hash so the pawn terms are always recorded.
    Score pawn_white, pawn_black;
    if constexpr (Trace) {
        pawn_white = evaluate_pawn_structure_basic<true>(pos, WHITE, trace);
        pawn_black = evaluate_pawn_structure_basic<true>(pos, BLACK, trace);
    } else {
        Key pk = pos.pawn_key();
        int pawn_idx = pk & (PAWN_HASH_SIZE - 1);
        PawnHashEntry& pe = pawn_hash_table[pawn_idx];

        if (pe.key == pk) {
            pawn_white = pe.scores[WHITE];
            pawn_black = pe.scores[BLACK];
        } else {
            pawn_white = evaluate_pawn_structure_basic<false>(pos, WHITE, trace);
            pawn_black = evaluate_pawn_structure_basic<false>(pos, BLACK, trace);
            pe.key = pk;
            pe.scores[WHITE] = pawn_white;
            pe.scores[BLACK] = pawn_black;
        }
    }

    if (us == WHITE)
        score += pawn_white - pawn_black;
    else
        score += pawn_black - pawn_white;

    // Piece-dependent passed pawn adjustments (not cached)
    score += evaluate_passed_pawn_pieces<Trace>(pos, us, trace);
    score -= evaluate_passed_pawn_pieces<Trace>(pos, them, trace);

    // King safety (weighted more heavily in middlegame)
    Score king_safety_us = evaluate_king_safety<Trace>(pos, us, trace);
    Score king_safety_them = evaluate_king_safety<Trace>(pos, them, trace);
    score += king_safety_us;
    score -= king_safety_them;

    // Mobility
    score += evaluate_mobility<Trace>(pos, us, trace);
    score -= evaluate_mobility<Trace>(pos, them, trace);

    // Taper the score based on game phase
    Value tapered = taper_score(score, phase);

    // Tempo bonus for side to move
    tapered += params.Tempo;

    if constexpr (Trace) {
        trace->phase = phase;
        trace->tempo = trace_sign(us);
    }

    // Mop-up: against a bare king, drive it to the edge and bring our king
    // closer, so the search has a gradient towards mate (KRK, KQK, KBNK)
    if (!pos.pieces(PAWN)) {
        for (Color strong : {WHITE, BLACK}) {
            if (pos.non_pawn_material(~strong) == VALUE_ZERO
                && pos.non_pawn_material(strong) >= RookValue) {
                Square wk = pos.square<KING>(~strong), sk = pos.square<KING>(strong);
                int wf = file_of(wk), wr = rank_of(wk);
                int edge = std::max(3 - wf, wf - 4) + std::max(3 - wr, wr - 4);  // 0..6
                int kdist = std::abs(wf - file_of(sk)) + std::abs(wr - rank_of(sk));
                int bonus = 20 * edge + 6 * (14 - kdist);
                tapered += strong == us ? Value(bonus) : Value(-bonus);
            }
        }
    }

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

        // No pawns: winning needs a clear material edge, not just enough
        // material to mate (KR vs K wins, KR vs KR and KR vs KB are drawish)
        if (total_pawns == 0) {
            Color strong = tapered > 0 ? us : ~us;
            Value strong_npm = pos.non_pawn_material(strong);
            Value weak_npm = pos.non_pawn_material(~strong);
            if (strong_npm <= BishopValue) {
                scale = 0;   // A lone minor piece can't force mate
            } else if (strong_npm == 2 * KnightValue && weak_npm == VALUE_ZERO
                       && popcount(pos.pieces(strong, KNIGHT)) == 2) {
                scale = 0;   // KNN vs K can't force mate
            } else if (strong_npm - weak_npm <= BishopValue) {
                scale = 16;  // Edge of a minor piece or less: very hard to win
            }
        }

        if constexpr (Trace) trace->scale = scale;

        if (scale != 128) {
            tapered = Value(tapered * scale / 128);
        }
    }

    return tapered;
}

#undef TRACE
#undef IDX

Value evaluate(const Position& pos) {
    return evaluate_impl<false>(pos, nullptr);
}

Value evaluate_trace(const Position& pos, EvalTrace& trace) {
    trace = EvalTrace();
    return evaluate_impl<true>(pos, &trace);
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
