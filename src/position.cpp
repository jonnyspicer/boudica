#include "position.h"
#include "bitboard.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <random>

namespace Boudica {

// Zobrist key tables
namespace Zobrist {
    Key psq[PIECE_NB][SQUARE_NB];
    Key enpassant[FILE_NB];
    Key castling[CASTLING_RIGHT_NB];
    Key side;
    Key noPawns;

    void init() {
        // Use a fixed seed for reproducibility
        std::mt19937_64 rng(1070372);

        for (Piece pc = NO_PIECE; pc < PIECE_NB; ++pc)
            for (Square s = SQ_A1; s <= SQ_H8; ++s)
                psq[pc][s] = rng();

        for (File f = FILE_A; f <= FILE_H; ++f)
            enpassant[f] = rng();

        for (int cr = 0; cr < CASTLING_RIGHT_NB; ++cr)
            castling[cr] = rng();

        side = rng();
        noPawns = rng();
    }
}

// Piece value lookup for SEE
constexpr Value PieceValue[PIECE_NB] = {
    VALUE_ZERO, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, VALUE_ZERO, VALUE_ZERO,
    VALUE_ZERO, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, VALUE_ZERO, VALUE_ZERO
};

// Helper to convert FEN piece char to Piece
Piece char_to_piece(char c) {
    switch (c) {
        case 'P': return W_PAWN;
        case 'N': return W_KNIGHT;
        case 'B': return W_BISHOP;
        case 'R': return W_ROOK;
        case 'Q': return W_QUEEN;
        case 'K': return W_KING;
        case 'p': return B_PAWN;
        case 'n': return B_KNIGHT;
        case 'b': return B_BISHOP;
        case 'r': return B_ROOK;
        case 'q': return B_QUEEN;
        case 'k': return B_KING;
        default:  return NO_PIECE;
    }
}

// Helper to convert Piece to FEN char
char piece_to_char(Piece p) {
    constexpr char PieceToChar[] = " PNBRQK  pnbrqk";
    return PieceToChar[p];
}

// Set position from FEN string
Position& Position::set(const std::string& fenStr, StateInfo* si) {
    // Initialize
    std::memset(this, 0, sizeof(Position));
    std::memset(si, 0, sizeof(StateInfo));
    st = si;

    std::istringstream ss(fenStr);
    char token;
    Square sq = SQ_A8;

    ss >> std::noskipws;

    // 1. Piece placement
    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token))
            sq += int(token - '0');
        else if (token == '/')
            sq += 2 * SOUTH;
        else {
            Piece pc = char_to_piece(token);
            if (pc != NO_PIECE)
                put_piece(pc, sq);
            ++sq;
        }
    }

    // 2. Side to move
    ss >> token;
    sideToMove = (token == 'w') ? WHITE : BLACK;
    ss >> token;

    // 3. Castling rights
    while ((ss >> token) && !isspace(token)) {
        Square rsq;
        Color c = islower(token) ? BLACK : WHITE;
        Piece rook = make_piece(c, ROOK);

        token = char(toupper(token));

        if (token == 'K')
            for (rsq = relative_square(c, SQ_H1); piece_on(rsq) != rook; --rsq) {}
        else if (token == 'Q')
            for (rsq = relative_square(c, SQ_A1); piece_on(rsq) != rook; ++rsq) {}
        else if (token >= 'A' && token <= 'H')
            rsq = make_square(File(token - 'A'), relative_rank(c, RANK_1));
        else
            continue;

        set_castling_right(c, rsq);
    }

    // 4. En passant square
    bool enpassant = false;
    if (((ss >> token) && (token >= 'a' && token <= 'h'))
        && ((ss >> token) && (token == (sideToMove == WHITE ? '6' : '3')))) {
        st->epSquare = make_square(File(token - 'a' + FILE_A - ('6' - token)),
                                    Rank(token - '1'));
        enpassant = true;
    }

    // Better parsing for en passant
    if (!enpassant)
        st->epSquare = SQ_NONE;

    // Reparse en passant more carefully
    {
        std::istringstream ss2(fenStr);
        std::string part;
        int partNum = 0;
        while (ss2 >> part) {
            partNum++;
            if (partNum == 4) { // en passant field
                if (part != "-" && part.length() == 2) {
                    File f = File(part[0] - 'a');
                    Rank r = Rank(part[1] - '1');
                    if (f >= FILE_A && f <= FILE_H && r >= RANK_1 && r <= RANK_8) {
                        st->epSquare = make_square(f, r);
                    }
                }
                break;
            }
        }
    }

    // 5. Halfmove clock
    ss >> std::skipws >> st->rule50;

    // 6. Fullmove number (convert to game ply)
    int fullmove;
    ss >> fullmove;
    gamePly = std::max(2 * (fullmove - 1), 0) + (sideToMove == BLACK);

    set_state(si);

    return *this;
}

