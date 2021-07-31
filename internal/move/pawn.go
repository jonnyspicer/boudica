package move

import (
	"math"
	"math/bits"
	"strconv"
)

func (g *Generator) WhitePawns() {
	// TODO: are these flipped?
	g.pawnCaptureRight()
	g.pawnCaptureLeft()
	g.pawnForwardOne()
	g.pawnForwardTwo()
	g.pawnPromoteRight()
	g.pawnPromoteLeft()
	g.pawnPromoteForward()
}

func (g *Generator) pawnCaptureRight() {
	pm := (g.Board.WhitePawns >> 7) & g.blackPieces &^ rank8 &^ fileA
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			// if a capture is possible, appends a move to the string in the form "x1y1x2y2"
			// uses 8th rank and 1st file as 0, in keeping with using a8 = 0 and h1 = 63
			g.possibilities += strconv.Itoa(i/8+1) + strconv.Itoa(i%8-1) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}
}

func (g *Generator) pawnCaptureLeft() {
	pm := (g.Board.WhitePawns >> 9) & g.blackPieces &^ rank8 &^ fileH
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			g.possibilities += strconv.Itoa(i/8+1) + strconv.Itoa(i%8+1) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}
}

func (g *Generator) pawnForwardOne() {
	pm := (g.Board.WhitePawns >> 8) & g.emptySquares &^ rank8
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			g.possibilities += strconv.Itoa(i/8+1) + strconv.Itoa(i%8) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}
}

func (g *Generator) pawnForwardTwo() {
	pm := (g.Board.WhitePawns >> 16) & g.emptySquares & (g.emptySquares >> 8) & rank4
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			g.possibilities += strconv.Itoa(i/8+2) + strconv.Itoa(i%8) + strconv.Itoa(i/8) + strconv.Itoa(i%8)
		}
	}
}

func (g *Generator) pawnPromoteRight() {
	pm := (g.Board.WhitePawns >> 7) & g.blackPieces & rank8 &^ fileA
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			g.possibilities += strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "QP" + strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "RP" + strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "BP" + strconv.Itoa(i%8-1) + strconv.Itoa(i%8) + "NP"
		}
	}
}

func (g *Generator) pawnPromoteLeft() {
	pm := (g.Board.WhitePawns >> 9) & g.blackPieces & rank8 &^ fileH
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			g.possibilities += strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "QP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "RP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "BP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "NP"
		}
	}
}

func (g *Generator) pawnPromoteForward() {
	pm := (g.Board.WhitePawns >> 8) & g.blackPieces & rank8 &^ fileH
	for i := bits.TrailingZeros64(uint64(pm)); i < 64-bits.LeadingZeros64(uint64(pm)); i++ {
		if (pm>>i)&1 == 1 {
			g.possibilities += strconv.Itoa(i%8) + strconv.Itoa(i%8) + "QP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "RP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "BP" + strconv.Itoa(i%8+1) + strconv.Itoa(i%8) + "NP"
		}
	}
}

func (g *Generator) enPassant() {
	// TODO: are there any gotchas to using len() here?
	// check if there is at least one move in the history
	if len(g.history) >= 4 {
		// worth noting that accessing an element of the string g.history will return a rune value (ie uint8), not a substring
		// eg returns 18 instead of "2"
		// check if the last move was a pawn moving forward two squares
		if g.history[len(g.history)-1] == g.history[len(g.history)-3] && math.Abs(float64(g.history[len(g.history)-2]-g.history[len(g.history)-4])) == 2.0 {
			eFile := g.history[len(g.history)-1] - '0'
			// TODO: get this compiling, obviously
			// do the other pawn move types need refactoring?
		}
	}
}
