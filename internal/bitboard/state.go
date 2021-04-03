// Package bitboard provides a way to visualise an 8x8 board as a 64 bit integer
// See https://www.chessprogramming.org/8x8_Board for an explanation
package bitboard

// TODO: maybe change this to xfen for 960? see https://en.wikipedia.org/wiki/X-FEN
type Fen string
type BitBoard uint64
type Board struct {
	WhiteRooks   BitBoard
	WhiteKnights BitBoard
	WhiteBishops BitBoard
	WhiteQueens  BitBoard
	WhiteKing    BitBoard
	WhitePawns   BitBoard
	BlackRooks   BitBoard
	BlackKnights BitBoard
	BlackBishops BitBoard
	BlackQueens  BitBoard
	BlackKing    BitBoard
	BlackPawns   BitBoard
}

// The starting position for a standard game of chess
// Splitting on spaces;
// The first substring is the piece placement
// the second substring is the side to move
// the third substring is the castling ability
// the fourth substring is the en passant target square
// the fifth substring is the halfmove clock (ie how many half moves have been made that contribute to the 50 move draw rule)
// the sixth substring is the fullmove counter
const standardStart Fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// TODO: review how much of this needs exporting

// WRITE THE TESTS FIRST

type State struct {
	Board Board
	// sideToMove bool
	// castlingAbility maybe byte/uint8
	// enPassantTargetSquare uint16, bool? (only 16 available squares, but could also be none)
	// halfmoveClock byte/uint8
	// fullmoveCounter uint16
}

// Func NewStandardGame will create a new State struct with the initial
// starting parameters of a standard chess game
func NewStandardGame() (State, error) {
	return State{}, nil
}

// Func NewCMLXGame will create a new State struct with the initial
// starting parameters of a chess 960 game
func NewCMLXGame() (State, error) {
	return State{}, nil
}

// Func NewCustomPosition will create a new State struct with the
// parameters from a specified FEN string
func NewCustomPosition(f Fen) (State, error) {
	return State{}, nil
}

// Func fenToBoard will create a Board struct, consisting of the
// 12 bitboard necessary to represent a chessboard, from a given
// fen string
// TODO: refactor this to be part of a complete fenToState function
func FenToBoard(f Fen) (Board, error) {
	return Board{}, nil
}
