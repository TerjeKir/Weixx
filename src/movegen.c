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
#include "makemove.h"
#include "move.h"
#include "movegen.h"


// Constructs and adds a move to the move list
INLINE void AddMove(MoveList *list, const Square from, const Square to, const int flag) {
    list->moves[list->count++].move = MOVE(from, to, flag);
}

// Generate moves
static void GenMoves(const Position *pos, MoveList *list, const Color color) {

    const Bitboard empty = ~pos->pieceBB;
    Bitboard pieces = colorBB(color);
    Bitboard singles = 0;

    while (pieces) {

        Square from = PopLsb(&pieces);
        singles |= SingleMoveBB(from, empty);
        Bitboard doubles = DoubleMoveBB(from, empty);

        while (doubles)
            AddMove(list, from, PopLsb(&doubles), FLAG_NONE);
    }

    while (singles)
        AddMove(list, 0, PopLsb(&singles), FLAG_SINGLE);
}

// Generate noisy moves
void GenAllMoves(const Position *pos, MoveList *list) {

    list->count = list->next = 0;
    GenMoves(pos, list, sideToMove);

    if (list->count == 0)
        list->moves[list->count++].move = NULLMOVE;
}
