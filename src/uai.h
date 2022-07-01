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

#pragma once

#include <string.h>

#include "board.h"
#include "threads.h"


#define NAME "Weixx 0.0-dev"

#define START_FEN "x5o/7/7/7/7/7/o5x x 0 1"
#define INPUT_SIZE 4096


enum InputCommands {
    // UAI
    GO          = 11,
    UAI         = 125,
    STOP        = 28,
    QUIT        = 29,
    ISREADY     = 113,
    POSITION    = 17,
    SETOPTION   = 96,
    UAINEWGAME  = 4,
    // Non-UAI
    EVAL        = 26,
    PRINT       = 112,
    PERFT       = 116,
};


// Reads a line from stdin and strips newline
INLINE bool GetInput(char *str) {

    memset(str, 0, INPUT_SIZE);

    if (fgets(str, INPUT_SIZE, stdin) == NULL)
        return false;

    str[strcspn(str, "\r\n")] = '\0';

    return true;
}

// Sets a limit to the corresponding value in line, if any
INLINE void SetLimit(const char *str, const char *token, int *limit) {
    char *ptr = NULL;
    if ((ptr = strstr(str, token)))
        *limit = atoi(ptr + strlen(token));
}

void PrintThinking(const Thread *thread, int score, int alpha, int beta);
void PrintConclusion(const Thread *thread);
