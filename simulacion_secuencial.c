#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "entidades.h"

#define RESET   "\033[0m"
#define CYAN    "\033[0;36m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define RED     "\033[0;31m"

char estado_semaforo(int estado) {
    if (estado == 0) return 'R';
    if (estado == 1) return 'A';
    return 'V';
}

void mostrar_lista(Interseccion inter, int iteracion, int fila, int columna) {
    printf("%s--- Iteración %d - Intersección [%d,%d] --- %s\n", CYAN, iteracion, fila, columna, RESET);
    for (int i = 0; i < inter.cantidadAutos; i++) {
        printf("Vehículo %d - Posición: %d - Activo: %d\n", inter.autos[i].numero, inter.autos[i].posicion, inter.autos[i].activo);
    }
    for (int i = 0; i < inter.cantidadSemaforos; i++) {
        char* color = RESET;
        if (inter.semaforos[i].estado == 0) color = RED;
        else if (inter.semaforos[i].estado == 1) color = YELLOW;
        else if (inter.semaforos[i].estado == 2) color = GREEN;

        printf("Semáforo %d - Estado: %s%c%s\n",
            inter.semaforos[i].id,
            color,
            estado_semaforo(inter.semaforos[i].estado),
            RESET
        );
    }
    printf("\n");
}

void actualizar_semaforos(Interseccion* inter) {
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        Semaforo* sem = &inter->semaforos[i];
        sem->tiempo_restante--;

        if (sem->tiempo_restante <= 0) {
            if (sem->estado == 2) sem->estado = 1, sem->tiempo_restante = 2;
            else if (sem->estado == 1) sem->estado = 0, sem->tiempo_restante = 5;
            else sem->estado = 2, sem->tiempo_restante = 5;
        }
    }
}

void mover_autos(Interseccion** grid, int nFilas, int nColumnas) {
    for (int f = 0; f < nFilas; f++) {
        for (int c = 0; c < nColumnas; c++) {
            Interseccion* inter = &grid[f][c];
            for (int i = 0; i < inter->cantidadAutos; i++) {
                Auto* auto_actual = &inter->autos[i];
                if (!auto_actual->activo) continue;

                // Buscar semáforo del carril
                Semaforo* sem = NULL;
                for (int j = 0; j < inter->cantidadSemaforos; j++) {
                    if (inter->semaforos[j].carril == auto_actual->carril) {
                        sem = &inter->semaforos[j];
                        break;
                    }
                }
                if (!sem) continue;

                if (auto_actual->posicion == 0) {
                    if (sem->estado == 2) auto_actual->posicion++;
                } else {
                    auto_actual->posicion++;
                }

                
                if (auto_actual->posicion >= inter->longitud) {
                    int nf = f + auto_actual->dir_fila;
                    int nc = c + auto_actual->dir_col;

                    if (nf >= 0 && nf < nFilas && nc >= 0 && nc < nColumnas) {
                        // Buscar un carril libre en la nueva intersección (misma lógica de carril)
                        Interseccion* destino = &grid[nf][nc];
                        for (int k = 0; k < destino->cantidadAutos; k++) {
                            if (!destino->autos[k].activo) {
                                destino->autos[k] = *auto_actual;
                                destino->autos[k].posicion = 0;
                                break;
                            }
                        }
                    }
                    auto_actual->activo = 0;
                    printf("Auto %d cruzó de intersección [%d,%d] a [%d,%d]\n", auto_actual->numero, f, c, nf, nc);
                }
            }
        }
    }
}

// Main
int main() {
    int nFilas = 2;
    int nColumnas = 2;
    int nAutos = 5;
    int nSemaforos = 2;
    int longitud = 5;

  
    Interseccion** grid = malloc(nFilas * sizeof(Interseccion*));
    for (int f = 0; f < nFilas; f++) {
        grid[f] = malloc(nColumnas * sizeof(Interseccion));
        for (int c = 0; c < nColumnas; c++) {
            grid[f][c] = crear_interseccion(nAutos, nSemaforos, longitud);

            // Crear autos
            for (int i = 0; i < nAutos; i++) {
                int dir_fila = (i % 2); // algunos hacia abajo
                int dir_col = ((i+1) % 2); // algunos hacia derecha
                grid[f][c].autos[i] = crear_auto(i, 0, i % nSemaforos, dir_fila, dir_col);
            }

            // Crear semáforos
            for (int i = 0; i < nSemaforos; i++) {
                int estado = i % 3;
                grid[f][c].semaforos[i] = crear_semaforo(i, estado, i);
            }
        }
    }

   
    int iteracion = 1;
    while (iteracion <= 10) {
        for (int f = 0; f < nFilas; f++) {
            for (int c = 0; c < nColumnas; c++) {
                mostrar_lista(grid[f][c], iteracion, f, c);
            }
        }

        mover_autos(grid, nFilas, nColumnas);

        for (int f = 0; f < nFilas; f++) {
            for (int c = 0; c < nColumnas; c++) {
                actualizar_semaforos(&grid[f][c]);
            }
        }

        sleep(1);
        iteracion++;
    }

   
    for (int f = 0; f < nFilas; f++) {
        for (int c = 0; c < nColumnas; c++) {
            free(grid[f][c].autos);
            free(grid[f][c].semaforos);
        }
        free(grid[f]);
    }
    free(grid);

    return 0;
}
