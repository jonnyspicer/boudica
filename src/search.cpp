#include "search.h"
#include "position.h"
#include "movegen.h"
#include "evaluate.h"
#include "tt.h"
#include "uci.h"
#include "book.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <cmath>

namespace Boudica {

// ============================================================================
// Global Variables
// ============================================================================
std::atomic<bool> stop_search{false};
MoveOrderingTables move_ordering;
SearchInfo search_info;
TimeManager time_manager;
int skill_level = 20;

// LMR reduction table
int LMR_TABLE[MAX_PLY][MAX_MOVES];

// ============================================================================
// LMR Table Initialization
// ============================================================================
void init_lmr_table() {
    for (int depth = 0; depth < MAX_PLY; ++depth) {
        for (int moves = 0; moves < MAX_MOVES; ++moves) {
            if (depth == 0 || moves == 0) {
                LMR_TABLE[depth][moves] = 0;
            } else {
                // Standard LMR formula: log(depth) * log(moves)
                LMR_TABLE[depth][moves] = static_cast<int>(
                    0.5 + std::log(depth) * std::log(moves) / 2.5
                );
            }
        }
    }
}

// ============================================================================
// Time Manager Implementation
// ============================================================================
void TimeManager::init(const SearchLimits& limits, Color us) {
    start_time = Clock::now();
    last_check = 0;

    if (limits.movetime > 0) {
        // Fixed time per move - use 90% of allocated for safety margin
        allocated_time = limits.movetime * 9 / 10;
        max_time = limits.movetime;
    } else if (limits.time[us] > 0) {
        // Time control: allocate time/moves_to_go + most of increment
        int time_left = limits.time[us];
        int increment = limits.inc[us];
        int moves_to_go = limits.movestogo > 0 ? limits.movestogo : 30;

        // Base allocation: time_left / moves_to_go + 75% of increment
        allocated_time = time_left / moves_to_go + increment * 3 / 4;

        // Don't use more than 25% of remaining time in a single move
        allocated_time = std::min(allocated_time, static_cast<uint64_t>(time_left / 4));

        // Hard limit: don't exceed 40% of remaining time
        max_time = static_cast<uint64_t>(time_left * 2 / 5);

        // Minimum allocation - 10ms
        allocated_time = std::max(allocated_time, static_cast<uint64_t>(10));
    } else if (limits.infinite || limits.depth > 0) {
        // Infinite or depth-limited search
        allocated_time = UINT64_MAX;
        max_time = UINT64_MAX;
    } else {
        // Default: 1 second
        allocated_time = 1000;
        max_time = 5000;
    }
}

bool TimeManager::should_stop(uint64_t nodes) const {
    // Check time periodically (every 1024 nodes is sufficient)
    if (nodes - last_check < 1024) {
        return false;
    }
    last_check = nodes;

    // Stop if we've used our allocated time
    uint64_t elapsed_time = elapsed();
    if (elapsed_time >= allocated_time) {
        return true;
    }

    // Also stop if we're close to max_time (hard limit safety)
    if (elapsed_time >= max_time * 9 / 10) {
        return true;
    }

    return false;
}

uint64_t TimeManager::elapsed() const {
    auto now = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time
    ).count();
}

// ============================================================================
// Search Initialization
// ============================================================================
void init_search() {
    stop_search = false;
    search_info.clear();
    init_lmr_table();
}

void clear_search() {
    move_ordering.clear();
    search_info.clear();
    stop_search = false;
}

// ============================================================================
// Mate Score Helpers
// ============================================================================
Value mated_in(int ply) {
    return Value(-VALUE_MATE + ply);
}

Value mate_in(int ply) {
    return Value(VALUE_MATE - ply);
}

// ============================================================================
// Draw Detection
// ============================================================================
bool is_draw(const Position& pos, int ply) {
    // Use Position's built-in draw detection
    return pos.is_draw(ply);
}

