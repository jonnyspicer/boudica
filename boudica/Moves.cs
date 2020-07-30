using System;
using System.Collections.Generic;
using System.Text;

namespace boudica
{
    public class Moves
    {
        // bitboards
        // for calculating where pieces can move
        static long aFile = 72340172838076673L;
        static long hFile = -9187201950435737472L;
        static long abFile = 217020518514230019L;
        static long ghFile = -4557430888798830400L;
        static long rankOne = -72057594037927936L;
        static long rankFour = 1095216660480L;
        static long rankFive = 4278190080L;
        static long rankEight = 255L;

        // for evalutation
        static long smallCentre = 103481868288L;
        static long bigCentre = 66229406269440L;
        static long kingSide = -1085102592571150096L;
        static long queenSide = 1085102592571150095L;
        static long kingBSeven = 460039L;
        static long knightCSix = 43234889994L;

        // for calculating captures
        static long whiteUncapturables; // pieces white can't capture, ie the white pieces and the black king
        static long blackPieces; // not including the black king to avoid illegal captures
        static long occupiedSquares;
        static long emptySquares;

        // also for calculating where pieces can move
        static ulong[] rankMasks = new ulong[] // rank 1 to rank 8
        {
            0xFFL, 0xFF00L, 0xFF0000L, 0xFF000000L, 0xFF00000000L, 0xFF0000000000L, 0xFF000000000000L, 0xFF00000000000000L
        };

        static ulong[] fileMasks = new ulong[] // file a to file h
        {
            0x101010101010101L, 0x202020202020202L, 0x404040404040404L, 0x808080808080808L,
            0x1010101010101010L, 0x2020202020202020L, 0x4040404040404040L, 0x8080808080808080L
        };

        static ulong[] diagonalMasks = new ulong[] // from top left to bottom right
        {
            0x1L, 0x102L, 0x10204L, 0x1020408L, 0x102040810L, 0x10204081020L, 0x1020408102040L,
            0x102040810204080L, 0x204081020408000L, 0x408102040800000L, 0x810204080000000L,
            0x1020408000000000L, 0x2040800000000000L, 0x4080000000000000L, 0x8000000000000000L
        };

        static ulong[] flippedDiagonalMasks = new ulong[] // from top right to bottom left
        {
            0x80L, 0x8040L, 0x804020L, 0x80402010L, 0x8040201008L, 0x804020100804L, 0x80402010080402L,
            0x8040201008040201L, 0x4020100804020100L, 0x2010080402010000L, 0x1008040201000000L,
            0x804020100000000L, 0x402010000000000L, 0x201000000000000L, 0x100000000000000L
        };

        static long straightMoves(int square)
        {
            // using hyperbola quintessence, see:
            // https://www.chessprogramming.org/Hyperbola_Quintessence
            long binarySquare = 1L << square;
            long horizontalPossibilities = (occupiedSquares - 2 * binarySquare) ^ Helpers.reverseBits(Helpers.reverseBits(occupiedSquares) - 2 * Helpers.reverseBits(binarySquare));
            long verticalPossibilities = ((occupiedSquares & (long)fileMasks[square % 8]) - (2 * binarySquare)) ^ Helpers.reverseBits(Helpers.reverseBits(occupiedSquares & (long)fileMasks[square % 8]) - (2 * Helpers.reverseBits(binarySquare)));
            drawBitboard((horizontalPossibilities & (long)rankMasks[square / 8]) | (verticalPossibilities & (long)fileMasks[square % 8]));
            return (horizontalPossibilities & (long)rankMasks[square / 8]) | (verticalPossibilities & (long)fileMasks[square % 8]);
        }

        static long diagonalMoves(int square)
        {
            long binarySquare = 1L << square;
            long diagonalPossibilities = ((occupiedSquares & (long)diagonalMasks[(square / 8) + (square % 8)]) - (2 * binarySquare)) ^ Helpers.reverseBits(Helpers.reverseBits(occupiedSquares & (long)diagonalMasks[(square / 8) + (square % 8)]) - (2 * Helpers.reverseBits(binarySquare)));
            long flippedDiagonalPossibilities = ((occupiedSquares & (long)flippedDiagonalMasks[(square / 8) + 7 - (square % 8)]) - (2 * binarySquare)) 
                ^ Helpers.reverseBits(Helpers.reverseBits(occupiedSquares & (long)flippedDiagonalMasks[(square / 8) + 7 - (square % 8)]) - (2 * Helpers.reverseBits(binarySquare)));
            return (diagonalPossibilities & (long)diagonalMasks[(square / 8) + (square % 8)]) | (flippedDiagonalPossibilities & (long)flippedDiagonalMasks[(square / 8) + 7 - (square %8)]);
        }

        public static string whitePossibleMoves(string history, long wPawn, long wkNight, long wBishop, long wRook, long wQueen, long wKing, long bPawn, long bkNight, long bBishop, long bRook, long bQueen, long bKing)
        {
            whiteUncapturables = ~ (wPawn | wkNight | wBishop | wRook | wQueen | wKing | bKing);
            blackPieces = bPawn | bkNight | bBishop | bRook | bQueen;
            occupiedSquares = wPawn | wkNight | wBishop | wRook | wQueen | wKing | bPawn | bkNight | bBishop | bRook | bQueen | bKing;
            emptySquares = ~ occupiedSquares;
            //basicTime(wPawn);
            straightMoves(36);
            string possibleMoves = allPossiblePawnMovesWhite(history, wPawn, bPawn);
            return possibleMoves;
        }

