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

#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "move.h"


uint8_t SqDistance[64][64];

// Zobrist key tables
uint64_t PieceKeys[PIECE_NB][64];
uint64_t SideKey;


// Initialize distance lookup table
CONSTR InitDistance() {

    for (Square sq1 = A1; sq1 <= H8; ++sq1)
        for (Square sq2 = A1; sq2 <= H8; ++sq2) {
            int vertical   = abs(RankOf(sq1) - RankOf(sq2));
            int horizontal = abs(FileOf(sq1) - FileOf(sq2));
            SqDistance[sq1][sq2] = MAX(vertical, horizontal);
        }
}

// Pseudo-random number generator
static uint64_t Rand64() {

    // http://vigna.di.unimi.it/ftp/papers/xorshift.pdf

    static uint64_t seed = 1070372ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717ull;
}

// Inits zobrist key tables
CONSTR InitHashKeys() {

    // Side to play
    SideKey = Rand64();

    // White pieces
    for (Square sq = A1; sq <= H8; ++sq)
        PieceKeys[w][sq] = Rand64();

    // Black pieces
    for (Square sq = A1; sq <= H8; ++sq)
        PieceKeys[b][sq] = Rand64();
}

// Generates a hash key for the position. During
// a search this is incrementally updated instead.
static Key GeneratePosKey(const Position *pos) {

    Key posKey = 0;

    // Pieces
    for (Square sq = A1; sq <= H8; ++sq) {
        Piece piece = pieceOn(sq);
        if (piece != EMPTY)
            posKey ^= PieceKeys[piece][sq];
    }

    // Side to play
    if (sideToMove == WHITE)
        posKey ^= SideKey;

    return posKey;
}

// Calculates the position key after a move. Fails
// for special moves.
Key KeyAfter(const Position *pos, const Move move) {

    Square from = fromSq(move);
    Square to   = toSq(move);
    Piece piece = pieceOn(from);

    return pos->key ^ PieceKeys[piece][from] ^ PieceKeys[piece][to] ^ SideKey;
}

// Clears the board
static void ClearPosition(Position *pos) {

    memset(pos, 0, sizeof(Position));
}

// Update the rest of a position to match pos->board
static void UpdatePosition(Position *pos) {

    // Loop through each square on the board
    for (Square sq = A1; sq <= H8; ++sq) {

        Piece piece = pieceOn(sq);

        // If it isn't empty we update the relevant lists
        if (piece != EMPTY) {

            Color color = ColorOf(piece);

            // Bitboards
            pos->pieceBB   |= BB(sq);
            colorBB(color) |= BB(sq);

            // Material score
            pos->material += PieceValue[piece];
        }
    }
}

// Parse FEN and set up the position as described
void ParseFen(const char *fen, Position *pos) {

    assert(fen != NULL);

    ClearPosition(pos);

    // Piece locations
    Square sq = A7;
    while (*fen != ' ') {

        Piece piece;
        int count = 1;

        switch (*fen) {
            // Pieces
            case 'x': piece = b; break;
            case 'o': piece = w; break;
            // Next rank
            case '/':
                sq -= 15;
                fen++;
                continue;
            // Numbers of empty squares
            default:
                piece = EMPTY;
                count = *fen - '0';
                break;
        }

        pieceOn(sq) = piece;
        sq += count;

        fen++;
    }
    fen++;

    // Update the rest of position to match pos->board
    UpdatePosition(pos);

    // Side to move
    sideToMove = (*fen == 'o') ? WHITE : BLACK;

    // 50 move rule and game moves
    pos->rule50 = atoi(fen += 2);
    pos->gameMoves = atoi(fen += 2);

    // Generate the position key
    pos->key = GeneratePosKey(pos);

    assert(PositionOk(pos));
}

#if defined DEV || !defined NDEBUG

const char PceChar[]  = ".x.o";

// Translates a move to a string
char *BoardToFen(const Position *pos) {
    static char fen[100];
    char *ptr = fen;

    // Board
    for (int rank = RANK_8; rank >= RANK_1; --rank) {

        int count = 0;

        for (int file = FILE_A; file <= FILE_H; ++file) {
            Square sq = (rank * 8) + file;
            Piece piece = pieceOn(sq);

            if (piece) {
                if (count)
                    *ptr++ = '0' + count;
                *ptr++ = PceChar[piece];
                count = 0;
            } else
                count++;
        }

        if (count)
            *ptr++ = '0' + count;

        *ptr++ = rank == RANK_1 ? ' ' : '/';
    }

    // Side to move
    *ptr++ = sideToMove == WHITE ? 'o' : 'x';

    // Add en passant, 50mr and game ply to the base
    sprintf(ptr, " %d %d", pos->rule50, pos->gameMoves);

    return fen;
}
// Print the board with misc info
void PrintBoard(const Position *pos) {

    // Print board
    printf("\n");
    for (int rank = RANK_8; rank >= RANK_1; --rank) {
        for (int file = FILE_A; file <= FILE_H; ++file) {
            Square sq = (rank * 8) + file;
            printf("%3c", PceChar[pieceOn(sq)]);
        }
        printf("\n");
    }
    printf("\n");

    // Print FEN and zobrist key
    puts(BoardToFen(pos));
    printf("Zobrist Key: %" PRIu64 "\n\n", pos->key);
    fflush(stdout);
}
#endif

#ifndef NDEBUG
// Check board state makes sense
bool PositionOk(const Position *pos) {

    assert(0 <= pos->histPly && pos->histPly < MAXGAMEMOVES);
    assert(    0 <= pos->ply && pos->ply < MAXDEPTH);

    int counts[PIECE_NB] = { 0 };

    for (Square sq = A1; sq <= H8; ++sq) {

        Piece piece = pieceOn(sq);
        counts[piece]++;
    }

    assert(PopCount(colorBB(WHITE)) == counts[MakePiece(WHITE)]);
    assert(PopCount(colorBB(BLACK)) == counts[MakePiece(BLACK)]);

    assert(PopCount(pos->pieceBB) ==  counts[MakePiece(WHITE)]
                                    + counts[MakePiece(BLACK)]);

    assert(pos->pieceBB == (colorBB(WHITE) | colorBB(BLACK)));

    assert(sideToMove == WHITE || sideToMove == BLACK);

    assert(GeneratePosKey(pos) == pos->key);

    return true;
}
#endif

#define BB(sq) (1ull << sq)

// Static Exchange Evaluation
bool SEE(const Position *pos, const Move move, const int threshold) {

    (void)pos; (void)move; (void)threshold;

    // TODO
    return true;
}
