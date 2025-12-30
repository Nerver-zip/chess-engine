#pragma once
#include <cstdint>
#include <array>
#include "bitboard.h"
#include "magic.h"

/**
 * @file attack.h
 * @brief Tabelas de ataque pré-computadas para peças não-deslizantes (pawn, knight, king).
 *
 * Este arquivo contém lookup tables constexpr que codificam, para cada casa do tabuleiro,
 * o conjunto de casas atacadas por cada tipo de peça que possui padrão de movimento fixo.
 *
 * Essas tabelas permitem gerar ataques em tempo O(1), usando apenas operações bitwise,
 * eliminando a necessidade de loops, branches e cálculos geométricos em tempo de execução.
 *
 * Essas tabelas são usadas em:
 *  - Detecção de xeque
 *  - Geração de lances legais
 *  - Avaliação posicional (controle de casas)
 *  - Verificação de segurança do rei
 */

/* ============================================================
                           PEÃO
   ============================================================ */

constexpr uint64_t pawnAttackWhiteFor(int sq) {
    uint64_t b = BB(sq);
    uint64_t attacks = 0;

    if ((b << 7) & ~0x0101010101010101ULL) attacks |= (b << 7);
    if ((b << 9) & ~0x8080808080808080ULL) attacks |= (b << 9);

    return attacks;
}

constexpr uint64_t pawnAttackBlackFor(int sq) {
    uint64_t b = BB(sq);
    uint64_t attacks = 0;

    if ((b >> 9) & ~0x0101010101010101ULL) attacks |= (b >> 9);
    if ((b >> 7) & ~0x8080808080808080ULL) attacks |= (b >> 7);

    return attacks;
}

constexpr uint64_t PAWN_ATTACKS[2][64] = {
{
    pawnAttackWhiteFor(0), pawnAttackWhiteFor(1), pawnAttackWhiteFor(2), pawnAttackWhiteFor(3),
    pawnAttackWhiteFor(4), pawnAttackWhiteFor(5), pawnAttackWhiteFor(6), pawnAttackWhiteFor(7),
    pawnAttackWhiteFor(8), pawnAttackWhiteFor(9), pawnAttackWhiteFor(10), pawnAttackWhiteFor(11),
    pawnAttackWhiteFor(12), pawnAttackWhiteFor(13), pawnAttackWhiteFor(14), pawnAttackWhiteFor(15),
    pawnAttackWhiteFor(16), pawnAttackWhiteFor(17), pawnAttackWhiteFor(18), pawnAttackWhiteFor(19),
    pawnAttackWhiteFor(20), pawnAttackWhiteFor(21), pawnAttackWhiteFor(22), pawnAttackWhiteFor(23),
    pawnAttackWhiteFor(24), pawnAttackWhiteFor(25), pawnAttackWhiteFor(26), pawnAttackWhiteFor(27),
    pawnAttackWhiteFor(28), pawnAttackWhiteFor(29), pawnAttackWhiteFor(30), pawnAttackWhiteFor(31),
    pawnAttackWhiteFor(32), pawnAttackWhiteFor(33), pawnAttackWhiteFor(34), pawnAttackWhiteFor(35),
    pawnAttackWhiteFor(36), pawnAttackWhiteFor(37), pawnAttackWhiteFor(38), pawnAttackWhiteFor(39),
    pawnAttackWhiteFor(40), pawnAttackWhiteFor(41), pawnAttackWhiteFor(42), pawnAttackWhiteFor(43),
    pawnAttackWhiteFor(44), pawnAttackWhiteFor(45), pawnAttackWhiteFor(46), pawnAttackWhiteFor(47),
    pawnAttackWhiteFor(48), pawnAttackWhiteFor(49), pawnAttackWhiteFor(50), pawnAttackWhiteFor(51),
    pawnAttackWhiteFor(52), pawnAttackWhiteFor(53), pawnAttackWhiteFor(54), pawnAttackWhiteFor(55),
    pawnAttackWhiteFor(56), pawnAttackWhiteFor(57), pawnAttackWhiteFor(58), pawnAttackWhiteFor(59),
    pawnAttackWhiteFor(60), pawnAttackWhiteFor(61), pawnAttackWhiteFor(62), pawnAttackWhiteFor(63)
},
{
    pawnAttackBlackFor(0), pawnAttackBlackFor(1), pawnAttackBlackFor(2), pawnAttackBlackFor(3),
    pawnAttackBlackFor(4), pawnAttackBlackFor(5), pawnAttackBlackFor(6), pawnAttackBlackFor(7),
    pawnAttackBlackFor(8), pawnAttackBlackFor(9), pawnAttackBlackFor(10), pawnAttackBlackFor(11),
    pawnAttackBlackFor(12), pawnAttackBlackFor(13), pawnAttackBlackFor(14), pawnAttackBlackFor(15),
    pawnAttackBlackFor(16), pawnAttackBlackFor(17), pawnAttackBlackFor(18), pawnAttackBlackFor(19),
    pawnAttackBlackFor(20), pawnAttackBlackFor(21), pawnAttackBlackFor(22), pawnAttackBlackFor(23),
    pawnAttackBlackFor(24), pawnAttackBlackFor(25), pawnAttackBlackFor(26), pawnAttackBlackFor(27),
    pawnAttackBlackFor(28), pawnAttackBlackFor(29), pawnAttackBlackFor(30), pawnAttackBlackFor(31),
    pawnAttackBlackFor(32), pawnAttackBlackFor(33), pawnAttackBlackFor(34), pawnAttackBlackFor(35),
    pawnAttackBlackFor(36), pawnAttackBlackFor(37), pawnAttackBlackFor(38), pawnAttackBlackFor(39),
    pawnAttackBlackFor(40), pawnAttackBlackFor(41), pawnAttackBlackFor(42), pawnAttackBlackFor(43),
    pawnAttackBlackFor(44), pawnAttackBlackFor(45), pawnAttackBlackFor(46), pawnAttackBlackFor(47),
    pawnAttackBlackFor(48), pawnAttackBlackFor(49), pawnAttackBlackFor(50), pawnAttackBlackFor(51),
    pawnAttackBlackFor(52), pawnAttackBlackFor(53), pawnAttackBlackFor(54), pawnAttackBlackFor(55),
    pawnAttackBlackFor(56), pawnAttackBlackFor(57), pawnAttackBlackFor(58), pawnAttackBlackFor(59),
    pawnAttackBlackFor(60), pawnAttackBlackFor(61), pawnAttackBlackFor(62), pawnAttackBlackFor(63)
}
};

