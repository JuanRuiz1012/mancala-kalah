#pragma once
#include "board.hpp"

/*
 * Motor Minimax con poda Alfa-Beta.
 *
 * Expone:
 *   - minimax_best_move         : Minimax puro SIN poda (referencia de corrección)
 *   - alphabeta_best_move       : Alfa-Beta secuencial (debe elegir el mismo mov que minimax)
 *   - alphabeta_best_move_parallel : Alfa-Beta paralelo con root parallelism
 *
 * La estrategia de paralelización es root parallelism:
 *   cada hilo evalúa un subárbol independiente del nodo raíz con
 *   #pragma omp parallel for. Es la más simple y cumple el requisito mínimo.
 *
 * Función heurística:
 *   h = (kalaha_propio - kalaha_rival) + alpha * (semillas_lado_propio - semillas_lado_rival)
 *   con alpha = 0.5 (definido en alphabeta.cpp, ajustable).
 */

struct AlphaBetaResult {
    int move;        // índice del hoyo elegido (0-12, excluyendo kalahas)
    int evaluation;  // valor heurístico del movimiento elegido
    long long nodes; // nodos explorados durante la búsqueda
    long long prunes;// podas realizadas (siempre 0 en minimax puro)
};

// Minimax puro SIN poda: referencia para verificar que Alfa-Beta elige el mismo movimiento
// CORRECCIÓN: esta función faltaba, haciendo el test SameMoveasMinimax incorrecto
AlphaBetaResult minimax_best_move(const Board& board, int depth);

// Búsqueda secuencial con poda Alfa-Beta
AlphaBetaResult alphabeta_best_move(const Board& board, int depth);

// Búsqueda paralela con root parallelism (OpenMP)
AlphaBetaResult alphabeta_best_move_parallel(const Board& board, int depth);
