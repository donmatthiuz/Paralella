#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include "entidades.h"

#define RESET   "\033[0m"
#define CYAN    "\033[0;36m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define RED     "\033[0;31m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"

char estado_semaforo(int estado) {
    if (estado == 0) return 'R';
    if (estado == 1) return 'A';
    return 'V';
}

void mostrar_lista(Interseccion inter, int iteracion, int fila, int columna) {
    #pragma omp critical (mostrar_output)
    {
        printf("%s=== Iteración %d - Intersección [%d,%d] [Hilo: %d] ===%s\n", 
               CYAN, iteracion, fila, columna, omp_get_thread_num(), RESET);
        
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
}

void actualizar_semaforos(Interseccion* inter) {
    #pragma omp parallel for num_threads(2) if(inter->cantidadSemaforos > 1)
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        #pragma omp atomic
        inter->semaforos[i].tiempo_restante--;
    }
    
    // Encontrar el semáforo que está en verde (operación crítica)
    int semaforo_verde = -1;
    #pragma omp parallel for
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        if (inter->semaforos[i].estado == 2) {
            #pragma omp critical
            {
                if (semaforo_verde == -1) {
                    semaforo_verde = i;
                }
            }
        }
    }
    
    // Cambios de estado de semáforos (secuencial para evitar condiciones de carrera)
    if (semaforo_verde != -1) {
        Semaforo* sem_verde = &inter->semaforos[semaforo_verde];
        if (sem_verde->tiempo_restante <= 0) {
            sem_verde->estado = 1;
            sem_verde->tiempo_restante = 2;
            #pragma omp critical (mostrar_output)
            printf("    [Hilo %d] Semáforo %d (Carril %d) cambia a AMARILLO\n", 
                   omp_get_thread_num(), sem_verde->id, sem_verde->carril);
        }
    }
    
    // Procesar semáforos amarillos
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        Semaforo* sem = &inter->semaforos[i];
        if (sem->estado == 1 && sem->tiempo_restante <= 0) {
            sem->estado = 0;
            sem->tiempo_restante = 5;
            #pragma omp critical (mostrar_output)
            printf("    [Hilo %d] Semáforo %d (Carril %d) cambia a ROJO\n", 
                   omp_get_thread_num(), sem->id, sem->carril);
            
            int otro_semaforo = (i + 1) % inter->cantidadSemaforos;
            inter->semaforos[otro_semaforo].estado = 2;
            inter->semaforos[otro_semaforo].tiempo_restante = 5;
            #pragma omp critical (mostrar_output)
            printf("    [Hilo %d] Semáforo %d (Carril %d) cambia a VERDE\n", 
                omp_get_thread_num(),
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
        inter->semaforos[0].estado = 2;
        inter->semaforos[0].tiempo_restante = 5;
        #pragma omp critical (mostrar_output)
        printf("    [Hilo %d] Iniciando: Semáforo 0 (Carril 0) cambia a VERDE\n", 
               omp_get_thread_num());
    }
}

// Declaraciones de funciones
int contar_autos_activos(Interseccion** grid, int nFilas, int nColumnas);

// Función auxiliar para procesar un auto individual
void procesar_auto_individual(Auto* auto_actual, Interseccion* inter, 
                             Interseccion** grid, int nFilas, int nColumnas, 
                             int f, int c) {
    if (!auto_actual->activo) return;

    // Buscar semáforo del carril
    Semaforo* sem = NULL;
    for (int j = 0; j < inter->cantidadSemaforos; j++) {
        if (inter->semaforos[j].carril == auto_actual->carril) {
            sem = &inter->semaforos[j];
            break;
        }
    }
    if (!sem) return;

    if (auto_actual->posicion == 0) {
        if (sem->estado == 2) {
            #pragma omp atomic
            auto_actual->posicion++;
            #pragma omp critical (mostrar_output)
            printf("   [Hilo %d] Auto %d avanza desde posición 0 (semáforo verde)\n", 
                   omp_get_thread_num(), auto_actual->numero);
        }
    } else {
        #pragma omp atomic
        auto_actual->posicion++;
    }

    // Si sale de la intersección
    if (auto_actual->posicion >= inter->longitud) {
        int nf = f + auto_actual->dir_fila;
        int nc = c + auto_actual->dir_col;

        if (nf >= 0 && nf < nFilas && nc >= 0 && nc < nColumnas) {
            Interseccion* destino = &grid[nf][nc];
            int colocado = 0;
            
            #pragma omp critical (mover_auto)
            {
                for (int k = 0; k < destino->cantidadAutos; k++) {
                    if (!destino->autos[k].activo) {
                        destino->autos[k] = *auto_actual;
                        destino->autos[k].posicion = 0;
                        destino->autos[k].activo = 1;
                        colocado = 1;
                        break;
                    }
                }
            }
            
            #pragma omp critical (mostrar_output)
            {
                if (colocado) {
                    printf("%s[Hilo %d] Auto %d cruzó de intersección [%d,%d] a [%d,%d]%s\n", 
                        GREEN, omp_get_thread_num(), auto_actual->numero, f, c, nf, nc, RESET);
                } else {
                    printf("%s[Hilo %d] Auto %d no pudo entrar a [%d,%d] (sin espacios libres)%s\n", 
                        YELLOW, omp_get_thread_num(), auto_actual->numero, nf, nc, RESET);
                }
            }
        } else {
            #pragma omp critical (mostrar_output)
            printf("%s[Hilo %d] Auto %d sale del sistema desde [%d,%d] (límite del grid)%s\n", 
                RED, omp_get_thread_num(), auto_actual->numero, f, c, RESET);
        }
        auto_actual->activo = 0;
    }
}

