package move

import (
	"testing"

	"github.com/jonnyspicer/boudica/internal/state"

	"github.com/stretchr/testify/assert"
)

func TestGenerator_GeneratePossibleMoves(t *testing.T) {
	cases := []struct {
		name     string
		position string
		// TODO: add history
		expectedMoves string
	}{
		// TODO: add more cases
		{"standard-start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 ", "6050615162526353645465556656675760406141624263436444654566466747"},
		{"all-pawns-forward-two", "rnbqkbnr/8/8/pppppppp/PPPPPPPP/8/8/RNBQKBNR w KQkq a6 0 9", "40314132423343344435453646374130423143324433453446354736"},
	}
	// TODO: mock state?

	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			// TODO: refactor this so only testing the functionality in generator.go
			s, _ := state.NewCustomPosition(state.Fen(c.position))

			mg := NewGenerator(s.Board, "")
			mg.PossibleMoves()

			assert.Equal(t, c.expectedMoves, mg.possibleMoves)
		})
	}
}
