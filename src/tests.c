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
#include <string.h>

#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movepicker.h"
#include "search.h"
#include "threads.h"
#include "time.h"
#include "transposition.h"


#ifdef DEV

// Depth 0 nodes                 1
// Depth 1 nodes                16
// Depth 2 nodes               256
// Depth 3 nodes             6,460
// Depth 4 nodes           155,888
// Depth 5 nodes         4,752,668
// Depth 6 nodes       141,865,520
// Depth 7 nodes     5,023,479,496
// Depth 8 nodes   176,821,532,236
// Depth 9 nodes 7,047,492,603,320

/* Perft */

static uint64_t RecursivePerft(Thread *thread, const Depth depth) {

    Position *pos = &thread->pos;

    if (depth == 0) return 1;
    if (!colorBB(sideToMove) || pos->pieceBB == full) return 0;

    uint64_t leafnodes = 0;

//     MoveList list;
//     GenAllMoves(pos, &list);

//     // if (depth == 1) return list.count; // Bulk counting

//     for (int i = list.next; i < list.count; ++i) {
//         Move move = list.moves[i].move;
//         MakeMove(pos, move);
//         leafnodes += RecursivePerft(thread, depth - 1);
//         TakeMove(pos);
//     }

    MovePicker mp;
    InitNormalMP(&mp, thread, NOMOVE);

    Move move;
    while ((move = NextMove(&mp))) {
        MakeMove(pos, move);
        leafnodes += RecursivePerft(thread, depth - 1);
        TakeMove(pos);
    }

    return leafnodes;
}

// static uint64_t SplitPerft(Thread *thread, const Depth depth) {

//     Position *pos = &thread->pos;

//     if (depth == 0) return 1;
//     if (!colorBB(sideToMove) || pos->pieceBB == full) return 0;

//     uint64_t leafnodes = 0;

//     MovePicker mp;
//     InitNormalMP(&mp, thread, NOMOVE);

//     Move move;
//     while ((move = NextMove(&mp))) {
//         MakeMove(pos, move);
//         uint64_t perft = RecursivePerft(thread, depth - 1);
//         printf("%s: %" PRIu64 "\n", MoveToStr(move), perft);
//         leafnodes += perft;
//         TakeMove(pos);
//     }

//     return leafnodes;
// }

// Counts number of moves that can be made in a position to some depth
void Perft(char *str) {

    char *default_fen = "x5o/7/7/7/7/7/o5x x 0 1";

    strtok(str, " ");
    char *d = strtok(NULL, " ");
    char *fen = strtok(NULL, "\0") ?: default_fen;

    Depth depth = d ? atoi(d) : 5;
    ParseFen(fen, &threads->pos);

    printf("\nPerft starting:\nDepth : %d\nFEN   : %s\n", depth, fen);
    fflush(stdout);

    const TimePoint start = Now();
    uint64_t leafNodes = RecursivePerft(threads, depth);
    const TimePoint elapsed = TimeSince(start) + 1;

    printf("\nPerft complete:"
           "\nTime : %" PRId64 "ms"
           "\nNPS  : %" PRId64
           "\nNodes: %" PRIu64 "\n",
           elapsed, leafNodes * 1000 / elapsed, leafNodes);
    fflush(stdout);
}

void PrintEval(Position *pos) {
    printf("%d\n", sideToMove == WHITE ? EvalPosition(pos) : -EvalPosition(pos));
    fflush(stdout);
}
#endif
