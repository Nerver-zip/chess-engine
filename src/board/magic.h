#pragma once
#include <cstdint>

/**
 * @file magic.h
 * @brief Constantes usadas para Magic Bitboards de bispos.
 *
 * Magic Bitboards é uma técnica para calcular ataques de peças deslizantes
 * (bispos e torres) em tempo O(1), usando multiplicação por constantes mágicas
 * e tabelas pré-computadas.
 *
 * A fórmula geral usada é:
 *
 *   index = ((occupancy & mask) * magic) >> shift
 *
 * Onde:
 *  - mask  : casas relevantes que podem bloquear o bispo naquela casa
 *  - magic : constante mágica escolhida para evitar colisões
 *  - shift : número de bits descartados após a multiplicação
 *
 * O index resultante aponta diretamente para uma tabela de ataques válida
 * para aquela configuração de bloqueios.
 *
 * Este arquivo contém apenas as constantes mágicas e shifts.
 */

/**
 * @brief Constantes mágicas para bispos.
 *
 * Cada entrada corresponde a uma casa (0..63).
 * Multiplicando a ocupação mascarada por esse valor e aplicando o shift
 * apropriado, obtemos um índice perfeito para a tabela de ataques.
 */
constexpr uint64_t BISHOP_MAGICS[64] = {
  0x40040844404084ULL,0x2004208a004208ULL,0x10190041080202ULL,0x108060845042010ULL,
  0x581104180800210ULL,0x2112080446200010ULL,0x1080820820060210ULL,0x3c0808410220200ULL,
  0x4050404440404ULL,0x21001420088ULL,0x24d0080801082102ULL,0x1020a0a020400ULL,
  0x40308200402ULL,0x4011002100800ULL,0x401484104104005ULL,0x801010402020200ULL,
  0x400210c3880100ULL,0x404022024108200ULL,0x810018200204102ULL,0x4002801a02003ULL,
  0x85040820080400ULL,0x810102c808880400ULL,0xe900410884800ULL,0x8002020480840102ULL,
  0x220200865090201ULL,0x2010100a02021202ULL,0x152048408022401ULL,0x20080002081110ULL,
  0x4001001021004000ULL,0x800040400a011002ULL,0xe4004081011002ULL,0x1c004001012080ULL,
  0x8004200962a00220ULL,0x8422100208500202ULL,0x2000402200300c08ULL,0x8646020080080080ULL,
  0x80020a0200100808ULL,0x2010004880111000ULL,0x623000a080011400ULL,0x42008c0340209202ULL,
  0x209188240001000ULL,0x400408a884001800ULL,0x110400a6080400ULL,0x1840060a44020800ULL,
  0x90080104000041ULL,0x201011000808101ULL,0x1a2208080504f080ULL,0x8012020600211212ULL,
  0x500861011240000ULL,0x180806108200800ULL,0x4000020e01040044ULL,0x300000261044000aULL,
  0x802241102020002ULL,0x20906061210001ULL,0x5a84841004010310ULL,0x4010801011c04ULL,
  0xa010109502200ULL,0x4a02012000ULL,0x500201010098b028ULL,0x8040002811040900ULL,
  0x28000010020204ULL,0x6000020202d0240ULL,0x8918844842082200ULL,0x4010011029020020ULL
};

/**
 * @brief Shifts usados após a multiplicação mágica.
 *
 * Define quantos bits mais significativos devem ser descartados
 * após a multiplicação. O valor é 64 - (#bits relevantes do mask).
 *
 * Cada casa tem um número diferente de casas relevantes, logo um shift próprio.
 */
constexpr int BISHOP_SHIFTS[64] = {
  58,59,59,59,59,59,59,58,
  59,59,59,59,59,59,59,59,
  59,59,57,57,57,57,59,59,
  59,59,57,55,55,57,59,59,
  59,59,57,55,55,57,59,59,
  59,59,57,57,57,57,59,59,
  59,59,59,59,59,59,59,59,
  58,59,59,59,59,59,59,58
};
