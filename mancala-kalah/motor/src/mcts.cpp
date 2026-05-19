#include "mcts.hpp"
#include <cmath>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <omp.h>

static constexpr double UCT_C = 1.41421356; // sqrt(2), constante de exploración

// ---------------------------------------------------------------------------
// Nodo del árbol MCTS
// ---------------------------------------------------------------------------
struct MCTSNode
{
     Board board;     // estado del tablero en este nodo
     int move;        // movimiento que llevó a este nodo (-1 en raíz)
     int player_root; // jugador que inició la búsqueda (para saber quién gana)

     double wins = 0.0; // victorias acumuladas en rollouts que pasaron por aquí
     int visits = 0;    // visitas totales

     MCTSNode *parent = nullptr;
     std::vector<std::unique_ptr<MCTSNode>> children;
     std::vector<int> untried_moves; // movimientos aún no expandidos

     MCTSNode(const Board &b, int mv, int root_player, MCTSNode *par = nullptr)
         : board(b), move(mv), player_root(root_player), parent(par)
     {
          untried_moves = b.legal_moves(b.current_player);
     }

     bool is_fully_expanded() const { return untried_moves.empty(); }
     bool is_terminal() { return board.is_terminal() || board.legal_moves(board.current_player).empty(); }
};

// ---------------------------------------------------------------------------
// UCT: selecciona el hijo con mayor valor UCT
// ---------------------------------------------------------------------------
static MCTSNode *uct_select(MCTSNode *node)
{
     MCTSNode *best = nullptr;
     double best_val = -1.0;

     for (auto &child : node->children)
     {
          double exploit = child->wins / child->visits;
          double explore = UCT_C * std::sqrt(std::log(node->visits) / child->visits);
          double val = exploit + explore;
          if (val > best_val)
          {
               best_val = val;
               best = child.get();
          }
     }
     return best;
}

// ---------------------------------------------------------------------------
// Expansión: agrega un hijo no visitado al nodo
// ---------------------------------------------------------------------------
static MCTSNode *expand(MCTSNode *node, std::mt19937 &rng)
{
     // Tomar un movimiento no probado al azar
     int idx = std::uniform_int_distribution<int>(0, (int)node->untried_moves.size() - 1)(rng);
     int pit = node->untried_moves[idx];
     node->untried_moves.erase(node->untried_moves.begin() + idx);

     Board child_board = node->board;
     child_board.apply_move(pit);

     node->children.push_back(
         std::make_unique<MCTSNode>(child_board, pit, node->player_root, node));
     return node->children.back().get();
}

// ---------------------------------------------------------------------------
// Simulación (rollout): juega al azar hasta terminal, devuelve profundidad
// y 1.0 si ganó el jugador raíz, 0.5 empate, 0.0 si perdió.
// ---------------------------------------------------------------------------
static double simulate(Board b, int player_root, std::mt19937 &rng, int &depth_out)
{
     depth_out = 0;
     while (!b.is_terminal())
     {
          auto moves = b.legal_moves(b.current_player);
          if (moves.empty())
               break;
          int pit = moves[std::uniform_int_distribution<int>(0, (int)moves.size() - 1)(rng)];
          b.apply_move(pit);
          depth_out++;
     }
     int k0 = b.pits[Board::kalaha_of(0)];
     int k1 = b.pits[Board::kalaha_of(1)];
     int kown = (player_root == 0) ? k0 : k1;
     int krival = (player_root == 0) ? k1 : k0;
     if (kown > krival)
          return 1.0;
     if (kown == krival)
          return 0.5;
     return 0.0;
}

// ---------------------------------------------------------------------------
// Retropropagación: sube actualizando wins y visits
// ---------------------------------------------------------------------------
static void backpropagate(MCTSNode *node, double result)
{
     while (node != nullptr)
     {
          node->visits++;
          node->wins += result;
          node = node->parent;
     }
}