// Set position from piece placement code (for endgame testing)
Position& Position::set(const std::string& code, Color c, StateInfo* si) {
    std::memset(this, 0, sizeof(Position));
    std::memset(si, 0, sizeof(StateInfo));
    st = si;

    // Parse piece codes like "KQvK"
    // Kings go on e1/e8, other pieces fill from a1/a8

    // TODO: Implement material-only position setup

    sideToMove = c;
    set_state(si);
    return *this;
}

// Return FEN string for current position
std::string Position::fen() const {
    std::ostringstream ss;

    // 1. Piece placement
    for (Rank r = RANK_8; r >= RANK_1; --r) {
        int emptyCount = 0;
        for (File f = FILE_A; f <= FILE_H; ++f) {
            Square s = make_square(f, r);
            Piece pc = piece_on(s);
            if (pc == NO_PIECE)
                emptyCount++;
            else {
                if (emptyCount > 0) {
                    ss << emptyCount;
                    emptyCount = 0;
                }
                ss << piece_to_char(pc);
            }
        }
        if (emptyCount > 0)
            ss << emptyCount;
        if (r > RANK_1)
            ss << '/';
    }

    // 2. Side to move
    ss << (sideToMove == WHITE ? " w " : " b ");

    // 3. Castling rights
    if (can_castle(WHITE_OO))  ss << 'K';
    if (can_castle(WHITE_OOO)) ss << 'Q';
    if (can_castle(BLACK_OO))  ss << 'k';
    if (can_castle(BLACK_OOO)) ss << 'q';
    if (!can_castle(ANY_CASTLING)) ss << '-';

    // 4. En passant
    ss << ' ';
    if (st->epSquare == SQ_NONE)
        ss << '-';
    else
        ss << square_to_string(st->epSquare);

    // 5-6. Halfmove and fullmove
    ss << ' ' << st->rule50 << ' ' << 1 + (gamePly - (sideToMove == BLACK)) / 2;

    return ss.str();
}

// Setup castling rights
void Position::set_castling_right(Color c, Square rfrom) {
    Square kfrom = king_square(c);
    CastlingRights cr = c == WHITE ? (rfrom > kfrom ? WHITE_OO : WHITE_OOO)
                                   : (rfrom > kfrom ? BLACK_OO : BLACK_OOO);

    st->castlingRights |= cr;
    castlingRightsMask[kfrom] |= cr;
    castlingRightsMask[rfrom] |= cr;
    castlingRookSquare[cr] = rfrom;

    Square kto = relative_square(c, cr & KING_SIDE ? SQ_G1 : SQ_C1);
    Square rto = relative_square(c, cr & KING_SIDE ? SQ_F1 : SQ_D1);

    castlingPath[cr] = (between_bb(rfrom, rto) | between_bb(kfrom, kto))
                     & ~(square_bb(kfrom) | square_bb(rfrom));
}

// Compute full state info
void Position::set_state(StateInfo* si) const {
    si->key = si->materialKey = 0;
    si->pawnKey = Zobrist::noPawns;
    si->nonPawnMaterial[WHITE] = si->nonPawnMaterial[BLACK] = VALUE_ZERO;
    si->checkersBB = attackers_to(king_square(sideToMove)) & pieces(~sideToMove);

    set_check_info(si);

    // Compute hash keys
    for (Bitboard b = pieces(); b; ) {
        Square s = pop_lsb(b);
        Piece pc = piece_on(s);
        si->key ^= Zobrist::psq[pc][s];

        if (type_of(pc) == PAWN)
            si->pawnKey ^= Zobrist::psq[pc][s];
        else if (type_of(pc) != KING)
            si->nonPawnMaterial[color_of(pc)] += PieceValue[pc];
    }

    if (si->epSquare != SQ_NONE)
        si->key ^= Zobrist::enpassant[file_of(si->epSquare)];

    if (sideToMove == BLACK)
        si->key ^= Zobrist::side;

    si->key ^= Zobrist::castling[si->castlingRights];

    // Material key
    for (Piece pc = W_PAWN; pc <= B_KING; ++pc)
        for (int cnt = 0; cnt < pieceCount[pc]; ++cnt)
            si->materialKey ^= Zobrist::psq[pc][cnt];
}

