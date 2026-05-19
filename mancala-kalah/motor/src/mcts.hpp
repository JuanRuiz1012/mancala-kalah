#pragma once
#include "board.hpp"


struct MCTSResult {
    int   move;           // índice del hoyo elegido
    double win_rate;      // victorias / visitas del nodo elegido
    long long rollouts;   // simulaciones totales ejecutadas
    double tree_depth_avg;// profundidad promedio alcanzada en los rollouts
};

// Búsqueda secuencial
MCTSResult mcts_best_move(const Board& board, int simulations);

// Búsqueda paralela (root parallelism, OpenMP)
MCTSResult mcts_best_move_parallel(const Board& board, int simulations);