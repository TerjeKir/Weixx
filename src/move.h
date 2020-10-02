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

#include "bitboard.h"
#include "types.h"

/* Move contents - total 13 bits used
0000 0000 0000 0000 0011 1111 -> From   <<  0
0000 0000 0000 1111 1100 0000 -> To     <<  6
0000 0000 0001 0000 0000 0000 -> Single << 12
0000 0000 0010 0000 0000 0000 -> Single << 13
*/

#define NOMOVE 0

// Fields
#define MOVE_FROM   0x00003F
#define MOVE_TO     0x000FC0

// Special move flags
#define FLAG_NONE    0
#define FLAG_SINGLE  0x1000

// Move constructor
#define MOVE(f, t, fl) ((f) | ((t) << 6) | (fl))

// Extract info from a move
#define fromSq(move)  ((move) & MOVE_FROM)
#define toSq(move)   (((move) & MOVE_TO) >> 6)

// Move types
#define moveIsSingle(move) (move & FLAG_SINGLE)


bool MoveIsLegal(const Position *pos, Move move);
char *MoveToStr(Move move);
Move ParseMove(const char *ptrChar);
