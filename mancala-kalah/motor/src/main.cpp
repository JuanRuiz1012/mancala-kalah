
#include "board.hpp"
#include "alphabeta.hpp"
#include "mcts.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <omp.h>

// POSIX sockets
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Parseo JSON mínimo (sin librerías externas)
// Extrae el valor entero de una clave simple en un JSON plano.
// ---------------------------------------------------------------------------
static int json_int(const std::string &json, const std::string &key, int fallback = 0)
{
     auto pos = json.find("\"" + key + "\"");
     if (pos == std::string::npos)
          return fallback;
     pos = json.find(':', pos);
     if (pos == std::string::npos)
          return fallback;
     return std::stoi(json.substr(pos + 1));
}

static std::string json_str(const std::string &json, const std::string &key)
{
     auto pos = json.find("\"" + key + "\"");
     if (pos == std::string::npos)
          return "";
     pos = json.find('"', json.find(':', pos));
     if (pos == std::string::npos)
          return "";
     pos++;
     auto end = json.find('"', pos);
     return json.substr(pos, end - pos);
}

// Extrae el arreglo de 14 enteros del campo "board"
static bool json_board(const std::string &json, std::array<int, BOARD_SIZE> &out)
{
     auto pos = json.find("\"board\"");
     if (pos == std::string::npos)
          return false;
     pos = json.find('[', pos);
     if (pos == std::string::npos)
          return false;
     pos++;
     for (int i = 0; i < BOARD_SIZE; i++)
     {
          out[i] = std::stoi(json.substr(pos));
          pos = json.find(',', pos);
          if (pos == std::string::npos && i < BOARD_SIZE - 1)
               return false;
          pos++;
     }
     return true;
}

// ---------------------------------------------------------------------------
// Construye respuesta HTTP 200 con cuerpo JSON
// ---------------------------------------------------------------------------
static std::string http_ok(const std::string &body)
{
     std::ostringstream r;
     r << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: application/json\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "\r\n"
       << body;
     return r.str();
}

static std::string http_err(int code, const std::string &msg)
{
     std::string body = "{\"error\":\"" + msg + "\"}";
     std::ostringstream r;
     r << "HTTP/1.1 " << code << " Error\r\n"
       << "Content-Type: application/json\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "\r\n"
       << body;
     return r.str();
}

// ---------------------------------------------------------------------------
// Manejo de una conexión entrante
// ---------------------------------------------------------------------------
static void handle_connection(int client_fd)
{
     char buf[8192] = {};
     ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
     if (n <= 0)
     {
          close(client_fd);
          return;
     }

     std::string req(buf, n);

     // Liveness probe
     if (req.find("GET /healthz") != std::string::npos)
     {
          std::string resp = http_ok("{\"status\":\"ok\"}");
          send(client_fd, resp.c_str(), resp.size(), 0);
          close(client_fd);
          return;
     }

     // Endpoint /move
     if (req.find("POST /move") == std::string::npos)
     {
          std::string resp = http_err(404, "Not found");
          send(client_fd, resp.c_str(), resp.size(), 0);
          close(client_fd);
          return;
     }

     // Cuerpo JSON: buscar la línea en blanco que separa headers del body
     auto body_pos = req.find("\r\n\r\n");
     if (body_pos == std::string::npos)
     {
          std::string resp = http_err(400, "Bad request");
          send(client_fd, resp.c_str(), resp.size(), 0);
          close(client_fd);
          return;
     }
     std::string body = req.substr(body_pos + 4);

     // Parsear campos
     std::array<int, BOARD_SIZE> pits{};
     if (!json_board(body, pits))
     {
          std::string resp = http_err(422, "Invalid board");
          send(client_fd, resp.c_str(), resp.size(), 0);
          close(client_fd);
          return;
     }

     int side = json_int(body, "side", 0);
     std::string algo = json_str(body, "algo");
     int depth = json_int(body, "depth", 8);
     int simulations = json_int(body, "simulations", 10000);
     int threads = json_int(body, "threads", 1);

     // Configurar número de hilos OpenMP según petición
     omp_set_num_threads(threads);

     // Construir tablero con el estado recibido
     Board board;
     board.pits = pits;
     board.current_player = side;

     std::ostringstream result;

     if (algo == "alphabeta")
     {
          double t0 = omp_get_wtime();
          AlphaBetaResult r = (threads > 1)
                                  ? alphabeta_best_move_parallel(board, depth)
                                  : alphabeta_best_move(board, depth);
          double elapsed = (omp_get_wtime() - t0) * 1000.0; // ms

          result << "{"
                 << "\"move\":" << r.move << ","
                 << "\"evaluation\":" << r.evaluation << ","
                 << "\"elapsed_ms\":" << (int)elapsed << ","
                 << "\"stats\":{"
                 << "\"algo\":\"alphabeta\","
                 << "\"nodes\":" << r.nodes << ","
                 << "\"prunes\":" << r.prunes
                 << "},"
                 << "\"threads_used\":" << threads
                 << "}";
     }
     else if (algo == "mcts")
     {
          double t0 = omp_get_wtime();
          MCTSResult r = (threads > 1)
                             ? mcts_best_move_parallel(board, simulations)
                             : mcts_best_move(board, simulations);
          double elapsed = (omp_get_wtime() - t0) * 1000.0;

          result << "{"
                 << "\"move\":" << r.move << ","
                 << "\"evaluation\":" << r.win_rate << ","
                 << "\"elapsed_ms\":" << (int)elapsed << ","
                 << "\"stats\":{"
                 << "\"algo\":\"mcts\","
                 << "\"rollouts\":" << r.rollouts << ","
                 << "\"tree_depth_avg\":" << r.tree_depth_avg << ","
                 << "\"win_rate\":" << r.win_rate
                 << "},"
                 << "\"threads_used\":" << threads
                 << "}";
     }
     else
     {
          std::string resp = http_err(422, "algo must be alphabeta or mcts");
          send(client_fd, resp.c_str(), resp.size(), 0);
          close(client_fd);
          return;
     }

     std::string resp = http_ok(result.str());
     send(client_fd, resp.c_str(), resp.size(), 0);
     close(client_fd);
}

// ---------------------------------------------------------------------------
// Punto de entrada: abre el socket y entra al loop de aceptación
// ---------------------------------------------------------------------------
int main()
{
     int port = 9000;
     const char *env_port = std::getenv("MOTOR_PORT");
     if (env_port)
          port = std::atoi(env_port);

     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server_fd < 0)
     {
          perror("socket");
          return 1;
     }

     int opt = 1;
     setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

     sockaddr_in addr{};
     addr.sin_family = AF_INET;
     addr.sin_addr.s_addr = INADDR_ANY;
     addr.sin_port = htons(port);

     if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
     {
          perror("bind");
          return 1;
     }
     listen(server_fd, 16);
     std::cout << "[motor] Escuchando en puerto " << port << "\n";

     // Loop de atención secuencial (el paralelismo está dentro de cada llamada)
     while (true)
     {
          int client_fd = accept(server_fd, nullptr, nullptr);
          if (client_fd < 0)
          {
               perror("accept");
               continue;
          }
          handle_connection(client_fd);
     }

     close(server_fd);
     return 0;
}