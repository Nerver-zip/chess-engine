#pragma once
#include <string>
#include "../board/board.h"
#include <vector>

#ifdef DEBUG

#include <iostream>
#include <chrono>

namespace Debug {
    
    // ============ STREAM ==============
    inline std::ostream& cout = std::cout;
    

    // ============== PRINT INFO API ================================
    /**
     * @brief Imprime um único bitboard (0s e 1s) formatado como tabuleiro 8x8.
     * Útil para ver máscaras de ataque, pathfinding, etc.
     */
    void printBitboard(uint64_t bb, const std::string& title = "Bitboard");

    /**
     * @brief Imprime o estado completo do jogo (Peças, Lado a jogar, Roque, EnPassant).
     * Usa caracteres ASCII (P, n, k...) e cores ANSI se o terminal suportar.
     */
    void printBoard(const Board& board);

    /**
     * @brief Atalho para imprimir os dois mapas de ataque (White Attacks vs Black Attacks).
     */
    void printAttackMaps(const Board& board);

    void printMove(const Move& m);  
    
    // ================== TIMERS =========================
    struct RAII_Timer {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;

        RAII_Timer(const std::string& n) : name(n), start(std::chrono::high_resolution_clock::now()) {}
        ~RAII_Timer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto us  = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            Debug::cout << "[TIMER] " << name << ": " << us << " us\n";
        }
    };
    
    struct Stopwatch {
        std::chrono::high_resolution_clock::time_point start;

        Stopwatch() { 
            start = std::chrono::high_resolution_clock::now(); 
        }

        uint64_t elapsed_us() const {
            auto end = std::chrono::high_resolution_clock::now();
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            return (us > 0) ? us : 1; 
        }
    };

    // ============== HELPER ==================
    // Converte índice 0-63 para "a1", "h8"
    std::string sqToStr(int sq);

    // Converte Flags e Peças para texto debugável
    std::string moveDebugString(const Move& m);

    // ============== PRINT COLLECTIONS ================================
    
    /**
     * @brief Imprime um vetor de movimentos 
     */
    void printMoveList(const std::vector<Move>& moves, const std::string& title = "Move List");

    /**
     * @brief Imprime a tabela de Killer Moves.
     * Mostra apenas plies que tenham movimentos definidos.
     * @param killerTable O array C-style [MAX_PLY][2]
     * @param maxPly Até onde varrer (Max: 64)
     */
    void printKillerTable(const Move killerTable[][2], int maxPly);
}

#else

namespace Debug {
    inline void printBitboard(uint64_t, const std::string& = "") {}
    inline void printBoard(const Board&) {}
    inline void printAttackMaps(const Board&) {}
    inline void printMove(const Move&) {}
    inline void printMoveList(const std::vector<Move>&, const std::string& = "") {}
    inline void printKillerTable(const Move[][2], int) {}    

    struct NullStream {
        template<typename T>
        constexpr const NullStream& operator<<(const T&) const { return *this; }

        constexpr const NullStream& operator<<(std::ostream&(*)(std::ostream&)) const {
            return *this;
        }
    };

    inline constexpr NullStream cout{};
    
    struct RAII_Timer {
        constexpr RAII_Timer(const std::string&) {}
    };

    struct Stopwatch {
        constexpr Stopwatch() noexcept {}

        constexpr uint64_t elapsed_us() const noexcept {
            return 1;
        }
    };
}

#endif 
