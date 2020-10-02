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

#pragma once

#include "types.h"
#include "board.h"


enum {
    fileABB = 0x0101010101010101,
    fileBBB = 0x0202020202020202,
    fileCBB = 0x0404040404040404,
    fileDBB = 0x0808080808080808,
    fileEBB = 0x1010101010101010,
    fileFBB = 0x2020202020202020,
    fileGBB = 0x4040404040404040,
    fileHBB = 0x8080808080808080,

    rank1BB = 0xFF,
    rank2BB = 0xFF00,
    rank3BB = 0xFF0000,
    rank4BB = 0xFF000000,
    rank5BB = 0xFF00000000,
    rank6BB = 0xFF0000000000,
    rank7BB = 0xFF000000000000,
    rank8BB = 0xFF00000000000000,

    unused  = fileHBB | rank8BB,
    full    = 0xFFFFFFFFFFFFFF & ~unused,
};

extern const Bitboard FileBB[8];
extern const Bitboard RankBB[8];

extern Bitboard SingleMove[64];
extern Bitboard DoubleMove[64];


// Population count/Hamming weight
INLINE int PopCount(const Bitboard bb) {

    return __builtin_popcountll(bb);
}

// Returns the index of the least significant bit
INLINE int Lsb(const Bitboard bb) {

    assert(bb);
    return __builtin_ctzll(bb);
}

// Returns the index of the least significant bit and unsets it
INLINE int PopLsb(Bitboard *bb) {

    int lsb = Lsb(*bb);
    *bb &= (*bb - 1);

    return lsb;
}

// Returns all single moves from square
INLINE Bitboard SingleMoveBB(Square sq, Bitboard targets) {

    return SingleMove[sq] & targets;
}

// Returns all double moves from square
INLINE Bitboard DoubleMoveBB(Square sq, Bitboard targets) {

    return DoubleMove[sq] & targets;
}
