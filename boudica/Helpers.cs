using System;
using System.Collections.Generic;
using System.Text;

namespace boudica
{
    public class Helpers
    {
        public static int getNumberOfTrailingZeros(long i)
        {
            // used for bitscan, see
            // https://www.chessprogramming.org/BitScan
            // and
            // https://stackoverflow.com/questions/18622130/how-to-count-trailing-zeros-in-a-binary-representation-of-an-integer-number-with
            // TODO: replace this with something less expensive
            int[] _lookup =
            {
                64,  0,  1, 39,  2, 15, 40, 23, 3, 12, 16, 59, 41, 19, 24, 54, 4, -1, 13, 10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44,
                55, 47, 5, 32, -1, 38, 14, 22, 11, 58, 18, 53, 63,  9, 61, 27, 29, 50, 43, 46, 31, 37, 21, 57, 52,  8, 26, 49, 45, 36,
                56, 7, 48, 35, 6, 34, 33, -1
            };

            return _lookup[(i & -i) % 67];
        }

        public static long reverseBits(long l)
        {
            long tmp = l;
            long r = 0L;

            if (tmp < 0)
                tmp *= -1;

            while (tmp > 0)
            {
                r = (r * 10) + (tmp - ((tmp / 10)) * 10);
                tmp = tmp / 10;
            }

            return r * (l < 0 ? -1 : 1);
        }
    }
}
