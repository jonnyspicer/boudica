package state

import "fmt"

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

// TODO: maybe move these to utils package?
func (bb *BitBoard) ToRunes() ([8][8]rune, error) {
	b := [8][8]rune{}

	for i := 0; i < 64; i++ {
		if ((int(*bb) >> i) & 1) == 1 {
			b[i/8][i%8] = '1'
		} else {
			b[i/8][i%8] = '0'
		}
	}

	for _, j := range b {
		for _, k := range j {
			fmt.Printf(string(k) + ",")
		}
		fmt.Printf("\n")
	}

	return b, nil
}

// Func ToRunes is a testing/debugging method to
// allow for human readable output from bitboards
func (board *Board) ToRunes() ([8][8]rune, error) {
	b := [8][8]rune{}

	for i := 0; i < 64; i++ {
		if ((board.WhiteRooks >> i) & 1) == 1 {
			b[i/8][i%8] = 'R'
		} else if ((board.WhiteKnights >> i) & 1) == 1 {
			b[i/8][i%8] = 'N'
		} else if ((board.WhiteBishops >> i) & 1) == 1 {
			b[i/8][i%8] = 'B'
		} else if ((board.WhiteQueens >> i) & 1) == 1 {
			b[i/8][i%8] = 'Q'
		} else if ((board.WhiteKing >> i) & 1) == 1 {
			b[i/8][i%8] = 'K'
		} else if ((board.WhitePawns >> i) & 1) == 1 {
			b[i/8][i%8] = 'P'
		} else if ((board.BlackRooks >> i) & 1) == 1 {
			b[i/8][i%8] = 'r'
		} else if ((board.BlackKnights >> i) & 1) == 1 {
			b[i/8][i%8] = 'n'
		} else if ((board.BlackBishops >> i) & 1) == 1 {
			b[i/8][i%8] = 'b'
		} else if ((board.BlackQueens >> i) & 1) == 1 {
			b[i/8][i%8] = 'q'
		} else if ((board.BlackKing >> i) & 1) == 1 {
			b[i/8][i%8] = 'k'
		} else if ((board.BlackPawns >> i) & 1) == 1 {
			b[i/8][i%8] = 'p'
		} else {
			b[i/8][i%8] = ' '
		}
	}

	for _, j := range b {
		for _, k := range j {
			fmt.Printf(string(k) + ",")
		}
		fmt.Printf("\n")
	}

	return b, nil
}
