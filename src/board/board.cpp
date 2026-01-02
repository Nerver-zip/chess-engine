#include <cctype>
#include "board.h"
#include "bitboard.h"
#include "attack.h"

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

void Board::updateAttackBoards() {
    whiteAttacks = 0;
    blackAttacks = 0;

    // Occupancy total é necessária para calcular bloqueios de peças deslizantes
    uint64_t occ = allPieces();

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
    // Os cavalos pulam em L. Usamos a tabela pré-calculada KNIGHT_ATTACKS.
    // Iteramos apenas sobre os bits ativos (onde realmente tem cavalo) para performance.
    
    // Cavalos Brancos
    uint64_t wKnights = whiteKnights;
    while (wKnights) {
        int sq = __builtin_ctzll(wKnights); // Pega o índice do primeiro bit ativo
        whiteAttacks |= KNIGHT_ATTACKS[sq];
        wKnights &= wKnights - 1;           // Remove esse bit para processar o próximo
    }

    // Cavalos Pretos
    uint64_t bKnights = blackKnights;
    while (bKnights) {
        int sq = __builtin_ctzll(bKnights);
        blackAttacks |= KNIGHT_ATTACKS[sq];
        bKnights &= bKnights - 1;
    }

    // =================== Bispos ================================
    // Usamos Magic Bitboards para calcular os ataques em O(1) considerando as peças que bloqueiam (occ).
    
    // Bispos Brancos
    uint64_t wBishops = whiteBishops;
    while (wBishops) {
        int sq = __builtin_ctzll(wBishops);
        whiteAttacks |= bishopAttacks(sq, occ);
        wBishops &= wBishops - 1;
    }

    // Bispos Pretos
    uint64_t bBishops = blackBishops;
    while (bBishops) {
        int sq = __builtin_ctzll(bBishops);
        blackAttacks |= bishopAttacks(sq, occ);
        bBishops &= bBishops - 1;
    }

    // =================== Torres ================================
    // Mesma lógica de Magic Bitboards.
    
    // Torres Brancas
    uint64_t wRooks = whiteRooks;
    while (wRooks) {
        int sq = __builtin_ctzll(wRooks);
        whiteAttacks |= rookAttacks(sq, occ);
        wRooks &= wRooks - 1;
    }

    // Torres Pretas
    uint64_t bRooks = blackRooks;
    while (bRooks) {
        int sq = __builtin_ctzll(bRooks);
        blackAttacks |= rookAttacks(sq, occ);
        bRooks &= bRooks - 1;
    }

    // =================== Rainhas ===============================
    // A rainha combina os movimentos de Torre e Bispo.
    // Basta unir (OR) os ataques de ambos a partir da mesma casa.
    
    // Rainhas Brancas
    uint64_t wQueens = whiteQueens;
    while (wQueens) {
        int sq = __builtin_ctzll(wQueens);
        whiteAttacks |= (bishopAttacks(sq, occ) | rookAttacks(sq, occ));
        wQueens &= wQueens - 1;
    }

    // Rainhas Pretas
    uint64_t bQueens = blackQueens;
    while (bQueens) {
        int sq = __builtin_ctzll(bQueens);
        blackAttacks |= (bishopAttacks(sq, occ) | rookAttacks(sq, occ));
        bQueens &= bQueens - 1;
    }

    // =================== Reis ==================================
    // O rei se move uma casa em qualquer direção. Usamos tabela pré-calculada.
    // Como só existe 1 rei (geralmente), verificamos se o bitboard não é zero.
    
    if (whiteKing) {
        whiteAttacks |= KING_ATTACKS[__builtin_ctzll(whiteKing)];
    }

    if (blackKing) {
        blackAttacks |= KING_ATTACKS[__builtin_ctzll(blackKing)];
    }
}
