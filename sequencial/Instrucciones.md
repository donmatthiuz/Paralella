# Compilar el sistema de particulas

Instalar SDL

´´´bash

sudo apt update
sudo apt install libsdl2-dev

´´´
Compilar 

´´´bash

gcc particles.c -O3 -march=native -o gray_scott -lSDL2 -lm

´´´

Correr

´´´bash

./gray_scott

´´´

