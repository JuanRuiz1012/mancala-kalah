"""
Backend FastAPI: wrapper HTTP entre el frontend y el motor C++/OpenMP.
Valida la entrada con Pydantic, delega el cálculo al motor por la red
interna del clúster y devuelve la respuesta con el movimiento óptimo.
"""

import os
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .schemas import MoveRequest, MoveResponse, HealthResponse, MetricsResponse
from .motor_client import call_motor, check_motor_health
from .metrics import metrics

app = FastAPI(title="Mancala Kalah API", version="1.0.0")

# ---------------------------------------------------------------------------
# CORS: orígenes explícitos del frontend (local y nube).
# No se usa el comodín "*" tal como exige el enunciado.
# Los orígenes se toman de la variable de entorno ALLOWED_ORIGINS
# (lista separada por comas) para poder cambiarlos sin recompilar.
# ---------------------------------------------------------------------------
_raw_origins = os.getenv(
    "ALLOWED_ORIGINS",
    "http://localhost:8080,http://frontend-svc:80"
)
allowed_origins = [o.strip() for o in _raw_origins.split(",") if o.strip()]

app.add_middleware(
    CORSMiddleware,
    allow_origins=allowed_origins,
    allow_methods=["GET", "POST", "OPTIONS"],
    allow_headers=["Content-Type"],
    allow_credentials=False,
)


# ---------------------------------------------------------------------------
# POST /move
# Recibe el estado del tablero, delega al motor y devuelve la jugada óptima.
# ---------------------------------------------------------------------------
@app.post("/move", response_model=MoveResponse)
async def move(request: MoveRequest):
    # Construir payload para el motor (campos que el motor entiende)
    payload = {
        "board":       request.board,
        "side":        request.side,
        "algo":        request.algo,
        "depth":       request.depth,
        "simulations": request.simulations,
        "threads":     request.threads,
    }

    result = await call_motor(payload)

    # Registrar métricas
    metrics.record(request.algo, result.get("elapsed_ms", 0))

    return result


# ---------------------------------------------------------------------------
# GET /healthz — liveness probe de Kubernetes
# Responde 200 siempre que el proceso esté vivo.
# ---------------------------------------------------------------------------
@app.get("/healthz", response_model=HealthResponse)
async def healthz():
    return {"status": "ok"}


# ---------------------------------------------------------------------------
# GET /readyz — readiness probe de Kubernetes
# Solo responde 200 si el motor es alcanzable.
# ---------------------------------------------------------------------------
@app.get("/readyz", response_model=HealthResponse)
async def readyz():
    motor_ok = await check_motor_health()
    if not motor_ok:
        from fastapi import HTTPException
        raise HTTPException(status_code=503, detail="Motor no disponible")
    return {"status": "ok"}


# ---------------------------------------------------------------------------
# GET /metrics — métricas agregadas en texto plano compatible con Prometheus
# ---------------------------------------------------------------------------
@app.get("/metrics")
async def get_metrics():
    # Formato texto plano simple (compatible con Prometheus si se prefiere)
    lines = [
        f"# HELP mancala_total_requests Peticiones totales recibidas",
        f"# TYPE mancala_total_requests counter",
        f"mancala_total_requests {metrics.total_requests}",
        f"mancala_alphabeta_requests {metrics.alphabeta_requests}",
        f"mancala_mcts_requests {metrics.mcts_requests}",
        f"mancala_avg_elapsed_ms {metrics.avg_elapsed_ms:.2f}",
    ]
    from fastapi.responses import PlainTextResponse
    return PlainTextResponse("\n".join(lines))