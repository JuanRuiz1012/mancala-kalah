# Informe del Proyecto: Motor paralelo de Mancala Kalah en Kubernetes

## Integrantes del grupo

| Nombre completo | Código | Correo institucional |
|---|---|---|
| Juan Felipe Ruiz | 2359397 | (juan.ruiz.lopez@correounivalle.edu.co) |
| (Nombre 2) | (Código) | (correo@correounivalle.edu.co) |
| (Nombre 3) | (Código) | (correo@correounivalle.edu.co) |
| (Nombre 4) | (Código) | (correo@correounivalle.edu.co) |

## Índice de archivos del informe

| # | Archivo | Contenido |
|---|---|---|
| 1 | [01-arquitectura.md](01-arquitectura.md) | Visión general, diagrama de orquestación, API REST, CORS |
| 2 | [02-motor.md](02-motor.md) | Reglas Kalah(6,4), Minimax+Alfa-Beta, MCTS con UCT, pruebas |
| 3 | [03-paralelizacion.md](03-paralelizacion.md) | Estrategia OpenMP, tablas T(p)/S(p)/E(p), speedup, profiling |
| 4 | [04-despliegue-local.md](04-despliegue-local.md) | Dockerfiles, docker-compose, manifiestos K8s local |
| 5 | [05-despliegue-nube.md](05-despliegue-nube.md) | Proveedor, manifiestos YAML nube, evidencias kubectl |
| 6 | [06-cicd.md](06-cicd.md) | GitHub Actions, SonarQube en YAML, pipeline |
| 7 | [07-analisis-comparativo.md](07-analisis-comparativo.md) | Local vs nube: latencia p50/p95, throughput |
| 8 | [08-conclusions.md](08-conclusions.md) | Limitaciones, retos, lecciones aprendidas |

## Mapeo criterio de rúbrica → archivo

| Criterio de la rúbrica | Archivo donde se evalúa |
|---|---|
| Motores de Mancala: corrección | [02-motor.md](02-motor.md) |
| Paralelización con OpenMP | [03-paralelizacion.md](03-paralelizacion.md) |
| Instrumentación local | [03-paralelizacion.md](03-paralelizacion.md) |
| Separación de componentes | [01-arquitectura.md](01-arquitectura.md) |
| Despliegue local | [04-despliegue-local.md](04-despliegue-local.md) |
| Despliegue en la nube con Kubernetes | [05-despliegue-nube.md](05-despliegue-nube.md) |
| CI/CD y calidad de código | [06-cicd.md](06-cicd.md) |
| Análisis comparativo local vs. nube | [07-analisis-comparativo.md](07-analisis-comparativo.md) |
| Claridad de explicaciones | Transversal a todos los archivos |
| Conclusiones | [08-conclusions.md](08-conclusions.md) |