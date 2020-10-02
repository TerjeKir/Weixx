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


/* Benchmark heavily inspired by Ethereal*/

static const char *BenchmarkFENs[] = {
    #include "bench.csv"
};

typedef struct BenchResult {

    TimePoint elapsed;
    uint64_t nodes;
    int score;
    Move best;

} BenchResult;

void Benchmark(int argc, char **argv) {

    // Default depth 15, 1 thread, and 32MB hash
    Limits.timelimit = false;
    Limits.depth     = argc > 2 ? atoi(argv[2]) : 15;
    int threadCount  = argc > 3 ? atoi(argv[3]) : 1;
    TT.requestedMB   = argc > 4 ? atoi(argv[4]) : DEFAULTHASH;

    Position pos;
    Thread *threads = InitThreads(threadCount);
    InitTT(threads);

    int FENCount = sizeof(BenchmarkFENs) / sizeof(char *);
    BenchResult results[FENCount];
    TimePoint totalElapsed = 1; // Avoid possible div/0
    uint64_t totalNodes = 0;

    for (int i = 0; i < FENCount; ++i) {

        printf("[# %2d] %s\n", i + 1, BenchmarkFENs[i]);

        // Search
        ParseFen(BenchmarkFENs[i], &pos);
        ABORT_SIGNAL = false;
        Limits.start = Now();
        SearchPosition(&pos, threads);

        // Collect results
        BenchResult *r = &results[i];
        r->elapsed = TimeSince(Limits.start);
        r->nodes   = TotalNodes(threads);
        r->score   = threads->score;
        r->best    = threads->bestMove;

        totalElapsed += r->elapsed;
        totalNodes   += r->nodes;

        ClearTT(threads);
    }

    puts("======================================================");

    for (int i = 0; i < FENCount; ++i) {
        BenchResult *r = &results[i];
        printf("[# %2d] %5d cp  %5s %10" PRIu64 " nodes %10d nps\n",
               i+1, r->score, MoveToStr(r->best), r->nodes,
               (int)(1000.0 * r->nodes / (r->elapsed + 1)));
    }

    puts("======================================================");

    printf("OVERALL: %7" PRIi64 " ms %13" PRIu64 " nodes %10d nps\n",
           totalElapsed, totalNodes, (int)(1000.0 * totalNodes / totalElapsed));
}

#ifdef DEV

/* Perft */

static uint64_t leafNodes;

// Generate all pseudo legal moves
void GenAllMoves(const Position *pos, MoveList *list) {

    list->count = list->next = 0;

    GenNoisyMoves(pos, list);
    GenQuietMoves(pos, list);
}

static void RecursivePerft(Position *pos, const Depth depth) {

    if (depth == 0) {
        leafNodes++;
        return;
    }

    MoveList list[1];
    GenAllMoves(pos, list);

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
           "\nLPS   : %" PRId64 "\n",
           elapsed, leafNodes, leafNodes * 1000 / elapsed);
    fflush(stdout);
}

void PrintEval(Position *pos) {

    printf("%d\n", sideToMove == WHITE ? EvalPosition(pos) : -EvalPosition(pos));
    fflush(stdout);
}
#endif