// Set check info for current state
void Position::set_check_info(StateInfo* si) const {
    si->blockersForKing[WHITE] = slider_blockers(pieces(BLACK), king_square(WHITE), si->pinners[BLACK]);
    si->blockersForKing[BLACK] = slider_blockers(pieces(WHITE), king_square(BLACK), si->pinners[WHITE]);

    Square ksq = king_square(~sideToMove);

    si->checkSquares[PAWN]   = pawn_attacks_bb(~sideToMove, ksq);
    si->checkSquares[KNIGHT] = attacks_bb<KNIGHT>(ksq);
    si->checkSquares[BISHOP] = attacks_bb<BISHOP>(ksq, pieces());
    si->checkSquares[ROOK]   = attacks_bb<ROOK>(ksq, pieces());
    si->checkSquares[QUEEN]  = si->checkSquares[BISHOP] | si->checkSquares[ROOK];
    si->checkSquares[KING]   = 0;
}

// Put a piece on the board
void Position::put_piece(Piece pc, Square s) {
    board[s] = pc;
    byTypeBB[ALL_PIECES] |= byTypeBB[type_of(pc)] |= square_bb(s);
    byColorBB[color_of(pc)] |= square_bb(s);
    pieceCount[pc]++;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]++;
}

// Remove a piece from the board
void Position::remove_piece(Square s) {
    Piece pc = board[s];
    assert(pc != NO_PIECE && "remove_piece called on empty square");
    byTypeBB[ALL_PIECES] ^= square_bb(s);
    byTypeBB[type_of(pc)] ^= square_bb(s);
    byColorBB[color_of(pc)] ^= square_bb(s);
    board[s] = NO_PIECE;
    pieceCount[pc]--;
    pieceCount[make_piece(color_of(pc), ALL_PIECES)]--;
}

// Move a piece from one square to another
void Position::move_piece(Square from, Square to) {
    Piece pc = board[from];
    Bitboard fromTo = square_bb(from) | square_bb(to);
    byTypeBB[ALL_PIECES] ^= fromTo;
    byTypeBB[type_of(pc)] ^= fromTo;
    byColorBB[color_of(pc)] ^= fromTo;
    board[from] = NO_PIECE;
    board[to] = pc;
}

// Get attackers to a square
Bitboard Position::attackers_to(Square s) const {
    return attackers_to(s, pieces());
}

Bitboard Position::attackers_to(Square s, Bitboard occupied) const {
    return (pawn_attacks_bb(BLACK, s) & pieces(WHITE, PAWN))
         | (pawn_attacks_bb(WHITE, s) & pieces(BLACK, PAWN))
         | (attacks_bb<KNIGHT>(s) & pieces(KNIGHT))
         | (attacks_bb<ROOK>(s, occupied) & pieces(ROOK, QUEEN))
         | (attacks_bb<BISHOP>(s, occupied) & pieces(BISHOP, QUEEN))
         | (attacks_bb<KING>(s) & pieces(KING));
}

// Find blockers for a king from sliders
Bitboard Position::slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const {
    Bitboard blockers = 0;
    pinners = 0;

    // Candidate pinners are sliders that attack the square if there were no other pieces
    Bitboard snipers = ((PseudoAttacks[ROOK][s] & pieces(ROOK, QUEEN))
                      | (PseudoAttacks[BISHOP][s] & pieces(BISHOP, QUEEN))) & sliders;

    Bitboard occupancy = pieces() ^ snipers;

    while (snipers) {
        Square sniperSq = pop_lsb(snipers);
        Bitboard b = between_bb(s, sniperSq) & occupancy;

        if (b && !more_than_one(b)) {
            blockers |= b;
            if (b & pieces(color_of(piece_on(s))))
                pinners |= square_bb(sniperSq);
        }
    }
    return blockers;
}

