"""
Pruebas del backend con TestClient de FastAPI (sin necesidad de levantar el motor).
Se mockea motor_client.call_motor para aislar el backend de la red.
"""

import pytest
from fastapi.testclient import TestClient
from unittest.mock import patch, AsyncMock

from app.main import app

client = TestClient(app)

# Respuesta simulada del motor para no depender del contenedor C++
MOCK_MOTOR_RESPONSE = {
    "move": 3,
    "evaluation": 7,
    "elapsed_ms": 10,
    "stats": {"algo": "alphabeta", "nodes": 100, "prunes": 20},
    "threads_used": 1,
}

BOARD_INITIAL = [4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 0]


# ---------------------------------------------------------------------------
# /healthz
# ---------------------------------------------------------------------------
def test_healthz_ok():
    resp = client.get("/healthz")
    assert resp.status_code == 200
    assert resp.json()["status"] == "ok"


# ---------------------------------------------------------------------------
# POST /move con alphabeta
# ---------------------------------------------------------------------------
@patch("app.main.call_motor", new_callable=AsyncMock, return_value=MOCK_MOTOR_RESPONSE)
def test_move_alphabeta(mock_motor):
    payload = {
        "board": BOARD_INITIAL,
        "side": 0,
        "algo": "alphabeta",
        "depth": 6,
        "threads": 1,
    }
    resp = client.post("/move", json=payload)
    assert resp.status_code == 200
    data = resp.json()
    assert data["move"] == 3
    assert "stats" in data


# ---------------------------------------------------------------------------
# POST /move con mcts
# ---------------------------------------------------------------------------
@patch("app.main.call_motor", new_callable=AsyncMock, return_value={
    "move": 2, "evaluation": 0.55, "elapsed_ms": 50,
    "stats": {"algo": "mcts", "rollouts": 1000, "tree_depth_avg": 10.0, "win_rate": 0.55},
    "threads_used": 1
})
def test_move_mcts(mock_motor):
    payload = {
        "board": BOARD_INITIAL,
        "side": 0,
        "algo": "mcts",
        "simulations": 1000,
        "threads": 1,
    }
    resp = client.post("/move", json=payload)
    assert resp.status_code == 200


# ---------------------------------------------------------------------------
# Validación: alphabeta sin depth debe devolver 422
# ---------------------------------------------------------------------------
def test_move_alphabeta_missing_depth():
    payload = {
        "board": BOARD_INITIAL,
        "side": 0,
        "algo": "alphabeta",
        "threads": 1,
        # depth omitido
    }
    resp = client.post("/move", json=payload)
    assert resp.status_code == 422


# ---------------------------------------------------------------------------
# Validación: mcts sin simulations debe devolver 422
# ---------------------------------------------------------------------------
def test_move_mcts_missing_simulations():
    payload = {
        "board": BOARD_INITIAL,
        "side": 1,
        "algo": "mcts",
        "threads": 1,
        # simulations omitido
    }
    resp = client.post("/move", json=payload)
    assert resp.status_code == 422


# ---------------------------------------------------------------------------
# Validación: board con tamaño incorrecto debe devolver 422
# ---------------------------------------------------------------------------
def test_move_invalid_board_size():
    payload = {
        "board": [4, 4, 4],  # tamaño incorrecto
        "side": 0,
        "algo": "alphabeta",
        "depth": 4,
        "threads": 1,
    }
    resp = client.post("/move", json=payload)
    assert resp.status_code == 422


# ---------------------------------------------------------------------------
# Validación: algo con valor desconocido debe devolver 422
# ---------------------------------------------------------------------------
def test_move_invalid_algo():
    payload = {
        "board": BOARD_INITIAL,
        "side": 0,
        "algo": "minimax_puro",  # no válido
        "depth": 4,
        "threads": 1,
    }
    resp = client.post("/move", json=payload)
    assert resp.status_code == 422