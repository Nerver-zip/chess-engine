#include <cctype>
#include "board.h"
#include "piece.h"


constexpr inline uint64_t BB(int sq) { return 1ULL << sq; }

constexpr inline bool onBoard(int r, int f) {
    return r >= 0 && r < 8 && f >= 0 && f < 8;
}

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

constexpr int knightJumps[8] = {17, 15, 10, 6, -6, -10, -15, -17};

void Board::updateAttackBoards() {
    whiteAttacks = 0;
    blackAttacks = 0;

    // ==================== Peões ===============================
    // Peões brancos atacam casas diagonais à frente
    // Para ataque a esquerda ir para o rank acima (8 casas) e subtrair uma, um shift de 7 em bitboard
    // Ataque a direita são 9 casas a mais. Preto é o contrário. Mas tem que tirar a coluna A
    // no ataque da esquerda e tirar a coluna H no ataque da direita feito com bitwise AND com uma mask hexa
    
    // Peões brancos
    uint64_t wPawnLeft  = (whitePawns << 7) & ~0x0101010101010101ULL; // remove coluna a
    uint64_t wPawnRight = (whitePawns << 9) & ~0x8080808080808080ULL; // remove coluna h
    whiteAttacks |= wPawnLeft | wPawnRight;

    // Peões pretos
    uint64_t bPawnLeft  = (blackPawns >> 9) & ~0x0101010101010101ULL;
    uint64_t bPawnRight = (blackPawns >> 7) & ~0x8080808080808080ULL;
    blackAttacks |= bPawnLeft | bPawnRight;

    // =================== Cavalos ===============================
    // Os cavalos se movimentam em deslocamentos pré definidos 
    // 17 -> Sobe dois ranks (+16) e vai para a direita (+1). Mapeamos cada movimento
    // Depois é só somar 
    for (int sq = 0; sq < 64; ++sq) {
        uint64_t mask = BB(sq);
        if (whiteKnights & mask) {
            for (int k = 0; k < 8; ++k) {
                int target = sq + knightJumps[k];
                if (target >= 0 && target < 64) whiteAttacks |= BB(target);
            }
        }
        if (blackKnights & mask) {
            for (int k = 0; k < 8; ++k) {
                int target = sq + knightJumps[k];
                if (target >= 0 && target < 64) blackAttacks |= BB(target);
            }
        }
    }

    // ===== Futuro: bispo/torre/rainha/rei =====
}
