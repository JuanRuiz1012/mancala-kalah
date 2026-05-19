#include "board.hpp"
#include <iostream>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Constructor: llena cada hoyo con SEEDS_INIT semillas, kalahas en 0
// ---------------------------------------------------------------------------
Board::Board() {
    pits.fill(0);
    for (int i = 0; i < HOLES; i++)      pits[i]           = SEEDS_INIT; // lado 0
    for (int i = HOLES + 1; i < KALAHA_1; i++) pits[i]     = SEEDS_INIT; // lado 1
    // pits[KALAHA_0] = 0 y pits[KALAHA_1] = 0 ya están en 0
    current_player = 0;
}

// ---------------------------------------------------------------------------
// Kalaha de cada jugador
// ---------------------------------------------------------------------------
int Board::kalaha_of(int player) {
    return (player == 0) ? KALAHA_0 : KALAHA_1;
}

// ---------------------------------------------------------------------------
// Hoyo opuesto en el otro lado del tablero.
// La fórmula funciona para cualquier hoyo 0-5 y 7-12:
//   opuesto(i) = 12 - i   (excluye los kalahas)
// ---------------------------------------------------------------------------
int Board::opposite_pit(int pit_index) {
    return KALAHA_1 - 1 - pit_index; // = 12 - pit_index
}

// ---------------------------------------------------------------------------
// Movimientos legales: hoyos propios con al menos una semilla
// ---------------------------------------------------------------------------
std::vector<int> Board::legal_moves(int player) const {
    std::vector<int> moves;
    int start = (player == 0) ? 0        : HOLES + 1;
    int end   = (player == 0) ? HOLES    : KALAHA_1;

    for (int i = start; i < end; i++) {
        if (pits[i] > 0) moves.push_back(i);
    }
    return moves;
}

// ---------------------------------------------------------------------------
// Aplica el movimiento: siembra, captura y turno extra.
// Devuelve true si el jugador obtiene turno extra.
// ---------------------------------------------------------------------------
bool Board::apply_move(int pit_index) {
    int seeds = pits[pit_index];
    if (seeds == 0) throw std::invalid_argument("Hoyo vacío seleccionado");

    pits[pit_index] = 0;
    int kalaha_skip = kalaha_of(1 - current_player); // kalaha del oponente, se salta

    int idx = pit_index;
    while (seeds > 0) {
        idx = (idx + 1) % BOARD_SIZE;
        if (idx == kalaha_skip) continue; // saltar kalaha del oponente
        pits[idx]++;
        seeds--;
    }

    // --- Turno extra: última semilla cayó en el kalaha propio ---
    if (idx == kalaha_of(current_player)) {
        return true; // no se cambia current_player, el llamador decide
    }

    // --- Captura: última semilla en hoyo propio vacío (antes tenía 0 → ahora 1) ---
    int own_start = (current_player == 0) ? 0     : HOLES + 1;
    int own_end   = (current_player == 0) ? HOLES : KALAHA_1;

    if (idx >= own_start && idx < own_end && pits[idx] == 1) {
        int opp = opposite_pit(idx);
        if (pits[opp] > 0) {
            // Captura: mover semillas del hoyo opuesto + la propia al kalaha
            pits[kalaha_of(current_player)] += pits[opp] + 1;
            pits[opp] = 0;
            pits[idx] = 0;
        }
    }

    // Cambio de turno normal
    current_player = 1 - current_player;
    return false;
}

// ---------------------------------------------------------------------------
// Detecta fin de juego. Si un lado está vacío, barre el otro al kalaha.
// ---------------------------------------------------------------------------
bool Board::is_terminal() {
    int sum0 = 0, sum1 = 0;
    for (int i = 0;        i < HOLES;   i++) sum0 += pits[i];
    for (int i = HOLES + 1; i < KALAHA_1; i++) sum1 += pits[i];

    if (sum0 == 0 || sum1 == 0) {
        // Barrido: el jugador con semillas restantes las mueve a su kalaha
        for (int i = 0;        i < HOLES;   i++) { pits[KALAHA_0] += pits[i]; pits[i] = 0; }
        for (int i = HOLES + 1; i < KALAHA_1; i++) { pits[KALAHA_1] += pits[i]; pits[i] = 0; }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Impresión del tablero (jugador 1 arriba, jugador 0 abajo)
// ---------------------------------------------------------------------------
void Board::print() const {
    // Fila superior: jugador 1 (de derecha a izquierda visualmente)
    std::cout << "   ";
    for (int i = KALAHA_1 - 1; i >= HOLES + 1; i--)
        std::cout << "[" << pits[i] << "] ";
    std::cout << "\n";

    // Kalahas a los lados
    std::cout << "[" << pits[KALAHA_1] << "]";
    std::cout << "                    ";
    std::cout << "[" << pits[KALAHA_0] << "]\n";

    // Fila inferior: jugador 0
    std::cout << "   ";
    for (int i = 0; i < HOLES; i++)
        std::cout << "[" << pits[i] << "] ";
    std::cout << "\n";
    std::cout << "   Turno: jugador " << current_player << "\n\n";
}