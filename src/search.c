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

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "makemove.h"
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "time.h"
#include "threads.h"
#include "transposition.h"
#include "search.h"
#include "uai.h"


SearchLimits Limits;
volatile bool ABORT_SIGNAL = false;
volatile bool SEARCH_STOPPED = true;


// Alpha Beta
static int AlphaBeta(Thread *thread, Stack *ss, int alpha, int beta, Depth depth) {

    Position *pos = &thread->pos;
    MovePicker mp;
    ss->pv.length = 0;

    const bool pvNode = alpha != beta - 1;
    const bool root   = ss->ply == 0;

    // Check time situation
    if (OutOfTime(thread) || ABORT_SIGNAL)
        longjmp(thread->jumpBuffer, true);

    // Early exits
    if (!root) {

        if (!colorBB(sideToMove))
            return -MATE + ss->ply;

        if (pos->pieceBB == full)
            return PopCount(colorBB( sideToMove)) >
                   PopCount(colorBB(!sideToMove)) ?  MATE_IN_MAX - ss->ply
                                                  : -MATE_IN_MAX + ss->ply;

        // Position is drawn
        if (IsRepetition(pos) || pos->rule50 >= 100)
            return 0;

        // Max depth reached
        if (ss->ply >= MAX_PLY)
            return EvalPosition(pos);

        // Mate distance pruning
        alpha = MAX(alpha, -MATE + ss->ply);
        beta  = MIN(beta,   MATE - ss->ply - 1);
        if (alpha >= beta)
            return alpha;
    }

    // Quiescence at the end of search
    if (depth <= 0)
        return EvalPosition(pos);

    InitNormalMP(&mp, thread);

    int moveCount = 0;
    int bestScore = -INFINITE;
    int score = -INFINITE;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        MakeMove(pos, move);

        Depth extension = 0;

        moveCount++;

        const Depth newDepth = depth - 1 + extension;

        score = -AlphaBeta(thread, ss+1, -beta, -alpha, newDepth);

        // Undo the move
        TakeMove(pos);

        // New best move
        if (score > bestScore) {

            bestScore = score;

            // Update PV
            if ((score > alpha && pvNode) || (root && moveCount == 1)) {
                ss->pv.length = 1 + (ss+1)->pv.length;
                ss->pv.line[0] = move;
                memcpy(ss->pv.line+1, (ss+1)->pv.line, sizeof(int) * (ss+1)->pv.length);
            }

            // If score beats alpha we update alpha
            if (score > alpha) {

                alpha = score;

                // If score beats beta we have a cutoff
                if (score >= beta)
                    break;
            }
        }
    }

    return bestScore;
}

// Aspiration window
static int AspirationWindow(Thread *thread, Stack *ss) {

    int depth = thread->depth;

    int alpha = -INFINITE;
    int beta  =  INFINITE;

    thread->doPruning = true;

    int score = AlphaBeta(thread, ss, alpha, beta, depth);

    PrintThinking(thread, ss, score, alpha, beta);

    return score;
}

// Iterative deepening
static void *IterativeDeepening(void *voidThread) {

    Thread *thread = voidThread;
    Position *pos = &thread->pos;
    Stack *ss = thread->ss+SS_OFFSET;
    bool mainThread = thread->index == 0;

    // Iterative deepening
    while (++thread->depth <= (mainThread ? Limits.depth : MAX_PLY)) {

        // Jump here and return if we run out of allocated time mid-search
        if (setjmp(thread->jumpBuffer)) break;

        // Search position, using aspiration windows for higher depths
        thread->score = AspirationWindow(thread, ss);

        // Only the main thread concerns itself with the rest
        if (!mainThread) continue;

        bool uncertain = ss->pv.line[0] != thread->bestMove;

        // Save bestMove and ponderMove before overwriting the pv next iteration
        thread->bestMove   = ss->pv.line[0];
        thread->ponderMove = ss->pv.length > 1 ? ss->pv.line[1] : NOMOVE;

        // If an iteration finishes after optimal time usage, stop the search
        if (   Limits.timelimit
            && TimeSince(Limits.start) > Limits.optimalUsage * (1 + uncertain))
            break;

        // Clear key history for seldepth calculation
        for (int i = 1; i < MAX_PLY; ++i)
            history(i).key = 0;
    }

    return NULL;
}

// Root of search
void *SearchPosition(void *pos) {

    SEARCH_STOPPED = false;

    InitTimeManagement();
    PrepareSearch(pos);

    // Start helper threads and begin searching
    StartHelpers(IterativeDeepening);
    IterativeDeepening(&threads[0]);

    // Wait for 'stop' in infinite search
    if (Limits.infinite) Wait(&ABORT_SIGNAL);

    // Signal helper threads to stop and wait for them to finish
    ABORT_SIGNAL = true;
    WaitForHelpers();

    // Print conclusion
    PrintConclusion(threads);

    SEARCH_STOPPED = true;
    Wake();

    return NULL;
}
