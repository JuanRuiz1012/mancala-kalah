"""
Lógica para comunicarse con el contenedor del motor C++/OpenMP.
El motor vive en la red interna del clúster; su URL se toma de la variable
de entorno MOTOR_URL (default: http://motor-svc:9000).
"""

import os
import httpx
from fastapi import HTTPException

# URL base del servicio motor (configurable por variable de entorno o ConfigMap)
MOTOR_URL = os.getenv("MOTOR_URL", "http://motor-svc:9000")

# Timeout en segundos para las peticiones al motor
MOTOR_TIMEOUT = float(os.getenv("MOTOR_TIMEOUT_S", "30"))


async def call_motor(payload: dict) -> dict:
    """
    Envía el payload al endpoint /move del motor y devuelve la respuesta.
    Lanza HTTPException con código apropiado si el motor falla o no responde.
    """
    url = f"{MOTOR_URL}/move"
    try:
        async with httpx.AsyncClient(timeout=MOTOR_TIMEOUT) as client:
            response = await client.post(url, json=payload)
    except httpx.ConnectError:
        raise HTTPException(status_code=503, detail="Motor no disponible")
    except httpx.TimeoutException:
        raise HTTPException(status_code=503, detail="Motor no respondió a tiempo")

    if response.status_code != 200:
        raise HTTPException(
            status_code=500,
            detail=f"Motor devolvió error {response.status_code}"
        )

    return response.json()


async def check_motor_health() -> bool:
    """
    Consulta GET /healthz del motor.
    Devuelve True si el motor está vivo, False en caso contrario.
    """
    try:
        async with httpx.AsyncClient(timeout=3.0) as client:
            resp = await client.get(f"{MOTOR_URL}/healthz")
        return resp.status_code == 200
    except Exception:
        return False