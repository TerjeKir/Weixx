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

#include "board.h"
#include "makemove.h"
#include "move.h"
#include "search.h"
#include "tests.h"
#include "threads.h"
#include "time.h"
#include "transposition.h"
#include "uai.h"


// Parses the time controls
static void ParseTimeControl(const char *str, const Color color) {

    memset(&Limits, 0, sizeof(SearchLimits));

    Limits.start = Now();

    // Parse relevant search constraints
    Limits.infinite = strstr(str, "infinite");
    SetLimit(str, color == WHITE ? "wtime" : "btime", &Limits.time);
    SetLimit(str, color == WHITE ? "winc"  : "binc" , &Limits.inc);
    SetLimit(str, "movestogo", &Limits.movestogo);
    SetLimit(str, "movetime",  &Limits.movetime);
    SetLimit(str, "depth",     &Limits.depth);

    Limits.timelimit = Limits.time || Limits.movetime;
    Limits.depth = Limits.depth ?: 100;
}

// Parses the given limits and creates a new thread to start the search
INLINE void Go(Position *pos, const char *str) {
    ABORT_SIGNAL = false;
    InitTT();
    TT.dirty = true;
    ParseTimeControl(str, sideToMove);
    StartMainThread(SearchPosition, pos);
}

// Parses a 'position' and sets up the board
static void Pos(Position *pos, char *str) {

    #define IsFen (!strncmp(str, "position fen", 12))

    // Set up original position. This will either be a
    // position given as FEN, or the normal start position
    ParseFen(IsFen ? str + 13 : START_FEN, pos);

    // Check if there are moves to be made from the initial position
    if ((str = strstr(str, "moves")) == NULL) return;

    // Loop over the moves and make them in succession
    char *move = strtok(str, " ");
    while ((move = strtok(NULL, " "))) {

        // Parse and make move
        MakeMove(pos, ParseMove(move));

        // Reset ply to avoid triggering asserts in debug mode in long games
        pos->ply = 0;

        // Keep track of how many moves have been played
        pos->gameMoves += sideToMove == WHITE;

        // Reset histPly so long games don't go out of bounds of arrays
        if (pos->rule50 == 0)
            pos->histPly = 0;
    }

    pos->nodes = 0;
}

// Parses a 'setoption' and updates settings
static void SetOption(char *str) {

    char *optionName  = (strstr(str, "name") + 5);
    char *optionValue = (strstr(str, "value") + 6);

    #define OptionNameIs(name) (!strncmp(optionName, name, strlen(name)))
    #define IntValue           (atoi(optionValue))

    if      (OptionNameIs("Hash"   )) RequestTTSize(IntValue);
    else if (OptionNameIs("Threads")) InitThreads(IntValue);
    else puts("info string No such option.");

    fflush(stdout);
}

// Prints UAI info
static void Info() {
    printf("id name %s\n", NAME);
    printf("id author Terje Kirstihagen\n");
    printf("option name Hash type spin default %d min %d max %d\n", DEFAULTHASH, MINHASH, MAXHASH);
    printf("option name Threads type spin default %d min %d max %d\n", 1, 1, 2048);
    printf("uaiok\n"); fflush(stdout);
}

// Stops searching
static void Stop() {
    ABORT_SIGNAL = true;
    Wake();
    Wait(&SEARCH_STOPPED);
}

// Signals the engine is ready
static void IsReady() {
    InitTT();
    printf("readyok\n");
    fflush(stdout);
}

// Reset for a new game
static void NewGame() {
    ClearTT();
    ResetThreads();
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
    InitThreads(1);
    Position pos;
    ParseFen(START_FEN, &pos);

    // Input loop
    char str[INPUT_SIZE];
    while (GetInput(str)) {
        switch (HashInput(str)) {
            case GO         : Go(&pos, str);  break;
            case UAI        : Info();         break;
            case ISREADY    : IsReady();      break;
            case POSITION   : Pos(&pos, str); break;
            case SETOPTION  : SetOption(str); break;
            case UAINEWGAME : NewGame();      break;
            case STOP       : Stop();         break;
            case QUIT       : Stop();         return 0;
#ifdef DEV
            // Non-UAI commands
            case EVAL       : PrintEval(&pos);  break;
            case PRINT      : PrintBoard(&pos); break;
            case PERFT      : Perft(str);       break;
#endif
        }
    }
}

// Translates an internal mate score into distance to mate
INLINE int MateScore(const int score) {
    int d = (MATE - abs(score) + 1) / 2;
    return score > 0 ? d : -d;
}

// Print thinking
void PrintThinking(const Thread *thread, int score, int alpha, int beta) {

    const Position *pos = &thread->pos;

    // Determine whether we have a centipawn or mate score
    char *type = abs(score) >= MATE_IN_MAX ? "mate" : "cp";

    // Determine if score is a lower bound, upper bound or exact
    char *bound = score >= beta  ? " lowerbound"
                : score <= alpha ? " upperbound"
                                 : "";

    // Translate internal score into printed score
    score = abs(score) >= MATE_IN_MAX ? MateScore(score) : score;

    TimePoint elapsed = TimeSince(Limits.start);
    uint64_t nodes    = TotalNodes(thread);
    int hashFull      = HashFull();
    int nps           = (int)(1000 * nodes / (elapsed + 1));

    Depth seldepth = MAX_PLY;
    for (; seldepth > 0; --seldepth)
        if (history(seldepth-1).key != 0) break;

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

// Print conclusion of search
void PrintConclusion(const Thread *thread) {
    printf("bestmove %s", MoveToStr(thread->bestMove));
    if (thread->ponderMove)
        printf(" ponder %s", MoveToStr(thread->ponderMove));
    printf("\n\n");
    fflush(stdout);
}
