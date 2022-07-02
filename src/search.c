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


int Reductions[32][32];

SearchLimits Limits;
volatile bool ABORT_SIGNAL = false;
volatile bool SEARCH_STOPPED = true;


// Initializes the late move reduction array
CONSTR InitReductions() {
    for (int depth = 1; depth < 32; ++depth)
        for (int moves = 1; moves < 32; ++moves)
            Reductions[depth][moves] = 0.75 + log(depth) * log(moves) / 2.25;
}

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

    // Probe transposition table
    bool ttHit;
    Key key = pos->key;
    TTEntry *tte = ProbeTT(key, &ttHit);

    Move ttMove = ttHit ? tte->move                         : NOMOVE;
    int ttScore = ttHit ? ScoreFromTT(tte->score, ss->ply) : NOSCORE;

    // Trust the ttScore in non-pvNodes as long as the entry depth is equal or higher
    if (!pvNode && ttHit && tte->depth >= depth) {

        // Check if ttScore causes a cutoff
        if (ttScore >= beta ? tte->bound & BOUND_LOWER
                            : tte->bound & BOUND_UPPER)

            return ttScore;
    }

    // Do a static evaluation for pruning considerations
    int eval = ss->eval = lastMoveNullMove ? -(ss-1)->eval + 2 * Tempo
                                           : EvalPosition(pos);

    // Use ttScore as eval if it is more informative
    if (   ttScore != NOSCORE
        && (tte->bound & (ttScore > eval ? BOUND_LOWER : BOUND_UPPER)))
        eval = ttScore;

    // Improving if not in check, and current eval is higher than 2 plies ago
    bool improving = ss->ply >= 2 && eval > (ss-2)->eval;

    InitNormalMP(&mp, thread, ttMove);

    const int oldAlpha = alpha;
    int moveCount = 0;
    Move bestMove = NOMOVE;
    int bestScore = -INFINITE;
    int score = -INFINITE;

    // Move loop
    Move move;
    while ((move = NextMove(&mp))) {

        __builtin_prefetch(GetEntry(KeyAfter(pos, move)));

        // Make the move, skipping to the next if illegal
        MakeMove(pos, move);

        Depth extension = 0;

        moveCount++;

        const Depth newDepth = depth - 1 + extension;

        bool doLMR =   depth > 2
                    && moveCount > (2 + pvNode)
                    && thread->doPruning;

        // Reduced depth zero-window search
        if (doLMR) {
            // Base reduction
            int R = Reductions[MIN(31, depth)][MIN(31, moveCount)];
            // Reduce less in pv nodes
            R -= pvNode;
            // Reduce less when improving
            R -= improving;

            // Depth after reductions, avoiding going straight to quiescence
            Depth RDepth = CLAMP(newDepth - R, 1, newDepth - 1);

            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, RDepth);
        }
        // Full depth zero-window search
        if (doLMR ? score > alpha : !pvNode || moveCount > 1)
            score = -AlphaBeta(thread, ss+1, -alpha-1, -alpha, newDepth);

        // Full depth alpha-beta window search
        if (pvNode && ((score > alpha && score < beta) || moveCount == 1))
            score = -AlphaBeta(thread, ss+1, -beta, -alpha, newDepth);

        // Undo the move
        TakeMove(pos);

        // Found a new best move in this position
        if (score > bestScore) {

            bestScore = score;
            bestMove  = move;

            // Update the Principle Variation
            if ((score > alpha && pvNode) || (root && moveCount == 1)) {
                ss->pv.length = 1 + (ss+1)->pv.length;
                ss->pv.line[0] = move;
                memcpy(ss->pv.line + 1, (ss+1)->pv.line, sizeof(int) * (ss+1)->pv.length);
            }

            // If score beats alpha we update alpha
            if (score > alpha) {

                alpha = score;

                // Update search history
                if (depth > 1)
                    thread->history[sideToMove][fromSq(move)][toSq(move)] += depth * depth;

                // If score beats beta we have a cutoff
                if (score >= beta)
                    break;
            }
        }
    }

    // Store in TT
    const int flag = bestScore >= beta ? BOUND_LOWER
                   : alpha != oldAlpha ? BOUND_EXACT
                                       : BOUND_UPPER;

    StoreTTEntry(tte, key, bestMove, ScoreToTT(bestScore, ss->ply), depth, flag);

    return bestScore;
}

// Aspiration window
static int AspirationWindow(Thread *thread, Stack *ss) {

    bool mainThread = thread->index == 0;
    int score = thread->score;
    int depth = thread->depth;

    const int initialWindow = 12;
    int delta = 16;

    int alpha = -INFINITE;
    int beta  =  INFINITE;

    int pruningLimit = Limits.timelimit ? (Limits.optimalUsage + 250) / 250 : 4;
    thread->doPruning = depth > MIN(4, pruningLimit);

    // Shrink the window at higher depths
    if (depth > 6)
        alpha = MAX(score - initialWindow, -INFINITE),
        beta  = MIN(score + initialWindow,  INFINITE);

    // Search with aspiration window until the result is inside the window
    while (true) {

        if (alpha < -3500) alpha = -INFINITE;
        if (beta  >  3500) beta  =  INFINITE;

        score = AlphaBeta(thread, ss, alpha, beta, depth);

        // Give an update when done, or after each iteration in long searches
        if (mainThread && (   (score > alpha && score < beta)
                           || TimeSince(Limits.start) > 3000))
            PrintThinking(thread, ss, score, alpha, beta);

        // Failed low, relax lower bound and search again
        if (score <= alpha) {
            alpha = MAX(alpha - delta, -INFINITE);
            beta  = (alpha + beta) / 2;
            depth = thread->depth;

        // Failed high, relax upper bound and search again
        } else if (score >= beta) {
            beta = MIN(beta + delta, INFINITE);
            depth -= (abs(score) < MATE_IN_MAX);

        // Score within the bounds is accepted as correct
        } else
            return score;

        delta += delta * 2 / 3;
    }
}

// Iterative deepening
static void *IterativeDeepening(void *voidThread) {

    Thread *thread = voidThread;
    Position *pos = &thread->pos;
    Stack *ss = thread->ss+SS_OFFSET;
    bool mainThread = thread->index == 0;

    // Iterative deepening
    for (thread->depth = 1; thread->depth <= Limits.depth; ++thread->depth) {

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
