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

#include "bitboard.h"
#include "board.h"
#include "evaluate.h"
#include "move.h"


#define HASH_PCE(piece, sq) (pos->key ^= PieceKeys[(piece)][(sq)])
#define HASH_SIDE           (pos->key ^= SideKey)


// Remove a piece from a square sq
static void ClearPiece(Position *pos, const Square sq, const bool hash) {

    const Piece piece = pieceOn(sq);
    const Color color = ColorOf(piece);

    assert(ValidPiece(piece));

    // Hash out the piece
    if (hash)
        HASH_PCE(piece, sq);

    // Set square to empty
    pieceOn(sq) = EMPTY;

    // Update material
    pos->material -= PieceValue[piece];

    // Update bitboards
    pos->pieceBB   ^= BB(sq);
    colorBB(color) ^= BB(sq);
}

// Add a piece piece to a square
static void AddPiece(Position *pos, const Square sq, const Piece piece, const bool hash) {

    assert(pieceOn(sq) == EMPTY);

    const Color color = ColorOf(piece);

    // Hash in piece at square
    if (hash)
        HASH_PCE(piece, sq);

    // Update square
    pieceOn(sq) = piece;

    // Update material
    pos->material += PieceValue[piece];

    // Update bitboards
    pos->pieceBB   |= BB(sq);
    colorBB(color) |= BB(sq);
}

// Move a piece from one square to another
static void MovePiece(Position *pos, const Square from, const Square to, const bool hash) {

    const Piece piece = pieceOn(from);
    const Color color = ColorOf(piece);

    assert(ValidPiece(piece));
    assert(pieceOn(to) == EMPTY);

    // Hash out piece on old square, in on new square
    if (hash)
        HASH_PCE(piece, from),
        HASH_PCE(piece, to);

    // Set old square to empty, new to piece
    pieceOn(from) = EMPTY;
    pieceOn(to)   = piece;

    // Update bitboards
    pos->pieceBB   ^= BB(from) ^ BB(to);
    colorBB(color) ^= BB(from) ^ BB(to);
}

// Make a move
void MakeMove(Position *pos, const Move move) {

    // Save position
    history(0).posKey = pos->key;
    history(0).move   = move;
    history(0).rule50 = pos->rule50;

    const Square from = fromSq(move);
    const Square to = toSq(move);
    const bool single = moveIsSingle(move);

    if (single)
        AddPiece(pos, to, MakePiece(sideToMove), true), pos->rule50 = 0;
    else
        MovePiece(pos, from, to, true);

    Bitboard captures = SingleMoveBB(to, colorBB(!sideToMove));
    history(0).captures = captures;

    while (captures) {
        Square victim = PopLsb(&captures);
        ClearPiece(pos, victim, true);
        AddPiece(pos, victim, MakePiece(sideToMove), true);
    }

    // Increment histPly, ply and 50mr
    pos->histPly++;
    pos->ply++;
    pos->rule50++;
    pos->nodes++;

    // Change turn to play
    sideToMove ^= 1;
    HASH_SIDE;

    assert(PositionOk(pos));
}

// Take back the previous move
void TakeMove(Position *pos) {

    // Decrement histPly, ply
    pos->histPly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Get the move from history
    const Move move = history(0).move;
    const Square from = fromSq(move);
    const Square to = toSq(move);
    const bool single = moveIsSingle(move);

    if (single)
        ClearPiece(pos, to, false);
    else
        MovePiece(pos, to, from, false);

    Bitboard captures = history(0).captures;

    while (captures) {
        Square victim = PopLsb(&captures);
        ClearPiece(pos, victim, false);
        AddPiece(pos, victim, MakePiece(!sideToMove), false);
    }

    // Get various info from history
    pos->key    = history(0).posKey;
    pos->rule50 = history(0).rule50;

    assert(PositionOk(pos));
}

// Pass the turn without moving
void MakeNullMove(Position *pos) {

    // Save misc info for takeback
    history(0).posKey = pos->key;
    history(0).move   = MOVE(0, 0, FLAG_NULL);
    history(0).rule50 = pos->rule50;

    // Increase ply
    pos->ply++;
    pos->histPly++;

    pos->rule50 = 0;

    // Change side to play
    sideToMove ^= 1;
    HASH_SIDE;

    assert(PositionOk(pos));
}

// Take back a null move
void TakeNullMove(Position *pos) {

    // Decrease ply
    pos->histPly--;
    pos->ply--;

    // Change side to play
    sideToMove ^= 1;

    // Get info from history
    pos->key    = history(0).posKey;
    pos->rule50 = history(0).rule50;

    assert(PositionOk(pos));
}
