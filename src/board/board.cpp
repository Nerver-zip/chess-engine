#include <cctype>
#include "board.h"
#include "attack.h"
#include "piece.h"
#include <iostream>

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
        
    b.computeHash();
    
    return b;
}


Board Board::applyMove(const Move& m) const {
    Board b = *this;
    
    // ==========================================================
    // ETAPA 0: Identificação (Quem está envolvido?)
    // ==========================================================
    
    // Quem está movendo?
    int pFrom = pieceAt(m.from); 

    // Quem foi capturado? (Se for captura)
    int pCaptured = EMPTY;
    int captureSq = m.to;
 
    if (m.flags & CAPTURE) {
        if (m.flags & EN_PASSANT) {
            captureSq = whiteToMove ? (m.to - 8) : (m.to + 8);
            pCaptured = whiteToMove ? BPAWN : WPAWN;
        } else { 
            pCaptured = this->pieceAt(m.to);
        }
    }

    // =======================================================
    // Atualização de Hash das PEÇAS (Tira Velha -> Põe Nova)
    // =======================================================

    // Remove a peça que está saindo da origem
    b.hashKey ^= Zobrist::pieces[pFrom][m.from];

    // Remove a peça capturada (se houver)
    if (pCaptured != EMPTY) {
        b.hashKey ^= Zobrist::pieces[pCaptured][captureSq];
    }

    // Adiciona a peça no destino
    int pTo = pFrom; 
    if (m.flags & PROMOTION) {
        pTo = m.promotion;
    }
    b.hashKey ^= Zobrist::pieces[pTo][m.to];

    // Bitboards de origem e destino
    uint64_t fromBB = 1ULL << m.from;
    uint64_t toBB   = 1ULL << m.to;

    // 1. Limpa o destino (Capturas)
    // Remove qualquer peça inimiga que esteja na casa de destino
    if (whiteToMove) {
        b.blackPawns   &= ~toBB; b.blackKnights &= ~toBB; b.blackBishops &= ~toBB;
        b.blackRooks   &= ~toBB; b.blackQueens  &= ~toBB; b.blackKing    &= ~toBB;
    } else {
        b.whitePawns   &= ~toBB; b.whiteKnights &= ~toBB; b.whiteBishops &= ~toBB;
        b.whiteRooks   &= ~toBB; b.whiteQueens  &= ~toBB; b.whiteKing    &= ~toBB;
    }

    // 2. Move a peça (Tratando Promoção)
    if (m.flags & PROMOTION) {
        // Se for promoção, remove o peão da origem e coloca a NOVA peça no destino
        if (whiteToMove) {
            b.whitePawns &= ~fromBB; // Remove peão
            switch (m.promotion) {
                case WQUEEN:  b.whiteQueens  |= toBB; break;
                case WROOK:   b.whiteRooks   |= toBB; break;
                case WBISHOP: b.whiteBishops |= toBB; break;
                case WKNIGHT: b.whiteKnights |= toBB; break;
            }
        } else {
            b.blackPawns &= ~fromBB; // Remove peão
            switch (m.promotion) {
                case BQUEEN:  b.blackQueens  |= toBB; break;
                case BROOK:   b.blackRooks   |= toBB; break;
                case BBISHOP: b.blackBishops |= toBB; break;
                case BKNIGHT: b.blackKnights |= toBB; break;
            }
        }
    } else {
        // Movimento Normal (não promoção)
        // Helper lambda para mover o bit no bitboard correto
        auto moveBit = [&](uint64_t& bb) {
            if (bb & fromBB) {
                bb &= ~fromBB;
                bb |= toBB;
            }
        };

        if (whiteToMove) {
            moveBit(b.whitePawns); moveBit(b.whiteKnights); moveBit(b.whiteBishops);
            moveBit(b.whiteRooks); moveBit(b.whiteQueens);  moveBit(b.whiteKing);
        } else {
            moveBit(b.blackPawns); moveBit(b.blackKnights); moveBit(b.blackBishops);
            moveBit(b.blackRooks); moveBit(b.blackQueens);  moveBit(b.blackKing);
        }
    }

    // 3. Tratamentos Especiais (Roque e En Passant)
    
    // En Passant (A captura não ocorre em 'to', mas na casa do peão inimigo)
    if (m.flags & EN_PASSANT) {
        uint64_t captureBB = 1ULL << captureSq;
        
        if (whiteToMove) b.blackPawns &= ~captureBB;
        else             b.whitePawns &= ~captureBB;
    }

    // Roque (Mover a torre correspondente)
    if (m.flags & KING_CASTLE) {
        if (whiteToMove) { // h1 -> f1
            b.whiteRooks &= ~(1ULL << 7); b.whiteRooks |= (1ULL << 5);
            b.hashKey ^= Zobrist::pieces[WROOK][7];
            b.hashKey ^= Zobrist::pieces[WROOK][5];
        } else {           // h8 -> f8
            b.hashKey ^= Zobrist::pieces[BROOK][63];
            b.hashKey ^= Zobrist::pieces[BROOK][61];
            b.blackRooks &= ~(1ULL << 63); b.blackRooks |= (1ULL << 61);
        }
    }
    if (m.flags & QUEEN_CASTLE) {
        if (whiteToMove) { // a1 -> d1
            b.whiteRooks &= ~(1ULL << 0); b.whiteRooks |= (1ULL << 3);
            b.hashKey ^= Zobrist::pieces[WROOK][0];
            b.hashKey ^= Zobrist::pieces[WROOK][3];
        } else {           // a8 -> d8
            b.hashKey ^= Zobrist::pieces[BROOK][56];
            b.hashKey ^= Zobrist::pieces[BROOK][59];
            b.blackRooks &= ~(1ULL << 56); b.blackRooks |= (1ULL << 59);
        }
    }

    // 4. Atualizar Estado do Jogo

    // Atualizar hash dos estados (remoção)
    if (b.enPassantSquare != -1) {
        b.hashKey ^= Zobrist::enPassant[b.enPassantSquare % 8];
    }

    b.hashKey ^= Zobrist::castling[b.castlingRights];
    
    // Atualiza En Passant Square
    b.enPassantSquare = -1; // Reset padrão
    if (m.flags & DOUBLE_PAWN_PUSH) {
        // A casa de en passant é a casa que foi "pulada"
        b.enPassantSquare = whiteToMove ? (m.from + 8) : (m.from - 8);
    }

    // Atualiza Direitos de Roque (Castling Rights)
    // Se rei ou torre moverem, ou torre for capturada, perde o direito.
    // Máscara simples: Se mexeu em a1, perde WQ; e1, perde ambos W; h1, perde WK...
    // castlingRights: 1=WK, 2=WQ, 4=BK, 8=BQ
    
    auto updateCastling = [&](int sq) {
        if (sq == 0)  b.castlingRights &= ~2; // a1 (Torre WQ)
        if (sq == 4)  b.castlingRights &= ~3; // e1 (Rei W)
        if (sq == 7)  b.castlingRights &= ~1; // h1 (Torre WK)
        if (sq == 56) b.castlingRights &= ~8; // a8 (Torre BQ)
        if (sq == 60) b.castlingRights &= ~12;// e8 (Rei B)
        if (sq == 63) b.castlingRights &= ~4; // h8 (Torre BK)
    };
    
    updateCastling(m.from);
    updateCastling(m.to);
    
    // Atualizar hash dos estados (adição)
    if (b.enPassantSquare != -1) {
        b.hashKey ^= Zobrist::enPassant[b.enPassantSquare % 8];
    }
    b.hashKey ^= Zobrist::castling[b.castlingRights];

    // Inverte o turno
    b.whiteToMove = !whiteToMove;
    b.hashKey ^= Zobrist::sideToMove;
    
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
    // Ataque a direita são 9 casas a mais. Preto é o contrário. 

    // --- Peões Brancos ---
    whiteAttacks |= ((whitePawns & ~FILE_A) << 7);
    whiteAttacks |= ((whitePawns & ~FILE_H) << 9);
    

    // --- Peões Pretos ---
    blackAttacks |= ((blackPawns & ~FILE_A) >> 9);
    blackAttacks |= ((blackPawns & ~FILE_H) >> 7);
    
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

uint64_t Board::attackersTo(int sq, uint64_t occupied) const {
    uint64_t attackers = 0;

    // Peões
    // Quem ataca 'sq' como peão branco? As casas de onde um peão PRETO atacaria.
    // Usamos sua tabela já existente PAWN_ATTACKS:
    // PAWN_ATTACKS[1] são ataques de peões pretos. A intersecção disso com peões brancos = peões brancos atacando sq.
    attackers |= (PAWN_ATTACKS[1][sq] & whitePawns);
    attackers |= (PAWN_ATTACKS[0][sq] & blackPawns);

    // Cavalos
    attackers |= (KNIGHT_ATTACKS[sq] & (whiteKnights | blackKnights));

    // Bispos e Rainhas (Diagonal)
    uint64_t diagAttacks = bishopAttacks(sq, occupied);
    attackers |= (diagAttacks & (whiteBishops | blackBishops | whiteQueens | blackQueens));

    // Torres e Rainhas (Ortogonal)
    uint64_t orthoAttacks = rookAttacks(sq, occupied);
    attackers |= (orthoAttacks & (whiteRooks | blackRooks | whiteQueens | blackQueens));

    // Reis
    attackers |= (KING_ATTACKS[sq] & (whiteKing | blackKing));

    return attackers;
}

void Board::computeHash() {
    hashKey = 0;

    uint64_t bb;
    
    bb = whitePawns;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[WPAWN][sq]; bb &= bb - 1; }
    
    bb = whiteKnights;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[WKNIGHT][sq]; bb &= bb - 1; }
    
    bb = whiteBishops;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[WBISHOP][sq]; bb &= bb - 1; }
    
    bb = whiteRooks;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[WROOK][sq]; bb &= bb - 1; }
    
    bb = whiteQueens;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[WQUEEN][sq]; bb &= bb - 1; }
    
    bb = whiteKing;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[WKING][sq]; bb &= bb - 1; }

    bb = blackPawns;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[BPAWN][sq]; bb &= bb - 1; }
    
    bb = blackKnights;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[BKNIGHT][sq]; bb &= bb - 1; }
    
    bb = blackBishops;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[BBISHOP][sq]; bb &= bb - 1; }
    
    bb = blackRooks;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[BROOK][sq]; bb &= bb - 1; }
    
    bb = blackQueens;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[BQUEEN][sq]; bb &= bb - 1; }
    
    bb = blackKing;
    while (bb) { int sq = __builtin_ctzll(bb); hashKey ^= Zobrist::pieces[BKING][sq]; bb &= bb - 1; }

    hashKey ^= Zobrist::castling[castlingRights];

    if (enPassantSquare != -1) {
        // Usa o índice da coluna (0-7)
        hashKey ^= Zobrist::enPassant[enPassantSquare % 8];
    }

    if (!whiteToMove) {
        hashKey ^= Zobrist::sideToMove;
    }   
}