// ============================================================================
// Move Scoring for Ordering
// ============================================================================
void score_moves(Position& pos, ScoredMove* moves, int move_count,
                 Move tt_move, int ply, Move prev_move) {
    Color us = pos.side_to_move();

    // Get previous move info for continuation history
    Piece prev_piece = NO_PIECE;
    Square prev_to = SQ_NONE;
    if (prev_move != MOVE_NONE && prev_move != MOVE_NULL) {
        prev_piece = pos.piece_on(to_sq(prev_move));
        prev_to = to_sq(prev_move);
    }

    for (int i = 0; i < move_count; ++i) {
        Move m = moves[i].move;
        int score = 0;

        if (m == tt_move) {
            // TT move gets highest priority
            score = SCORE_TT_MOVE;
        }
        else if (pos.capture(m)) {
            // Captures: use MVV-LVA
            PieceType victim = type_of(pos.piece_on(to_sq(m)));
            PieceType attacker = type_of(pos.piece_on(from_sq(m)));

            // Check if capture is winning using SEE or simple comparison
            int mvv_lva = MVV_LVA[victim][attacker];

            // Simple heuristic: capturing more valuable piece is good
            if (victim >= attacker) {
                score = SCORE_GOOD_CAPTURE + mvv_lva * 100;
            } else {
                // Potentially losing capture - check with SEE
                if (pos.see_ge(m, Value(0))) {
                    score = SCORE_GOOD_CAPTURE + mvv_lva * 100;
                } else {
                    score = SCORE_BAD_CAPTURE + mvv_lva * 100;
                }
            }
        }
        else if (type_of(m) == PROMOTION) {
            // Promotions are high priority
            PieceType promo = promotion_type(m);
            score = SCORE_GOOD_CAPTURE + (promo == QUEEN ? 1000 : 500);
        }
        else {
            // Quiet moves: use killer moves, history, and continuation history
            if (move_ordering.killers[ply][0] == m) {
                score = SCORE_KILLER_1;
            }
            else if (move_ordering.killers[ply][1] == m) {
                score = SCORE_KILLER_2;
            }
            else if (prev_move != MOVE_NONE && prev_move != MOVE_NULL) {
                // Check counter move
                if (prev_piece != NO_PIECE &&
                    move_ordering.countermoves[prev_piece][prev_to] == m) {
                    score = SCORE_COUNTERMOVE;
                } else {
                    // History heuristic + continuation history
                    Piece curr_piece = pos.piece_on(from_sq(m));
                    Square curr_to = to_sq(m);
                    score = SCORE_QUIET_BASE + move_ordering.get_history(us, m);
                    score += move_ordering.get_continuation_history(prev_piece, prev_to, curr_piece, curr_to);
                }
            }
            else {
                // History heuristic only
                score = SCORE_QUIET_BASE + move_ordering.get_history(us, m);
            }
        }

        moves[i].score = score;
    }
}

// ============================================================================
// Move Picking (Selection Sort - good for small N with early cutoffs)
// ============================================================================
Move pick_next_move(ScoredMove* moves, int move_count, int current) {
    // Find the best move from current position onwards
    int best_idx = current;
    int best_score = moves[current].score;

    for (int i = current + 1; i < move_count; ++i) {
        if (moves[i].score > best_score) {
            best_score = moves[i].score;
            best_idx = i;
        }
    }

    // Swap best move to current position
    if (best_idx != current) {
        std::swap(moves[current], moves[best_idx]);
    }

    return moves[current].move;
}

// ============================================================================
// UCI Info Output
// ============================================================================
void print_uci_info(const Position& pos, int depth, Value score,
                    const Move* pv, int pv_length, uint64_t nodes, uint64_t time_ms) {
    std::cout << "info depth " << depth;

    // Selective depth (max ply reached)
    if (search_info.seldepth > 0) {
        std::cout << " seldepth " << search_info.seldepth;
    }

    // Score (mate or centipawns)
    if (std::abs(score) >= VALUE_MATE_IN_MAX_PLY) {
        int mate_ply = VALUE_MATE - std::abs(score);
        int mate_moves = (mate_ply + 1) / 2;
        if (score > 0) {
            std::cout << " score mate " << mate_moves;
        } else {
            std::cout << " score mate -" << mate_moves;
        }
    } else {
        std::cout << " score cp " << score;
    }

    // Nodes and time
    std::cout << " nodes " << nodes;
    std::cout << " time " << time_ms;

    // NPS (nodes per second)
    if (time_ms > 0) {
        uint64_t nps = nodes * 1000 / time_ms;
        std::cout << " nps " << nps;
    }

    // Principal variation
    if (pv_length > 0) {
        std::cout << " pv";
        for (int i = 0; i < pv_length; ++i) {
            std::cout << " " << UCI::moveToUCI(pv[i]);
        }
    }

    std::cout << std::endl;
}

std::string pv_to_string(const Move* pv, int length) {
    std::ostringstream ss;
    for (int i = 0; i < length; ++i) {
        if (i > 0) ss << " ";
        ss << square_to_string(from_sq(pv[i])) << square_to_string(to_sq(pv[i]));
        if (type_of(pv[i]) == PROMOTION) {
            const char promo_chars[] = " nbrq";
            ss << promo_chars[promotion_type(pv[i])];
        }
    }
    return ss.str();
}

