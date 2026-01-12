#include "tt.h"
#include <cstring>
#include <iostream>

TranspositionTable TT;

// ========================================================
// Lógica de Normalização de Mate
// ========================================================
/*
    Problema: A busca retorna "Mate em 2" (Score 29998) no Ply 10.
    Se guardarmos 29998 e lermos no Ply 20, a engine achará que é "Mate em 2"
    a partir do Ply 20 (falso, o mate já passou ou está mais longe).
    
    Solução: Guardamos "Mate no Ply X do Tabuleiro"
*/

int TranspositionTable::scoreToTT(int score, int ply) {
    if (score > MATE_IN_MAXPLY)  return score + ply;
    if (score < -MATE_IN_MAXPLY) return score - ply;
    return score;
}

int TranspositionTable::scoreFromTT(int score, int ply) {
    if (score > MATE_IN_MAXPLY)  return score - ply;
    if (score < -MATE_IN_MAXPLY) return score + ply;
    return score;
}

void TranspositionTable::resize(int mbSize) {
    uint64_t sizeBytes = (uint64_t)mbSize * 1024 * 1024;
    uint64_t clusterCount = sizeBytes / sizeof(TTCluster);

    // POTÊNCIA DE 2 (Fast Indexing)
    // Precisamos que numClusters seja potência de 2 para usar '& mask' em vez de '%'
    // Vamos arredondar para baixo para não estourar a memória
    // Ex: Se cabe 1000, usamos 512. Se cabe 2000, usamos 1024
    
    // Achar a maior potência de 2 <= clusterCount
    uint64_t pow2 = 1;
    while (pow2 << 1 <= clusterCount) {
        pow2 = pow2 << 1;
    }
    numClusters = pow2;

    table.clear();
    table.resize(numClusters);
    table.shrink_to_fit();
    clear();

    std::cout << "TT: Resized to " << mbSize << "MB -> " 
              << numClusters << " clusters (Power of 2). Mask: " 
              << std::hex << (numClusters - 1) << std::dec << std::endl;
}

void TranspositionTable::clear() {
    if (numClusters == 0) return;
    std::fill(table.begin(), table.end(), TTCluster{});
    generation = 0;
}

bool TranspositionTable::probe(uint64_t key, TTEntry& entry, int ply) {
    if (numClusters == 0) return false;

    // FAST INDEXING (Usando bitwise AND)
    // Funciona porque numClusters é potência de 2.
    // Ex: Se numClusters = 16 (10000), a mask é 15 (01111).
    uint64_t index = key & (numClusters - 1);
    const TTCluster& cluster = table[index];

    // Varre bucket
    for (int i = 0; i < 4; i++) {
        if (cluster.entry[i].key == key) {
            entry = cluster.entry[i];
            
            // Recupera score relativo ao ply atual
            entry.score = (int16_t)scoreFromTT(entry.score, ply);
            return true;
        }
    }
    return false;
}

void TranspositionTable::store(uint64_t key, int depth, int score, int flag, Move bestMove, int ply) {
    if (numClusters == 0) return;

    // Normaliza score para absoluto antes de guardar
    int ttScore = scoreToTT(score, ply);

    uint64_t index = key & (numClusters - 1);
    TTCluster& cluster = table[index];

    int targetIdx = -1;
    int replaceScore = -9999; // Pontuação para decidir quem morre

    /*
       POLÍTICA DE SUBSTITUIÇÃO (AGING + DEPTH)
       Queremos substituir:
       1. Entradas de gerações antigas (Generation diferente).
       2. Entradas com depth menor.
    */

    for (int i = 0; i < 4; i++) {
        // PRIORIDADE MÁXIMA: Mesma chave (Update)
        if (cluster.entry[i].key == key) {
            targetIdx = i;
            // Proteção simples contra overwrite fraco:
            // Só sobrescreve se novo depth for maior OU se a hash for antiga
            // OU se o score for EXATO (vale ouro).
            // Mas para simplificar e não bugar, a convenção moderna é:
            // "Se é a mesma posição, atualize, a menos que o depth novo seja MUITO lixo."
            // Para simplificar vamos atualizar sempre por enquanto para garantir que o Aging funcione.
            break; 
        }

        // CALCULA O "SCORE DE SUBSTITUIÇÃO"
        // Quanto maior esse valor, melhor candidato para ser deletado.
        // Fórmula heurística:
        // - Se generation for antiga: +1000 pontos (tirar ela)
        // - Se depth for baixo: + (100 - depth) pontos (tirar os superficiais)
        
        int entryScore = 0;
        
        // AGING: Verifica geração
        // Se a geração da entrada for diferente da atual, ela é velha
        if (cluster.entry[i].generation != generation) {
            entryScore += 1000; 
        }

        // Inverso do Depth (Menor depth = Maior score de substituição)
        // Somamos um offset para evitar negativo
        entryScore += (255 - cluster.entry[i].depth);

        if (entryScore > replaceScore) {
            replaceScore = entryScore;
            targetIdx = i;
        }
    }

    // Grava
    TTEntry& e = cluster.entry[targetIdx];
    e.key = key;
    e.move = packMove(bestMove);
    e.score = (int16_t)ttScore;
    e.depth = (int8_t)depth;
    e.flag = (uint8_t)flag;
    e.generation = generation; // Carimba com a geração atual
}

int TranspositionTable::hashfull() const {
    if (numClusters == 0) return 0;
    int samples = 0;
    int occupied = 0;
    uint64_t limit = std::min<uint64_t>(1000, numClusters);
    for (uint64_t i = 0; i < limit; i++) {
        for (int j = 0; j < 4; j++) {
            if (table[i].entry[j].key != 0) occupied++;
            samples++;
        }
    }
    return (occupied * 1000) / samples;
}
