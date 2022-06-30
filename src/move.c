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

#include <stdlib.h>

#include "bitboard.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "transposition.h"


// Checks whether a move is legal
bool MoveIsLegal(const Position *pos, const Move move) {

    if (!move) return false;

    const Color color = sideToMove;
    const Square from = fromSq(move);
    const Square to = toSq(move);
    const bool blocked = pos->pieceBB & BB(to);

    // Can only move to empty squares
    if (blocked) return false;

    // A single move requires an allied piece adjacent to the destination
    if (moveIsSingle(move))
        return SingleMoveBB(to, colorBB(color));

    // A double move requires an allied piece on the from square
    return colorBB(color) & BB(from);
}

// Translates a move to a string
char *MoveToStr(const Move move) {

    static char moveStr[6];

    int ff = FileOf(fromSq(move));
    int rf = RankOf(fromSq(move));
    int ft = FileOf(toSq(move));
    int rt = RankOf(toSq(move));

    if (moveIsNull(move))
        sprintf(moveStr, "0000");
    else if (moveIsSingle(move))
        sprintf(moveStr, "%c%c", ('a' + ft), ('1' + rt));
    else
        sprintf(moveStr, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));

    return moveStr;
}

// Translates a string to a move
Move ParseMove(const char *str) {

    // Translate coordinates into square numbers
    Square from = StrToSq(str);
    Square to   = StrToSq(str+2);

    int flag = Distance(from, to) == 1 ? FLAG_SINGLE : FLAG_NONE;

    return MOVE(from, to, flag);
}
