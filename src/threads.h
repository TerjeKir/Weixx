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

#include <setjmp.h>

#include "board.h"
#include "types.h"


#define SS_OFFSET 10


typedef struct {
    int eval;
    Depth ply;
    PV pv;
} Stack;

typedef struct Thread {

    Stack ss[128];
    jmp_buf jumpBuffer;
    Depth depth;
    bool doPruning;
    bool uncertain;
    int multiPV;

    int score;
    Move bestMove;
    Move ponderMove;

    // Anything below here is not zeroed out between searches
    Position pos;

    int index;
    int count;

} Thread;


extern Thread *threads;


void InitThreads(int threadCount);
uint64_t TotalNodes();
uint64_t TotalTBHits();
void PrepareSearch(Position *pos);
void StartMainThread(void *(*func)(void *), Position *pos);
void StartHelpers(void *(*func)(void *));
void WaitForHelpers();
void ResetThreads();
void RunWithAllThreads(void *(*func)(void *));
void Wait(volatile bool *condition);
void Wake();