        public static string allPossiblePawnMovesWhite(string history, long wPawn, long bPawn)
        {
            string allPossiblePawnMoves = "";

            // TODO: maybe extract the for loops into a helper function?

            // moves are generated in format x1, y1, x2, y2

            // possible moves where the pawn captures to the right
            // if our chessboard is co-ord 0 in the top left and 63 in the bottom right then the corresponding integer for
            // each pawn's potential right capture can be found by subtracting 7 from the value of the square the pawn is currently on.
            // However, pawns cannot leave the board so are bound by the last rank and file
            long pawnMoves = (wPawn >> 7) & blackPieces & ~rankEight & ~aFile;
            long possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                // TODO: work out how this works
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i / 8 + 1) + (i % 8 - 1) + (i / 8) + (i % 8); // the starting square is one rank lower and one file to the left of the ending square
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }

            // capture to the left
            pawnMoves = (wPawn >> 9) & blackPieces & ~rankEight & ~hFile;
            possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i / 8 + 1) + (i % 8 + 1) + (i / 8) + (i % 8); // the starting square is one rank lower and one file to the right of the ending square
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }


            // move one square forward
            pawnMoves = (wPawn >> 8) & emptySquares & ~rankEight;
            possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i / 8 + 1) + (i % 8) + (i / 8) + (i % 8); // the starting square is one rank lower than the ending square, both on the same file
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }

            // move two squares forward
            // ensure that the pawn is on its starting square by checking the destination is rank four, and that both squares in front of it are empty
            pawnMoves = (wPawn >> 16) & emptySquares & (emptySquares >> 8) & rankFour;
            possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i / 8 + 2) + (i % 8) + (i / 8) + (i % 8); // the starting square is two ranks lower than the ending square, both on the same file
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }

            // Promotions
            // moves are generated in format y1, y2, "P" (promotion type)

            // promote by capturing right
            pawnMoves = (wPawn >> 7) & blackPieces & rankEight & ~aFile;
            possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i % 8 - 1) + (i % 8) + "QP" + (i % 8 - 1) + (i % 8) + "RP" + (i % 8 - 1) + (i % 8) + "BP" + (i % 8 - 1) + (i % 8) + "NP";
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }

            // promote by capturing left
            pawnMoves = (wPawn >> 9) & blackPieces & rankEight & ~hFile;
            possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i % 8 + 1) + (i % 8) + "QP" + (i % 8 + 1) + (i % 8) + "RP" + (i % 8 + 1) + (i % 8) + "BP" + (i % 8 + 1) + (i % 8) + "NP";
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }

            // promote by moving one forward
            pawnMoves = (wPawn >> 8) & emptySquares & rankEight;
            possibilities = pawnMoves & ~(pawnMoves - 1);
            while (possibilities != 0)
            {
                int i = Helpers.getNumberOfTrailingZeros(possibilities);
                allPossiblePawnMoves += "" + (i % 8) + (i % 8) + "QP" + (i % 8) + (i % 8) + "RP" + (i % 8) + (i % 8) + "BP" + (i % 8) + (i % 8) + "NP";
                pawnMoves &= ~possibilities;
                possibilities = pawnMoves & ~(pawnMoves - 1);
            }

            // en passants
            // in format y1, y2, " ", "E"
            if (history.Length >= 4)
            {
                // checks if the pawn moved along the same file, and then if it moved two squares (either up the board or down the board)
                if ((history[history.Length - 1]) == history[history.Length - 3] && Math.Abs(history[history.Length - 2]) - history[history.Length - 4] == 2)
                {
                    int eFile = history[history.Length - 1] - '0';

                    // en passant right
                    possibilities = (wPawn << 1) & bPawn & rankFive & ~aFile & (long)fileMasks[eFile]; // will find the piece to remove, not the destination
                    if (possibilities != 0)
                    {
                        int i = Helpers.getNumberOfTrailingZeros(possibilities);
                        allPossiblePawnMoves += "" + (i % 8 - 1) + (i % 8) + " E";
                    }

                    // en passant left
                    possibilities = (wPawn >> 1) & bPawn & rankFive & ~hFile & (long)fileMasks[eFile];
                    if (possibilities != 0)
                    {
                        int i = Helpers.getNumberOfTrailingZeros(possibilities);
                        allPossiblePawnMoves += "" + (i % 8 + 1) + (i % 8) + " E";
                    }
                }
            }

            return allPossiblePawnMoves;
        }

        // TODO extract this into an overloaded helper method with Program.drawBitboardFromArray
        // NB this is just a debugging method
        public static void drawBitboard(long bitboard)
        {
            string[,] board = new string[8, 8];
            for (int i = 0; i < 64; i++)
            {
                // every square on the board starts empty
                board[i / 8, i % 8] = "";
            }

            for (int i = 0; i < 64; i++)
            {
                if ((((long)bitboard >> i) & 1) == 1)
                    board[i / 8, i % 8] = "P";
                if ("".Equals(board[i / 8, i % 8]))
                    board[i / 8, i % 8] = " ";
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

        // TODO extract this into a helper function as well
        //public static void basicTime(long wPawn)
        //{
        //    int loopLength = 1000;
        //    long startTime = DateTime.Now.Millisecond;
        //    testMethodA(loopLength, wPawn);
        //    long endTime = DateTime.Now.Millisecond;
        //    Console.WriteLine("That took " + (endTime - startTime) + " milliseconds for the first method");
        //}
    }
}
