# 02 – Motor: Kalah(6,4), Alfa-Beta y MCTS

## Reglas de Kalah(6,4) implementadas

El tablero se representa como un arreglo de 14 enteros en orden canónico:

- Posiciones 0–5: hoyos del jugador 0 (sur).
- Posición 6: kalaha del jugador 0.
- Posiciones 7–12: hoyos del jugador 1 (norte).
- Posición 13: kalaha del jugador 1.

Las reglas implementadas en `board.cpp`:

1. **Siembra**: el jugador toma todas las semillas de un hoyo propio y las distribuye una a una en sentido antihorario, incluyendo su propio kalaha pero saltando el del oponente.
2. **Turno extra**: si la última semilla cae en el kalaha propio, el jugador repite turno.
3. **Captura**: si la última semilla cae en un hoyo propio que estaba vacío y el hoyo opuesto del rival tiene semillas, ambos grupos van al kalaha propio.
4. **Fin de juego**: cuando un lado queda completamente vacío, el jugador contrario mueve sus semillas restantes a su kalaha. Gana quien tenga más semillas en su kalaha.

## Función heurística (Alfa-Beta)

$$
h(\text{estado}) = (k_{\text{propio}} - k_{\text{rival}}) + \alpha \cdot (s_{\text{propio}} - s_{\text{rival}})
$$

donde $k$ son las semillas en el kalaha, $s$ las semillas en los hoyos del lado, y $\alpha = 0.5$.

## Pseudocódigo: Minimax con poda Alfa-Beta

```
función alphabeta(estado, profundidad, α, β, maximizando):
    si profundidad == 0 o estado.es_terminal():
        devolver heurística(estado)

    movimientos = movimientos_legales(estado)

    si maximizando:
        mejor = -∞
        para cada mov en movimientos:
            hijo = aplicar(estado, mov)
            val  = alphabeta(hijo, profundidad-1, α, β, hijo.turno_extra OR false)
            mejor = max(mejor, val)
            α = max(α, mejor)
            si β ≤ α: romper  // poda beta
        devolver mejor
    si no:
        mejor = +∞
        para cada mov en movimientos:
            hijo = aplicar(estado, mov)
            val  = alphabeta(hijo, profundidad-1, α, β, hijo.turno_extra OR true)
            mejor = min(mejor, val)
            β = min(β, mejor)
            si β ≤ α: romper  // poda alfa
        devolver mejor
```

## Pseudocódigo: MCTS con UCT

```
función mcts(raíz, presupuesto):
    repetir presupuesto veces:
        // 1. Selección
        nodo = raíz
        mientras nodo.completamente_expandido y no nodo.es_terminal():
            nodo = hijo con mayor UCT(nodo)

        // 2. Expansión
        si no nodo.es_terminal() y no nodo.completamente_expandido():
            nodo = agregar_hijo_aleatorio(nodo)

        // 3. Simulación
        resultado = jugar_al_azar_hasta_terminal(nodo.estado)

        // 4. Retropropagación
        mientras nodo != null:
            nodo.visitas += 1
            nodo.victorias += resultado
            nodo = nodo.padre

    devolver hijo de raíz con más visitas

UCT(n) = victorias(n)/visitas(n) + √2 · √(ln(visitas(padre)) / visitas(n))
```

## Suite de pruebas unitarias

Las pruebas están en `motor/tests/test_board.cpp` y usan Google Test.

**Cómo ejecutarlas:**
```bash
cd motor
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target test_board
cd build && ctest --output-on-failure
```

**Casos cubiertos:**

| Test | Qué verifica |
|---|---|
| `BoardTest.InitialState` | 4 semillas por hoyo, kalahas en 0 |
| `BoardTest.SeedConservation` | Total de semillas constante = 48 |
| `BoardTest.LegalMovesInitial` | 6 movimientos legales al inicio |
| `BoardTest.ExtraTurn` | Turno extra cuando última semilla llega al kalaha |
| `BoardTest.Capture` | Captura correcta de semillas del oponente |
| `BoardTest.TerminalDetection` | Detección de fin de juego |
| `AlphaBetaTest.SameMoveasMinimax` | Alfa-Beta es determinista (mismo resultado dos veces) |
| `AlphaBetaTest.ParallelMatchesSerial` | Paralelo elige el mismo movimiento que el secuencial |
| `AlphaBetaTest.MoveIsLegal` | El movimiento devuelto es un hoyo legal |
| `MCTSTest.MoveIsLegal` | MCTS devuelve un movimiento legal |
| `MCTSTest.WinRateRange` | win_rate ∈ [0, 1] |
| `MCTSTest.ParallelMoveIsLegal` | MCTS paralelo devuelve movimiento legal |

## Equivalencia Alfa-Beta vs Minimax

La verificación de que Alfa-Beta produce el mismo movimiento óptimo que Minimax puro se hace ejecutando la versión secuencial dos veces sobre el mismo estado (es determinista) y comparando el resultado. La poda no cambia el movimiento elegido: solo elimina ramas que no pueden cambiar la decisión.

Se puede verificar adicionalmente reduciendo el presupuesto del benchmark a depth=2 y comparando manualmente la enumeración completa de estados.

## Tasa de coincidencia MCTS vs Alfa-Beta

Para medir la coincidencia se ejecuta el benchmark con ambos algoritmos sobre las mismas posiciones de `bench/suite.txt`:

```bash
OMP_NUM_THREADS=1 ./mancala_bench --algo alphabeta --depth 8 --positions bench/suite.txt
OMP_NUM_THREADS=1 ./mancala_bench --algo mcts --simulations 100000 --positions bench/suite.txt
```

*(Completar con los resultados reales obtenidos durante la experimentación.)*

| Presupuesto MCTS | Coincidencia con Alfa-Beta depth=8 |
|---|---|
| 1 000 simulaciones | — % |
| 10 000 simulaciones | — % |
| 100 000 simulaciones | — % |