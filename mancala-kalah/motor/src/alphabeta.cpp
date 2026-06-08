#include "alphabeta.hpp"
#include <algorithm>
#include <limits>
#include <omp.h>

// Peso del segundo término de la heurística
static constexpr double ALPHA_WEIGHT = 0.5;

// Centinelas de infinito para alfa y beta
static constexpr int INF = std::numeric_limits<int>::max() / 2;

// ---------------------------------------------------------------------------
// Función heurística
// h = (kalaha_propio - kalaha_rival) + ALPHA_WEIGHT * (semillas_lado_propio - semillas_lado_rival)
// Se evalúa desde el punto de vista del jugador que inició la búsqueda (player_root).
// ---------------------------------------------------------------------------
static int heuristic(const Board& b, int player_root) {
    int rival = 1 - player_root;

    int k_own   = b.pits[Board::kalaha_of(player_root)];
    int k_rival = b.pits[Board::kalaha_of(rival)];

    int own_start   = (player_root == 0) ? 0        : HOLES + 1;
    int rival_start = (player_root == 0) ? HOLES + 1 : 0;

    int seeds_own = 0, seeds_rival = 0;
    for (int i = own_start;   i < own_start + HOLES;   i++) seeds_own   += b.pits[i];
    for (int i = rival_start; i < rival_start + HOLES; i++) seeds_rival += b.pits[i];

    return (k_own - k_rival)
         + static_cast<int>(ALPHA_WEIGHT * (seeds_own - seeds_rival));
}

// ---------------------------------------------------------------------------
// MINIMAX PURO sin poda Alfa-Beta — referencia de corrección
// CORRECCIÓN: esta función faltaba. El test SameMoveasMinimax la necesita
// para verificar que Alfa-Beta elige el mismo movimiento óptimo.
// ---------------------------------------------------------------------------
static int minimax(Board b, int depth, bool maximizing, int player_root,
                   long long& nodes) {
    nodes++;

    if (depth == 0 || b.is_terminal()) {
        return heuristic(b, player_root);
    }

    std::vector<int> moves = b.legal_moves(b.current_player);
    if (moves.empty()) return heuristic(b, player_root);

    if (maximizing) {
        int best = -INF;
        for (int pit : moves) {
            Board child = b;
            bool extra_turn = child.apply_move(pit);
            int val = minimax(child, depth - 1,
                              extra_turn ? true : false,
                              player_root, nodes);
            best = std::max(best, val);
        }
        return best;
    } else {
        int best = INF;
        for (int pit : moves) {
            Board child = b;
            bool extra_turn = child.apply_move(pit);
            int val = minimax(child, depth - 1,
                              extra_turn ? false : true,
                              player_root, nodes);
            best = std::min(best, val);
        }
        return best;
    }
}

AlphaBetaResult minimax_best_move(const Board& board, int depth) {
    int player_root = board.current_player;
    std::vector<int> moves = board.legal_moves(player_root);

    AlphaBetaResult result{-1, -INF, 0, 0};

    for (int pit : moves) {
        Board child = board;
        bool extra_turn = child.apply_move(pit);

        long long nodes = 0;
        int val = minimax(child, depth - 1,
                          extra_turn ? true : false,
                          player_root, nodes);

        result.nodes += nodes;

        if (val > result.evaluation || result.move == -1) {
            result.evaluation = val;
            result.move       = pit;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Minimax con poda Alfa-Beta (versión secuencial)
// ---------------------------------------------------------------------------
static int alphabeta(Board b, int depth, int alpha, int beta,
                     bool maximizing, int player_root,
                     long long& nodes, long long& prunes) {
    nodes++;

    if (depth == 0 || b.is_terminal()) {
        return heuristic(b, player_root);
    }

    std::vector<int> moves = b.legal_moves(b.current_player);
    if (moves.empty()) return heuristic(b, player_root);

    if (maximizing) {
        int best = -INF;
        for (int pit : moves) {
            Board child = b;
            bool extra_turn = child.apply_move(pit);
            int val = alphabeta(child, depth - 1, alpha, beta,
                                extra_turn ? true : false,
                                player_root, nodes, prunes);
            best  = std::max(best, val);
            alpha = std::max(alpha, best);
            if (beta <= alpha) { prunes++; break; } // poda beta
        }
        return best;
    } else {
        int best = INF;
        for (int pit : moves) {
            Board child = b;
            bool extra_turn = child.apply_move(pit);
            int val = alphabeta(child, depth - 1, alpha, beta,
                                extra_turn ? false : true,
                                player_root, nodes, prunes);
            best = std::min(best, val);
            beta = std::min(beta, best);
            if (beta <= alpha) { prunes++; break; } // poda alfa
        }
        return best;
    }
}

AlphaBetaResult alphabeta_best_move(const Board& board, int depth) {
    int player_root = board.current_player;
    std::vector<int> moves = board.legal_moves(player_root);

    AlphaBetaResult result{-1, -INF, 0, 0};

    for (int pit : moves) {
        Board child = board;
        bool extra_turn = child.apply_move(pit);

        long long nodes = 0, prunes = 0;
        int val = alphabeta(child, depth - 1, -INF, INF,
                            extra_turn ? true : false,
                            player_root, nodes, prunes);

        result.nodes  += nodes;
        result.prunes += prunes;

        if (val > result.evaluation || result.move == -1) {
            result.evaluation = val;
            result.move       = pit;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Versión paralela: root parallelism con OpenMP.
// Cada hilo evalúa un movimiento raíz independiente.
// Al no compartir alfa/beta entre hilos se pierden algunas podas,
// pero se elimina toda contención y sincronización.
// ---------------------------------------------------------------------------
AlphaBetaResult alphabeta_best_move_parallel(const Board& board, int depth) {
    int player_root = board.current_player;
    std::vector<int> moves = board.legal_moves(player_root);
    int n = static_cast<int>(moves.size());

    std::vector<int>       vals(n, -INF);
    std::vector<long long> thread_nodes(n, 0);
    std::vector<long long> thread_prunes(n, 0);

    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n; i++) {
        Board child = board;
        bool extra_turn = child.apply_move(moves[i]);

        long long nodes = 0, prunes = 0;
        vals[i] = alphabeta(child, depth - 1, -INF, INF,
                            extra_turn ? true : false,
                            player_root, nodes, prunes);
        thread_nodes[i]  = nodes;
        thread_prunes[i] = prunes;
    }

    AlphaBetaResult result{-1, -INF, 0, 0};
    for (int i = 0; i < n; i++) {
        result.nodes  += thread_nodes[i];
        result.prunes += thread_prunes[i];
        if (vals[i] > result.evaluation || result.move == -1) {
            result.evaluation = vals[i];
            result.move       = moves[i];
        }
    }
    return result;
}
