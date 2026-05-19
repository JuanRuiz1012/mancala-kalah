#pragma once
#include "board.hpp"

/*
 * Motor Minimax con poda Alfa-Beta.
 *
 * Expone:
 *   - alphabeta_best_move : versión secuencial (referencia de corrección)
 *   - alphabeta_best_move_parallel : versión paralela con root parallelism
 *
 * La estrategia de paralelización elegida es root parallelism:
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
    long long prunes;// podas realizadas
};

// Búsqueda secuencial: referencia correcta para validar la versión paralela
AlphaBetaResult alphabeta_best_move(const Board& board, int depth);

// Búsqueda paralela con root parallelism (OpenMP)
AlphaBetaResult alphabeta_best_move_parallel(const Board& board, int depth);