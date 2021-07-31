package move

import (
	"testing"

	"github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

func TestGenerator_Possible(t *testing.T) {
	cases := []struct {
		name     string
		position string
		// TODO: add history
		expectedMoves string
	}{
		// TODO: add more cases
		{"standard-start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 ", "6050615162526353645465556656675760406141624263436444654566466747"},
		{"all-pawns-forward-two", "rnbqkbnr/8/8/pppppppp/PPPPPPPP/8/8/RNBQKBNR w KQkq a6 0 9", "40314132423343344435453646374130423143324433453446354736"},
		{"promotions", "n1n1r1r1/1P1P1P1P/8/8/8/2k5/8/2K5 w - - 0 1", "12QP12RP12BP12NP34QP34RP34BP34NP56QP56RP56BP56NP10QP10RP10BP10NP32QP32RP32BP32NP54QP54RP54BP54NP76QP76RP76BP76NP"},
	}
	// TODO: mock state?

	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			// TODO: refactor this so only testing the functionality in generator.go
			s, _ := state.NewCustomPosition(state.Fen(c.position))

			g := NewGenerator(s.Board, "")
			g.Possible()

			g.Board.ToRunes()

			assert.Equal(t, c.expectedMoves, g.possibilities)
		})
	}
}
