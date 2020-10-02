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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "search.h"
#include "threads.h"
#include "time.h"
#include "transposition.h"


#define PERFT_FEN "x5o/7/7/7/7/7/o5x x 0 1"


#ifdef DEV

/* Perft */

static uint64_t leafNodes;

static void RecursivePerft(Position *pos, const Depth depth) {

    if (depth == 0) {
        leafNodes++;
        return;
    }

    MoveList list[1];
    list->count = list->next = 0;
    GenAllMoves(pos, list);

    if (depth == 1) {
        leafNodes += list->count;
        return;
    }

    if (list->count == 0 && colorBB(sideToMove)) {

        MakeNullMove(pos);
        RecursivePerft(pos, depth - 1);
        TakeNullMove(pos);
    }

    for (int i = 0; i < list->count; ++i) {

        MakeMove(pos, list->moves[i].move);
        RecursivePerft(pos, depth - 1);
        TakeMove(pos);
    }
}

// Counts number of moves that can be made in a position to some depth
void Perft(char *line) {

    Position pos[1];
    Depth depth = 5;
    sscanf(line, "perft %d", &depth);

    char *perftFen = line + 8;
    !*perftFen ? ParseFen(PERFT_FEN, pos)
               : ParseFen(perftFen,  pos);

    printf("\nStarting perft to depth %d\n\n", depth);
    fflush(stdout);

    const TimePoint start = Now();
    leafNodes = 0;

    MoveList list[1];
    list->count = list->next = 0;
    GenAllMoves(pos, list);

    if (list->count == 0 && colorBB(sideToMove)) {

        MakeNullMove(pos);
        RecursivePerft(pos, depth - 1);
        TakeNullMove(pos);
    }

    for (int i = 0; i < list->count; ++i) {

        Move move = list->moves[i].move;

        uint64_t oldCount = leafNodes;

        MakeMove(pos, move);
        RecursivePerft(pos, depth - 1);
        TakeMove(pos);
        uint64_t newCount = leafNodes - oldCount;

        printf("move %d : %s : %" PRId64 "\n", i + 1, MoveToStr(move), newCount);
        fflush(stdout);
    }

    const TimePoint elapsed = TimeSince(start) + 1;

    printf("\nPerft complete:"
           "\nTime  : %" PRId64 "ms"
           "\nLeaves: %" PRIu64
           "\nLPS   : %" PRId64 "\n\n",
           elapsed, leafNodes, leafNodes * 1000 / elapsed);
    fflush(stdout);
}

void PrintEval(Position *pos) {

    printf("%d\n", sideToMove == WHITE ? EvalPosition(pos) : -EvalPosition(pos));
    fflush(stdout);
}
#endif