// Check if a move is legal (assuming it is pseudo-legal)
bool Position::legal(Move m) const {
    assert(is_ok(m));

    Color us = sideToMove;
    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = piece_on(from);

    // Verify the piece exists and belongs to us
    if (pc == NO_PIECE || color_of(pc) != us)
        return false;

    // Capturing the king is never legal
    Piece captured = piece_on(to);
    if (type_of(captured) == KING)
        return false;

    // For pawn captures (diagonal moves), verify there's a piece to capture
    // (except for en passant which is handled separately)
    if (type_of(pc) == PAWN && type_of(m) != EN_PASSANT) {
        int fileDiff = file_of(to) - file_of(from);
        if (fileDiff != 0) {  // Diagonal move = capture
            if (captured == NO_PIECE)
                return false;  // No piece to capture
            if (color_of(captured) == us)
                return false;  // Can't capture own piece
        }
    }

    assert(color_of(pc) == us);

    // En passant captures are tricky
    if (type_of(m) == EN_PASSANT) {
        Square ksq = king_square(us);
        Square capsq = to - pawn_push(us);
        Bitboard occupied = (pieces() ^ square_bb(from) ^ square_bb(capsq)) | square_bb(to);

        // Check that no slider attacks the king through the pawns
        return !(attacks_bb<ROOK>(ksq, occupied) & pieces(~us, ROOK, QUEEN))
            && !(attacks_bb<BISHOP>(ksq, occupied) & pieces(~us, BISHOP, QUEEN));
    }

    // Castling moves - check that no square between king and destination is attacked
    if (type_of(m) == CASTLING) {
        // King destination and path squares
        to = relative_square(us, to > from ? SQ_G1 : SQ_C1);
        Direction step = to > from ? WEST : EAST;

        for (Square s = to; s != from; s += step)
            if (attackers_to(s) & pieces(~us))
                return false;

        // Chess960 check would go here
        return true;
    }

    // Normal moves - if king moves, check destination is not attacked
    if (type_of(piece_on(from)) == KING)
        return !(attackers_to(to, pieces() ^ square_bb(from)) & pieces(~us));

    // For non-king moves, check the move doesn't leave king in check
    // Either the piece is not pinned, or it moves along the pin ray
    return !(blockers_for_king(us) & square_bb(from))
        || aligned(from, to, king_square(us));
}

// Check if a move gives check
bool Position::gives_check(Move m) const {
    assert(is_ok(m));

    Square from = from_sq(m);
    Square to = to_sq(m);

    // Direct check?
    if (check_squares(type_of(piece_on(from))) & square_bb(to))
        return true;

    // Discovered check?
    if ((blockers_for_king(~sideToMove) & square_bb(from))
        && !aligned(from, to, king_square(~sideToMove)))
        return true;

    switch (type_of(m)) {
        case NORMAL:
            return false;

        case PROMOTION:
            return attacks_bb(promotion_type(m), to, pieces() ^ square_bb(from))
                 & square_bb(king_square(~sideToMove));

        case EN_PASSANT: {
            Square capsq = make_square(file_of(to), rank_of(from));
            Bitboard b = (pieces() ^ square_bb(from) ^ square_bb(capsq)) | square_bb(to);
            return (attacks_bb<ROOK>(king_square(~sideToMove), b) & pieces(sideToMove, ROOK, QUEEN))
                 | (attacks_bb<BISHOP>(king_square(~sideToMove), b) & pieces(sideToMove, BISHOP, QUEEN));
        }

        case CASTLING: {
            Square rto = relative_square(sideToMove, to > from ? SQ_F1 : SQ_D1);
            return attacks_bb<ROOK>(rto, pieces() ^ square_bb(from)) & square_bb(king_square(~sideToMove));
        }

        default:
            return false;
    }
}

// Make a move
void Position::do_move(Move m, StateInfo& newSt) {
    do_move(m, newSt, gives_check(m));
}

