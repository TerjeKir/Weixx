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

#include "types.h"


typedef struct {
    Key key;
    Bitboard captures;
    Move move;
    int rule50;
} History;

typedef struct Position {

    uint8_t board[64];
    Bitboard pieceBB;
    Bitboard colorBB[COLOR_NB];

    Color stm;
    int rule50;
    uint16_t histPly;
    uint16_t gameMoves;

    Key key;

    uint64_t nodes;

    History gameHistory[256];

} Position;


extern uint8_t SqDistance[64][64];

// Zobrist keys
extern uint64_t PieceKeys[PIECE_NB][64];
extern uint64_t CastleKeys[16];
extern uint64_t SideKey;


void InitDistance();
void ParseFen(const char *fen, Position *pos);
Key KeyAfter(const Position *pos, Move move);
#ifndef NDEBUG
void PrintBoard(const Position *pos);
bool PositionOk(const Position *pos);
#endif
#ifdef DEV
void PrintBoard(const Position *pos);
#endif

INLINE int FileOf(const Square square) {
    return square & 7;
}

INLINE int RankOf(const Square square) {
    return square >> 3;
}

INLINE int Distance(const Square sq1, const Square sq2) {
    return SqDistance[sq1][sq2];
}

INLINE Color ColorOf(const Piece piece) {
    return piece >> 1;
}

INLINE Piece MakePiece(const Color color) {
    return (color << 1) + 1;
}

INLINE Square MakeSquare(const int rank, const int file) {
    return (rank * FILE_NB) + file;
}

INLINE Square StrToSq(const char *str) {
    return MakeSquare(str[1] - '1', str[0] - 'a');
}

INLINE bool ValidPiece(const Piece piece) {
    return piece == w || piece == b;
}

INLINE bool IsRepetition(const Position *pos) {
    for (int i = 4; i <= pos->rule50 && i <= pos->histPly; i += 2)
        if (pos->key == history(-i).key)
            return true;
    return false;
}
