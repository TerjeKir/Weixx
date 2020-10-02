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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "makemove.h"
#include "move.h"
#include "search.h"
#include "tests.h"
#include "threads.h"
#include "time.h"
#include "transposition.h"
#include "uci.h"


// Parses the time controls
static void ParseTimeControl(char *str, Color color) {

    memset(&Limits, 0, sizeof(SearchLimits));

    Limits.start = Now();

    // Read in relevant search constraints
    Limits.infinite = strstr(str, "infinite");
    if (color == WHITE)
        SetLimit(str, "wtime", &Limits.time),
        SetLimit(str, "winc",  &Limits.inc);
    else
        SetLimit(str, "btime", &Limits.time),
        SetLimit(str, "binc",  &Limits.inc);
    SetLimit(str, "movestogo", &Limits.movestogo);
    SetLimit(str, "movetime",  &Limits.movetime);
    SetLimit(str, "depth",     &Limits.depth);

    Limits.timelimit = Limits.time || Limits.movetime;

    // If no depth limit is given, use MAXDEPTH - 1
    Limits.depth = Limits.depth == 0 ? MAXDEPTH - 1 : Limits.depth;
}

// Begins a search with the given setup
static void *BeginSearch(void *voidEngine) {

    Engine *engine = voidEngine;
    SearchPosition(&engine->pos, engine->threads);
    return NULL;
}

// Parses the given limits and creates a new thread to start the search
INLINE void UAIGo(Engine *engine, char *str) {

    ABORT_SIGNAL = false;
    InitTT(engine->threads);
    ParseTimeControl(str, engine->pos.stm);
    pthread_create(&engine->threads->pthreads[0], NULL, &BeginSearch, engine);
    pthread_detach(engine->threads->pthreads[0]);
}

// Parses a 'position' and sets up the board
static void UAIPosition(Position *pos, char *str) {

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    BeginsWith(str, "position fen") ? ParseFen(str + 13, pos)
                                    : ParseFen(START_FEN, pos);

    // Check if there are moves to be made from the initial position
    if ((str = strstr(str, "moves")) == NULL)
        return;

    // Loop over the moves and make them in succession
    char *move = strtok(str, " ");
    while ((move = strtok(NULL, " "))) {

        // Parse and make move
        MakeMove(pos, ParseMove(move));

        // Reset ply to avoid triggering asserts in debug mode in long games
        pos->ply = 0;

        // Keep track of how many moves have been played so far for TM
        pos->gameMoves += sideToMove == WHITE;

        // Reset histPly so long games don't go out of bounds of arrays
        if (pos->rule50 == 0)
            pos->histPly = 0;
    }
}

// Parses a 'setoption' and updates settings
static void UAISetOption(Engine *engine, char *str) {

    // Sets the size of the transposition table
    if (OptionName(str, "Hash")) {

        TT.requestedMB = atoi(OptionValue(str));

        printf("Hash will use %" PRI_SIZET "MB after next 'isready'.\n", TT.requestedMB);

    // Sets number of threads to use for searching
    } else if (OptionName(str, "Threads")) {

        free(engine->threads->pthreads);
        free(engine->threads);
        engine->threads = InitThreads(atoi(OptionValue(str)));

        printf("Search will use %d threads.\n", engine->threads->count);
    }

    fflush(stdout);
}

// Prints UAI info
static void UAIInfo() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name Threads type spin default %d min %d max %d\n", 1, 1, 2048);
    printf("uciok\n"); fflush(stdout);
}

// Stops searching
static void UAIStop(Engine *engine) {
    ABORT_SIGNAL = true;
    Wake(engine->threads);
}

// Signals the engine is ready
static void UAIIsReady(Engine *engine) {
    InitTT(engine->threads);
    printf("readyok\n");
    fflush(stdout);
}

// Reset for a new game
static void UAINewGame(Engine *engine) {
    ClearTT(engine->threads);
}

// Hashes the first token in a string
static int HashInput(char *str) {
    int hash = 0;
    int len = 1;
    while (*str && *str != ' ')
        hash ^= *(str++) ^ len++;
    return hash;
}

// Sets up the engine and follows UAI protocol commands
int main() {

    // Init engine
    Engine engine = { .threads = InitThreads(1) };
    Position *pos = &engine.pos;

    // Setup the default position
    ParseFen(START_FEN, pos);

    // Input loop
    char str[INPUT_SIZE];
    while (GetInput(str)) {
        switch (HashInput(str)) {
            case GO         : UAIGo(&engine, str);        break;
            case UAI        : UAIInfo();                  break;
            case ISREADY    : UAIIsReady(&engine);        break;
            case POSITION   : UAIPosition(pos, str);      break;
            case SETOPTION  : UAISetOption(&engine, str); break;
            case UAINEWGAME : UAINewGame(&engine);        break;
            case STOP       : UAIStop(&engine);           break;
            case QUIT       : UAIStop(&engine);           return 0;
#ifdef DEV
            // Non-UAI commands
            case EVAL       : PrintEval(pos);      break;
            case PRINT      : PrintBoard(pos);     break;
            case PERFT      : Perft(str);          break;
#endif
        }
    }
}

// Translates an internal mate score into distance to mate
INLINE int MateScore(const int score) {
    return score > 0 ?  ((MATE - score) / 2) + 1
                     : -((MATE + score) / 2);
}

// Print thinking
void PrintThinking(const Thread *thread, int score, int alpha, int beta) {

    const Position *pos = &thread->pos;

    // Determine whether we have a centipawn or mate score
    char *type = abs(score) >= MATE_IN_MAX ? "mate" : "cp";

    // Determine if score is an upper or lower bound
    char *bound = score >= beta  ? " lowerbound"
                : score <= alpha ? " upperbound"
                                 : "";

    // Translate internal score into printed score
    score = abs(score) >= MATE_IN_MAX ? MateScore(score) : score;

    TimePoint elapsed = TimeSince(Limits.start);
    uint64_t nodes    = TotalNodes(thread);
    int hashFull      = HashFull();
    int nps           = (int)(1000 * nodes / (elapsed + 1));

    Depth seldepth = MAXDEPTH;
    for (; seldepth > 0; --seldepth)
        if (history(seldepth-1).posKey != 0) break;

    // Basic info
    printf("info depth %d seldepth %d score %s %d%s time %" PRId64
           " nodes %" PRIu64 " nps %d hashfull %d pv",
            thread->depth, seldepth, type, score, bound, elapsed,
            nodes, nps, hashFull);

    // Principal variation
    for (int i = 0; i < thread->pv.length; i++)
        printf(" %s", MoveToStr(thread->pv.line[i]));

    printf("\n");
    fflush(stdout);
}

// Print conclusion of search - best move and ponder move
void PrintConclusion(const Thread *thread) {

    printf("bestmove %s", MoveToStr(thread->bestMove));
    if (thread->ponderMove)
        printf(" ponder %s", MoveToStr(thread->ponderMove));
    printf("\n\n");
    fflush(stdout);
}
