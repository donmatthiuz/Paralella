#ifndef ECOSYSTEM_H
#define ECOSYSTEM_H

#include "entity.h"


typedef struct {
    int type;
} Cell;

typedef struct {
    Entity **grid;      
    int size;         
    int plant_count;
    int herbivore_count;
    int carnivore_count;
    int tick;
} Ecosystem;

Ecosystem* crearEcosistema(int size);
void liberarEcosistema(Ecosystem *eco);
void mostrarEcosistema(Ecosystem* eco);
void iniciarEcosistema(Ecosystem *eco, int n_plantas, int n_herb, int n_carn);
void colocarEntidades(Ecosystem *eco, int tipo, int cantidad);

#endif
