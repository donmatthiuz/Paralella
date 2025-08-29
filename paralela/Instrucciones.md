# Compilar el sistema de particulas

Instalar SDL

´´´bash

sudo apt update
sudo apt install libsdl2-dev

´´´
Compilar 

´´´bash
gcc -O3 -march=native -fopenmp -o particles_omp particles.c `sdl2-config --cflags --libs` -lm
´´´

Correr

´´´bash

./gray_scott

´´´

