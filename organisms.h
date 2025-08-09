#ifndef ORGANISM_H
#define ORGANISM_H
#include "ecosystem.h"
#include "entity.h"


void actualizar_plantas(Entity** local_grid, int size, int i, int j, int* plant_changes);
void actualizar_herbivoros(Entity** local_grid, int size, int i, int j, int* herb_changes, int* plant_changes);
void actualizar_carnivoros(Entity** local_grid, int size, int i, int j, int* carn_changes, int* herb_changes);

#endif
