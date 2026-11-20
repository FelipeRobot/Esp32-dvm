# Entorno de compilacion reproducible: ESP32 + PlatformIO (Arduino Framework).
#
# Todas las versiones estan fijadas:
#   - Imagen base por digest (multi-arquitectura)
#   - PlatformIO Core con version explicita
#   - Plataforma espressif32 y librerias fijadas en platformio.ini
#
# La plataforma, el toolchain y las librerias se descargan DURANTE el build
# de la imagen: el contenedor compila sin acceso a red y de forma identica
# en cualquier maquina.

FROM python:3.12-slim-bookworm@sha256:935c3c5b12357a7e10caa9c1e6ca5692344d97cf229832ef584be2cec9fc03df

# ------------------------------- Argumentos ----------------------------------
# UID/GID del usuario del contenedor. Deben coincidir con los del usuario del
# host para que los artefactos de .pio/ queden accesibles al cargar desde el
# host sin fricciones de permisos. docker-compose.yml los pasa desde el host.
ARG USER_UID=1000
ARG USER_GID=1000
ARG PLATFORMIO_VERSION=6.2.0

# ------------------------- Herramientas del sistema --------------------------
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        git \
    && rm -rf /var/lib/apt/lists/*

# ----------------------------- PlatformIO Core --------------------------------
RUN pip install --no-cache-dir "platformio==${PLATFORMIO_VERSION}"

# -------------------------- Usuario sin privilegios ---------------------------
RUN groupadd --gid "${USER_GID}" pio \
    && useradd --uid "${USER_UID}" --gid "${USER_GID}" --create-home pio \
    && mkdir -p /tmp/build \
    && chown pio:pio /tmp/build

# ----------------------- Preinstalacion de paquetes ---------------------------
# pio run de smoke test: descarga espressif32, toolchains (~2 GB) y librerias
# al HOME del usuario pio y valida de paso que la imagen compila.
USER pio
WORKDIR /tmp/build
COPY --chown=pio:pio platformio.ini ./
COPY --chown=pio:pio src ./src
RUN pio run \
    && rm -rf /tmp/build

WORKDIR /workspace