// ---------------------------------------------------------------------------
// Ciclo MCTS completo sobre un árbol.
// Devuelve profundidad promedio acumulada (para estadísticas).
// ---------------------------------------------------------------------------
static double run_mcts(MCTSNode *root, int simulations, std::mt19937 &rng)
{
     double total_depth = 0.0;
     for (int s = 0; s < simulations; s++)
     {
          // 1. Selección
          MCTSNode *node = root;
          while (!node->is_terminal() && node->is_fully_expanded())
          {
               node = uct_select(node);
          }
          // 2. Expansión
          if (!node->is_terminal() && !node->is_fully_expanded())
          {
               node = expand(node, rng);
          }
          // 3. Simulación
          int depth = 0;
          double result = simulate(node->board, node->player_root, rng, depth);
          total_depth += depth;
          // 4. Retropropagación
          backpropagate(node, result);
     }
     return total_depth / simulations;
}

// ---------------------------------------------------------------------------
// Interfaz secuencial
// ---------------------------------------------------------------------------
MCTSResult mcts_best_move(const Board &board, int simulations)
{
     std::mt19937 rng(std::random_device{}());
     MCTSNode root(board, -1, board.current_player);

     double depth_avg = run_mcts(&root, simulations, rng);

     // Elegir el hijo con más visitas (criterio estándar MCTS)
     MCTSNode *best = nullptr;
     for (auto &child : root.children)
     {
          if (best == nullptr || child->visits > best->visits)
               best = child.get();
     }

     MCTSResult res;
     res.move = best ? best->move : -1;
     res.win_rate = best ? (best->wins / best->visits) : 0.0;
     res.rollouts = simulations;
     res.tree_depth_avg = depth_avg;
     return res;
}

// ---------------------------------------------------------------------------
// Interfaz paralela: root parallelism con OpenMP.
// Cada hilo corre su propio árbol MCTS con simulations/nthreads simulaciones.
// Al terminar, se suman visitas y victorias de todos los árboles por movimiento.
// ---------------------------------------------------------------------------
MCTSResult mcts_best_move_parallel(const Board &board, int simulations)
{
     int nthreads = omp_get_max_threads();
     int sims_per_thread = simulations / nthreads;

     // Cada hilo guarda su árbol en un unique_ptr para evitar aliasing
     std::vector<std::unique_ptr<MCTSNode>> roots(nthreads);
     std::vector<double> depths(nthreads, 0.0);

#pragma omp parallel for
     for (int t = 0; t < nthreads; t++)
     {
          // Semilla distinta por hilo para no repetir secuencias aleatorias
          std::mt19937 rng(std::random_device{}() + t * 1000);
          roots[t] = std::make_unique<MCTSNode>(board, -1, board.current_player);
          depths[t] = run_mcts(roots[t].get(), sims_per_thread, rng);
     }

     // Combinar: acumular visitas y victorias de cada movimiento en un mapa
     // Usamos el índice del hoyo como clave
     std::vector<int> total_visits(BOARD_SIZE, 0);
     std::vector<double> total_wins(BOARD_SIZE, 0.0);

     for (int t = 0; t < nthreads; t++)
     {
          for (auto &child : roots[t]->children)
          {
               total_visits[child->move] += child->visits;
               total_wins[child->move] += child->wins;
          }
     }

     // Elegir el movimiento con más visitas acumuladas
     int best_move = -1;
     int best_v = -1;
     for (int pit = 0; pit < BOARD_SIZE; pit++)
     {
          if (total_visits[pit] > best_v)
          {
               best_v = total_visits[pit];
               best_move = pit;
          }
     }

     double total_depth = 0.0;
     for (double d : depths)
          total_depth += d;

     MCTSResult res;
     res.move = best_move;
     res.win_rate = (best_v > 0) ? total_wins[best_move] / best_v : 0.0;
     res.rollouts = simulations;
     res.tree_depth_avg = total_depth / nthreads;
     return res;
}