package state

import (
	"errors"
	"math"
	"strings"
)

// TODO: maybe change this to xfen for 960? see https://en.wikipedia.org/wiki/X-FEN
type Fen string

// Func fenToBoard will create a Board struct, consisting of the
// 12 state necessary to represent a chessboard, from a given fen string
// TODO: refactor this to be part of a complete ToState function
// TODO: does this need to take a pointer or a value?
func (f Fen) ToBoard() (Board, error) {
	b := Board{}

	fp := strings.Split(string(f), " ")

	rows := strings.Split(fp[0], "/")

	// TODO: refactor this for happy path left aligned
	for i, row := range rows {
		runeRow := []rune(row)

		// if all the squares on a chessboard are numbered 0-63
		// from top left to bottom right
		// s is the leftmost square in every row
		// ie 0, 8, 16, 24, 32, 40, 48, 56
		s := 8 * i

		// j represents how far along the row we are
		for j, r := range runeRow {
			// e lets us skip squares
			e := 0
			switch r {
			case '1', '8':
				continue
			case '2', '3', '4', '5', '6', '7':
				e += int(r) - 1
			case 'r':
				// raising 2 to the power of the number of the square on the
				// board the piece is occupying allows us to represent its
				// position in binary
				b.BlackRooks += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'n':
				b.BlackKnights += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'b':
				b.BlackBishops += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'q':
				b.BlackQueens += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'k':
				b.BlackKing += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'p':
				b.BlackPawns += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'R':
				b.WhiteRooks += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'N':
				b.WhiteKnights += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'B':
				b.WhiteBishops += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'Q':
				b.WhiteQueens += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'K':
				b.WhiteKing += BitBoard(math.Pow(2, float64(s+j+e)))
			case 'P':
				b.WhitePawns += BitBoard(math.Pow(2, float64(s+j+e)))
			default:
				return b, errors.New("unexpected character in fen string: " + string(r))
			}
		}
	}

	return b, nil
}