/* ============================================================
                           CAVALO
   ============================================================ */

constexpr uint64_t knightAttackFor(int sq) {
    int r = sq / 8;
    int f = sq % 8;

    uint64_t attacks = 0;

    constexpr int dr[8] = { 2,2,-2,-2, 1,1,-1,-1 };
    constexpr int df[8] = { 1,-1,1,-1, 2,-2,2,-2 };

    for (int i = 0; i < 8; ++i) {
        int rr = r + dr[i];
        int ff = f + df[i];
        if (onBoard(rr, ff))
            attacks |= BB(rr * 8 + ff);
    }

    return attacks;
}

constexpr uint64_t KNIGHT_ATTACKS[64] = {
    knightAttackFor(0),  knightAttackFor(1),  knightAttackFor(2),  knightAttackFor(3),
    knightAttackFor(4),  knightAttackFor(5),  knightAttackFor(6),  knightAttackFor(7),
    knightAttackFor(8),  knightAttackFor(9),  knightAttackFor(10), knightAttackFor(11),
    knightAttackFor(12), knightAttackFor(13), knightAttackFor(14), knightAttackFor(15),
    knightAttackFor(16), knightAttackFor(17), knightAttackFor(18), knightAttackFor(19),
    knightAttackFor(20), knightAttackFor(21), knightAttackFor(22), knightAttackFor(23),
    knightAttackFor(24), knightAttackFor(25), knightAttackFor(26), knightAttackFor(27),
    knightAttackFor(28), knightAttackFor(29), knightAttackFor(30), knightAttackFor(31),
    knightAttackFor(32), knightAttackFor(33), knightAttackFor(34), knightAttackFor(35),
    knightAttackFor(36), knightAttackFor(37), knightAttackFor(38), knightAttackFor(39),
    knightAttackFor(40), knightAttackFor(41), knightAttackFor(42), knightAttackFor(43),
    knightAttackFor(44), knightAttackFor(45), knightAttackFor(46), knightAttackFor(47),
    knightAttackFor(48), knightAttackFor(49), knightAttackFor(50), knightAttackFor(51),
    knightAttackFor(52), knightAttackFor(53), knightAttackFor(54), knightAttackFor(55),
    knightAttackFor(56), knightAttackFor(57), knightAttackFor(58), knightAttackFor(59),
    knightAttackFor(60), knightAttackFor(61), knightAttackFor(62), knightAttackFor(63)
};