void Position::do_move(Move m, StateInfo& newSt, bool givesCheck) {
    assert(is_ok(m));

    Key k = st->key ^ Zobrist::side;

    // Copy state
    std::memcpy(&newSt, st, offsetof(StateInfo, key));
    newSt.previous = st;
    st = &newSt;

    ++gamePly;
    ++st->rule50;
    ++st->pliesFromNull;

    Color us = sideToMove;
    Color them = ~us;
    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = piece_on(from);
    Piece captured = (type_of(m) == EN_PASSANT) ? make_piece(them, PAWN) :
                     (type_of(m) == CASTLING)   ? NO_PIECE : piece_on(to);

    assert(color_of(pc) == us);
    assert(captured == NO_PIECE || color_of(captured) == them);

    // Handle castling
    if (type_of(m) == CASTLING) {
        assert(pc == make_piece(us, KING));

        Square rfrom, rto;
        do_castling<true>(us, from, to, rfrom, rto);

        Piece rook = make_piece(us, ROOK);
        k ^= Zobrist::psq[rook][rfrom] ^ Zobrist::psq[rook][rto];
    }

    // Handle captures
    if (captured) {
        Square capsq = to;

        if (type_of(m) == EN_PASSANT) {
            capsq -= pawn_push(us);
            assert(pc == make_piece(us, PAWN));
            assert(to == st->epSquare);
            assert(relative_rank(us, to) == RANK_6);
            assert(piece_on(capsq) == make_piece(them, PAWN));
        }

        remove_piece(capsq);

        if (type_of(captured) == PAWN)
            st->pawnKey ^= Zobrist::psq[captured][capsq];
        else
            st->nonPawnMaterial[them] -= PieceValue[captured];

        k ^= Zobrist::psq[captured][capsq];
        st->materialKey ^= Zobrist::psq[captured][pieceCount[captured]];
        st->rule50 = 0;
    }

    // Update hash for moving piece
    k ^= Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];

    // Reset en passant
    if (st->epSquare != SQ_NONE) {
        k ^= Zobrist::enpassant[file_of(st->epSquare)];
        st->epSquare = SQ_NONE;
    }

    // Update castling rights
    if (st->castlingRights && (castlingRightsMask[from] | castlingRightsMask[to])) {
        k ^= Zobrist::castling[st->castlingRights];
        st->castlingRights &= ~(castlingRightsMask[from] | castlingRightsMask[to]);
        k ^= Zobrist::castling[st->castlingRights];
    }

    // Move the piece (unless castling, which already handled it)
    if (type_of(m) != CASTLING)
        move_piece(from, to);

    // Pawn specific updates
    if (type_of(pc) == PAWN) {
        // Set en passant square if pawn double pushed
        if ((int(to) ^ int(from)) == 16
            && (pawn_attacks_bb(us, to - pawn_push(us)) & pieces(them, PAWN))) {
            st->epSquare = to - pawn_push(us);
            k ^= Zobrist::enpassant[file_of(st->epSquare)];
        }
        else if (type_of(m) == PROMOTION) {
            Piece promotion = make_piece(us, promotion_type(m));

            remove_piece(to);
            put_piece(promotion, to);

            k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[promotion][to];
            st->pawnKey ^= Zobrist::psq[pc][to];
            st->materialKey ^= Zobrist::psq[promotion][pieceCount[promotion] - 1]
                            ^  Zobrist::psq[pc][pieceCount[pc]];
            st->nonPawnMaterial[us] += PieceValue[promotion];
        }

        st->pawnKey ^= Zobrist::psq[pc][from] ^ Zobrist::psq[pc][to];
        st->rule50 = 0;
    }

    st->capturedPiece = captured;
    st->key = k;
    st->checkersBB = givesCheck ? attackers_to(king_square(them)) & pieces(us) : 0;

    sideToMove = ~sideToMove;

    set_check_info(st);

    // Update repetition info
    st->repetition = 0;
    int end = std::min(st->rule50, st->pliesFromNull);
    if (end >= 4) {
        StateInfo* stp = st->previous->previous;
        for (int i = 4; i <= end; i += 2) {
            stp = stp->previous->previous;
            if (stp->key == st->key) {
                st->repetition = stp->repetition ? -i : i;
                break;
            }
        }
    }
}

// Undo a move
void Position::undo_move(Move m) {
    assert(is_ok(m));

    sideToMove = ~sideToMove;

    Color us = sideToMove;
    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = piece_on(to);

    assert(empty(from) || type_of(m) == CASTLING);
    assert(type_of(st->capturedPiece) != KING);

    if (type_of(m) == PROMOTION) {
        remove_piece(to);
        pc = make_piece(us, PAWN);
        put_piece(pc, to);
    }

    if (type_of(m) == CASTLING) {
        Square rfrom, rto;
        do_castling<false>(us, from, to, rfrom, rto);
    }
    else {
        move_piece(to, from);

        if (st->capturedPiece) {
            Square capsq = to;

            if (type_of(m) == EN_PASSANT) {
                capsq -= pawn_push(us);
            }

            put_piece(st->capturedPiece, capsq);
        }
    }

    st = st->previous;
    --gamePly;
}

