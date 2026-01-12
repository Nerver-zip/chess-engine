#pragma once
#include "../move/move.h"
#include <cstdint>
#include <vector>

// Constantes de mate para normalização
constexpr int MATE_BOUND = 30000;      // Score base de Mate
constexpr int MATE_IN_MAXPLY = 29000;  // Faixa onde consideramos que é mate

// Tipos de Score para saber se é exato ou um limite
enum TTFlag : uint8_t {
    TT_EXACT, // O score é exato (estava entre Alpha e Beta)
    TT_ALPHA, // O score é um limite superior (Upper Bound - falhou low)
    TT_BETA   // O score é um limite inferior (Lower Bound - falhou high/cutoff)
};

// 16 bytes
struct TTEntry {
    uint64_t key;       // [8 bytes] Hash Check
    PackedMove move;    // [2 bytes] Melhor lance compactado
    int16_t score;      // [2 bytes] Avaliação (-32k a +32k)
    int8_t depth;       // [1 byte]  Profundidade da busca
    uint8_t flag;       // [1 byte]  Tipo de score
    uint8_t generation; // [1 byte]  Idade da entrada (0-255)
    uint8_t padding;    // Sobrou 1 byte ainda (total 16 bytes)

    TTEntry() : key(0), move(0), score(0), depth(0), flag(0), padding(0) {}
};

// 64 BYTES - 1 CACHE LINE
struct TTCluster {
    TTEntry entry[4];
};

class TranspositionTable {
public:
    // Redimensiona para a potência de 2 mais próxima (arredondada para baixo)
    void resize(int mbSize);

    // Zera toda a memória
    void clear();

    /**
     * @brief Guarda uma entrada na TT usando estratégia de Cluster
     * * Lógica de Substituição (dentro do cluster de 4):
     * 1. Se encontrar a mesma Key -> Sobrescreve (Update)
     * 2. Se encontrar slot Vazio -> Ocupa
     * 3. Se estiver cheio -> Substitui a entrada com menor depth
     */
    void store(uint64_t key, int depth, int score, int flag, Move bestMove, int ply);

    // Incrementa a geração (chamado a cada novo lance na raiz / nova busca)
    void newSearch() { ++generation; }

    /**
     * @brief Recupera uma entrada
     * Varre as 4 entradas do cluster correspondente
     * Retorna true se encontrar uma chave igual
     */
    bool probe(uint64_t key, TTEntry& entry, int ply);

    // Retorna a ocupação da tabela (em permilagem, 0-1000)
    int hashfull() const;

private:
    std::vector<TTCluster> table;
    uint64_t numClusters = 0;
    uint8_t generation = 0; // wraparound em 255
    
    // Ajuste de Score Mate (Relativo <-> Absoluto)
    int scoreToTT(int score, int ply);
    int scoreFromTT(int score, int ply);
};

extern TranspositionTable TT;
