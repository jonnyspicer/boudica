package state

import (
	"errors"
	"math"
	"math/rand"
	"strings"
	"time"
)

// TODO: maybe change this to xfen for 960? see https://en.wikipedia.org/wiki/X-FEN
type Fen string

// Func ToBoard will create a Board struct, consisting of the
// 12 bitboards necessary to represent a chessboard, from a given fen string
// TODO: refactor this to be part of a complete ToState function
func (f Fen) ToBoard() (Board, error) {
	b := Board{}

	fp := strings.Split(string(f), " ")

	rows := strings.Split(fp[0], "/")

	// TODO: refactor this? Happy path left aligned?
	for i, row := range rows {
		runeRow := []rune(row)

		// if all the squares on a chessboard are numbered 0-63
		// from top left to bottom right
		// l represents the leftmost square in every row
		l := 8 * i

		// s lets us skip squares when the fen rune is a number
		s := 0

		// the sum of s & j represents how far along the row we are
		for j, r := range runeRow {
			switch r {
			case '1', '8':
				continue
			case '2', '3', '4', '5', '6', '7':
				s += int(r-'0') - 1
			case 'r':
				// raising 2 to the power of the number of the square on the
				// board the piece is occupying allows us to represent its
				// position in binary
				b.BlackRooks += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'n':
				b.BlackKnights += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'b':
				b.BlackBishops += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'q':
				b.BlackQueens += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'k':
				b.BlackKing += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'p':
				b.BlackPawns += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'R':
				b.WhiteRooks += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'N':
				b.WhiteKnights += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'B':
				b.WhiteBishops += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'Q':
				b.WhiteQueens += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'K':
				b.WhiteKing += BitBoard(math.Pow(2, float64(l+j+s)))
			case 'P':
				b.WhitePawns += BitBoard(math.Pow(2, float64(l+j+s)))
			default:
				return b, errors.New("unexpected character in fen string: " + string(r))
			}
		}
	}

	return b, nil
}

// func GenerateCMLXFen generates a random number between 0 - 959
// and maps it to the corresponding position using Scharnagl's table methods
// see https://en.wikipedia.org/wiki/Fischer_random_chess_numbering_scheme
func GenerateCMLXFen() (Fen, error) {
	seed := rand.NewSource(time.Now().UnixNano())

	r := rand.New(seed)

	// var n is the number of the position to lookup in the tables
	n := r.Intn(959)

	row := [8]rune{}

	// lookup the bishops
	bishops := bt[n%16]

	row[bishops[0]] = 'b'
	row[bishops[1]] = 'b'

	// lookup everything else
	pieces := kt[n-(n%16)]

	// loop over each piece from the table
	for _, piece := range pieces {
		// loop over each square in the row
		for i, square := range row {
			// if the square is empty, place the piece there
			if square == 0 {
				row[i] = piece
				break
			}
		}
	}

	// only the strings for the first and eighth ranks will be
	// different across chess960 positions
	// TODO: fix the castling ability portion when swapped to xfen
	return Fen(row[:]) + "/pppppppp/8/8/8/8/PPPPPPPP/" + Fen(strings.ToUpper(string(row[:]))) + " w KQkq - 0 1", nil
}
