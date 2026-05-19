#pragma once
#include "board.hpp"

/*
 * Motor Monte Carlo Tree Search (MCTS) con política UCT.
 *
 * Expone:
 *   - mcts_best_move          : versión secuencial
 *   - mcts_best_move_parallel : versión paralela con root parallelism
 *
 * Estrategia de paralelización elegida: root parallelism.
 *   Cada hilo construye su propio árbol MCTS independiente sobre la misma
 *   posición raíz. Al terminar, se suman las visitas y victorias de los
 *   hijos raíz de todos los árboles y se elige el movimiento con más visitas.
 *   No requiere sincronización durante la búsqueda → sin contención.
 *
 * Fases canónicas del ciclo MCTS:
 *   1. Selección   : bajar por UCT hasta una hoja
 *   2. Expansión   : agregar un hijo no visitado
 *   3. Simulación  : rollout aleatorio hasta terminal
 *   4. Retropropagación : actualizar w y N en el camino
 */

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