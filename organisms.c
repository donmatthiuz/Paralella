#include "ecosystem.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void actualizar_plantas(Ecosystem* eco, int i, int j) {
    Entity *e = &eco->grid[i][j];
    int size = eco->size;
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};

    if (!e->alive || e->type != 1) return; // Solo plantas vivas

    // Muerte por energía 0
    if (e->energy == 0) {
        e->alive = 0;
        e->type = 0;
        eco->plant_count--;
        printf("Planta en (%d, %d) murió por energía 0\n", i, j);
        return;
    }

    // Contar espacios libres para reproducción (tipo 0 y alive 0)
    int espacios_libres = 0;
    for (int d = 0; d < 4; d++) {
        int nx = i + dx[d];
        int ny = j + dy[d];
        if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;
        Entity *neighbor = &eco->grid[nx][ny];
        if (!neighbor->alive && neighbor->type == 0) espacios_libres++;
    }

    // Muerte por falta de espacio para reproducirse
    if (espacios_libres == 0) {
        e->alive = 0;
        e->type = 0;
        eco->plant_count--;
        return;
    }

    // Reproducción con 30% de probabilidad en cada celda vacía
    for (int d = 0; d < 4; d++) {
        int nx = i + dx[d];
        int ny = j + dy[d];
        if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;
        Entity *neighbor = &eco->grid[nx][ny];

        if (!neighbor->alive && neighbor->type == 0) {
            int prob = rand() % 100;
            if (prob < 30) {
                neighbor->type = 1;    
                neighbor->energy = 10; 
                neighbor->age = 0;
                neighbor->alive = 1;
                eco->plant_count++;
            }
        }
    }
}


void actualizar_herbivoros(Ecosystem* eco, int i, int j) {
    Entity *e = &eco->grid[i][j];
    if (!e->alive || e->type != 2) return;

    int size = eco->size;
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};

    e->age++;
    if (e->energy == 0) {
        e->alive = 0;
        e->type = 0;
        eco->herbivore_count--;
        return;
    }

    int target_x = -1, target_y = -1;
    int found_plant = 0;

    // Buscar plantas vecinas para comer
    for (int d = 0; d < 4; d++) {
        int nx = i + dx[d];
        int ny = j + dy[d];
        if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;

        Entity *neighbor = &eco->grid[nx][ny];
        if (neighbor->alive && neighbor->type == 1 && neighbor->energy > 0) {
            target_x = nx;
            target_y = ny;
            found_plant = 1;
            break;
        }
    }

    if (found_plant) {
        // Comer planta
        eco->grid[target_x][target_y].energy -= 1;
        if (eco->grid[target_x][target_y].energy == 0) {
            eco->grid[target_x][target_y].alive = 0;
            eco->grid[target_x][target_y].type = 0;
            eco->plant_count--;
        }

        e->energy += 1; //aqui si come planta se suma una cantidad de vida
        e->consume = 0;
        e->eatit += 1;

        // Mover herbívoro a la celda de la planta
        eco->grid[target_x][target_y] = *e;
        e->alive = 0;   // vaciar celda anterior
        e->type = 0;

        return;
    }

    // No encontró planta: mover a celda vacía adyacente para explorar
    int moved = 0;
    for (int d = 0; d < 4 && !moved; d++) {
        int nx = i + dx[d];
        int ny = j + dy[d];
        if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;

        Entity *neighbor = &eco->grid[nx][ny];
        if (!neighbor->alive && neighbor->type == 0) {
            eco->grid[nx][ny] = *e;   // mover herbívoro
            e->alive = 0;             // vaciar celda previa
            e->type = 0;
            moved = 1;
        }
    }

    if (!moved) {
        // No se movió ni comió
        e->consume++;
    }

    // Reproducirse si comió 3 veces
    if (e->eatit >= 3) {
        for (int d = 0; d < 4; d++) {
            int nx = i + dx[d];
            int ny = j + dy[d];
            if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;
            Entity *neighbor = &eco->grid[nx][ny];
            if (!neighbor->alive && neighbor->type == 0) {
                neighbor->type = 2;
                neighbor->energy = 5;
                neighbor->age = 0;
                neighbor->alive = 1;
                neighbor->consume = 0;
                neighbor->eatit = 0;
                eco->herbivore_count++;
                e->eatit = 0;
                printf("Herbívoro reproducido en (%d,%d)\n", nx, ny);
                break;
            }
        }
    }

    // Morir si no come en 3 ticks consecutivos
    if (e->consume >= 3) {
        e->alive = 0;
        e->type = 0;
        eco->herbivore_count--;
        printf("Herbívoro en (%d,%d) murió por no alimentarse 3 ticks consecutivos\n", i, j);
    }


    // vida media de un herbivoro es de 30 años

    if (e->age >= 30) {
        e->alive = 0;
        e->type = 0;
        eco->herbivore_count--;
        printf("Herbívoro en (%d,%d) murió por no alimentarse 3 ticks consecutivos\n", i, j);
    }
}



