/*
  Weixx is a UAI compliant ataxx engine.
  Copyright (C) 2020  Terje Kirstihagen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bitboard.h"
#include "board.h"


const Bitboard FileBB[8] = {
    fileABB, fileBBB, fileCBB, fileDBB, fileEBB, fileFBB, fileGBB, fileHBB
};

const Bitboard RankBB[8] = {
    rank1BB, rank2BB, rank3BB, rank4BB, rank5BB, rank6BB, rank7BB, rank8BB
};

Bitboard SingleMove[64];
Bitboard DoubleMove[64];


// Helper function that returns a bitboard with the landing square of
// the step, or an empty bitboard if the step would go outside the board
INLINE Bitboard LandingSquareBB(const Square sq, const int step) {

    const Square to = sq + step;
    return (Bitboard)(to <= H8 && Distance(sq, to) <= 2) << to;
}

// Initializes non-slider attack lookups
CONSTR InitNonSliderAttacks() {

    Bitboard unused = fileHBB | rank8BB;

    int SingleSteps[8]  = {  -9, -8, -7, -1,  1,  7,  8,  9 };
    int DoubleSteps[16] = { -18,-17,-16,-15,-14,-10, -6, -2, 2, 6, 10, 14, 15, 16, 17, 18 };

    for (Square sq = A1; sq <= H8; ++sq) {

        // SingleMove
        for (int i = 0; i < 8; ++i)
            SingleMove[sq] |= LandingSquareBB(sq, SingleSteps[i]) & ~unused;

        // DoubleMove
        for (int i = 0; i < 16; ++i)
            DoubleMove[sq] |= LandingSquareBB(sq, DoubleSteps[i]) & ~unused;
    }
}
