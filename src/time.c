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

#include "search.h"
#include "time.h"
#include "types.h"


// Decide how much time to spend this turn
void InitTimeManagement() {

    const int overhead = 5;

    // No time to manage
    if (!Limits.timelimit)
        return;

    // In movetime mode just use all the time given each turn
    if (Limits.movetime) {
        Limits.maxUsage = Limits.optimalUsage = Limits.movetime - overhead;
        return;
    }

    Limits.maxUsage = Limits.time / 30;
    Limits.optimalUsage = Limits.time / 30;
}

// Check time situation
bool OutOfTime(Thread *thread) {
    return (thread->pos.nodes & 4095) == 4095
        && thread->index == 0
        && Limits.timelimit
        && TimeSince(Limits.start) >= Limits.maxUsage;
}
