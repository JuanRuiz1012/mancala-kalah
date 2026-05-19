# Etapa 1: compilación
FROM ubuntu:22.04 AS builder

# Instalar dependencias de compilación
RUN apt-get update && apt-get install -y \
     cmake \
     g++ \
     libgomp1 \
     make \
     wget \
     unzip \
     && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .

# Compilar en modo Release
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
     && cmake --build build --target mancala_motor -j$(nproc)

# ---------------------------------------------------------------------------
# Etapa 2: imagen final mínima
# ---------------------------------------------------------------------------
FROM ubuntu:22.04

# Solo la librería de runtime de OpenMP
RUN apt-get update && apt-get install -y libgomp1 \
     && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/build/mancala_motor .

# Puerto por defecto del motor
ENV MOTOR_PORT=9000
EXPOSE 9000

CMD ["./mancala_motor"]