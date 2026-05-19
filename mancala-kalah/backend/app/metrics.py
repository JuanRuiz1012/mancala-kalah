"""
Métricas agregadas en memoria para el endpoint GET /metrics.
No requiere base de datos: los contadores se resetean al reiniciar el pod.
"""

from dataclasses import dataclass, field


@dataclass
class AppMetrics:
    total_requests: int = 0
    alphabeta_requests: int = 0
    mcts_requests: int = 0
    total_elapsed_ms: float = 0.0

    def record(self, algo: str, elapsed_ms: int) -> None:
        """Registra una petición completada."""
        self.total_requests += 1
        self.total_elapsed_ms += elapsed_ms
        if algo == "alphabeta":
            self.alphabeta_requests += 1
        elif algo == "mcts":
            self.mcts_requests += 1

    @property
    def avg_elapsed_ms(self) -> float:
        if self.total_requests == 0:
            return 0.0
        return self.total_elapsed_ms / self.total_requests


# Instancia global compartida por la aplicación
metrics = AppMetrics()