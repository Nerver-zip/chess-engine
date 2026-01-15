#pragma once
#include <cstdint>
#include <string>

#define OFFSET 10000

enum MoveFlags : uint8_t {
    QUIET      = 0,
    CAPTURE    = 1 << 0,
    DOUBLE_PAWN_PUSH = 1 << 1,
    KING_CASTLE = 1 << 2,
    QUEEN_CASTLE = 1 << 3,
    EN_PASSANT = 1 << 4,
    PROMOTION  = 1 << 5
};

struct Move {
    uint8_t from;        // 0..63
    uint8_t to;          // 0..63
    uint8_t promotion;  // Piece (só usado se PROMOTION)
    uint8_t flags;      // MoveFlags
    int score;          // Pontuação MVV-LVA
    
    // Ignora flags e score
    bool operator==(const Move& other) const {
        return from == other.from && 
               to == other.to && 
               promotion == other.promotion;
    }

    bool operator!=(const Move& other) const {
        return !(*this == other);
    }

};

static constexpr int MVV_LVA_VALUES[13] = {
    0,      // EMPTY
    100,    // WPAWN
    320,    // WKNIGHT
    330,    // WBISHOP
    500,    // WROOK
    900,    // WQUEEN
    20000,  // WKING
    100,    // BPAWN
    320,    // BKNIGHT
    330,    // BBISHOP
    500,    // BROOK
    900,    // BQUEEN
    20000   // BKING
};

using PackedMove = uint16_t;

// Compacta: Move (8 bytes) -> PackedMove (2 bytes)
inline PackedMove packMove(const Move& m) {
    // Pegamos os 6 bits de from, 6 bits de to, e 4 bits da peça de promoção
    // O '& 0x3F' garante que só pegamos 6 bits (0-63)
    // O '& 0xF' garante que só pegamos 4 bits (0-15)
    return (m.from & 0x3F) | 
           ((m.to & 0x3F) << 6) | 
           ((m.promotion & 0xF) << 12);
}

// Descompacta: PackedMove (2 bytes) -> Move (8 bytes)
// O lance descompactado é "cru". Ele perde as flags (Capture, Castle, etc).
// Ele serve apenas para comparação (Matching).
inline Move unpackMove(PackedMove pm) {
    Move m;
    m.from = pm & 0x3F;
    m.to   = (pm >> 6) & 0x3F;
    m.promotion = (pm >> 12) & 0xF;
    
    // Recuperação parcial de flags (apenas Promotion dá pra saber com certeza)
    m.flags = (m.promotion != 0) ? PROMOTION : QUIET; 
    
    // Zera o resto (não sabemos se é captura ou castle só olhando os bits)
    m.score = 0; 
    return m;
}

struct Board;

// Converte para notação simples (ex: "e2e4", "a7a8q")
std::string moveToUCI(const Move& m);

// Converte para notação Algébrica Padrão
std::string moveToSAN(const Move& m, const Board& board);
