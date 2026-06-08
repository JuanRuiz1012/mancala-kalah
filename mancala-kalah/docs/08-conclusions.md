# 08 – Conclusiones

## Resumen del proyecto

Se implementó un motor completo del juego Mancala Kalah(6,4) con dos algoritmos de inteligencia artificial: Minimax con poda Alfa-Beta y Monte Carlo Tree Search con UCT. Ambos algoritmos tienen versiones secuencial y paralela usando OpenMP con root parallelism. El motor se expone como servicio HTTP, se envuelve con una API REST en FastAPI y se entrega al usuario a través de un frontend en HTML/JS servido por nginx. Los tres componentes se despliegan como contenedores Docker independientes, orquestados con Kubernetes tanto en local como en la nube, con un pipeline de CI/CD en GitHub Actions y análisis de calidad con SonarQube.

---

## Qué funcionó bien

**Separación de componentes.** Tener el motor, el backend y el frontend como contenedores independientes facilitó desarrollar y depurar cada capa por separado. Fue posible probar el motor directamente con `curl` sin levantar el resto del sistema.

**Root parallelism como estrategia de paralelización.** Aunque no es la estrategia con mayor eficiencia teórica, es la más simple de implementar correctamente sin bugs de condición de carrera. En el caso de MCTS fue especialmente efectiva porque cada árbol es completamente independiente y la combinación final es trivial.

**Pydantic en el backend.** La validación automática con Pydantic eliminó la necesidad de código de validación manual y produjo errores HTTP 422 descriptivos cuando el frontend enviaba datos incorrectos.

**Multi-stage build en el Dockerfile del motor.** Separar la etapa de compilación de la imagen final redujo el tamaño de la imagen del motor de ~700 MB a ~50 MB, lo que acelera los despliegues.

**Google Test para el motor.** Las pruebas unitarias detectaron un bug en la lógica de captura antes del primer despliegue, ahorrando tiempo de depuración con el sistema completo levantado.

---

## Limitaciones encontradas

**Root parallelism reduce la eficacia de las podas en Alfa-Beta.** Al no compartir cotas α/β entre hilos, cada subárbol se explora como si fuera el primero, reduciendo el número de podas por hilo respecto al algoritmo secuencial. El speedup real es inferior al teórico.

**El motor no escala horizontalmente.** Se decidió tener una sola réplica del motor para evitar la complejidad de sincronizar estado. En producción real habría que usar nodos dedicados por réplica del motor.

**El servidor HTTP del motor es secuencial.** Atiende una petición a la vez. En un escenario de varios usuarios simultáneos esto crea cola. La solución correcta sería agregar un thread pool para las conexiones entrantes.

**Sin persistencia.** Las métricas del endpoint `/metrics` del backend se resetean al reiniciar el pod. Para un sistema de producción se necesitaría un volumen persistente o exportar las métricas a Prometheus/Grafana.

---

## Retos técnicos y cómo se resolvieron

**Parseo JSON sin librerías en C++.** El enunciado no permite dependencias externas para el motor, lo que obligó a implementar un parser JSON mínimo a mano. Funciona para el schema fijo del proyecto pero no es un parser genérico.

**Sincronización de semillas aleatorias en MCTS paralelo.** Si todos los hilos usan la misma semilla, generan la misma secuencia y los árboles son idénticos, eliminando el beneficio del paralelismo. Se solucionó sumando `t * 1000` a la semilla de cada hilo.

**CORS en producción.** Al cambiar la URL del frontend de local a nube, las peticiones fallaban por el preflight CORS. Se resolvió configurando `ALLOWED_ORIGINS` como variable de entorno en el ConfigMap en lugar de hardcodearlo en el código.

**Bug de ninja-build en el Dockerfile.** El CMakeLists original usaba Ninja como generador pero el Dockerfile no lo instalaba, causando fallos de compilación. Se corrigió instalando `ninja-build` explícitamente.

---

## Lecciones aprendidas

- Definir el contrato de la API (schema JSON) antes de implementar el motor y el frontend evita mucho retrabajo.
- Las pruebas unitarias del motor detectaron bugs antes del primer despliegue, ahorrando tiempo de depuración.
- El pipeline de CI que compila y testea automáticamente dio confianza para hacer cambios frecuentes.
- Kubernetes añade complejidad operacional significativa que no se justifica para un solo desarrollador. Docker Compose es suficiente para desarrollo y demostraciones. Kubernetes aporta valor real cuando se necesitan múltiples réplicas, rolling updates y probes de salud automáticos.
- La separación estricta en contenedores, aunque más trabajo inicial, hace el sistema mucho más fácil de mantener y depurar a largo plazo.

---

## Recomendaciones de mejoras futuras

- Implementar **YBWC (Young Brothers Wait Concept)** para Alfa-Beta paralelo: explorar el primer hijo secuencialmente para obtener una buena cota β antes de paralelizar los hermanos. Reduciría la pérdida de podas.
- Agregar **tree parallelization** para MCTS con virtual loss, que permite explorar más regiones del árbol con el mismo presupuesto de simulaciones.
- Hacer el servidor HTTP del motor **multi-hilo** (un hilo por conexión) para atender múltiples usuarios simultáneamente sin espera.
- Integrar **Prometheus y Grafana** para visualizar las métricas de rendimiento en tiempo real durante los benchmarks.
- Implementar **iterative deepening** en Alfa-Beta para convertirlo de anytime: permite detenerlo cuando se acaba el tiempo y devolver la mejor jugada encontrada hasta ese momento, igual que MCTS.
