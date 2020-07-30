using System;

namespace boudica
{
    class Program
    {
        static void Main(string[] args)
        {
            bool gameResult = false;

            if (!gameResult)
            {
                BoardGeneration.initStartingPosition();
            }
        }

        public class BoardGeneration
        {
            public static void initStartingPosition()
            {
                // A chess position can be represented through 12 bitboards - one for each type of piece for each colour (eg white pawn, black rook etc)
                // initialise 12 empty longs to represent each bitboard
                long wPawn = 0L, wkNight = 0L, wBishop = 0L, wRook = 0L, wQueen = 0L, wKing = 0L, bPawn = 0L, bkNight = 0L, bBishop = 0L, bRook = 0L, bQueen = 0L, bKing = 0L;

                // initialise the starting board - lowercase letters are black pieces, uppercase are white. NB n/N = knight and k/K = king.
                string[,] board = new string[8, 8]
                {
                    {"r","n","b","q","k","b","n","r",},
                    {"p","p","p","p","p","p","p","p",},
                    {" "," "," "," "," "," "," "," ",},
                    {" "," "," "," "," ","p"," "," ",},
                    {" ","p"," "," ","Q"," "," "," ",},
                    {" "," "," "," "," "," "," "," ",},
                    {"P","P","P","P","P","P","P","P",},
                    {"R","N","B","Q","K","B","N","R",}
                };

                arrayToBitboards(board, wPawn, wkNight, wBishop, wRook, wQueen, wKing, bPawn, bkNight, bBishop, bRook, bQueen, bKing);
            }

            public static void arrayToBitboards(string[,] board, long wPawn, long wkNight, long wBishop, long wRook, long wQueen, long wKing, long bPawn, long bkNight, long bBishop, long bRook, long bQueen, long bKing)
            {
                string binary = System.String.Empty;
                for (int i = 0; i < 64; i++)
                {
                    // 64 character string to be converted into binary
                    binary = "0000000000000000000000000000000000000000000000000000000000000000";
                    // place a 1 at the square we're currently examining
                    binary = binary.Substring(i + 1) + "1" + binary.Substring(0, i);
                    // check what's in the square
                    switch (board[i / 8, i % 8])
                    {
                        case "P": 
                            wPawn += stringToBitboard(binary);
                            break;
                        case "N":
                            wkNight += stringToBitboard(binary);
                            break;
                        case "B":
                            wBishop += stringToBitboard(binary);
                            break;
                        case "R":
                            wRook += stringToBitboard(binary);
                            break;
                        case "Q":
                            wQueen += stringToBitboard(binary);
                            break;
                        case "K":
                            wKing += stringToBitboard(binary);
                            break;
                        case "p":
                            bPawn += stringToBitboard(binary);
                            break;
                        case "n":
                            bkNight += stringToBitboard(binary);
                            break;
                        case "b":
                            bBishop += stringToBitboard(binary);
                            break;
                        case "r":
                            bRook += stringToBitboard(binary);
                            break;
                        case "q":
                            bQueen += stringToBitboard(binary);
                            break;
                        case "k":
                            bKing += stringToBitboard(binary);
                            break;
                    }
                }

                renderBoardFromArray(wPawn, wkNight, wBishop, wRook, wQueen, wKing, bPawn, bkNight, bBishop, bRook, bQueen, bKing);
            }

            public static long stringToBitboard(string binary)
            {
                // check if the number is negative
                if(binary[0] == '0')
                {
                    // if not, return the long of the bitboard
                    return Convert.ToInt64(binary, 2);
                } else
                {
                    // TODO: workout how this works
                    return Convert.ToInt64("1" + binary.Substring(2), 2) * 2;
                }
            }

            public static void renderBoardFromArray(long wPawn, long wkNight, long wBishop, long wRook, long wQueen, long wKing, long bPawn, long bkNight, long bBishop, long bRook, long bQueen, long bKing)
            {
                Console.WriteLine(Moves.whitePossibleMoves("", wPawn, wkNight, wBishop, wRook, wQueen, wKing, bPawn, bkNight, bBishop, bRook, bQueen, bKing));

                // convert the bitboard back into an array of strings, for debugging/testing purposes
                string[,] board = new string[8, 8];
                for (int i = 0; i < 64; i++)
                {
                    // every square on the board starts empty
                    board[i / 8, i % 8] = " ";
                }

                for (int i = 0; i < 64; i++)
                {
                    if (((wPawn >> i) & 1) == 1) { board[i / 8, i % 8] = "P"; }
                    if (((wkNight >> i) & 1) == 1) { board[i / 8, i % 8] = "N"; }
                    if (((wBishop >> i) & 1) == 1) { board[i / 8, i % 8] = "B"; }
                    if (((wRook >> i) & 1) == 1) { board[i / 8, i % 8] = "R"; }
                    if (((wQueen >> i) & 1) == 1) { board[i / 8, i % 8] = "Q"; }
                    if (((wKing >> i) & 1) == 1) { board[i / 8, i % 8] = "K"; }
                    if (((bPawn >> i) & 1) == 1) { board[i / 8, i % 8] = "p"; }
                    if (((bkNight >> i) & 1) == 1) { board[i / 8, i % 8] = "n"; }
                    if (((bBishop >> i) & 1) == 1) { board[i / 8, i % 8] = "b"; }
                    if (((bRook >> i) & 1) == 1) { board[i / 8, i % 8] = "r"; }
                    if (((bQueen >> i) & 1) == 1) { board[i / 8, i % 8] = "q"; }
                    if (((bKing >> i) & 1) == 1) { board[i / 8, i % 8] = "k"; }
                }

                for (int i = 0; i < 8; i++)
                {
                    string str = "";
                    for (int j = 0; j < 8; j++)
                    {
                        str += board[i, j];
                    }
                    Console.WriteLine(str);
                }
            }

            public static void newGame()
            {
                BoardGeneration.initStartingPosition();
            }
        }
    }
}
