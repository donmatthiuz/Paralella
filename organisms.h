#ifndef ORGANISM_H
#define ORGANISM_H
#include "ecosystem.h"
#include "entity.h"


void actualizar_plantas(Ecosystem* eco, int i, int j);
void actualizar_herbivoros(Ecosystem* eco, int i, int j);
void actualizar_carnivoros(Ecosystem* eco, int i, int j);

#endif
