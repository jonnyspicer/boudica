#include "movegen.h"
#include "position.h"
#include "bitboard.h"

// Boudica Chess Engine - Move Generation Implementation
// Efficient bitboard-based pseudo-legal move generation

namespace Boudica {

namespace {

// Helper to create ExtMove with zero score
inline ExtMove make_ext_move(Move m) {
    ExtMove em;
    em.move = m;
    em.value = 0;
    return em;
}

// Generate pawn promotions (4 moves per promotion square)
template<Direction D>
ExtMove* make_promotions(ExtMove* moveList, Square to) {
    Square from = to - D;
    *moveList++ = make_ext_move(make<PROMOTION>(from, to, QUEEN));
    *moveList++ = make_ext_move(make<PROMOTION>(from, to, ROOK));
    *moveList++ = make_ext_move(make<PROMOTION>(from, to, BISHOP));
    *moveList++ = make_ext_move(make<PROMOTION>(from, to, KNIGHT));
    return moveList;
}

// Generate pawn moves for a given color
template<Color Us, GenType Type>
ExtMove* generate_pawn_moves(const Position& pos, ExtMove* moveList, Bitboard target) {

    constexpr Color Them = ~Us;
    constexpr Direction Up = pawn_push(Us);
    constexpr Direction UpRight = (Us == WHITE ? NORTH_EAST : SOUTH_WEST);
    constexpr Direction UpLeft = (Us == WHITE ? NORTH_WEST : SOUTH_EAST);
    constexpr Bitboard Rank7BB_C = (Us == WHITE ? Rank7BB : Rank2BB);
    constexpr Bitboard Rank3BB_C = (Us == WHITE ? Rank3BB : Rank6BB);

    Bitboard pawns = pos.pieces(Us, PAWN);
    Bitboard enemies = pos.pieces(Them);
    Bitboard empty = ~pos.pieces();

    Bitboard promotingPawns = pawns & Rank7BB_C;
    Bitboard nonPromotingPawns = pawns & ~Rank7BB_C;

    // Pawn captures (non-promoting)
    if constexpr (Type == CAPTURES || Type == EVASIONS || Type == NON_EVASIONS) {
        // For pawn captures, always need enemy pieces - diagonal pawn moves must be captures
        // In evasions, captures must also be to target squares (capture the checker)
        Bitboard capTargets = (Type == EVASIONS) ? (enemies & target) : enemies;

        // Right captures
        Bitboard b1 = shift<UpRight>(nonPromotingPawns) & capTargets;
        while (b1) {
            Square to = pop_lsb(b1);
            *moveList++ = make_ext_move(make_move(to - UpRight, to));
        }

        // Left captures
        Bitboard b2 = shift<UpLeft>(nonPromotingPawns) & capTargets;
        while (b2) {
            Square to = pop_lsb(b2);
            *moveList++ = make_ext_move(make_move(to - UpLeft, to));
        }

        // Promotion captures - must also respect target for evasions
        Bitboard promCapTargets = (Type == EVASIONS) ? (enemies & target) : enemies;
        Bitboard b3 = shift<UpRight>(promotingPawns) & promCapTargets;
        while (b3) {
            Square to = pop_lsb(b3);
            moveList = make_promotions<UpRight>(moveList, to);
        }

        Bitboard b4 = shift<UpLeft>(promotingPawns) & promCapTargets;
        while (b4) {
            Square to = pop_lsb(b4);
            moveList = make_promotions<UpLeft>(moveList, to);
        }

        // En passant captures - for evasions, only if ep square is in target
        // (capturing the ep pawn can only help if the pawn is the checker or blocks)
        if (pos.ep_square() != SQ_NONE) {
            Square epSq = pos.ep_square();
            // For evasions, check if the captured pawn square is in target
            // The captured pawn is on the same file as epSq, one rank behind
            Square capSq = epSq - Up;
            if constexpr (Type != EVASIONS) {
                Bitboard epPawns = nonPromotingPawns & pawn_attacks_bb(Them, epSq);
                while (epPawns) {
                    Square from = pop_lsb(epPawns);
                    *moveList++ = make_ext_move(make<EN_PASSANT>(from, epSq));
                }
            } else {
                // For evasions: ep is valid if landing on target OR capturing a piece on target
                if ((target & square_bb(epSq)) || (target & square_bb(capSq))) {
                    Bitboard epPawns = nonPromotingPawns & pawn_attacks_bb(Them, epSq);
                    while (epPawns) {
                        Square from = pop_lsb(epPawns);
                        *moveList++ = make_ext_move(make<EN_PASSANT>(from, epSq));
                    }
                }
            }
        }
    }

    // Quiet pawn moves (non-capturing)
    if constexpr (Type == QUIETS || Type == EVASIONS || Type == NON_EVASIONS) {
        Bitboard emptyTarget = (Type == EVASIONS) ? (target & empty) : empty;

        // Single push (non-promoting)
        Bitboard b1 = shift<Up>(nonPromotingPawns) & emptyTarget;

        // Double push - for evasions, only the destination needs to be in target,
        // the intermediate square just needs to be empty
        Bitboard singlePushAll = shift<Up>(nonPromotingPawns) & empty;
        Bitboard b2 = shift<Up>(singlePushAll & Rank3BB_C) & emptyTarget;

        while (b1) {
            Square to = pop_lsb(b1);
            *moveList++ = make_ext_move(make_move(to - Up, to));
        }

        while (b2) {
            Square to = pop_lsb(b2);
            *moveList++ = make_ext_move(make_move(to - Up - Up, to));
        }

        // Promotion pushes (non-capturing) - must also respect target for evasions
        Bitboard promPushTarget = (Type == EVASIONS) ? emptyTarget : empty;
        Bitboard b3 = shift<Up>(promotingPawns) & promPushTarget;
        while (b3) {
            Square to = pop_lsb(b3);
            moveList = make_promotions<Up>(moveList, to);
        }
    }

    return moveList;
}

// Generate piece moves (knight, bishop, rook, queen)
template<PieceType Pt, bool Checks>
ExtMove* generate_piece_moves(const Position& pos, ExtMove* moveList, Color us, Bitboard target) {

    Bitboard pieces = pos.pieces(us, Pt);

    while (pieces) {
        Square from = pop_lsb(pieces);
        Bitboard attacks;

        if constexpr (Pt == KNIGHT) {
            attacks = attacks_bb<KNIGHT>(from);
        } else if constexpr (Pt == BISHOP) {
            attacks = attacks_bb<BISHOP>(from, pos.pieces());
        } else if constexpr (Pt == ROOK) {
            attacks = attacks_bb<ROOK>(from, pos.pieces());
        } else if constexpr (Pt == QUEEN) {
            attacks = attacks_bb<QUEEN>(from, pos.pieces());
        }

        // For quiet checks, filter to only moves that give check
        if constexpr (Checks) {
            Square ksq = pos.king_square(~us);
            if constexpr (Pt == KNIGHT) {
                attacks &= attacks_bb<KNIGHT>(ksq);
            } else if constexpr (Pt == BISHOP) {
                attacks &= attacks_bb<BISHOP>(ksq, pos.pieces()) |
                           (aligned(from, ksq, from) ? pos.blockers_for_king(~us) : 0);
            } else if constexpr (Pt == ROOK) {
                attacks &= attacks_bb<ROOK>(ksq, pos.pieces()) |
                           (aligned(from, ksq, from) ? pos.blockers_for_king(~us) : 0);
            } else if constexpr (Pt == QUEEN) {
                attacks &= (attacks_bb<BISHOP>(ksq, pos.pieces()) |
                            attacks_bb<ROOK>(ksq, pos.pieces())) |
                           (aligned(from, ksq, from) ? pos.blockers_for_king(~us) : 0);
            }
        }

        Bitboard b = attacks & target;

        while (b) {
            Square to = pop_lsb(b);
            *moveList++ = make_ext_move(make_move(from, to));
        }
    }

    return moveList;
}

// Generate king moves (excluding castling)
ExtMove* generate_king_moves(const Position& pos, ExtMove* moveList, Color us, Bitboard target) {
    Square ksq = pos.king_square(us);
    Bitboard attacks = attacks_bb<KING>(ksq) & target;

    while (attacks) {
        Square to = pop_lsb(attacks);
        *moveList++ = make_ext_move(make_move(ksq, to));
    }

    return moveList;
}

// Generate castling moves
template<Color Us, CastlingRights Cr>
ExtMove* generate_castling(const Position& pos, ExtMove* moveList) {

    constexpr bool KingSide = (Cr == WHITE_OO || Cr == BLACK_OO);
    constexpr Square KingFrom = (Us == WHITE ? SQ_E1 : SQ_E8);
    constexpr Square KingTo = KingSide ? (Us == WHITE ? SQ_G1 : SQ_G8)
                                        : (Us == WHITE ? SQ_C1 : SQ_C8);
    constexpr Square RookFrom = KingSide ? (Us == WHITE ? SQ_H1 : SQ_H8)
                                          : (Us == WHITE ? SQ_A1 : SQ_A8);

    // Check if castling rights are available
    if (!(pos.castling_rights() & Cr))
        return moveList;

    // Check if rook is still there
    if (pos.piece_on(RookFrom) != make_piece(Us, ROOK))
        return moveList;

    // Define squares that must be empty
    Bitboard between;
    if constexpr (KingSide) {
        between = (Us == WHITE) ? (square_bb(SQ_F1) | square_bb(SQ_G1))
                                : (square_bb(SQ_F8) | square_bb(SQ_G8));
    } else {
        between = (Us == WHITE) ? (square_bb(SQ_B1) | square_bb(SQ_C1) |
                                   square_bb(SQ_D1))
                                : (square_bb(SQ_B8) | square_bb(SQ_C8) |
                                   square_bb(SQ_D8));
    }

    // Check if squares between king and rook are empty
    if (between & pos.pieces())
        return moveList;

    // Define squares king passes through (must not be attacked)
    // This includes the starting square, the destination, and squares in between
    Square kPath[3];
    int pathLen;
    if constexpr (KingSide) {
        kPath[0] = KingFrom;
        kPath[1] = KingFrom + EAST;
        kPath[2] = KingTo;
        pathLen = 3;
    } else {
        kPath[0] = KingFrom;
        kPath[1] = KingFrom + WEST;
        kPath[2] = KingTo;
        pathLen = 3;
    }

    // Check if king passes through check
    for (int i = 0; i < pathLen; ++i) {
        if (pos.attackers_to(kPath[i]) & pos.pieces(~Us))
            return moveList;
    }

    // Castling is legal - add the move
    *moveList++ = make_ext_move(make<CASTLING>(KingFrom, KingTo));
    return moveList;
}

// Generate all castling moves for a color
template<Color Us>
ExtMove* generate_all_castling(const Position& pos, ExtMove* moveList) {
    if constexpr (Us == WHITE) {
        moveList = generate_castling<WHITE, WHITE_OO>(pos, moveList);
        moveList = generate_castling<WHITE, WHITE_OOO>(pos, moveList);
    } else {
        moveList = generate_castling<BLACK, BLACK_OO>(pos, moveList);
        moveList = generate_castling<BLACK, BLACK_OOO>(pos, moveList);
    }
    return moveList;
}

// Generate all moves for the given generation type
template<Color Us, GenType Type>
ExtMove* generate_all(const Position& pos, ExtMove* moveList) {

    constexpr bool Checks = (Type == QUIET_CHECKS);
    Bitboard target;

    if constexpr (Type == CAPTURES) {
        target = pos.pieces(~Us);
    } else if constexpr (Type == QUIETS || Type == QUIET_CHECKS) {
        target = ~pos.pieces();
    } else if constexpr (Type == NON_EVASIONS) {
        target = ~pos.pieces(Us);
    } else {
        // EVASIONS - handled separately
        static_assert(Type != EVASIONS, "Use generate_evasions instead");
    }

    // Pawn moves
    moveList = generate_pawn_moves<Us, Type>(pos, moveList, target);

    // Piece moves (not pawns, not king)
    moveList = generate_piece_moves<KNIGHT, Checks>(pos, moveList, Us, target);
    moveList = generate_piece_moves<BISHOP, Checks>(pos, moveList, Us, target);
    moveList = generate_piece_moves<ROOK, Checks>(pos, moveList, Us, target);
    moveList = generate_piece_moves<QUEEN, Checks>(pos, moveList, Us, target);

    // King moves (excluding castling for QUIET_CHECKS)
    if constexpr (Type != QUIET_CHECKS) {
        moveList = generate_king_moves(pos, moveList, Us, target);
    }

    // Castling (only for QUIETS and NON_EVASIONS)
    if constexpr (Type == QUIETS || Type == NON_EVASIONS) {
        if (!pos.checkers()) {
            moveList = generate_all_castling<Us>(pos, moveList);
        }
    }

    return moveList;
}

// Generate evasion moves (when in check)
template<Color Us>
ExtMove* generate_evasions(const Position& pos, ExtMove* moveList) {

    Square ksq = pos.king_square(Us);
    Bitboard checkers = pos.checkers();
    Bitboard occupied = pos.pieces();

    // King can always try to move out of check
    Bitboard kingMoves = attacks_bb<KING>(ksq) & ~pos.pieces(Us);

    // Remove squares attacked by enemy sliding pieces along the check ray
    Bitboard sliderAttacks = 0;
    Bitboard tempCheckers = checkers;
    while (tempCheckers) {
        Square checker = pop_lsb(tempCheckers);
        Piece pc = pos.piece_on(checker);
        PieceType pt = type_of(pc);
        // For sliders, extend attack ray through king
        if (pt == BISHOP || pt == ROOK || pt == QUEEN) {
            sliderAttacks |= line_bb(checker, ksq) & ~square_bb(checker);
        }
    }
    kingMoves &= ~sliderAttacks;

    // Filter out squares attacked by enemy
    Bitboard tempKingMoves = kingMoves;
    while (tempKingMoves) {
        Square to = pop_lsb(tempKingMoves);
        // Temporarily remove king to check if square is safe
        if (!(pos.attackers_to(to, occupied ^ square_bb(ksq)) & pos.pieces(~Us))) {
            *moveList++ = make_ext_move(make_move(ksq, to));
        }
    }

    // If double check, only king moves are legal
    if (more_than_one(checkers))
        return moveList;

    // Single check - can block or capture the checker
    Square checker = lsb(checkers);

    // Target squares: the checker square (capture) or squares between king and checker (block)
    Bitboard target = between_bb(ksq, checker) | square_bb(checker);

    // Generate moves that capture or block
    moveList = generate_pawn_moves<Us, EVASIONS>(pos, moveList, target);
    moveList = generate_piece_moves<KNIGHT, false>(pos, moveList, Us, target);
    moveList = generate_piece_moves<BISHOP, false>(pos, moveList, Us, target);
    moveList = generate_piece_moves<ROOK, false>(pos, moveList, Us, target);
    moveList = generate_piece_moves<QUEEN, false>(pos, moveList, Us, target);

    return moveList;
}

} // anonymous namespace

// Template specializations for the generate function

template<>
ExtMove* generate<CAPTURES>(const Position& pos, ExtMove* moveList) {
    return pos.side_to_move() == WHITE
        ? generate_all<WHITE, CAPTURES>(pos, moveList)
        : generate_all<BLACK, CAPTURES>(pos, moveList);
}

template<>
ExtMove* generate<QUIETS>(const Position& pos, ExtMove* moveList) {
    return pos.side_to_move() == WHITE
        ? generate_all<WHITE, QUIETS>(pos, moveList)
        : generate_all<BLACK, QUIETS>(pos, moveList);
}

template<>
ExtMove* generate<QUIET_CHECKS>(const Position& pos, ExtMove* moveList) {
    return pos.side_to_move() == WHITE
        ? generate_all<WHITE, QUIET_CHECKS>(pos, moveList)
        : generate_all<BLACK, QUIET_CHECKS>(pos, moveList);
}

template<>
ExtMove* generate<EVASIONS>(const Position& pos, ExtMove* moveList) {
    return pos.side_to_move() == WHITE
        ? generate_evasions<WHITE>(pos, moveList)
        : generate_evasions<BLACK>(pos, moveList);
}

template<>
ExtMove* generate<NON_EVASIONS>(const Position& pos, ExtMove* moveList) {
    return pos.side_to_move() == WHITE
        ? generate_all<WHITE, NON_EVASIONS>(pos, moveList)
        : generate_all<BLACK, NON_EVASIONS>(pos, moveList);
}

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList) {
    ExtMove* cur = moveList;

    // Generate pseudo-legal moves based on check status
    ExtMove* end = pos.checkers()
        ? generate<EVASIONS>(pos, moveList)
        : generate<NON_EVASIONS>(pos, moveList);

    // Filter to only legal moves
    Color us = pos.side_to_move();
    Square ksq = pos.king_square(us);
    Bitboard pinned = pos.blockers_for_king(us) & pos.pieces(us);

    while (cur != end) {
        Move m = cur->move;
        Square from = from_sq(m);

        // Check if move is legal
        bool legal = true;

        if (type_of(m) == EN_PASSANT) {
            // En passant requires special legality check (discovered check through ep pawn)
            legal = pos.legal(m);
        } else if (type_of(m) == CASTLING) {
            // Castling already validated during generation
            legal = true;
        } else if (from == ksq) {
            // King move - check destination is not attacked
            Bitboard occ = pos.pieces() ^ square_bb(from);
            legal = !(pos.attackers_to(to_sq(m), occ) & pos.pieces(~us));
        } else if (pinned & square_bb(from)) {
            // Pinned piece - can only move along pin ray
            legal = aligned(from, to_sq(m), ksq);
        }

        if (legal) {
            *moveList++ = *cur;
        }
        ++cur;
    }

    return moveList;
}

} // namespace Boudica
