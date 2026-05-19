/*
 * Benchmark independiente: lee posiciones de prueba desde un archivo,
 * ejecuta los motores y reporta métricas de tiempo, speedup y eficiencia.
 *
 * Uso:
 *   OMP_NUM_THREADS=4 ./mancala_bench --algo alphabeta --depth 8 --positions suite.txt
 *   OMP_NUM_THREADS=4 ./mancala_bench --algo mcts --simulations 10000 --positions suite.txt
 *
 * Formato de suite.txt: una posición por línea, 15 enteros separados por espacio:
 *   <14 valores del tablero> <current_player>
 */

#include "board.hpp"
#include "alphabeta.hpp"
#include "mcts.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <omp.h>

// ---------------------------------------------------------------------------
// Carga las posiciones del archivo de suite
// ---------------------------------------------------------------------------
static std::vector<Board> load_positions(const std::string &path)
{
     std::vector<Board> positions;
     std::ifstream file(path);
     if (!file.is_open())
     {
          std::cerr << "No se pudo abrir: " << path << "\n";
          return positions;
     }

     std::string line;
     while (std::getline(file, line))
     {
          if (line.empty() || line[0] == '#')
               continue; // ignorar comentarios
          std::istringstream ss(line);
          Board b;
          for (int i = 0; i < BOARD_SIZE; i++)
               ss >> b.pits[i];
          ss >> b.current_player;
          positions.push_back(b);
     }
     return positions;
}

// ---------------------------------------------------------------------------
// Imprime tabla de resultados en formato Markdown (fácil de copiar al informe)
// ---------------------------------------------------------------------------
static void print_table_header(const std::string &algo)
{
     std::cout << "\n## Resultados " << algo << "\n";
     std::cout << "| Hilos | T(p) ms | S(p) | E(p) | Nodos/Rollouts | Podas/WinRate |\n";
     std::cout << "|-------|---------|------|------|----------------|---------------|\n";
}

// ---------------------------------------------------------------------------
// Benchmark de Alfa-Beta
// ---------------------------------------------------------------------------
static void bench_alphabeta(const std::vector<Board> &positions, int depth)
{
     print_table_header("Alfa-Beta  depth=" + std::to_string(depth));

     double t1_ms = 0.0; // tiempo con 1 hilo (para speedup)

     for (int threads : {1, 2, 4, 8})
     {
          omp_set_num_threads(threads);

          double t_start = omp_get_wtime();
          long long total_nodes = 0, total_prunes = 0;

          for (const Board &b : positions)
          {
               AlphaBetaResult r = (threads > 1)
                                       ? alphabeta_best_move_parallel(b, depth)
                                       : alphabeta_best_move(b, depth);
               total_nodes += r.nodes;
               total_prunes += r.prunes;
          }

          double elapsed = (omp_get_wtime() - t_start) * 1000.0; // ms

          if (threads == 1)
               t1_ms = elapsed;

          double speedup = (t1_ms > 0) ? t1_ms / elapsed : 1.0;
          double efficiency = speedup / threads;

          std::cout << "| " << threads
                    << " | " << (int)elapsed
                    << " | " << speedup
                    << " | " << efficiency
                    << " | " << total_nodes
                    << " | " << total_prunes
                    << " |\n";
     }
}

// ---------------------------------------------------------------------------
// Benchmark de MCTS
// ---------------------------------------------------------------------------
static void bench_mcts(const std::vector<Board> &positions, int simulations)
{
     print_table_header("MCTS  simulations=" + std::to_string(simulations));

     double t1_ms = 0.0;

     for (int threads : {1, 2, 4, 8})
     {
          omp_set_num_threads(threads);

          double t_start = omp_get_wtime();
          double total_depth = 0.0;
          long long total_rollouts = 0;

          for (const Board &b : positions)
          {
               MCTSResult r = (threads > 1)
                                  ? mcts_best_move_parallel(b, simulations)
                                  : mcts_best_move(b, simulations);
               total_depth += r.tree_depth_avg;
               total_rollouts += r.rollouts;
          }

          double elapsed = (omp_get_wtime() - t_start) * 1000.0;

          if (threads == 1)
               t1_ms = elapsed;

          double speedup = (t1_ms > 0) ? t1_ms / elapsed : 1.0;
          double efficiency = speedup / threads;
          double avg_depth = total_depth / positions.size();

          std::cout << "| " << threads
                    << " | " << (int)elapsed
                    << " | " << speedup
                    << " | " << efficiency
                    << " | " << total_rollouts
                    << " | " << avg_depth
                    << " |\n";
     }
}

// ---------------------------------------------------------------------------
// Punto de entrada del benchmark
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
     std::string algo = "alphabeta";
     std::string positions_file = "bench/suite.txt";
     int depth = 8;
     int simulations = 10000;

     // Parseo simple de argumentos
     for (int i = 1; i < argc; i++)
     {
          std::string arg = argv[i];
          if (arg == "--algo" && i + 1 < argc)
               algo = argv[++i];
          if (arg == "--depth" && i + 1 < argc)
               depth = std::stoi(argv[++i]);
          if (arg == "--simulations" && i + 1 < argc)
               simulations = std::stoi(argv[++i]);
          if (arg == "--positions" && i + 1 < argc)
               positions_file = argv[++i];
     }

     std::vector<Board> positions = load_positions(positions_file);
     if (positions.empty())
     {
          std::cerr << "Sin posiciones de prueba. Abortando.\n";
          return 1;
     }
     std::cout << "Posiciones cargadas: " << positions.size() << "\n";

     if (algo == "alphabeta")
     {
          // Dos profundidades distintas como exige el enunciado
          bench_alphabeta(positions, depth);
          bench_alphabeta(positions, depth + 4); // segunda configuración
     }
     else if (algo == "mcts")
     {
          bench_mcts(positions, simulations);
          bench_mcts(positions, simulations * 10); // segunda configuración
     }
     else
     {
          std::cerr << "algo desconocido: " << algo << "\n";
          return 1;
     }

     return 0;
}