#pragma once
#include "../board/board.h"

// Pesos das peças em centipawns
constexpr int P_VAL = 100;
constexpr int N_VAL = 320;
constexpr int B_VAL = 330;
constexpr int R_VAL = 500;
constexpr int Q_VAL = 900;
constexpr int K_VAL = 20000;

class Eval {
public:
    /**
     * @brief Avalia a posição estática do tabuleiro.
     * @param board Estado atual.
     * @return Score em centipawns (100 = 1 peão). 
     * Positivo significa vantagem para o lado que tem a vez (White ou Black).
     */
    static int evaluate(const Board& board);
};

// PST - Piece Square Tables -> Uma para cada peça separada em duas fases de jogo (MG E EG)
// ========================= PAWNS ================================
// No meio jogo, avançar centro, nem tanto os peões da borda
// Prefere jogar e4
// No final, avançar o máximo possível, os peões ficam mais fortes conforme avançam
//
constexpr int PST_P_MG[64] = {
   0,   0,   0,   0,   0,   0,   0,   0,
   5,  10,  10, -20, -20,  10,  10,   5,
   5,  -5, -10,   0,   0, -10,  -5,   5,
   0,   0,   0,  20,  21,   0,   0,   0,
   5,   5,  10,  25,  25,  10,   5,   5,
  10,  10,  20,  30,  30,  20,  10,  10,
  50,  50,  50,  50,  50,  50,  50,  50,
   0,   0,   0,   0,   0,   0,   0,   0
};

constexpr int PST_P_EG[64] = {
   0,   0,   0,   0,   0,   0,   0,   0,
  10,  10,  10,  10,  10,  10,  10,  10,
  20,  20,  20,  20,  20,  20,  20,  20,
  30,  30,  30,  30,  30,  30,  30,  30,
  50,  50,  50,  50,  50,  50,  50,  50,
  70,  70,  70,  70,  70,  70,  70,  70,
  90,  90,  90,  90,  90,  90,  90,  90,
   0,   0,   0,   0,   0,   0,   0,   0
};

// ========================= KNIGHTS ================================
// No meio jogo, favorecer cavalos centralizados
// Penalizar muito cavalos nos cantos
// No final ser mais flexível com cavalos avançados
// Principal penalidade sempre sendo o cavalo nas casas vértice, onde sua mobilidade é baixa

constexpr int PST_N_MG[64] = {
 -50,-40,-30,-30,-30,-30,-40,-50,
 -40,-20,  0,  5,  5,  0,-20,-40,
 -30,  5, 10, 15, 15, 10,  5,-30,
 -30,  0, 15, 20, 20, 15,  0,-30,
 -30,  5, 15, 20, 20, 15,  5,-30,
 -30,  0, 10, 15, 15, 10,  0,-30,
 -40,-20,  0,  0,  0,  0,-20,-40,
 -50,-40,-30,-30,-30,-30,-40,-50
};

constexpr int PST_N_EG[64] = {
 -20,-10,  0,  0,  0,  0,-10,-20,
 -10,  5, 10, 15, 15, 10,  5,-10,
   0, 10, 15, 20, 20, 15, 10,  0,
   0, 15, 20, 25, 25, 20, 15,  0,
   0, 15, 20, 25, 25, 20, 15,  0,
   0, 10, 15, 20, 20, 15, 10,  0,
 -10,  5, 10, 15, 15, 10,  5,-10,
 -20,-10,  0,  0,  0,  0,-10,-20
};

// ========================= BISHOPS ================================
// Favorecer casas que aumentam o raio de alcançe dos bispos
// A casa do Fianchetto recebe uma pontuação especial por ser importante
// ofensiva e defensivamente
constexpr int PST_B_MG[64] = {
  -30, -10, -10, -10, -10, -10, -10, -30,
  -10,  15,   0,   0,   0,   0,  20, -10,
  -10,  10,  10,  10,  10,  10,  10, -10,
  -10,   0,  10,  15,  15,  10,   0, -10,
  -10,   5,  15,  20,  20,  15,   5, -10,
  -10,  10,  10,  15,  15,  10,  10, -10,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -30, -10, -10, -10, -10, -10, -10, -30
};

