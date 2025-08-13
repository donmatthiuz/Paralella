#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "entidades.h"

#define RESET   "\033[0m"
#define CYAN    "\033[0;36m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define RED     "\033[0;31m"
#define BLUE    "\033[0;34m"

char estado_semaforo(int estado) {
    if (estado == 0) return 'R';
    if (estado == 1) return 'A';
    return 'V';
}

void mostrar_lista(Interseccion inter, int iteracion, int fila, int columna) {
    printf("%s=== Iteración %d - Intersección [%d,%d] ===%s\n", CYAN, iteracion, fila, columna, RESET);
    
    // Mostrar autos activos
    int autos_activos = 0;
    for (int i = 0; i < inter.cantidadAutos; i++) {
        if (inter.autos[i].activo) {
            printf("%s Vehículo %d%s - Posición: %d, Carril: %d, Dirección: [%d,%d]\n", 
                BLUE, inter.autos[i].numero, RESET,
                inter.autos[i].posicion, 
                inter.autos[i].carril,
                inter.autos[i].dir_fila, 
                inter.autos[i].dir_col);
            autos_activos++;
        }
    }
    
    if (autos_activos == 0) {
        printf("   Sin vehículos activos\n");
    }
    
    // Mostrar semáforos
    for (int i = 0; i < inter.cantidadSemaforos; i++) {
        char* color = RESET;
        if (inter.semaforos[i].estado == 0) color = RED;
        else if (inter.semaforos[i].estado == 1) color = YELLOW;
        else if (inter.semaforos[i].estado == 2) color = GREEN;

        printf("Semáforo %d (Carril %d) - Estado: %s%c%s (tiempo restante: %d)\n",
            inter.semaforos[i].id,
            inter.semaforos[i].carril,
            color,
            estado_semaforo(inter.semaforos[i].estado),
            RESET,
            inter.semaforos[i].tiempo_restante
        );
    }
    printf("\n");
}


void actualizar_semaforos(Interseccion* inter) {
    // Verificar si algún semáforo necesita cambiar de estado
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        Semaforo* sem = &inter->semaforos[i];
        sem->tiempo_restante--;
    }
    
    // Encontrar el semáforo que está en verde (si hay alguno)
    int semaforo_verde = -1;
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        if (inter->semaforos[i].estado == 2) {
            semaforo_verde = i;
            break;
        }
    }
    
    // Si hay un semáforo en verde, verificar si debe cambiar
    if (semaforo_verde != -1) {
        Semaforo* sem_verde = &inter->semaforos[semaforo_verde];
        if (sem_verde->tiempo_restante <= 0) {
            // Cambiar a amarillo
            sem_verde->estado = 1;
            sem_verde->tiempo_restante = 2;
            printf("    Semáforo %d (Carril %d) cambia a AMARILLO\n", sem_verde->id, sem_verde->carril);
        }
    }
    
    // los amarillos cambiarlos a rojo
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        Semaforo* sem = &inter->semaforos[i];
        if (sem->estado == 1 && sem->tiempo_restante <= 0) {
            // Cambiar a rojo y activar el otro semáforo
            sem->estado = 0;
            sem->tiempo_restante = 5;
            printf("    Semáforo %d (Carril %d) cambia a ROJO\n", sem->id, sem->carril);
            
            // Activar el otro semáforo (alternar)
            int otro_semaforo = (i + 1) % inter->cantidadSemaforos;
            inter->semaforos[otro_semaforo].estado = 2;
            inter->semaforos[otro_semaforo].tiempo_restante = 5;
            printf("    Semáforo %d (Carril %d) cambia a VERDE\n", 
                inter->semaforos[otro_semaforo].id, 
                inter->semaforos[otro_semaforo].carril);
        }
    }
    

    int todos_rojos = 1;
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        if (inter->semaforos[i].estado != 0) {
            todos_rojos = 0;
            break;
        }
    }
    
    if (todos_rojos) {
        // Activar el primer semáforo
        inter->semaforos[0].estado = 2;
        inter->semaforos[0].tiempo_restante = 5;
        printf("    Iniciando: Semáforo 0 (Carril 0) cambia a VERDE\n");
    }
}

