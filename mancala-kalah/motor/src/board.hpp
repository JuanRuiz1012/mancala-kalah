#pragma once
#include <array>
#include <vector>

/*
 * Representación del tablero Kalah(6,4).
 *
 * Índices del arreglo de 14 posiciones (orden canónico):
 *   0-5  : hoyos del jugador 0 (sur)
 *   6    : kalaha del jugador 0
 *   7-12 : hoyos del jugador 1 (norte), en orden de siembra (7 es el más
 *           cercano al kalaha 0, 12 el más lejano)
 *   13   : kalaha del jugador 1
 *
 * Sentido de siembra: 0→1→2→3→4→5→6→7→8→9→10→11→12→13→0→...
 * El jugador salta el kalaha del oponente durante la siembra.
 */

constexpr int HOLES      = 6;   // hoyos por lado
constexpr int SEEDS_INIT = 4;   // semillas iniciales por hoyo
constexpr int BOARD_SIZE = 14;  // total de posiciones en el arreglo

// Índices de los kalahas
constexpr int KALAHA_0 = 6;
constexpr int KALAHA_1 = 13;

struct Board {
    std::array<int, BOARD_SIZE> pits;  // estado completo del tablero
    int current_player;                // 0 o 1: de quién es el turno

    // Inicializa el tablero en posición de inicio estándar
    Board();

    // Devuelve los índices de los hoyos propios del jugador dado
    // Jugador 0: pits[0..5], Jugador 1: pits[7..12]
    std::vector<int> legal_moves(int player) const;

    // Aplica un movimiento (índice del hoyo elegido) y devuelve true
    // si el jugador gana turno extra (última semilla cayó en su kalaha).
    // Modifica el tablero en su lugar.
    bool apply_move(int pit_index);

    // Detecta si el juego terminó (un lado queda completamente vacío).
    // Si termina, mueve las semillas restantes al kalaha correspondiente.
    bool is_terminal();

    // Devuelve el índice del kalaha del jugador indicado
    static int kalaha_of(int player);

    // Devuelve el índice del hoyo opuesto al pit_index dado
    static int opposite_pit(int pit_index);

    // Imprime el tablero en consola (útil para depuración)
    void print() const;
};