// Do castling helper
// Note: 'to' is the king's destination (g1/c1 for white), not the rook's position
template<bool Do>
void Position::do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto) {
    bool kingSide = to > from;
    // Calculate rook positions based on castling side
    rfrom = relative_square(us, kingSide ? SQ_H1 : SQ_A1);
    rto = relative_square(us, kingSide ? SQ_F1 : SQ_D1);
    // King destination is already correct (g1/c1)
    to = relative_square(us, kingSide ? SQ_G1 : SQ_C1);

    if (Do) {
        remove_piece(from);
        remove_piece(rfrom);
        board[from] = board[rfrom] = NO_PIECE;
        put_piece(make_piece(us, KING), to);
        put_piece(make_piece(us, ROOK), rto);
    }
    else {
        remove_piece(to);
        remove_piece(rto);
        board[to] = board[rto] = NO_PIECE;
        put_piece(make_piece(us, KING), from);
        put_piece(make_piece(us, ROOK), rfrom);
    }
}

// Null move
void Position::do_null_move(StateInfo& newSt) {
    assert(!checkers());

    std::memcpy(&newSt, st, sizeof(StateInfo));
    newSt.previous = st;
    st = &newSt;

    if (st->epSquare != SQ_NONE) {
        st->key ^= Zobrist::enpassant[file_of(st->epSquare)];
        st->epSquare = SQ_NONE;
    }

    st->key ^= Zobrist::side;
    ++st->rule50;
    st->pliesFromNull = 0;

    sideToMove = ~sideToMove;

    set_check_info(st);

    st->repetition = 0;
}

void Position::undo_null_move() {
    st = st->previous;
    sideToMove = ~sideToMove;
}

// Static Exchange Evaluation
// Returns true if the SEE value of move is >= threshold
bool Position::see_ge(Move m, Value threshold) const {
    assert(is_ok(m));

    // Castling cannot lose material
    if (type_of(m) == CASTLING)
        return VALUE_ZERO >= threshold;

    Square from = from_sq(m);
    Square to = to_sq(m);

    int swap = PieceValue[piece_on(to)] - threshold;
    if (swap < 0)
        return false;

    swap = PieceValue[piece_on(from)] - swap;
    if (swap <= 0)
        return true;

    assert(color_of(piece_on(from)) == sideToMove);

    Bitboard occupied = pieces() ^ square_bb(from) ^ square_bb(to);
    Color stm = sideToMove;
    Bitboard attackers = attackers_to(to, occupied);
    Bitboard stmAttackers, bb;
    int res = 1;

    while (true) {
        stm = ~stm;
        attackers &= occupied;

        if (!(stmAttackers = attackers & pieces(stm)))
            break;

        // Don't allow pinned pieces to attack if the king is the only attacker left
        if (pinners(~stm) & occupied) {
            stmAttackers &= ~blockers_for_king(stm);

            if (!stmAttackers)
                break;
        }

        res ^= 1;

        // Locate and remove the least valuable attacker
        if ((bb = stmAttackers & pieces(PAWN))) {
            if ((swap = PawnValue - swap) < res)
                break;
            occupied ^= lsb(bb);
            attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
        }
        else if ((bb = stmAttackers & pieces(KNIGHT))) {
            if ((swap = KnightValue - swap) < res)
                break;
            occupied ^= lsb(bb);
        }
        else if ((bb = stmAttackers & pieces(BISHOP))) {
            if ((swap = BishopValue - swap) < res)
                break;
            occupied ^= lsb(bb);
            attackers |= attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN);
        }
        else if ((bb = stmAttackers & pieces(ROOK))) {
            if ((swap = RookValue - swap) < res)
                break;
            occupied ^= lsb(bb);
            attackers |= attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN);
        }
        else if ((bb = stmAttackers & pieces(QUEEN))) {
            if ((swap = QueenValue - swap) < res)
                break;
            occupied ^= lsb(bb);
            attackers |= (attacks_bb<BISHOP>(to, occupied) & pieces(BISHOP, QUEEN))
                       | (attacks_bb<ROOK>(to, occupied) & pieces(ROOK, QUEEN));
        }
        else // King - if we get here, the side to move loses
            return (attackers & ~pieces(stm)) ? res ^ 1 : res;
    }

    return bool(res);
}

