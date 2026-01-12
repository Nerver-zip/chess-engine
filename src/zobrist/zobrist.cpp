#include "zobrist.h"
#include <array>
#include <random>

namespace Zobrist {
    uint64_t pieces[13][64];
    uint64_t castling[16];
    uint64_t enPassant[9];
    uint64_t sideToMove;
    
    constexpr std::array<uint32_t,8> seed_data = {
        0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E,
        0x5A3B9F29, 0xE4C3D7A1, 0x8F1BBCDC, 0xC4D1F5E3
    };

    void init() {
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        std::mt19937_64 gen(seq); 
        std::uniform_int_distribution<uint64_t> dist;

        for (int p = 1; p <= 12; ++p) {
            for (int sq = 0; sq < 64; ++sq) {
                pieces[p][sq] = dist(gen);
            }
        }

        for (int i = 0; i < 16; ++i) {
            castling[i] = dist(gen);
        }

        for (int i = 0; i < 9; ++i) {
            enPassant[i] = dist(gen);
        }

        sideToMove = dist(gen);
    }
}
