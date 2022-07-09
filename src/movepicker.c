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

#include "board.h"
#include "move.h"
#include "movepicker.h"


// Return the next best move
static Move PickNextMove(MovePicker *mp) {

    MoveList *list = &mp->list;

    if (list->next == list->count)
        return NOMOVE;

    return list->moves[list->next++].move;
}

// Returns the next move to try in a position
Move NextMove(MovePicker *mp) {

    Position *pos = &mp->thread->pos;

    // Switch on stage, falls through to the next stage
    // if a move isn't returned in the current stage.
    switch (mp->stage) {
        case GEN:
            GenAllMoves(pos, &mp->list);
            mp->stage++;

            // fall through
        case PLAY:
            return PickNextMove(mp);

        default:
            assert(0);
            return NOMOVE;
    }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, Thread *thread) {
    mp->list.count = mp->list.next = 0;
    mp->thread = thread;
    mp->stage = GEN;
}
