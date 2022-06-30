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
#include "evaluate.h"


// Bonus for being the side to move
const int Tempo = 15;


// Calculate a static evaluation of a position
int EvalPosition(const Position *pos) {

    // Material
    int eval = 200 * (PopCount(colorBB(WHITE)) - PopCount(colorBB(BLACK)));

    // Return the evaluation, negated if we are black
    return (sideToMove == WHITE ? eval : -eval) + Tempo;
}
