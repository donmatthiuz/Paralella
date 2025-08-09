#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ecosystem.h"
#include "organisms.h"
#include <omp.h>
#include "entity.h" 


void copiar_grid(Ecosystem* origen, Entity** destino_grid, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            destino_grid[i][j] = origen->grid[i][j];
        }
    }
}


Entity** crear_grid_local(int size) {
    Entity** local_grid = malloc(size * sizeof(Entity*));
    for (int i = 0; i < size; i++) {
        local_grid[i] = malloc(size * sizeof(Entity));
    }
    return local_grid;
}

void liberar_grid_local(Entity** grid, int size) {
    for (int i = 0; i < size; i++) {
        free(grid[i]);
    }
    free(grid);
}


void generar_cantidades(int size, int *p, int *h, int *c) {
    int plantas, herbivoros, carnivoros;

    do {
        carnivoros = 1 + rand() % size;
        herbivoros = carnivoros + 1 + rand() % (size - carnivoros + 1);
        if (herbivoros > size) herbivoros = size;

        int suma_hc = herbivoros + carnivoros;

        if (suma_hc >= size) {
            plantas = size;
        } else {
            plantas = suma_hc + 1 + rand() % (size - suma_hc);
        }
    } while (!(plantas > (herbivoros + carnivoros) && herbivoros > carnivoros && plantas <= size && herbivoros <= size && carnivoros <= size));

    *p = plantas;
    *h = herbivoros;
    *c = carnivoros;
}


int main() {
    // Inicializar cuadrícula y especies
    int size;
    printf("Ingrese el tamaño del ecosistema: ");
    scanf("%d", &size);
    srand(time(NULL));
    
    int p, h, c;
    generar_cantidades(size, &p, &h, &c);
    Ecosystem *eco = crearEcosistema(size);
    eco->tick = 1;
    iniciarEcosistema(eco, p, h, c);
    
    int tick_max;
    printf("Ingrese el tick máximo para la simulación: ");
    scanf("%d", &tick_max);

    printf("===================Ecosistema inicial=============\n");
    mostrarEcosistema(eco);
    printf("==================================================\n");
    
    // Para cada tick de la simulación:
    for (; eco->tick <= tick_max; eco->tick++) {
        
        // PARALELIZACIÓN POR FILAS
        #pragma omp parallel
        {
            int thread_id = omp_get_thread_num();
            int num_threads = omp_get_num_threads();
            
            // Calcular rango de filas para este thread
            int start_row = (size * thread_id) / num_threads;
            int end_row = (size * (thread_id + 1)) / num_threads;
            
            // Crear grid local para este thread
            Entity** local_grid = crear_grid_local(size);
            copiar_grid(eco, local_grid, size);
            
            // Contadores locales para cambios
            int plant_changes = 0, herb_changes = 0, carn_changes = 0;
            
            // Procesar solo las filas asignadas a este thread
            for (int i = start_row; i < end_row; i++) {
                for (int j = 0; j < size; j++) {
                    
                    if (local_grid[i][j].type == 1) {
                        actualizar_plantas(local_grid, size, i, j, &plant_changes);
                    }
                    else if (local_grid[i][j].type == 2) {
                        actualizar_herbivoros(local_grid, size, i, j, &herb_changes, &plant_changes);
                    }
                    else if (local_grid[i][j].type == 3) {
                        actualizar_carnivoros(local_grid, size, i, j, &carn_changes, &herb_changes);
                    }
                }
            }
            
            // aqui esperamos aque todos terminen
            #pragma omp barrier
            
            // Actualizamos los datos en el grid principal
            #pragma omp critical
            {
                for (int i = start_row; i < end_row; i++) {
                    for (int j = 0; j < size; j++) {
                        eco->grid[i][j] = local_grid[i][j];
                    }
                }
                
                
                eco->plant_count += plant_changes;
                eco->herbivore_count += herb_changes;
                eco->carnivore_count += carn_changes;
            }
            
            // Liberar memoria local
            liberar_grid_local(local_grid, size);
        }
        
        // Aqui mantemmos consistenca de los contadores
        #pragma omp single
        {
            int plantas = 0, herbivoros = 0, carnivoros = 0;
            
            #pragma omp parallel for collapse(2) reduction(+:plantas,herbivoros,carnivoros)
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    if (eco->grid[i][j].alive) {
                        switch(eco->grid[i][j].type) {
                            case 1: plantas++; break;
                            case 2: herbivoros++; break;
                            case 3: carnivoros++; break;
                        }
                    }
                }
            }
            
            eco->plant_count = plantas;
            eco->herbivore_count = herbivoros;
            eco->carnivore_count = carnivoros;
        }
        
        mostrarEcosistema(eco);
    }
    
    liberarEcosistema(eco);
    return 0;
}