void actualizar_carnivoros(Ecosystem* eco, int i, int j) {
    Entity *e = &eco->grid[i][j];
    if (!e->alive || e->type != 3) return;

    int size = eco->size;
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};

    e->age++;

    // Muere si energía 0
    if (e->energy == 0) {
        e->alive = 0;
        e->type = 0;
        eco->carnivore_count--;
        return;
    }

    int target_x = -1, target_y = -1;
    int found_herbivore = 0;

    // Buscar herbívoros adyacentes para cazar
    for (int d = 0; d < 4; d++) {
        int nx = i + dx[d];
        int ny = j + dy[d];
        if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;

        Entity *neighbor = &eco->grid[nx][ny];
        if (neighbor->alive && neighbor->type == 2) {
            target_x = nx;
            target_y = ny;
            found_herbivore = 1;
            break;
        }
    }

    if (found_herbivore) {
        // Cazar herbívoro: matar (energy = 0)
        eco->grid[target_x][target_y].energy = 0;
        eco->grid[target_x][target_y].alive = 0;
        eco->grid[target_x][target_y].type = 0;
        eco->herbivore_count--;

        e->energy += 2;  // gana +2 energía
        e->consume = 0;  // reset ticks sin comer
        e->eatit += 1;

        // Mover carnívoro a la celda del herbívoro
        eco->grid[target_x][target_y] = *e;
        e->alive = 0;    // vaciar celda anterior
        e->type = 0;

        return;
    }

    // No encontró herbívoro: intentar moverse a celda vacía adyacente para explorar
    int moved = 0;
    for (int d = 0; d < 4 && !moved; d++) {
        int nx = i + dx[d];
        int ny = j + dy[d];
        if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;

        Entity *neighbor = &eco->grid[nx][ny];
        if (!neighbor->alive && neighbor->type == 0) {
            eco->grid[nx][ny] = *e;   // mover carnívoro
            e->alive = 0;             // vaciar celda previa
            e->type = 0;
            moved = 1;
        }
    }

    if (!moved) {
        // No se movió ni comió
        e->consume++;
    }

    // Reproducirse al comer 3 herbívoros
    if (e->eatit >= 3) {
        for (int d = 0; d < 4; d++) {
            int nx = i + dx[d];
            int ny = j + dy[d];
            if (nx < 0 || nx >= size || ny < 0 || ny >= size) continue;
            Entity *neighbor = &eco->grid[nx][ny];
            if (!neighbor->alive && neighbor->type == 0) {
                neighbor->type = 3;
                neighbor->energy = 5;
                neighbor->age = 0;
                neighbor->alive = 1;
                neighbor->consume = 0;
                neighbor->eatit = 0;
                eco->carnivore_count++;
                e->eatit = 0;
                printf("Carnívoro reproducido en (%d,%d)\n", nx, ny);
                break;
            }
        }
    }

    // Morir si no encuentra herbívoro en 2 ticks
    if (e->consume >= 2) {
        e->alive = 0;
        e->type = 0;
        eco->carnivore_count--;
        
        return;
    }

    // Vida promedio de 15 años
    if (e->age >= 15) {
        e->alive = 0;
        e->type = 0;
        eco->carnivore_count--;
        
    }
}
