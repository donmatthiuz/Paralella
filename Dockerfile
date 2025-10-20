# Imagen base: Ubuntu 22.04 (sin latest)
FROM ubuntu:22.04

# Instalar herramientas de compilación, OpenMP y OpenMPI
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    gdb \
    libgomp1 \
    openmpi-bin \
    libopenmpi-dev \
    vim \
    git && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Crear y establecer directorio de trabajo
WORKDIR /usr/src/app

# Copiar todo el código fuente desde la carpeta local "program"
COPY program/ ./program/

# Establecer el directorio por defecto al interior del código
WORKDIR /usr/src/app/program

# Comando por defecto: abrir bash
CMD ["/bin/bash"]
