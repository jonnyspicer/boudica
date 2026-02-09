#ifndef POSITION_H
#define POSITION_H

#include "types.h"
#include "bitboard.h"
#include <string>
#include <cstring>

namespace Boudica {

// StateInfo stores information needed to restore a position
// when unmaking a move. Linked list structure via previous pointer.
struct StateInfo {
    // Copied from previous state
    Key         materialKey;
    Key         pawnKey;
    Value       nonPawnMaterial[COLOR_NB];
    int         castlingRights;
    int         rule50;
    int         pliesFromNull;
    Square      epSquare;

    // Computed on do_move
    Key         key;
    Bitboard    checkersBB;
    StateInfo*  previous;
    Bitboard    blockersForKing[COLOR_NB];
    Bitboard    pinners[COLOR_NB];
    Bitboard    checkSquares[PIECE_TYPE_NB];
    Piece       capturedPiece;
    int         repetition;
};

// Zobrist key tables
namespace Zobrist {
    extern Key psq[PIECE_NB][SQUARE_NB];
    extern Key enpassant[FILE_NB];
    extern Key castling[CASTLING_RIGHT_NB];
    extern Key side;
    extern Key noPawns;

    void init();
}

class Position {
public:
    static constexpr const char* StartFEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Position() = default;
    Position(const Position&) = delete;
    Position& operator=(const Position&) = delete;

    // Setup
    Position& set(const std::string& fenStr, StateInfo* si);
    Position& set(const std::string& code, Color c, StateInfo* si);
    std::string fen() const;

    // Piece access
    Bitboard pieces(PieceType pt = ALL_PIECES) const;
    Bitboard pieces(PieceType pt1, PieceType pt2) const;
    Bitboard pieces(Color c) const;
    Bitboard pieces(Color c, PieceType pt) const;
    Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
    Piece piece_on(Square s) const;
    Square ep_square() const;
    bool empty(Square s) const;

    template<PieceType Pt> int count(Color c) const;
    template<PieceType Pt> int count() const;
    template<PieceType Pt> Square square(Color c) const;

    // Castling
    CastlingRights castling_rights() const;
    CastlingRights castling_rights(Color c) const;
    bool can_castle(CastlingRights cr) const;
    bool castling_impeded(CastlingRights cr) const;
    Square castling_rook_square(CastlingRights cr) const;

    // Attack information
    Bitboard checkers() const;
    Bitboard blockers_for_king(Color c) const;
    Bitboard check_squares(PieceType pt) const;
    Bitboard pinners(Color c) const;

    // Attacks to/from
    Bitboard attackers_to(Square s) const;
    Bitboard attackers_to(Square s, Bitboard occupied) const;
    Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const;

    // Move checking
    bool legal(Move m) const;
    bool pseudo_legal(Move m) const;
    bool gives_check(Move m) const;
    Piece moved_piece(Move m) const;
    bool capture(Move m) const;
    bool capture_stage(Move m) const;

    // Doing and undoing moves
    void do_move(Move m, StateInfo& newSt);
    void do_move(Move m, StateInfo& newSt, bool givesCheck);
    void undo_move(Move m);
    void do_null_move(StateInfo& newSt);
    void undo_null_move();

    // Static Exchange Evaluation
    bool see_ge(Move m, Value threshold = VALUE_ZERO) const;

    // King position
    Square king_square(Color c) const;

    // State access
    Color side_to_move() const;
    int game_ply() const;
    int rule50_count() const;
    Key key() const;
    Key key_after(Move m) const;
    Key material_key() const;
    Key pawn_key() const;

    // Other position info
    bool is_draw(int ply) const;
    bool has_repeated() const;
    int repetition_count() const;
    Value non_pawn_material(Color c) const;
    Value non_pawn_material() const;

    // Position validation
    bool pos_is_ok() const;

    // Display
    void print() const;

private:
    // Helper functions
    void set_castling_right(Color c, Square rfrom);
    void set_check_info(StateInfo* si) const;
    void set_state(StateInfo* si) const;
    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);

    template<bool Do>
    void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

    // Board representation
    Piece board[SQUARE_NB];
    Bitboard byTypeBB[PIECE_TYPE_NB];
    Bitboard byColorBB[COLOR_NB];
    int pieceCount[PIECE_NB];
    int castlingRightsMask[SQUARE_NB];
    Square castlingRookSquare[CASTLING_RIGHT_NB];
    Bitboard castlingPath[CASTLING_RIGHT_NB];

    // Game state
    Color sideToMove;
    int gamePly;
    StateInfo* st;
};

// Inline implementations

inline Color Position::side_to_move() const {
    return sideToMove;
}

inline Piece Position::piece_on(Square s) const {
    assert(s >= SQ_A1 && s <= SQ_H8);
    return board[s];
}

inline bool Position::empty(Square s) const {
    return piece_on(s) == NO_PIECE;
}

inline Bitboard Position::pieces(PieceType pt) const {
    return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const {
    return pieces(pt1) | pieces(pt2);
}

inline Bitboard Position::pieces(Color c) const {
    return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const {
    return pieces(c) & pieces(pt);
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const {
    return pieces(c) & (pieces(pt1) | pieces(pt2));
}

template<PieceType Pt>
inline int Position::count(Color c) const {
    return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt>
inline int Position::count() const {
    return count<Pt>(WHITE) + count<Pt>(BLACK);
}

template<PieceType Pt>
inline Square Position::square(Color c) const {
    assert(count<Pt>(c) == 1);
    return lsb(pieces(c, Pt));
}

inline Square Position::king_square(Color c) const {
    return square<KING>(c);
}

inline Square Position::ep_square() const {
    return st->epSquare;
}

inline bool Position::can_castle(CastlingRights cr) const {
    return st->castlingRights & cr;
}

inline CastlingRights Position::castling_rights() const {
    return CastlingRights(st->castlingRights);
}

inline CastlingRights Position::castling_rights(Color c) const {
    return CastlingRights(st->castlingRights & (c == WHITE ? WHITE_CASTLING : BLACK_CASTLING));
}

inline bool Position::castling_impeded(CastlingRights cr) const {
    return pieces() & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRights cr) const {
    return castlingRookSquare[cr];
}

inline Bitboard Position::checkers() const {
    return st->checkersBB;
}

inline Bitboard Position::blockers_for_king(Color c) const {
    return st->blockersForKing[c];
}

inline Bitboard Position::check_squares(PieceType pt) const {
    return st->checkSquares[pt];
}

inline Bitboard Position::pinners(Color c) const {
    return st->pinners[c];
}

inline int Position::game_ply() const {
    return gamePly;
}

inline int Position::rule50_count() const {
    return st->rule50;
}

inline Key Position::key() const {
    return st->key;
}

inline Key Position::pawn_key() const {
    return st->pawnKey;
}

inline Key Position::material_key() const {
    return st->materialKey;
}

inline Value Position::non_pawn_material(Color c) const {
    return st->nonPawnMaterial[c];
}

inline Value Position::non_pawn_material() const {
    return non_pawn_material(WHITE) + non_pawn_material(BLACK);
}

inline Piece Position::moved_piece(Move m) const {
    return piece_on(from_sq(m));
}

inline bool Position::capture(Move m) const {
    assert(is_ok(m));
    return (!empty(to_sq(m)) && type_of(m) != CASTLING) || type_of(m) == EN_PASSANT;
}

inline bool Position::capture_stage(Move m) const {
    assert(is_ok(m));
    return capture(m) || type_of(m) == PROMOTION;
}

} // namespace Boudica

#endif // POSITION_H