// Mover autos con paralelismo dinámico por vehículo
void mover_autos(Interseccion** grid, int nFilas, int nColumnas) {
    // Contar autos activos para ajustar dinámicamente los hilos
    int autos_activos_totales = contar_autos_activos(grid, nFilas, nColumnas);
    
    // Ajustar número de hilos según la cantidad de autos activos
    int hilos_nivel1 = (autos_activos_totales > 8) ? 4 : 
                       (autos_activos_totales > 4) ? 2 : 1;
    
    printf("%s[INFO] Usando %d hilos para %d autos activos%s\n", 
           MAGENTA, hilos_nivel1, autos_activos_totales, RESET);

    // Paralelismo dinámico nivel 1: Intersecciones
    #pragma omp parallel for num_threads(hilos_nivel1) collapse(2) schedule(dynamic)
    for (int f = 0; f < nFilas; f++) {
        for (int c = 0; c < nColumnas; c++) {
            Interseccion* inter = &grid[f][c];
            
            // Contar autos activos en esta intersección
            int autos_activos_local = 0;
            for (int i = 0; i < inter->cantidadAutos; i++) {
                if (inter->autos[i].activo) autos_activos_local++;
            }
            
            if (autos_activos_local == 0) continue;
            
            // Paralelismo dinámico nivel 2: Autos dentro de cada intersección
            int hilos_nivel2 = (autos_activos_local > 3) ? 3 : autos_activos_local;
            
            #pragma omp parallel for num_threads(hilos_nivel2) schedule(dynamic, 1) if(autos_activos_local > 1)
            for (int i = 0; i < inter->cantidadAutos; i++) {
                procesar_auto_individual(&inter->autos[i], inter, grid, 
                                       nFilas, nColumnas, f, c);
            }
        }
    }
}

// Función para contar autos activos con paralelismo
int contar_autos_activos(Interseccion** grid, int nFilas, int nColumnas) {
    int total = 0;
    
    #pragma omp parallel for collapse(2) reduction(+:total)
    for (int f = 0; f < nFilas; f++) {
        for (int c = 0; c < nColumnas; c++) {
            int local_count = 0;
            for (int i = 0; i < grid[f][c].cantidadAutos; i++) {
                if (grid[f][c].autos[i].activo) local_count++;
            }
            total += local_count;
        }
    }
    return total;
}

int main() {
    omp_set_nested(1);  // Habilitar paralelismo anidado
    omp_set_dynamic(1); // Permitir ajuste dinámico de hilos
    omp_set_max_active_levels(3); // Permitir hasta 3 niveles de anidamiento
    
    int nFilas = 2;
    int nColumnas = 2;
    int nAutos = 5;
    int nSemaforos = 2;
    int longitud = 5;

    printf("%s SIMULACIÓN DE TRÁFICO CON PARALELISMO DINÁMICO INICIADA %s\n", CYAN, RESET);
    printf("Grid: %dx%d, Autos: %d, Longitud intersección: %d\n", nFilas, nColumnas, nAutos, longitud);
    printf("Hilos disponibles: %d, Paralelismo anidado: %s\n", 
           omp_get_max_threads(), omp_get_nested() ? "HABILITADO" : "DESHABILITADO");
    printf("Niveles activos máximos: %d\n\n", omp_get_max_active_levels());

    //  grid
    Interseccion** grid = malloc(nFilas * sizeof(Interseccion*));
    for (int f = 0; f < nFilas; f++) {
        grid[f] = malloc(nColumnas * sizeof(Interseccion));
        for (int c = 0; c < nColumnas; c++) {
            grid[f][c] = crear_interseccion(nAutos, nSemaforos, longitud);

            // Solo la primera intersección tiene autos inicialmente
            if (f == 0 && c == 0) {
                for (int i = 0; i < nAutos; i++) {
                    int dir_fila = (i % 2);
                    int dir_col = ((i+1) % 2); 
                    grid[f][c].autos[i] = crear_auto(i, 0, i % nSemaforos, dir_fila, dir_col);
                }
            }

            // Semáforos con inicialización coordinada
            for (int i = 0; i < nSemaforos; i++) {
                if (i == 0) {
                    grid[f][c].semaforos[i] = crear_semaforo(i, 2, i);
                    grid[f][c].semaforos[i].tiempo_restante = 5;
                } else {
                    grid[f][c].semaforos[i] = crear_semaforo(i, 0, i);
                    grid[f][c].semaforos[i].tiempo_restante = 5;
                }
            }
        }
    }


    int iteracion = 1;
    int max_iteraciones = 20;
    
    while (iteracion <= max_iteraciones) {
        printf("%s--- ITERACIÓN %d - ESTADO DEL SISTEMA ---%s\n", CYAN, iteracion, RESET);
        
        // Mostrar estado con paralelismo
        #pragma omp parallel for collapse(2) schedule(static)
        for (int f = 0; f < nFilas; f++) {
            for (int c = 0; c < nColumnas; c++) {
                mostrar_lista(grid[f][c], iteracion, f, c);
            }
        }

        int autos_activos = contar_autos_activos(grid, nFilas, nColumnas);
        printf("%sAutos activos en el sistema: %d%s\n\n", YELLOW, autos_activos, RESET);

        if (autos_activos == 0) {
            printf("%s SIMULACIÓN TERMINADA - No hay más autos en el sistema%s\n", GREEN, RESET);
            break;
        }

        // Ejecutar un paso de simulación con paralelismo dinámico
        mover_autos(grid, nFilas, nColumnas);
        
        // Actualizar semáforos con paralelismo
        #pragma omp parallel for collapse(2) schedule(static)
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