// ============================================================================
// Quiescence Search
// ============================================================================
Value qsearch(Position& pos, SearchStack* ss, Value alpha, Value beta) {
    // Initialize PV for this node (prevent stale PV data)
    ss->pv_length = 0;

    // Check for stop signal
    if (stop_search) {
        return VALUE_ZERO;
    }

    // Update node count and check time periodically
    search_info.nodes++;
    if (time_manager.should_stop(search_info.nodes)) {
        stop_search = true;
        return VALUE_ZERO;
    }

    // Update selective depth
    if (ss->ply > search_info.seldepth) {
        search_info.seldepth = ss->ply;
    }

    // Check for immediate draw (pass ss->ply for repetition detection within search tree)
    if (is_draw(pos, ss->ply)) {
        return VALUE_DRAW;
    }

    // Max ply check - limit total search depth to prevent explosion
    if (ss->ply >= MAX_PLY - 1) {
        return !ss->in_check ? Eval::evaluate(pos) : VALUE_DRAW;
    }

    // Qsearch depth limit: if we're more than QSEARCH_MAX_PLY beyond the
    // main search depth recorded in seldepth baseline, return static eval
    // This prevents qsearch from exploding in complex tactical positions
    if (ss->ply > search_info.depth + QSEARCH_MAX_PLY && !ss->in_check) {
        return Eval::evaluate(pos);
    }

    // Determine if we're in check
    bool in_check = bool(pos.checkers());
    ss->in_check = in_check;

    Value best_value;
    Value futility_base;

    if (!in_check) {
        // Stand pat: use static evaluation as a lower bound
        best_value = Eval::evaluate(pos);

        if (best_value >= beta) {
            return best_value;  // Fail high (stand pat is good enough)
        }

        if (best_value > alpha) {
            alpha = best_value;
        }

        // Delta pruning setup
        futility_base = best_value + DELTA_MARGIN;
    } else {
        // In check: no stand pat, must search all evasions
        best_value = mated_in(ss->ply);
        futility_base = VALUE_INFINITE;  // Don't do delta pruning in check
    }

    // Generate captures (and checks at depth 0)
    ExtMove moves[MAX_MOVES];
    int move_count;

    if (in_check) {
        // When in check, generate all evasions
        move_count = generate<EVASIONS>(pos, moves) - moves;
    } else {
        // Only captures and queen promotions
        move_count = generate<CAPTURES>(pos, moves) - moves;
    }

    if (move_count == 0) {
        if (in_check) {
            return mated_in(ss->ply);  // Checkmate
        }
        return best_value;  // No captures, return stand pat
    }

    // Score captures for ordering
    ScoredMove scored_moves[MAX_MOVES];
    for (int i = 0; i < move_count; ++i) {
        scored_moves[i].move = moves[i];
        // Simple MVV-LVA scoring for qsearch
        PieceType victim = type_of(pos.piece_on(to_sq(moves[i])));
        PieceType attacker = type_of(pos.piece_on(from_sq(moves[i])));
        scored_moves[i].score = MVV_LVA[victim][attacker] * 100;
    }

    // Search captures
    for (int i = 0; i < move_count; ++i) {
        Move m = pick_next_move(scored_moves, move_count, i);

        // Delta pruning: skip if capture can't raise alpha
        if (!in_check && futility_base <= alpha) {
            PieceType captured = type_of(pos.piece_on(to_sq(m)));
            Value piece_value = Value(0);
            switch (captured) {
                case PAWN:   piece_value = PawnValue; break;
                case KNIGHT: piece_value = KnightValue; break;
                case BISHOP: piece_value = BishopValue; break;
                case ROOK:   piece_value = RookValue; break;
                case QUEEN:  piece_value = QueenValue; break;
                default: break;
            }

            if (futility_base + piece_value <= alpha) {
                continue;  // Delta prune
            }
        }

        // SEE pruning: skip obviously losing captures (not in check)
        if (!in_check && !pos.see_ge(m, Value(0))) {
            continue;
        }

        // Check legality and make the move
        if (!pos.legal(m)) {
            continue;
        }

        StateInfo st;
        pos.do_move(m, st);

        // Recursive search
        (ss + 1)->ply = ss->ply + 1;
        Value value = -qsearch(pos, ss + 1, -beta, -alpha);

        // Unmake the move
        pos.undo_move(m);

        if (stop_search) {
            return VALUE_ZERO;
        }

        // Update best value
        if (value > best_value) {
            best_value = value;

            if (value > alpha) {
                alpha = value;

                if (value >= beta) {
                    break;  // Fail high (beta cutoff)
                }
            }
        }
    }

    return best_value;
}

