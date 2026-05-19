
from typing import List, Literal, Optional
from pydantic import BaseModel, Field, model_validator


# ---------------------------------------------------------------------------
# Request: POST /move
# ---------------------------------------------------------------------------
class MoveRequest(BaseModel):
    board: List[int] = Field(
        ...,
        min_length=14,
        max_length=14,
        description="Estado del tablero: 14 enteros en orden canónico (0-5 lado0, 6 kalaha0, 7-12 lado1, 13 kalaha1)"
    )
    side: int = Field(..., ge=0, le=1, description="Jugador activo: 0 o 1")
    algo: Literal["alphabeta", "mcts"] = Field(..., description="Algoritmo a usar")
    depth: Optional[int] = Field(None, ge=1, le=20, description="Profundidad (requerido para alphabeta)")
    simulations: Optional[int] = Field(None, ge=1, description="Simulaciones (requerido para mcts)")
    threads: int = Field(1, ge=1, le=64, description="Hilos OpenMP")

    # Validación cruzada: depth obligatorio para alphabeta, simulations para mcts
    @model_validator(mode="after")
    def check_algo_params(self):
        if self.algo == "alphabeta" and self.depth is None:
            raise ValueError("depth es obligatorio cuando algo='alphabeta'")
        if self.algo == "mcts" and self.simulations is None:
            raise ValueError("simulations es obligatorio cuando algo='mcts'")
        return self


# ---------------------------------------------------------------------------
# Stats específicos por algoritmo (campo anidado en la respuesta)
# ---------------------------------------------------------------------------
class AlphaBetaStats(BaseModel):
    algo: Literal["alphabeta"] = "alphabeta"
    nodes: int
    prunes: int


class MCTSStats(BaseModel):
    algo: Literal["mcts"] = "mcts"
    rollouts: int
    tree_depth_avg: float
    win_rate: float


# ---------------------------------------------------------------------------
# Response: POST /move
# ---------------------------------------------------------------------------
class MoveResponse(BaseModel):
    move: int = Field(..., description="Índice del hoyo elegido")
    evaluation: float = Field(..., description="Valor heurístico o win_rate")
    elapsed_ms: int = Field(..., description="Tiempo de cómputo en milisegundos")
    stats: dict = Field(..., description="Métricas específicas del algoritmo")
    threads_used: int


# ---------------------------------------------------------------------------
# Response: GET /healthz y GET /readyz
# ---------------------------------------------------------------------------
class HealthResponse(BaseModel):
    status: str


# ---------------------------------------------------------------------------
# Response: GET /metrics
# ---------------------------------------------------------------------------
class MetricsResponse(BaseModel):
    total_requests: int
    alphabeta_requests: int
    mcts_requests: int
    avg_elapsed_ms: float