# Mancala Kalah — Motor paralelo con Kubernetes

Motor de IA para el juego Mancala Kalah(6,4) con algoritmos **Alfa-Beta** y **MCTS**, paralelizados con **OpenMP**, expuestos como **API REST (FastAPI)** y desplegados con **Kubernetes**.

## Estructura del repositorio

```
mancala-kalah/
├── README.md
├── .gitignore
│
├── docs/
│   ├── README.md
│   ├── 01-arquitectura.md
│   ├── 02-motor.md
│   ├── 03-paralelizacion.md
│   ├── 04-despliegue-local.md
│   ├── 05-despliegue-nube.md
│   ├── 06-cicd.md
│   ├── 07-analisis-comparativo.md
│   └── 08-conclusions.md
│
├── motor/                           
│   ├── src/
│   │   ├── main.cpp
│   │   ├── board.hpp           
│   │   ├── board.cpp
│   │   ├── alphabeta.hpp           
│   │   ├── alphabeta.cpp
│   │   ├── mcts.hpp                
│   │   ├── mcts.cpp
│   │   └── bench.cpp             
│   ├── tests/
│   │   └── test_board.cpp       
│   ├── bench/
│   │   └── suite.txt               
│   ├── CMakeLists.txt
│   └── Dockerfile
│
├── backend/         
│   ├── app/
│   │   ├── main.py           
│   │   ├── schemas.py     
│   │   ├── motor_client.py  
│   │   └── metrics.py
│   ├── tests/
│   │   └── test_api.py
│   ├── requirements.txt
│   └── Dockerfile
│
├── frontend/                
│   ├── index.html
│   ├── style.css
│   ├── app.js           
│   ├── nginx.conf
│   └── Dockerfile
│
├── deploy/
│   ├── local/
│   │   ├── docker-compose.yml
│   │   ├── motor-deployment.yaml
│   │   ├── motor-service.yaml
│   │   ├── backend-deployment.yaml
│   │   ├── backend-service.yaml
│   │   ├── frontend-deployment.yaml
│   │   ├── frontend-service.yaml
│   │   └── configmap.yaml
│   └── cloud/
│       ├── motor-deployment.yaml
│       ├── motor-service.yaml
│       ├── backend-deployment.yaml
│       ├── backend-service.yaml
│       ├── frontend-deployment.yaml
│       ├── frontend-service.yaml
│       ├── frontend-ingress.yaml
│       └── configmap.yaml
│
└── .github/
    └── workflows/
        ├── ci.yml                  
        └── sonarqube.yml
```

## Inicio rápido (docker compose)

```bash
cd deploy/local
docker compose up --build
```

Abrir http://localhost:8080

## Documentación

Ver la carpeta [`docs/`](docs/README.md) para el informe completo del proyecto.

## Tecnologías

| Capa | Tecnología |
|---|---|
| Motor de IA | C++17, OpenMP |
| Backend | Python 3.11, FastAPI, Pydantic, httpx |
| Frontend | HTML5, JavaScript, nginx |
| Contenedores | Docker, docker compose |
| Orquestación | Kubernetes (minikube / GKE) |
| CI/CD | GitHub Actions, SonarQube |