// ============================================================================
// Main Alpha-Beta Search
// ============================================================================
Value search(Position& pos, SearchStack* ss, int depth, Value alpha, Value beta, bool cut_node) {
    // At leaf node, drop into quiescence
    if (depth <= 0) {
        return qsearch(pos, ss, alpha, beta);
    }

    // Check for stop signal
    if (stop_search) {
        return VALUE_ZERO;
    }

    // Update node count and check time periodically
    search_info.nodes++;
    if (time_manager.should_stop(search_info.nodes)) {
        stop_search = true;
        return VALUE_ZERO;
    }

    // Initialize this node
    const bool root_node = (ss->ply == 0);
    const bool pv_node = (beta - alpha > 1);
    const Value original_alpha = alpha;  // Save for TT bound determination
    bool in_check = bool(pos.checkers());
    ss->in_check = in_check;
    ss->pv_length = 0;

    // Node-level check extension: when in check, extend depth by 1
    // This ensures all evasion moves get adequate depth (matches TSCP behavior)
    if (in_check) {
        depth += 1;
    }

    // Update selective depth
    if (ss->ply > search_info.seldepth) {
        search_info.seldepth = ss->ply;
    }

    // Check for immediate draw (pass ss->ply for repetition detection within search tree)
    if (!root_node && is_draw(pos, ss->ply)) {
        return VALUE_DRAW;
    }

    // Max ply check
    if (ss->ply >= MAX_PLY - 1) {
        return !in_check ? Eval::evaluate(pos) : VALUE_DRAW;
    }

    // Mate distance pruning
    if (!root_node) {
        alpha = std::max(mated_in(ss->ply), alpha);
        beta = std::min(mate_in(ss->ply + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }
    }

    // ========================================================================
    // Transposition Table Probe
    // ========================================================================
    Key pos_key = pos.key();
    Move tt_move = MOVE_NONE;
    Value tt_value = VALUE_NONE;
    int tt_depth = DEPTH_NONE;
    Bound tt_bound = BOUND_NONE;
    bool tt_hit = false;

    TTEntry* tte = TT.probe(pos_key, tt_hit);

    if (tt_hit) {
        tt_move = tte->move();
        tt_value = value_from_tt(tte->score(), ss->ply);  // Adjust mate score for ply
        tt_depth = tte->depth();
        tt_bound = tte->bound();

        // Use TT cutoff in non-PV nodes
        if (!pv_node && tt_depth >= depth) {
            if ((tt_bound == BOUND_EXACT) ||
                (tt_bound == BOUND_LOWER && tt_value >= beta) ||
                (tt_bound == BOUND_UPPER && tt_value <= alpha)) {
                return tt_value;
            }
        }
    }

    ss->tt_hit = tt_hit;
    ss->tt_move = tt_move;

    // ========================================================================
    // Static Evaluation
    // ========================================================================
    Value static_eval;
    if (in_check) {
        static_eval = VALUE_NONE;
    } else {
        static_eval = Eval::evaluate(pos);
    }
    ss->static_eval = static_eval;

    // ========================================================================
    // Improving Flag
    // ========================================================================
    // Position is "improving" if our static eval is better than 2 plies ago.
    // Used to adjust pruning aggressiveness throughout the search.
    bool improving = !in_check && ss->ply >= 2 &&
                     (ss - 2)->static_eval != VALUE_NONE &&
                     static_eval > (ss - 2)->static_eval;

    // ========================================================================
    // Internal Iterative Reductions (IIR)
    // ========================================================================
    if (depth >= 4 && !tt_move && !in_check) {
        depth -= 1;
    }

    // ========================================================================
    // Razoring
    // ========================================================================
    if (!pv_node && !in_check && depth <= RAZOR_MAX_DEPTH && depth >= 1) {
        Value razor_margin = RAZOR_MARGIN_BASE + RAZOR_MARGIN_PER_DEPTH * depth;
        if (static_eval + razor_margin <= alpha) {
            Value razor_value = qsearch(pos, ss, alpha, beta);
            if (razor_value <= alpha) {
                return razor_value;
            }
        }
    }

    // ========================================================================
    // Futility Pruning Setup
    // ========================================================================
    // When not improving, use smaller margin (prune more aggressively)
    bool futility_pruning = false;
    if (!pv_node && !in_check && depth <= FUTILITY_MAX_DEPTH && depth >= 1) {
        Value margin = FUTILITY_MARGIN[depth];
        if (!improving) margin = Value(int(margin) * 3 / 4);  // 25% smaller margin when not improving
        futility_pruning = (static_eval + margin <= alpha);
    }

    // ========================================================================
    // Reverse Futility Pruning (Static Null Move Pruning)
    // ========================================================================
    // When improving: margin = 80/depth (prune more, position is reliably good)
    // When not improving: margin = 120/depth (prune less, eval less trustworthy)
    if (!pv_node && !in_check && depth <= 5 &&
        static_eval - (improving ? 80 : 120) * depth >= beta &&
        static_eval < VALUE_KNOWN_WIN) {
        return static_eval;
    }

    // ========================================================================
    // Null Move Pruning
    // ========================================================================
    if (!pv_node &&
        !in_check &&
        depth >= NULL_MOVE_MIN_DEPTH &&
        static_eval >= beta &&
        pos.non_pawn_material(pos.side_to_move()) > VALUE_ZERO &&
        (ss - 1)->current_move != MOVE_NULL) {

        int R = NULL_MOVE_REDUCTION + depth / 4 + improving;

        ss->current_move = MOVE_NULL;
        StateInfo null_st;
        pos.do_null_move(null_st);

        (ss + 1)->ply = ss->ply + 1;
        Value null_value = -search(pos, ss + 1, depth - 1 - R, -beta, -beta + 1, !cut_node);

        pos.undo_null_move();

        if (stop_search) {
            return VALUE_ZERO;
        }

        if (null_value >= beta) {
            // Don't return unproven mate scores
            if (null_value >= VALUE_MATE_IN_MAX_PLY) {
                null_value = beta;
            }
            return null_value;
        }
    }

    // ========================================================================
    // Probcut - If a shallow search with raised beta confirms we're far
    // above beta, prune. Only for non-PV nodes at moderate depth.
    // ========================================================================
    if (!pv_node && !in_check && depth >= 5 &&
        std::abs(beta) < VALUE_MATE_IN_MAX_PLY) {

        Value probcut_beta = beta + 200;

        // Try captures that might confirm we're above beta
        ExtMove pc_moves[MAX_MOVES];
        int pc_count = generate<CAPTURES>(pos, pc_moves) - pc_moves;

        for (int i = 0; i < pc_count; ++i) {
            Move m = pc_moves[i];
            if (!pos.legal(m)) continue;

            // Only try captures with positive SEE
            if (!pos.see_ge(m, Value(0))) continue;

            StateInfo st;
            pos.do_move(m, st);
            (ss + 1)->ply = ss->ply + 1;

            // Search at reduced depth with raised beta
            Value value = -search(pos, ss + 1, depth - 4, -probcut_beta, -probcut_beta + 1, !cut_node);

            pos.undo_move(m);

            if (stop_search) return VALUE_ZERO;

            if (value >= probcut_beta) {
                return value;
            }
        }
    }

    // ========================================================================
    // Move Generation and Ordering
    // ========================================================================
    ExtMove moves[MAX_MOVES];
    int move_count;

    if (in_check) {
        move_count = generate<EVASIONS>(pos, moves) - moves;
    } else {
        move_count = generate<LEGAL>(pos, moves) - moves;
    }

    // No legal moves: checkmate or stalemate
    if (move_count == 0) {
        return in_check ? mated_in(ss->ply) : VALUE_DRAW;
    }

    // Score and order moves
    ScoredMove scored_moves[MAX_MOVES];
    for (int i = 0; i < move_count; ++i) {
        scored_moves[i].move = moves[i];
    }

    Move prev_move = (ss - 1)->current_move;
    score_moves(pos, scored_moves, move_count, tt_move, ss->ply, prev_move);

    // ========================================================================
    // Move Loop
    // ========================================================================
    Move best_move = MOVE_NONE;
    Value best_value = mated_in(ss->ply);
    int moves_searched = 0;

    for (int i = 0; i < move_count; ++i) {
        // Extra time check at each move in the loop for safety
        if (time_manager.should_stop(search_info.nodes)) {
            stop_search = true;
            break;
        }

        Move m = pick_next_move(scored_moves, move_count, i);

        // Skip excluded move (for singular extension, not implemented here)
        if (m == ss->excluded_move) {
            continue;
        }

        bool is_capture = pos.capture(m);
        bool is_promotion = type_of(m) == PROMOTION;
        bool gives_check = pos.gives_check(m);

        // ====================================================================
        // Futility Pruning
        // ====================================================================
        // Skip quiet moves at shallow depths when static eval + margin < alpha
        if (futility_pruning &&
            moves_searched > 0 &&  // Don't prune the first move
            !is_capture &&
            !is_promotion &&
            !gives_check) {
            continue;
        }

        // ====================================================================
        // Late Move Pruning (LMP)
        // ====================================================================
        // Skip quiet moves at shallow depths after searching N moves
        // When improving, search 50% more moves before pruning
        if (!pv_node &&
            !in_check &&
            depth <= LMP_MAX_DEPTH &&
            !is_capture &&
            !is_promotion &&
            !gives_check) {
            // When not improving, use half the move count (prune more aggressively)
            int lmp_count = LMP_MOVE_COUNT[depth] / (1 + !improving);
            if (moves_searched >= lmp_count) {
                continue;
            }
        }

        // ====================================================================
        // SEE Pruning for Bad Captures
        // ====================================================================
        // At shallow depths, skip captures that lose significant material
        if (!pv_node && !in_check && depth <= 6 &&
            moves_searched > 0 && is_capture && !is_promotion &&
            !pos.see_ge(m, Value(-20 * depth * depth))) {
            continue;
        }

        // ====================================================================
        // Late Move Reductions (LMR)
        // ====================================================================
        int new_depth = depth - 1;
        int reduction = 0;

        if (depth >= LMR_MIN_DEPTH &&
            moves_searched >= LMR_FULL_MOVES &&
            !in_check &&
            !is_capture &&
            !is_promotion) {

                // Bounds check to prevent array overflow with extensions
            int lmr_depth = std::min(depth, MAX_PLY - 1);
            int lmr_moves = std::min(moves_searched, MAX_MOVES - 1);
            reduction = LMR_TABLE[lmr_depth][lmr_moves];

            // Reduce less for killer moves
            if (move_ordering.is_killer(m, ss->ply)) {
                reduction = std::max(0, reduction - 1);
            }

            // Increase reduction for cut nodes (expected to fail high)
            if (cut_node) {
                reduction += 1;
            }

            // Clamp: don't reduce below 0 or beyond new_depth - 1
            reduction = std::max(0, reduction);
            reduction = std::min(reduction, new_depth - 1);
        }

        // Check legality and make the move
        if (!pos.legal(m)) {
            continue;
        }

        // ====================================================================
        // Extensions (computed BEFORE making the move)
        // ====================================================================
        int extension = 0;

        // Singular extension: if the TT move is significantly better than
        // all alternatives, extend its search
        if (depth >= 8 && m == tt_move && !ss->excluded_move &&
            tte && (tte->bound() & BOUND_LOWER) && tte->depth() >= depth - 3) {

            Value tt_value = value_from_tt(tte->score(), ss->ply);
            Value singular_beta = tt_value - 2 * depth;

            ss->excluded_move = m;
            Value singular_value = search(pos, ss, (depth - 1) / 2, singular_beta - 1, singular_beta, cut_node);
            ss->excluded_move = MOVE_NONE;

            if (singular_value < singular_beta) {
                extension = 1;  // TT move is singular, extend it
            }
        }

        // Note: check extension is now done at node level (when in check)
        // rather than per-move (when gives check). This matches TSCP's approach
        // and ensures ALL evasion moves get extended, not just checking moves.

        int extended_depth = new_depth + extension;

        StateInfo st;
        Piece moved_piece = pos.piece_on(from_sq(m));
        Square moved_to = to_sq(m);
        pos.do_move(m, st);

        moves_searched++;
        ss->current_move = m;
        ss->moved_piece = moved_piece;
        ss->to_square = moved_to;
        ss->move_count = moves_searched;
        (ss + 1)->ply = ss->ply + 1;
        (ss + 1)->excluded_move = MOVE_NONE;

        Value value;

        // ====================================================================
        // Principal Variation Search (PVS)
        // ====================================================================
        if (moves_searched == 1) {
            // First move: full window search
            value = -search(pos, ss + 1, extended_depth, -beta, -alpha, false);
        } else {
            // Reduced depth search with null window
            value = -search(pos, ss + 1, extended_depth - reduction, -alpha - 1, -alpha, true);

            // If reduced search beat alpha, re-search at full depth
            if (value > alpha && reduction > 0) {
                value = -search(pos, ss + 1, extended_depth, -alpha - 1, -alpha, !cut_node);
            }

            // If still beats alpha in PV node, re-search with full window
            if (value > alpha && value < beta && pv_node) {
                value = -search(pos, ss + 1, extended_depth, -beta, -alpha, false);
            }
        }

        // Unmake the move
        pos.undo_move(m);

        if (stop_search) {
            return VALUE_ZERO;
        }

        // ====================================================================
        // Update Best Value
        // ====================================================================
        if (value > best_value) {
            best_value = value;
            best_move = m;

            if (value > alpha) {
                alpha = value;

                // Update PV (triangular PV table)
                ss->pv[0] = m;
                ss->pv_length = 1 + (ss + 1)->pv_length;
                for (int j = 0; j < (ss + 1)->pv_length && j < MAX_PLY - 1; ++j) {
                    ss->pv[j + 1] = (ss + 1)->pv[j];
                }

                // Copy to global search info at root
                if (root_node) {
                    search_info.pv_length = ss->pv_length;
                    for (int j = 0; j < ss->pv_length; ++j) {
                        search_info.pv[j] = ss->pv[j];
                    }
                }

                if (value >= beta) {
                    // Beta cutoff - update move ordering
                    if (!is_capture && !is_promotion) {
                        // Update killer moves
                        move_ordering.update_killers(m, ss->ply);

                        // Update history
                        int bonus = depth * depth;
                        move_ordering.update_history(pos.side_to_move(), m, bonus);

                        // Update continuation history
                        if ((ss - 1)->moved_piece != NO_PIECE) {
                            move_ordering.update_continuation_history(
                                (ss - 1)->moved_piece, (ss - 1)->to_square,
                                moved_piece, moved_to, bonus);
                        }

                        // Penalize other quiet moves that didn't cause cutoff
                        for (int j = 0; j < i; ++j) {
                            Move prev = scored_moves[j].move;
                            if (!pos.capture(prev)) {
                                move_ordering.update_history(pos.side_to_move(), prev, -bonus);
                                // Also penalize in continuation history
                                if ((ss - 1)->moved_piece != NO_PIECE) {
                                    Piece prev_piece_j = type_of(prev) == CASTLING ?
                                        make_piece(pos.side_to_move(), KING) :
                                        pos.piece_on(from_sq(prev));
                                    if (prev_piece_j != NO_PIECE) {
                                        move_ordering.update_continuation_history(
                                            (ss - 1)->moved_piece, (ss - 1)->to_square,
                                            prev_piece_j, to_sq(prev), -bonus);
                                    }
                                }
                            }
                        }

                        // Update counter move
                        if (prev_move != MOVE_NONE && prev_move != MOVE_NULL) {
                            Piece prev_piece = pos.piece_on(to_sq(prev_move));
                            if (prev_piece != NO_PIECE) {
                                move_ordering.countermoves[prev_piece][to_sq(prev_move)] = m;
                            }
                        }
                    }
                    break;  // Beta cutoff
                }
            }
        }
    }

    // ========================================================================
    // Store in Transposition Table
    // ========================================================================
    Bound bound;
    if (best_value >= beta) {
        bound = BOUND_LOWER;
    } else if (best_value <= original_alpha) {
        bound = BOUND_UPPER;
    } else {
        bound = BOUND_EXACT;
    }

    TT.store(pos_key, value_to_tt(best_value, ss->ply), bound, Depth(depth), best_move, static_eval);

    return best_value;
}

// ============================================================================
// Iterative Deepening - Main Entry Point
// ============================================================================
Move iterative_deepening(Position& pos, SearchLimits& limits) {
    // Initialize search
    init_search();

    // Increment TT generation for proper replacement
    TT.new_search();

    // Age move ordering tables - decay history by 50% instead of clearing
    // This preserves learned patterns from previous searches
    move_ordering.age();

    // Probe opening book first
    Move book_move = Book::probe(pos);
    if (book_move != MOVE_NONE) {
        search_info.score = VALUE_DRAW;
        search_info.pv_length = 1;
        search_info.pv[0] = book_move;
        std::cout << "info depth 1 score cp 0 nodes 1 pv "
                  << UCI::moveToUCI(book_move) << std::endl;
        return book_move;
    }

    // Initialize time manager
    Color us = pos.side_to_move();
    time_manager.init(limits, us);
    time_manager.set_start_time();

    // Initialize search stack
    SearchStack stack[MAX_PLY + 4];
    SearchStack* ss = stack + 2;  // Allow (ss-2) access

    for (int i = -2; i < MAX_PLY + 2; ++i) {
        (ss + i)->ply = i;
        (ss + i)->current_move = MOVE_NONE;
        (ss + i)->excluded_move = MOVE_NONE;
        (ss + i)->static_eval = VALUE_NONE;
        (ss + i)->in_check = false;
        (ss + i)->moved_piece = NO_PIECE;
        (ss + i)->to_square = SQ_NONE;
        (ss + i)->cont_history = nullptr;
    }

    Move best_move = MOVE_NONE;
    Value best_value = -VALUE_INFINITE;
    Move pv[MAX_PLY];
    int pv_length = 0;
    Move prev_best_move = MOVE_NONE;
    int best_move_stable = 0;  // Consecutive iterations with same best move

    // Determine max depth - the time manager handles stopping
    int max_depth = limits.depth > 0 ? limits.depth : DEPTH_MAX;

    // Skill Level depth cap: level 0 = depth 1, level 20 = no cap
    if (skill_level < 20) {
        int skill_max = 1 + skill_level;
        max_depth = std::min(max_depth, skill_max);
    }

    // ========================================================================
    // Iterative Deepening Loop
    // ========================================================================
    for (int depth = 1; depth <= max_depth; ++depth) {
        // Time check before starting new depth
        if (!limits.infinite && !limits.depth && depth > 1) {
            uint64_t elapsed = time_manager.elapsed();
            uint64_t allocated = time_manager.get_allocated_time();
            // Don't start new depth if we've used more than 50% of allocated time
            // (the next depth likely takes 2-3x the current depth)
            if (elapsed > allocated / 2) {
                break;
            }
        }

        search_info.depth = depth;
        search_info.seldepth = 0;

        // ====================================================================
        // Aspiration Windows (for depth > 4)
        // ====================================================================
        Value alpha = -VALUE_INFINITE;
        Value beta = VALUE_INFINITE;
        Value delta = ASPIRATION_WINDOW;

        if (depth >= 5 && std::abs(best_value) < VALUE_KNOWN_WIN) {
            alpha = std::max(Value(-VALUE_INFINITE), best_value - delta);
            beta = std::min(Value(VALUE_INFINITE), best_value + delta);
        }

        // Aspiration loop - widen window on fail high/low
        while (true) {
            Value value = search(pos, ss, depth, alpha, beta, false);

            if (stop_search) {
                // On timeout, do NOT overwrite best_value with VALUE_ZERO
                // from the interrupted search. Keep the value from the last
                // completed depth or aspiration iteration.
                break;
            }

            if (value <= alpha) {
                // Fail low: widen lower bound
                // Update best_value to track the failing score
                best_value = value;
                beta = Value((int(alpha) + int(beta)) / 2);
                alpha = std::max(Value(-VALUE_INFINITE), alpha - delta);
                delta = delta + delta / 2;
            } else if (value >= beta) {
                // Fail high: widen upper bound
                // Update best_value to track the failing score
                best_value = value;
                beta = std::min(Value(VALUE_INFINITE), beta + delta);
                delta = delta + delta / 2;
            } else {
                // Value within window - done
                best_value = value;
                break;
            }

            // Prevent infinite loop with extreme values
            if (delta >= ASPIRATION_MAX_DELTA) {
                alpha = -VALUE_INFINITE;
                beta = VALUE_INFINITE;
            }
        }

        if (stop_search && depth == 1) {
            // If we had to stop even at depth 1, we need some move
            ExtMove moves[MAX_MOVES];
            int count = bool(pos.checkers()) ?
                       generate<EVASIONS>(pos, moves) - moves :
                       generate<LEGAL>(pos, moves) - moves;
            if (count > 0) {
                best_move = moves[0];
            }
            break;
        }

        if (stop_search) {
            break;
        }

        // ====================================================================
        // Update Best Move and PV
        // ====================================================================
        if (search_info.pv_length > 0) {
            best_move = search_info.pv[0];
            pv_length = search_info.pv_length;
            for (int i = 0; i < pv_length; ++i) {
                pv[i] = search_info.pv[i];
            }
            // Track best move stability
            if (best_move == prev_best_move) {
                best_move_stable++;
            } else {
                best_move_stable = 0;
                prev_best_move = best_move;
            }
        }

        // ====================================================================
        // Print UCI Info
        // ====================================================================
        uint64_t time_ms = time_manager.elapsed();
        print_uci_info(pos, depth, best_value, pv, pv_length,
                       search_info.nodes, time_ms);

        // ====================================================================
        // Time Management - Early Exit with Best Move Stability
        // ====================================================================
        if (!limits.infinite && !limits.depth) {
            uint64_t allocated = time_manager.get_allocated_time();
            // If best move has been stable for 4+ iterations, exit earlier (40%)
            // If unstable (changed recently), allow more time (70%)
            int time_pct;
            if (best_move_stable >= 4) {
                time_pct = 40;  // Very stable, save time
            } else if (best_move_stable >= 2) {
                time_pct = 50;  // Moderately stable
            } else {
                time_pct = 65;  // Unstable, use more time
            }
            if (time_ms > allocated * time_pct / 100) {
                break;
            }
        }

        // Check node limit
        if (limits.nodes > 0 && search_info.nodes >= limits.nodes) {
            break;
        }

        // Check if found mate
        if (std::abs(best_value) >= VALUE_MATE_IN_MAX_PLY) {
            int mate_ply = VALUE_MATE - std::abs(best_value);
            if (depth >= mate_ply) {
                break;  // Found forced mate, no need to search deeper
            }
        }
    }

    // ========================================================================
    // Return Best Move
    // ========================================================================
    if (best_move == MOVE_NONE) {
        // Emergency: pick first legal move
        ExtMove moves[MAX_MOVES];
        int count = bool(pos.checkers()) ?
                   generate<EVASIONS>(pos, moves) - moves :
                   generate<LEGAL>(pos, moves) - moves;
        if (count > 0) {
            best_move = moves[0];
        }
    }

    // Store final search info
    search_info.score = best_value;
    search_info.time_ms = time_manager.elapsed();

    return best_move;
}

} // namespace Boudica
