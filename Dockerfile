FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

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

WORKDIR /usr/src/app
COPY program/ ./program/
WORKDIR /usr/src/app/program

CMD ["/bin/bash"]