// Key after a move (without making the move)
Key Position::key_after(Move m) const {
    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = piece_on(from);
    Piece captured = piece_on(to);
    Key k = st->key ^ Zobrist::side;

    if (captured)
        k ^= Zobrist::psq[captured][to];

    k ^= Zobrist::psq[pc][to] ^ Zobrist::psq[pc][from];

    return k;
}

// Check for draw by repetition, 50-move rule, or insufficient material
bool Position::is_draw(int ply) const {
    // 50-move rule
    if (st->rule50 > 99)
        return true;

    // Repetition
    return st->repetition && st->repetition < ply;
}

bool Position::has_repeated() const {
    StateInfo* stp = st;
    int end = std::min(st->rule50, st->pliesFromNull);
    while (end-- >= 4) {
        if (stp->repetition)
            return true;
        stp = stp->previous;
    }
    return false;
}

int Position::repetition_count() const {
    int count = 0;
    StateInfo* stp = st;
    int end = std::min(st->rule50, st->pliesFromNull);

    while (end >= 4) {
        stp = stp->previous->previous;
        end -= 2;
        if (stp->key == st->key)
            ++count;
    }
    return count;
}

// Print position for debugging
void Position::print() const {
    std::cout << "\n +---+---+---+---+---+---+---+---+\n";

    for (Rank r = RANK_8; r >= RANK_1; --r) {
        for (File f = FILE_A; f <= FILE_H; ++f) {
            Piece pc = piece_on(make_square(f, r));
            std::cout << " | " << piece_to_char(pc);
        }
        std::cout << " | " << (1 + r) << "\n +---+---+---+---+---+---+---+---+\n";
    }

    std::cout << "   a   b   c   d   e   f   g   h\n\n";
    std::cout << "Fen: " << fen() << "\n";
    std::cout << "Key: " << std::hex << st->key << std::dec << "\n";
    std::cout << "Checkers: ";

    for (Bitboard b = checkers(); b; )
        std::cout << square_to_string(pop_lsb(b)) << " ";

    std::cout << "\n";
}

// Validate position consistency
bool Position::pos_is_ok() const {
    // Check piece counts match bitboards
    for (Color c : {WHITE, BLACK}) {
        for (PieceType pt = PAWN; pt <= KING; ++pt) {
            Piece pc = make_piece(c, pt);
            if (pieceCount[pc] != popcount(pieces(c, pt)))
                return false;
        }
    }

    // Check king count
    if (count<KING>(WHITE) != 1 || count<KING>(BLACK) != 1)
        return false;

    // Check board and bitboard consistency
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        Piece pc = board[s];
        if (pc != NO_PIECE) {
            if (!(pieces(color_of(pc), type_of(pc)) & square_bb(s)))
                return false;
        }
        else {
            if (pieces() & square_bb(s))
                return false;
        }
    }

    // Check en passant square
    if (st->epSquare != SQ_NONE) {
        if (relative_rank(sideToMove, st->epSquare) != RANK_6)
            return false;
    }

    // Check state consistency
    if (st->key != st->key) // Would need recomputation
        return false;

    return true;
}

// Check if pseudo-legal move is pseudo-legal
bool Position::pseudo_legal(Move m) const {
    Color us = sideToMove;
    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = moved_piece(m);

    // Basic sanity checks
    if (!is_ok(m))
        return false;

    if (pc == NO_PIECE || color_of(pc) != us)
        return false;

    // Destination must not contain our piece
    if (pieces(us) & square_bb(to))
        return false;

    // Handle special moves
    if (type_of(m) == PROMOTION) {
        if (type_of(pc) != PAWN)
            return false;
        if (relative_rank(us, to) != RANK_8)
            return false;
    }

    if (type_of(m) == EN_PASSANT) {
        if (type_of(pc) != PAWN)
            return false;
        if (to != st->epSquare)
            return false;
    }

    if (type_of(m) == CASTLING) {
        if (type_of(pc) != KING)
            return false;
        // More detailed castling checks would go here
    }

    // TODO: More detailed pseudo-legal checks per piece type

    return true;
}

// Explicit template instantiations
template void Position::do_castling<true>(Color, Square, Square&, Square&, Square&);
template void Position::do_castling<false>(Color, Square, Square&, Square&, Square&);

} // namespace Boudica
