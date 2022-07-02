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
static Move PickNextMove(MoveList *list, const Move ttMove) {

    if (list->next == list->count)
        return NOMOVE;

    int bestIdx = list->next;
    int bestScore = list->moves[bestIdx].score;

    for (int i = list->next + 1; i < list->count; ++i)
        if (list->moves[i].score > bestScore)
            bestScore = list->moves[i].score,
            bestIdx = i;

    Move bestMove = list->moves[bestIdx].move;
    list->moves[bestIdx] = list->moves[list->next++];

    // Avoid returning the TT or killer moves again
    if (bestMove == ttMove)
        return PickNextMove(list, ttMove);

    return bestMove;
}

// Gives a score to each move left in the list
static void ScoreMoves(MoveList *list, const Thread *thread) {

    const Position *pos = &thread->pos;

    for (int i = list->next; i < list->count; ++i) {
        Move move = list->moves[i].move;
        list->moves[i].score = thread->history[sideToMove][fromSq(move)][toSq(move)];
    }
}

// Returns the next move to try in a position
Move NextMove(MovePicker *mp) {

    Move move;
    Position *pos = &mp->thread->pos;

    // Switch on stage, falls through to the next stage
    // if a move isn't returned in the current stage.
    switch (mp->stage) {

        case TTMOVE:
            mp->stage++;
            return mp->ttMove;

            // fall through
        case GEN:
            GenAllMoves(pos, mp->list);
            ScoreMoves(mp->list, mp->thread);
            mp->stage++;

            // fall through
        case PLAY:
            while ((move = PickNextMove(mp->list, mp->ttMove)))
                    return move;

            return NOMOVE;

        default:
            assert(0);
            return NOMOVE;
    }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, MoveList *list, Thread *thread, Move ttMove) {
    mp->list   = list;
    mp->thread = thread;
    mp->ttMove = MoveIsLegal(&thread->pos, ttMove) ? ttMove : NOMOVE;
    mp->stage  = mp->ttMove ? TTMOVE : GEN;
}
