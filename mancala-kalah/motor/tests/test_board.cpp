/*
 * Suite de pruebas unitarias.
 * Valida: reglas de Kalah, turno extra, captura, terminal,
 * equivalencia Minimax == AlfaBeta, y convergencia básica de MCTS.
 *
 * Compilar con: cmake --build build --target test_board
 * Ejecutar con: ctest --test-dir build -V
 */

#include <gtest/gtest.h>
#include "../src/board.hpp"
#include "../src/alphabeta.hpp"
#include "../src/mcts.hpp"

// ---------------------------------------------------------------------------
// Tests del tablero
// ---------------------------------------------------------------------------

// El tablero inicial debe tener 4 semillas en cada hoyo y kalahas en 0
TEST(BoardTest, InitialState)
{
     Board b;
     for (int i = 0; i < HOLES; i++)
          EXPECT_EQ(b.pits[i], SEEDS_INIT) << "Hoyo " << i << " incorrecto";
     for (int i = HOLES + 1; i < KALAHA_1; i++)
          EXPECT_EQ(b.pits[i], SEEDS_INIT) << "Hoyo " << i << " incorrecto";
     EXPECT_EQ(b.pits[KALAHA_0], 0);
     EXPECT_EQ(b.pits[KALAHA_1], 0);
     EXPECT_EQ(b.current_player, 0);
}

// Total de semillas debe conservarse: 6*2*4 = 48
TEST(BoardTest, SeedConservation)
{
     Board b;
     b.apply_move(0);
     int total = 0;
     for (int v : b.pits)
          total += v;
     EXPECT_EQ(total, HOLES * 2 * SEEDS_INIT);
}

// Movimientos legales iniciales: los 6 hoyos del jugador 0
TEST(BoardTest, LegalMovesInitial)
{
     Board b;
     auto moves = b.legal_moves(0);
     EXPECT_EQ((int)moves.size(), 6);
}

// Turno extra: si la última semilla cae en el kalaha propio
TEST(BoardTest, ExtraTurn)
{
     Board b;
     // Con 4 semillas en pits[2], siembra 2→3→4→5→kalaha0: turno extra
     b.pits.fill(0);
     b.pits[2] = 4; // semillas justo para llegar al kalaha
     b.current_player = 0;
     bool extra = b.apply_move(2);
     EXPECT_TRUE(extra) << "Debería otorgar turno extra";
     EXPECT_EQ(b.pits[KALAHA_0], 1); // una semilla en el kalaha
}

// Captura: última semilla en hoyo propio vacío, oponente tiene semillas enfrente
TEST(BoardTest, Capture)
{
     Board b;
     b.pits.fill(0);
     b.current_player = 0;
     b.pits[0] = 1;                      // una semilla que caerá en pits[1]
     b.pits[1] = 0;                      // hoyo destino vacío (recibirá la semilla → captura)
     b.pits[Board::opposite_pit(1)] = 3; // oponente tiene semillas enfrente
     b.apply_move(0);
     // Las 3 + 1 semillas deben estar en el kalaha 0
     EXPECT_EQ(b.pits[KALAHA_0], 4);
     EXPECT_EQ(b.pits[1], 0);
     EXPECT_EQ(b.pits[Board::opposite_pit(1)], 0);
}

// Detección de fin de juego
TEST(BoardTest, TerminalDetection)
{
     Board b;
     b.pits.fill(0);
     b.pits[KALAHA_0] = 25;
     b.pits[KALAHA_1] = 23;
     // Todo vacío → is_terminal debe devolver true
     EXPECT_TRUE(b.is_terminal());
}

// ---------------------------------------------------------------------------
// Tests de Alfa-Beta
// ---------------------------------------------------------------------------

// AlfaBeta y Minimax puro deben elegir el mismo movimiento a igual profundidad
TEST(AlphaBetaTest, SameMoveasMinimax)
{
    Board b;
    // Se usa profundidad baja (4) para que minimax puro sea tratable
    AlphaBetaResult mm  = minimax_best_move(b, 4);
    AlphaBetaResult ab  = alphabeta_best_move(b, 4);
    EXPECT_EQ(mm.move, ab.move)
        << "Minimax puro y Alfa-Beta deben elegir el mismo movimiento";
    EXPECT_EQ(mm.evaluation, ab.evaluation)
        << "Minimax puro y Alfa-Beta deben tener la misma evaluación";
}

// La versión paralela debe elegir el mismo movimiento que la secuencial
TEST(AlphaBetaTest, ParallelMatchesSerial)
{
     Board b;
     AlphaBetaResult seq = alphabeta_best_move(b, 6);
     AlphaBetaResult par = alphabeta_best_move_parallel(b, 6);
     EXPECT_EQ(seq.move, par.move)
         << "Paralelo y secuencial deben elegir el mismo movimiento";
}

// El movimiento devuelto debe ser un hoyo legal
TEST(AlphaBetaTest, MoveIsLegal)
{
     Board b;
     AlphaBetaResult r = alphabeta_best_move(b, 4);
     auto moves = b.legal_moves(b.current_player);
     bool found = false;
     for (int m : moves)
          if (m == r.move)
               found = true;
     EXPECT_TRUE(found) << "El movimiento no es legal";
}
// Minimax puro también debe devolver un movimiento legal
TEST(AlphaBetaTest, MinimaxMoveIsLegal)
{
    Board b;
    AlphaBetaResult r = minimax_best_move(b, 3);
    auto moves = b.legal_moves(b.current_player);
    bool found = false;
    for (int m : moves)
        if (m == r.move) found = true;
    EXPECT_TRUE(found) << "Minimax puro devolvió un movimiento ilegal";
}


// ---------------------------------------------------------------------------
// Tests de MCTS
// ---------------------------------------------------------------------------

// MCTS debe devolver un movimiento legal
TEST(MCTSTest, MoveIsLegal)
{
     Board b;
     MCTSResult r = mcts_best_move(b, 500);
     auto moves = b.legal_moves(b.current_player);
     bool found = false;
     for (int m : moves)
          if (m == r.move)
               found = true;
     EXPECT_TRUE(found) << "MCTS devolvió un movimiento ilegal";
}

// win_rate debe estar en [0, 1]
TEST(MCTSTest, WinRateRange)
{
     Board b;
     MCTSResult r = mcts_best_move(b, 200);
     EXPECT_GE(r.win_rate, 0.0);
     EXPECT_LE(r.win_rate, 1.0);
}

// La versión paralela también debe devolver un movimiento legal
TEST(MCTSTest, ParallelMoveIsLegal)
{
     Board b;
     MCTSResult r = mcts_best_move_parallel(b, 500);
     auto moves = b.legal_moves(b.current_player);
     bool found = false;
     for (int m : moves)
          if (m == r.move)
               found = true;
     EXPECT_TRUE(found) << "MCTS paralelo devolvió un movimiento ilegal";
}

int main(int argc, char **argv)
{
     ::testing::InitGoogleTest(&argc, argv);
     return RUN_ALL_TESTS();
}