constexpr int PST_B_EG[64] = {
 -10,  -5,  -5,  -5,  -5,  -5,  -5, -10,
  -5,   5,   5,   5,   5,   5,   5,  -5,
  -5,   5,  10,  10,  10,  10,   5,  -5,
  -5,   5,  10,  15,  15,  10,   5,  -5,
  -5,   5,  10,  15,  15,  10,   5,  -5,
  -5,   5,  10,  10,  10,  10,   5,  -5,
  -5,   5,   5,   5,   5,   5,   5,  -5,
 -10,  -5,  -5,  -5,  -5,  -5,  -5, -10
};

// ========================= ROOKS ================================
// No meio jogo preferir torres centralizadas
// Bonificar invasão na sétima
// No final, bonificar mobilidade lateral
constexpr int PST_R_MG[64] = {
    0,   0,   5,  10,  10,   5,   0,   0,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   -5,   0,   0,   0,   0,   0,   0,  -5,
   15,  20,  20,  20,  20,  20,  20,  15,
    0,   0,   5,  10,  10,   5,   0,   0
};

constexpr int PST_R_EG[64] = {
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   0,   5,  10,  10,   5,   0,   0,
   10,  15,  15,  20,  20,  15,  15,  10,
   10,  15,  20,  25,  25,  20,  15,  10,
   10,  15,  20,  25,  25,  20,  15,  10,
   10,  15,  15,  20,  20,  15,  15,  10,
    0,   0,   5,  10,  10,   5,   0,   0,
    0,   0,   5,  10,  10,   5,   0,   0
};

// ========================= QUEENS ================================
// Sem muita bonificação no meio jogo comparado com peças menores
// Isso evita desenvolvimento precoce de dama
// No final, bonificar dama centralizada

constexpr int PST_Q_MG[64] = {
  -20, -10, -10,  -5,  -5, -10, -10, -20,
  -10,   0,   0,   0,   0,   0,   0, -10,
  -10,   0,   5,   5,   5,   5,   0, -10,
   -5,   0,   5,  10,  10,   5,   0,  -5,
    0,   0,   5,  10,  10,   5,   0,  -5,
  -10,   5,   5,   5,   5,   5,   0, -10,
  -10,   0,   5,   0,   0,   0,   0, -10,
  -20, -10, -10,  -5,  -5, -10, -10, -20
};

constexpr int PST_Q_EG[64] = {
 -10,  -5,  -5,  -5,  -5,  -5,  -5, -10,
  -5,   5,   5,   5,   5,   5,   5,  -5,
  -5,   5,  10,  10,  10,  10,   5,  -5,
  -5,   5,  10,  15,  15,  10,   5,  -5,
  -5,   5,  10,  15,  15,  10,   5,  -5,
  -5,   5,  10,  10,  10,  10,   5,  -5,
  -5,   5,   5,   5,   5,   5,   5,  -5,
 -10,  -5,  -5,  -5,  -5,  -5,  -5, -10
};

// ========================= KINGS ================================
// No meio jogo, penalizar rei exposto, encorajando o roque
// No final, virar o rei do Ivanchuk, bonificando rei ativo
// e penalizando rei passivo

constexpr int PST_K_MG[64] = {
  -50, -60, -70, -80, -80, -70, -60, -50,
  -50, -60, -70, -80, -80, -70, -60, -50,
  -40, -50, -60, -70, -70, -60, -50, -40,
  -30, -40, -50, -60, -60, -50, -40, -30,
  -20, -30, -40, -50, -50, -40, -30, -20,
  -10, -20, -30, -40, -40, -30, -20, -10,
   20,  20,   0,   0,   0,   0,  20,  20,
   30,  40,  20,   0,   0,  20,  40,  30
};

constexpr int PST_K_EG[64] = {
  -50, -40, -30, -20, -20, -30, -40, -50,
  -30, -20, -10,   0,   0, -10, -20, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  30,  40,  40,  30, -10, -30,
  -30, -10,  20,  30,  30,  20, -10, -30,
  -30, -30,   0,   0,   0,   0, -30, -30,
  -50, -40, -30, -20, -20, -30, -40, -50
};
