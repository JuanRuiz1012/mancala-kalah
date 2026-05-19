const API_URL = window.API_URL || "http://localhost:8000";

let gameState = {
  board: [], 
  currentPlayer: 0, 
  gameOver: false,
};

function initBoardDOM() {
  const rowTop = document.getElementById("row-top");
  const rowBottom = document.getElementById("row-bottom");
  rowTop.innerHTML = "";
  rowBottom.innerHTML = "";


  for (let i = 12; i >= 7; i--) {
    rowTop.appendChild(createPitElement(i, false));
  }

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

function newGame() {
  gameState.board = [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0];
  gameState.currentPlayer = 0;
  gameState.gameOver = false;

  document.getElementById("status").textContent = "Turno: Jugador 0 (tú)";
  document.getElementById("last-move").textContent = "";
  document.getElementById("error-msg").textContent = "";

  renderBoard();
}


async function onHumanMove(pitIndex) {
  if (gameState.gameOver || gameState.currentPlayer !== 0) return;
  if (gameState.board[pitIndex] === 0) return;

  clearError();
  await sendMove(pitIndex);
}

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


  if (algo === "alphabeta") payload.depth = depth;
  else payload.simulations = sims;


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


  applyMoveToBoard(gameState.board, gameState.currentPlayer, data.move);


  const evalStr =
    algo === "alphabeta"
      ? `eval=${data.evaluation}`
      : `win_rate=${(data.evaluation * 100).toFixed(1)}%`;
  document.getElementById("last-move").textContent =
    `Jugador ${gameState.currentPlayer}: hoyo ${data.move} | ${evalStr} | ${data.elapsed_ms}ms`;


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


  gameState.currentPlayer = 1 - gameState.currentPlayer;
  renderBoard();

  if (gameState.currentPlayer === 1) {
    setStatus("Turno: IA (Jugador 1) - pensando...");

    setTimeout(() => sendMove(-1), 100);
  } else {
    setStatus("Turno: Jugador 0 (tú)");
  }
}


function applyMoveToBoard(board, player, pitIndex) {
  if (pitIndex < 0) return; 
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


function setStatus(msg) {
  document.getElementById("status").textContent = msg;
}

function showError(msg) {
  document.getElementById("error-msg").textContent = msg;
}

function clearError() {
  document.getElementById("error-msg").textContent = "";
}


document.getElementById("algo").addEventListener("change", function () {
  document.getElementById("depth-group").style.display =
    this.value === "alphabeta" ? "" : "none";
  document.getElementById("sims-group").style.display =
    this.value === "mcts" ? "" : "none";
});


document.getElementById("depth").addEventListener("input", function () {
  document.getElementById("depth-value").textContent = this.value;
});


document.getElementById("simulations").addEventListener("input", function () {
  const value = parseInt(this.value, 10);
  document.getElementById("sims-value").textContent = value.toLocaleString("es-ES");
});


document.getElementById("threads").addEventListener("input", function () {
  document.getElementById("threads-value").textContent = this.value;
});

document.getElementById("btn-new").addEventListener("click", newGame);


initBoardDOM();
newGame();