// Mover autos entre intersecciones con mejor control de límites
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
                    // Solo si esta en verde
                    if (sem->estado == 2) {
                        auto_actual->posicion++;
                        printf("   Auto %d avanza desde posición 0 (semáforo verde)\n", auto_actual->numero);
                    }
                } else {
                    // Una vez en movimiento, continúa avanzando
                    auto_actual->posicion++;
                }

                // Si sale de la intersección
                if (auto_actual->posicion >= inter->longitud) {
                    int nf = f + auto_actual->dir_fila;
                    int nc = c + auto_actual->dir_col;

                    // Verificar límites del grid
                    if (nf >= 0 && nf < nFilas && nc >= 0 && nc < nColumnas) {
                        Interseccion* destino = &grid[nf][nc];
                        // Buscar un espacio libre en destino
                        int colocado = 0;
                        for (int k = 0; k < destino->cantidadAutos; k++) {
                            if (!destino->autos[k].activo) {
                                destino->autos[k] = *auto_actual;
                                destino->autos[k].posicion = 0;
                                destino->autos[k].activo = 1;
                                colocado = 1;
                                break;
                            }
                        }
                        if (colocado) {
                            printf("%sAuto %d cruzó de intersección [%d,%d] a [%d,%d]%s\n", 
                                GREEN, auto_actual->numero, f, c, nf, nc, RESET);
                        } else {
                            printf("%s Auto %d no pudo entrar a [%d,%d] (sin espacios libres)%s\n", 
                                YELLOW, auto_actual->numero, nf, nc, RESET);
                        }
                    } else {
                        printf("%s Auto %d sale del sistema desde [%d,%d] (límite del grid)%s\n", 
                            RED, auto_actual->numero, f, c, RESET);
                    }
                    // Desactivar auto en intersección actual
                    auto_actual->activo = 0;
                }
            }
        }
    }
}

// Función para contar autos activos en todo el sistema
int contar_autos_activos(Interseccion** grid, int nFilas, int nColumnas) {
    int total = 0;
    for (int f = 0; f < nFilas; f++) {
        for (int c = 0; c < nColumnas; c++) {
            for (int i = 0; i < grid[f][c].cantidadAutos; i++) {
                if (grid[f][c].autos[i].activo) total++;
            }
        }
    }
    return total;
}


int main() {
    int nFilas = 2;
    int nColumnas = 2;
    int nAutos = 5;
    int nSemaforos = 2;
    int longitud = 5;

    printf("%s SIMULACIÓN DE TRÁFICO INICIADA %s\n", CYAN, RESET);
    printf("Grid: %dx%d, Autos: %d, Longitud intersección: %d\n\n", nFilas, nColumnas, nAutos, longitud);

    // Crear grid
    Interseccion** grid = malloc(nFilas * sizeof(Interseccion*));
    for (int f = 0; f < nFilas; f++) {
        grid[f] = malloc(nColumnas * sizeof(Interseccion));
        for (int c = 0; c < nColumnas; c++) {
            grid[f][c] = crear_interseccion(nAutos, nSemaforos, longitud);

            // Solo la primera intersección tiene autos inicialmente
            if (f == 0 && c == 0) {
                for (int i = 0; i < nAutos; i++) {
                    int dir_fila = (i % 2); // alternando direcciones
                    int dir_col = ((i+1) % 2); 
                    grid[f][c].autos[i] = crear_auto(i, 0, i % nSemaforos, dir_fila, dir_col);
                }
            }

            // Semáforos con inicialización coordinada
            for (int i = 0; i < nSemaforos; i++) {
                if (i == 0) {
                    // Primer semáforo inicia en verde
                    grid[f][c].semaforos[i] = crear_semaforo(i, 2, i);
                    grid[f][c].semaforos[i].tiempo_restante = 5;
                } else {
                    // Otros semáforos inician en rojo
                    grid[f][c].semaforos[i] = crear_semaforo(i, 0, i);
                    grid[f][c].semaforos[i].tiempo_restante = 5;
                }
            }
        }
    }

    // Simulación
    int iteracion = 1;
    int max_iteraciones = 20;
    
    while (iteracion <= max_iteraciones) {
        printf("%s--- ESTADO ACTUAL DEL SISTEMA ---%s\n", CYAN, RESET);
        
        // Mostrar estado de todas las intersecciones
        for (int f = 0; f < nFilas; f++) {
            for (int c = 0; c < nColumnas; c++) {
                mostrar_lista(grid[f][c], iteracion, f, c);
            }
        }

       
        int autos_activos = contar_autos_activos(grid, nFilas, nColumnas);
        printf("%sAutos activos en el sistema: %d%s\n\n", YELLOW, autos_activos, RESET);

        // Si no hay autos activos, terminar simulación
        if (autos_activos == 0) {
            printf("%s SIMULACIÓN TERMINADA - No hay más autos en el sistema%s\n", GREEN, RESET);
            break;
        }

        // Ejecutar un paso de simulación
        mover_autos(grid, nFilas, nColumnas);
        
        for (int f = 0; f < nFilas; f++) {
            for (int c = 0; c < nColumnas; c++) {
                actualizar_semaforos(&grid[f][c]);
            }
        }

        sleep(1);
        iteracion++;
        
       
        printf("%s===================================================%s\n", CYAN, RESET);
    }

    printf("%s SIMULACIÓN COMPLETADA DESPUÉS DE %d ITERACIONES%s\n", GREEN, iteracion-1, RESET);

    // Liberar memoria
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