/**
 * Lógica del cliente web para Mancala Kalah.
 * El jugador 0 (humano, fila inferior) juega contra la IA (jugador 1, fila superior).
 * Toda la IA corre en el backend/motor; el frontend solo muestra el estado.
 *
 * La URL del backend se configura con la variable global API_URL (nginx la inyecta)
 * o se cae al default local.
 */

const API_URL = window.API_URL || "http://localhost:8000";

// Estado global de la partida
let gameState = {
  board: [], // 14 enteros
  currentPlayer: 0, // 0: humano, 1: IA
  gameOver: false,
};

// ---------------------------------------------------------------------------
// Inicialización del tablero visual (crea los elementos DOM de los hoyos)
// ---------------------------------------------------------------------------
function initBoardDOM() {
  const rowTop = document.getElementById("row-top");
  const rowBottom = document.getElementById("row-bottom");
  rowTop.innerHTML = "";
  rowBottom.innerHTML = "";

  // Fila superior: hoyos 12 → 7 (jugador 1, de derecha a izquierda visualmente)
  for (let i = 12; i >= 7; i--) {
    rowTop.appendChild(createPitElement(i, false));
  }

  // Fila inferior: hoyos 0 → 5 (jugador 0)
  for (let i = 0; i <= 5; i++) {
    rowBottom.appendChild(createPitElement(i, true));
  }
}

function createPitElement(index, clickable) {
  const div = document.createElement("div");
  div.classList.add("pit");
  div.id = `pit-${index}`;
  if (clickable) {
    div.classList.add("clickable");
    div.addEventListener("click", () => onHumanMove(index));
  } else {
    div.classList.add("disabled");
  }
  div.textContent = "0";
  return div;
}

// ---------------------------------------------------------------------------
// Actualiza los valores en el DOM a partir de gameState.board
// ---------------------------------------------------------------------------
function renderBoard() {
  const board = gameState.board;

  // Kalahas
  document.getElementById("k0-count").textContent = board[6];
  document.getElementById("k1-count").textContent = board[13];

  // Hoyos
  for (let i = 0; i < 14; i++) {
    if (i === 6 || i === 13) continue;
    const el = document.getElementById(`pit-${i}`);
    if (el) el.textContent = board[i];
  }

  // Habilitar/deshabilitar hoyos según a quién le toca
  const isHumanTurn = gameState.currentPlayer === 0 && !gameState.gameOver;
  for (let i = 0; i <= 5; i++) {
    const el = document.getElementById(`pit-${i}`);
    if (!el) continue;
    // Un hoyo es clicable solo si tiene semillas y es el turno del humano
    if (isHumanTurn && board[i] > 0) {
      el.classList.add("clickable");
      el.classList.remove("disabled");
    } else {
      el.classList.remove("clickable");
      el.classList.add("disabled");
    }
  }
}

// ---------------------------------------------------------------------------
// Inicia una nueva partida con el tablero estándar Kalah(6,4)
// ---------------------------------------------------------------------------
function newGame() {
  gameState.board = [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0];
  gameState.currentPlayer = 0;
  gameState.gameOver = false;

  document.getElementById("status").textContent = "Turno: Jugador 0 (tú)";
  document.getElementById("last-move").textContent = "";
  document.getElementById("error-msg").textContent = "";

  renderBoard();
}

// ---------------------------------------------------------------------------
// Maneja el movimiento del humano (jugador 0)
// ---------------------------------------------------------------------------
async function onHumanMove(pitIndex) {
  if (gameState.gameOver || gameState.currentPlayer !== 0) return;
  if (gameState.board[pitIndex] === 0) return;

  clearError();
  await sendMove(pitIndex);
}

