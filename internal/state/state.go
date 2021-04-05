// Package state provides a way to visualise an 8x8 board as a 64 bit integer
// See https://www.chessprogramming.org/8x8_Board for an explanation
package state

import "errors"

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
	s := State{}

	b, err := standardStart.ToBoard()

	if err != nil {
		return s, errors.New("error generating board from fen")
	}

	s.Board = b

	return s, nil
}

// Func NewCMLXGame will create a new State struct with the initial
// starting parameters of a chess 960 game
func NewCMLXGame() (State, error) {
	return State{}, nil
}

// Func NewCustomPosition will create a new State struct with the
// parameters from a specified FEN string
func NewCustomPosition(f Fen) (State, error) {
	s := State{}

	b, err := f.ToBoard()

	if err != nil {
		return s, errors.New("error generating board from fen")
	}

	s.Board = b

	return s, nil
}
