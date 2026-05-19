FROM python:3.11-slim

WORKDIR /app

# Instalar dependencias primero (capa cacheada si requirements no cambia)
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copiar el código de la aplicación
COPY app/ ./app/

# Puerto de la API pública
EXPOSE 8000

# Uvicorn como servidor ASGI
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000"]