// ---------------------------------------------------------------------------
// Envía el movimiento al backend y actualiza el estado con la respuesta
// ---------------------------------------------------------------------------
async function sendMove(pitIndex) {
  const algo = document.getElementById("algo").value;
  const depth = parseInt(document.getElementById("depth").value, 10);
  const sims = parseInt(document.getElementById("simulations").value, 10);
  const threads = parseInt(document.getElementById("threads").value, 10);

  const payload = {
    board: gameState.board,
    side: gameState.currentPlayer,
    algo: algo,
    threads: threads,
  };

  // El campo obligatorio varía según el algoritmo
  if (algo === "alphabeta") payload.depth = depth;
  else payload.simulations = sims;

  // Indicar que se está pensando
  setStatus("Pensando...");

  let data;
  try {
    const resp = await fetch(`${API_URL}/move`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });

    if (!resp.ok) {
      const err = await resp.json();
      showError("Error del servidor: " + (err.detail || resp.status));
      setStatus("Error");
      return;
    }

    data = await resp.json();
  } catch (e) {
    showError("No se pudo conectar al backend: " + e.message);
    setStatus("Sin conexión");
    return;
  }

  // Aplicar el movimiento devuelto por el motor al tablero local
  applyMoveToBoard(gameState.board, gameState.currentPlayer, data.move);

  // Mostrar información del movimiento
  const evalStr =
    algo === "alphabeta"
      ? `eval=${data.evaluation}`
      : `win_rate=${(data.evaluation * 100).toFixed(1)}%`;
  document.getElementById("last-move").textContent =
    `Jugador ${gameState.currentPlayer}: hoyo ${data.move} | ${evalStr} | ${data.elapsed_ms}ms`;

  // Detectar fin de juego
  if (isTerminal(gameState.board)) {
    gameState.gameOver = true;
    const k0 = gameState.board[6],
      k1 = gameState.board[13];
    const winner =
      k0 > k1 ? "Jugador 0 (tú)" : k1 > k0 ? "IA (Jugador 1)" : "Empate";
    setStatus(`¡Juego terminado! Ganador: ${winner} (${k0} vs ${k1})`);
    renderBoard();
    return;
  }

  // Cambiar turno (el motor ya aplicó el movimiento, incluyendo turnos extra si los hay)
  // El motor devuelve su movimiento; el turn extra lo maneja el motor internamente.
  // El frontend simplemente alterna el turno.
  gameState.currentPlayer = 1 - gameState.currentPlayer;
  renderBoard();

  // Si ahora le toca a la IA, pedir su movimiento automáticamente
  if (gameState.currentPlayer === 1) {
    setStatus("Turno: IA (Jugador 1) - pensando...");
    // Pequeño delay para que el DOM se actualice antes de bloquear
    setTimeout(() => sendMove(-1), 100);
  } else {
    setStatus("Turno: Jugador 0 (tú)");
  }
}

// ---------------------------------------------------------------------------
// Aplica un movimiento al arreglo del tablero (lógica mínima de siembra
// en el cliente, solo para reflejo visual inmediato).
// La fuente de verdad es siempre el motor.
// Como el motor devuelve el movimiento pero no el tablero resultante,
// simplemente refrescamos con el estado actual y confiamos en el motor.
// Esta función es solo para actualizar el array local de forma básica.
// ---------------------------------------------------------------------------
function applyMoveToBoard(board, player, pitIndex) {
  if (pitIndex < 0) return; // sin movimiento (no debería ocurrir)
  let seeds = board[pitIndex];
  board[pitIndex] = 0;
  const kalahaSkip = player === 0 ? 13 : 6;
  let idx = pitIndex;
  while (seeds > 0) {
    idx = (idx + 1) % 14;
    if (idx === kalahaSkip) continue;
    board[idx]++;
    seeds--;
  }
}

// Detecta si el juego terminó (un lado completamente vacío)
function isTerminal(board) {
  const side0 = board.slice(0, 6).reduce((a, b) => a + b, 0);
  const side1 = board.slice(7, 13).reduce((a, b) => a + b, 0);
  if (side0 === 0 || side1 === 0) {
    // Barrido al kalaha correspondiente
    for (let i = 0; i < 6; i++) {
      board[6] += board[i];
      board[i] = 0;
    }
    for (let i = 7; i < 13; i++) {
      board[13] += board[i];
      board[i] = 0;
    }
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Utilidades de UI
// ---------------------------------------------------------------------------
function setStatus(msg) {
  document.getElementById("status").textContent = msg;
}

function showError(msg) {
  document.getElementById("error-msg").textContent = msg;
}

function clearError() {
  document.getElementById("error-msg").textContent = "";
}

// Mostrar/ocultar campos según el algoritmo seleccionado
document.getElementById("algo").addEventListener("change", function () {
  document.getElementById("depth-group").style.display =
    this.value === "alphabeta" ? "" : "none";
  document.getElementById("sims-group").style.display =
    this.value === "mcts" ? "" : "none";
});

// Actualizar valores en tiempo real para profundidad
document.getElementById("depth").addEventListener("input", function () {
  document.getElementById("depth-value").textContent = this.value;
});

// Actualizar valores en tiempo real para simulaciones
document.getElementById("simulations").addEventListener("input", function () {
  const value = parseInt(this.value, 10);
  document.getElementById("sims-value").textContent = value.toLocaleString("es-ES");
});

// Actualizar valores en tiempo real para hilos
document.getElementById("threads").addEventListener("input", function () {
  document.getElementById("threads-value").textContent = this.value;
});

document.getElementById("btn-new").addEventListener("click", newGame);

// ---------------------------------------------------------------------------
// Arranque
// ---------------------------------------------------------------------------
initBoardDOM();
newGame();