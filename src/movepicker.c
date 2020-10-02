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
static Move PickNextMove(MoveList *list, const Move ttMove, const Move kill1, const Move kill2) {

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
    if (bestMove == ttMove || bestMove == kill1 || bestMove == kill2)
        return PickNextMove(list, ttMove, kill1, kill2);

    return bestMove;
}

// Gives a score to each move left in the list
static void ScoreMoves(MoveList *list, const Thread *thread, const int stage) {

    const Position *pos = &thread->pos;

    for (int i = list->next; i < list->count; ++i) {

        Move move = list->moves[i].move;

        // TODO FIX
        if (stage == GEN_NOISY)
            list->moves[i].score = thread->history[sideToMove][fromSq(move)][toSq(move)];

        if (stage == GEN_QUIET)
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
        case GEN_NOISY:
            GenAllMoves(pos, mp->list);
            ScoreMoves(mp->list, mp->thread, GEN_NOISY);
            mp->stage++;

            // fall through
        case NOISY_GOOD:
            while ((move = PickNextMove(mp->list, mp->ttMove, NOMOVE, NOMOVE)))
                    return move;

            mp->stage++;

            // fall through
        case KILLER1:
            mp->stage++;
            if (   mp->kill1 != mp->ttMove
                && MoveIsLegal(pos, mp->kill1))
                return mp->kill1;

            // fall through
        case KILLER2:
            mp->stage++;
            if (   mp->kill2 != mp->ttMove
                && MoveIsLegal(pos, mp->kill2))
                return mp->kill2;

            // fall through
        case GEN_QUIET:
            mp->stage++;

            // fall through
        case QUIET:
            mp->stage++;
            mp->list->next = 0;
            mp->list->moves[mp->bads].move = NOMOVE;

            // fall through
        case NOISY_BAD:
            return mp->list->moves[mp->list->next++].move;

        default:
            assert(0);
            return NOMOVE;
        }
}

// Init normal movepicker
void InitNormalMP(MovePicker *mp, MoveList *list, Thread *thread, Move ttMove, Move kill1, Move kill2) {
    mp->list      = list;
    mp->thread    = thread;
    mp->ttMove    = MoveIsLegal(&thread->pos, ttMove) ? ttMove : NOMOVE;
    mp->stage     = mp->ttMove ? TTMOVE : GEN_NOISY;
    mp->kill1     = kill1;
    mp->kill2     = kill2;
    mp->bads      = 0;
    mp->onlyNoisy = false;
}