/* ============================================================
                           REI
   ============================================================ */

constexpr uint64_t kingAttackFor(int sq) {
    int r = sq / 8;
    int f = sq % 8;

    uint64_t attacks = 0;

    for (int dr = -1; dr <= 1; ++dr)
        for (int df = -1; df <= 1; ++df) {
            if (dr == 0 && df == 0) continue;
            int rr = r + dr;
            int ff = f + df;
            if (onBoard(rr, ff))
                attacks |= BB(rr * 8 + ff);
        }

    return attacks;
}

constexpr uint64_t KING_ATTACKS[64] = {
    kingAttackFor(0),  kingAttackFor(1),  kingAttackFor(2),  kingAttackFor(3),
    kingAttackFor(4),  kingAttackFor(5),  kingAttackFor(6),  kingAttackFor(7),
    kingAttackFor(8),  kingAttackFor(9),  kingAttackFor(10), kingAttackFor(11),
    kingAttackFor(12), kingAttackFor(13), kingAttackFor(14), kingAttackFor(15),
    kingAttackFor(16), kingAttackFor(17), kingAttackFor(18), kingAttackFor(19),
    kingAttackFor(20), kingAttackFor(21), kingAttackFor(22), kingAttackFor(23),
    kingAttackFor(24), kingAttackFor(25), kingAttackFor(26), kingAttackFor(27),
    kingAttackFor(28), kingAttackFor(29), kingAttackFor(30), kingAttackFor(31),
    kingAttackFor(32), kingAttackFor(33), kingAttackFor(34), kingAttackFor(35),
    kingAttackFor(36), kingAttackFor(37), kingAttackFor(38), kingAttackFor(39),
    kingAttackFor(40), kingAttackFor(41), kingAttackFor(42), kingAttackFor(43),
    kingAttackFor(44), kingAttackFor(45), kingAttackFor(46), kingAttackFor(47),
    kingAttackFor(48), kingAttackFor(49), kingAttackFor(50), kingAttackFor(51),
    kingAttackFor(52), kingAttackFor(53), kingAttackFor(54), kingAttackFor(55),
    kingAttackFor(56), kingAttackFor(57), kingAttackFor(58), kingAttackFor(59),
    kingAttackFor(60), kingAttackFor(61), kingAttackFor(62), kingAttackFor(63)
};

// Aqui já entram magic bitboards

/* ============================================================
                           BISPO
   ============================================================ */

constexpr uint64_t bishopMaskFor(int sq) {
    uint64_t mask = 0;
    int r = sq / 8;
    int f = sq % 8;

    // NE
    for (int rr = r + 1, ff = f + 1; rr < 7 && ff < 7; ++rr, ++ff)
        mask |= BB(rr * 8 + ff);
    // NW
    for (int rr = r + 1, ff = f - 1; rr < 7 && ff > 0; ++rr, --ff)
        mask |= BB(rr * 8 + ff);
    // SE
    for (int rr = r - 1, ff = f + 1; rr > 0 && ff < 7; --rr, ++ff)
        mask |= BB(rr * 8 + ff);
    // SW
    for (int rr = r - 1, ff = f - 1; rr > 0 && ff > 0; --rr, --ff)
        mask |= BB(rr * 8 + ff);

    return mask;
}

