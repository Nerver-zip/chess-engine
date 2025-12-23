#include <cctype>
#include "board.h"
#include "piece.h"


inline uint64_t BB(int sq) { return 1ULL << sq; }

Piece Board::pieceAt(int sq) const {
    uint64_t b = BB(sq);

    if (whitePawns & b)   return WPAWN;
    if (whiteKnights & b) return WKNIGHT;
    if (whiteBishops & b) return WBISHOP;
    if (whiteRooks & b)   return WROOK;
    if (whiteQueens & b)  return WQUEEN;
    if (whiteKing & b)    return WKING;

    if (blackPawns & b)   return BPAWN;
    if (blackKnights & b) return BKNIGHT;
    if (blackBishops & b) return BBISHOP;
    if (blackRooks & b)   return BROOK;
    if (blackQueens & b)  return BQUEEN;
    if (blackKing & b)    return BKING;

    return EMPTY;
}

Board Board::fromFEN(const char* fen) {
    Board b = {};
    b.enPassantSquare = -1;

    int sq = 56; // a8

    while (*fen && *fen != ' ') {
        char c = *fen++;

        if (c == '/') {
            sq -= 16;
        } else if (isdigit(c)) {
            sq += c - '0';
        } else {
            uint64_t bit = 1ULL << sq;

            switch (c) {
                case 'P': b.whitePawns   |= bit; break;
                case 'N': b.whiteKnights |= bit; break;
                case 'B': b.whiteBishops |= bit; break;
                case 'R': b.whiteRooks   |= bit; break;
                case 'Q': b.whiteQueens  |= bit; break;
                case 'K': b.whiteKing    |= bit; break;

                case 'p': b.blackPawns   |= bit; break;
                case 'n': b.blackKnights |= bit; break;
                case 'b': b.blackBishops |= bit; break;
                case 'r': b.blackRooks   |= bit; break;
                case 'q': b.blackQueens  |= bit; break;
                case 'k': b.blackKing    |= bit; break;
            }
            sq++;
        }
    }

    fen++; // espaço
    b.whiteToMove = (*fen == 'w');
    fen += 2;

    b.castlingRights = 0;
    if (*fen != '-') {
        while (*fen != ' ') {
            if (*fen == 'K') b.castlingRights |= 1;
            if (*fen == 'Q') b.castlingRights |= 2;
            if (*fen == 'k') b.castlingRights |= 4;
            if (*fen == 'q') b.castlingRights |= 8;
            fen++;
        }
    } else fen++;

    fen++;

    if (*fen != '-') {
        int file = fen[0] - 'a';
        int rank = fen[1] - '1';
        b.enPassantSquare = rank * 8 + file;
    }

    return b;
}


Board Board::applyMove(const Move& m) const {
    Board b = *this;

    uint64_t fromBB = 1ULL << m.from;
    uint64_t toBB   = 1ULL << m.to;

    auto movePiece = [&](uint64_t& bb) {
        if (bb & fromBB) {
            bb ^= fromBB;
            bb |= toBB;
        }
    };

    // Remover capturas, se tinha alguém em toBB, remover
    b.blackPawns   &= ~toBB; b.blackKnights &= ~toBB; b.blackBishops &= ~toBB;
    b.blackRooks   &= ~toBB; b.blackQueens  &= ~toBB; b.blackKing    &= ~toBB;
    b.whitePawns   &= ~toBB; b.whiteKnights &= ~toBB; b.whiteBishops &= ~toBB;
    b.whiteRooks   &= ~toBB; b.whiteQueens  &= ~toBB; b.whiteKing    &= ~toBB;


    // Apenas um bitboard vai conter fromBB
    movePiece(b.whitePawns);
    movePiece(b.whiteKnights);
    movePiece(b.whiteBishops);
    movePiece(b.whiteRooks);
    movePiece(b.whiteQueens);
    movePiece(b.whiteKing);

    movePiece(b.blackPawns);
    movePiece(b.blackKnights);
    movePiece(b.blackBishops);
    movePiece(b.blackRooks);
    movePiece(b.blackQueens);
    movePiece(b.blackKing);

    b.whiteToMove = !whiteToMove;
    b.enPassantSquare = -1;

    return b;
}
