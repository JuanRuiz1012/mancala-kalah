# 03 – Paralelización con OpenMP e Instrumentación

## Estrategia elegida para Alfa-Beta: Root Parallelism

La estrategia implementada es **root parallelism**: los movimientos legales del nodo raíz se reparten entre hilos con `#pragma omp parallel for`. Cada hilo ejecuta una búsqueda Alfa-Beta secuencial completa sobre su subárbol asignado.

```cpp
#pragma omp parallel for schedule(dynamic)
for (int i = 0; i < n_movimientos; i++) {
    // Cada iteración es independiente: sin datos compartidos
    vals[i] = alphabeta(hijo[i], depth-1, -INF, INF, ...);
}
```

**Ventaja**: no requiere sincronización de cotas α/β entre hilos, lo que elimina contención.

**Costo de sincronización**: al no compartir cotas, cada hilo parte de α=-∞ y β=+∞, lo que reduce la eficacia de las podas respecto al algoritmo secuencial. El speedup teórico es el número de movimientos en la raíz, pero en la práctica es menor porque los subárboles tienen tamaños muy distintos (desequilibrio de carga). Se usa `schedule(dynamic)` para mitigarlo.

## Estrategia elegida para MCTS: Root Parallelism

Cada hilo construye su propio árbol MCTS de forma completamente independiente sobre la misma posición raíz. Al terminar, se suman las visitas y victorias de los hijos raíz de todos los árboles y se elige el movimiento con más visitas acumuladas.

```cpp
#pragma omp parallel for
for (int t = 0; t < nthreads; t++) {
    // Árbol propio por hilo, sin mutex, sin atomic
    roots[t] = new MCTSNode(board, ...);
    run_mcts(roots[t], simulations / nthreads, rng_por_hilo);
}
// Combinar: sumar visitas y victorias por movimiento
```

**Ventaja**: cero contención durante la búsqueda; la única sincronización ocurre al final (combinación de resultados).

**Costo de sincronización**: la combinación final es O(nthreads × movimientos_raíz), completamente despreciable. El costo real es que cada hilo solo hace `simulations/nthreads` simulaciones, por lo que el árbol de cada hilo es más superficial. La calidad estadística se mantiene porque el promedio de muchos árboles converge al mismo resultado.

## Instrumentación local

Métricas medidas con `omp_get_wtime()` en modo benchmark (ejecutable `mancala_bench`):

$$T(p) = \text{tiempo de pared con } p \text{ hilos}$$

$$S(p) = \frac{T(1)}{T(p)} \quad \text{(speedup)}$$

$$E(p) = \frac{S(p)}{p} \quad \text{(eficiencia)}$$

### Tablas de resultados

*(Completar con valores reales obtenidos. Los encabezados y el formato están listos para copiar los números del benchmark.)*

#### Alfa-Beta depth=8

| Hilos $p$ | $T(p)$ ms | $S(p)$ | $E(p)$ | Nodos explorados | Podas |
|---|---|---|---|---|---|
| 1 | — | 1.00 | 1.00 | — | — |
| 2 | — | — | — | — | — |
| 4 | — | — | — | — | — |
| 8 | — | — | — | — | — |

#### Alfa-Beta depth=12

| Hilos $p$ | $T(p)$ ms | $S(p)$ | $E(p)$ | Nodos explorados | Podas |
|---|---|---|---|---|---|
| 1 | — | 1.00 | 1.00 | — | — |
| 2 | — | — | — | — | — |
| 4 | — | — | — | — | — |
| 8 | — | — | — | — | — |

#### MCTS simulations=10 000

| Hilos $p$ | $T(p)$ ms | $S(p)$ | $E(p)$ | Rollouts | Prof. media |
|---|---|---|---|---|---|
| 1 | — | 1.00 | 1.00 | — | — |
| 2 | — | — | — | — | — |
| 4 | — | — | — | — | — |
| 8 | — | — | — | — | — |

#### MCTS simulations=100 000

| Hilos $p$ | $T(p)$ ms | $S(p)$ | $E(p)$ | Rollouts | Prof. media |
|---|---|---|---|---|---|
| 1 | — | 1.00 | 1.00 | — | — |
| 2 | — | — | — | — | — |
| 4 | — | — | — | — | — |
| 8 | — | — | — | — | — |

### Gráfica de speedup

*(Insertar captura del gráfico speedup vs. número de hilos para ambos algoritmos.)*

## Herramientas de profiling usadas

### perf stat

```bash
OMP_NUM_THREADS=8 perf stat -e cycles,instructions,cache-misses \
  ./mancala_bench --algo alphabeta --depth 12 --positions bench/suite.txt
```

*(Insertar captura de la salida de perf stat.)*

### htop

*(Insertar captura de htop mostrando la ocupación de los núcleos durante la búsqueda paralela.)*

## Comparación directa Alfa-Beta vs MCTS

Sobre el mismo conjunto de posiciones con el mismo presupuesto de tiempo de pared:

| Algoritmo | Movimiento correcto (%) | Speedup con 8 hilos | Observación |
|---|---|---|---|
| Alfa-Beta depth=8 | 100% (óptimo) | — | Dependencia de podas reduce eficiencia |
| MCTS 100k sims | —% | — | Escala mejor porque simulaciones son independientes |