constexpr uint64_t bishopAttacksFor(int sq, uint64_t blockers) {
    uint64_t attacks = 0;
    int r = sq / 8;
    int f = sq % 8;

    // NE
    for (int rr = r + 1, ff = f + 1; rr < 8 && ff < 8; ++rr, ++ff) {
        int s = rr * 8 + ff;
        attacks |= BB(s);
        if (blockers & BB(s)) break;
    }
    // NW
    for (int rr = r + 1, ff = f - 1; rr < 8 && ff >= 0; ++rr, --ff) {
        int s = rr * 8 + ff;
        attacks |= BB(s);
        if (blockers & BB(s)) break;
    }
    // SE
    for (int rr = r - 1, ff = f + 1; rr >= 0 && ff < 8; --rr, ++ff) {
        int s = rr * 8 + ff;
        attacks |= BB(s);
        if (blockers & BB(s)) break;
    }
    // SW
    for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff) {
        int s = rr * 8 + ff;
        attacks |= BB(s);
        if (blockers & BB(s)) break;
    }

    return attacks;
}

constexpr uint64_t BISHOP_MASKS[64] = {
    bishopMaskFor(0),  bishopMaskFor(1),  bishopMaskFor(2),  bishopMaskFor(3),
    bishopMaskFor(4),  bishopMaskFor(5),  bishopMaskFor(6),  bishopMaskFor(7),
    bishopMaskFor(8),  bishopMaskFor(9),  bishopMaskFor(10), bishopMaskFor(11),
    bishopMaskFor(12), bishopMaskFor(13), bishopMaskFor(14), bishopMaskFor(15),
    bishopMaskFor(16), bishopMaskFor(17), bishopMaskFor(18), bishopMaskFor(19),
    bishopMaskFor(20), bishopMaskFor(21), bishopMaskFor(22), bishopMaskFor(23),
    bishopMaskFor(24), bishopMaskFor(25), bishopMaskFor(26), bishopMaskFor(27),
    bishopMaskFor(28), bishopMaskFor(29), bishopMaskFor(30), bishopMaskFor(31),
    bishopMaskFor(32), bishopMaskFor(33), bishopMaskFor(34), bishopMaskFor(35),
    bishopMaskFor(36), bishopMaskFor(37), bishopMaskFor(38), bishopMaskFor(39),
    bishopMaskFor(40), bishopMaskFor(41), bishopMaskFor(42), bishopMaskFor(43),
    bishopMaskFor(44), bishopMaskFor(45), bishopMaskFor(46), bishopMaskFor(47),
    bishopMaskFor(48), bishopMaskFor(49), bishopMaskFor(50), bishopMaskFor(51),
    bishopMaskFor(52), bishopMaskFor(53), bishopMaskFor(54), bishopMaskFor(55),
    bishopMaskFor(56), bishopMaskFor(57), bishopMaskFor(58), bishopMaskFor(59),
    bishopMaskFor(60), bishopMaskFor(61), bishopMaskFor(62), bishopMaskFor(63)
};


constexpr uint64_t bishopMagicIndex(uint64_t blockers, int sq) {
    return (blockers * BISHOP_MAGICS[sq]) >> BISHOP_SHIFTS[sq];
}


constexpr uint64_t subsetFromIndex(uint64_t mask, int index) {
    uint64_t result = 0;
    int bit = 0;

    for (uint64_t m = mask; m; m &= m - 1) {
        if (index & (1 << bit))
            result |= (m & -m);
        bit++;
    }
    return result;
}

constexpr auto generateBishopAttackTable() {
    std::array<std::array<uint64_t, 512>, 64> table{};

    for (int sq = 0; sq < 64; ++sq) {
        uint64_t mask = BISHOP_MASKS[sq];
        int bits = __builtin_popcountll(mask);

        for (int i = 0; i < (1 << bits); ++i) {
            uint64_t blockers = subsetFromIndex(mask, i);
            uint64_t attack = bishopAttacksFor(sq, blockers);

            uint64_t index = bishopMagicIndex(blockers, sq);
            table[sq][index] = attack;
        }
    }
    return table;
}

constexpr auto BISHOP_ATTACKS = generateBishopAttackTable();

inline uint64_t bishopAttacks(int sq, uint64_t occAll) {
    uint64_t blockers = occAll & BISHOP_MASKS[sq];
    return BISHOP_ATTACKS[sq][bishopMagicIndex(blockers, sq)];
}
