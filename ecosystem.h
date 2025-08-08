// ecosystem.h
#ifndef ECOSYSTEM_H
#define ECOSYSTEM_H

#define GRID_SIZE 20

typedef struct {
    int type;  // 0: vacío, 1: planta, 2: herbívoro, 3: carnívoro
} Cell;

typedef struct {
    Cell grid[GRID_SIZE][GRID_SIZE];
    int plant_count;
    int herbivore_count;
    int carnivore_count;
    int tick;
} Ecosystem;

// Si deseas declarar funciones aquí, también van:
void mostrarEcosistema(Ecosystem* eco);

#endif
