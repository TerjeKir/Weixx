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
    Key posKey;
    Bitboard captures;
    Move move;
    int eval;
    uint8_t rule50;
} History;

typedef struct Position {

    uint8_t board[64];
    Bitboard pieceBB;
    Bitboard colorBB[2];

    int material;

    Color stm;
    uint8_t rule50;
    uint8_t ply;
    uint16_t histPly;
    uint16_t gameMoves;

    Key key;

    uint64_t nodes;

    History gameHistory[MAXGAMEMOVES];

} Position;


extern uint8_t SqDistance[64][64];

// Zobrist keys
extern uint64_t PieceKeys[PIECE_NB][64];
extern uint64_t CastleKeys[16];
extern uint64_t SideKey;


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

INLINE Square AlgebraicToSq(const char file, const char rank) {
    return (file - 'a') + 8 * (rank - '1');
}

INLINE bool ValidPiece(const Piece piece) {
    return piece == w || piece == b;
}
