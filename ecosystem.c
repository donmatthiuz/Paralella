#include <stdio.h>
#include <stdlib.h>
#include "ecosystem.h"
#include <time.h>

// Colores ANSI
#define RESET   "\033[0m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define RED     "\033[0;31m"
#define GRAY_BG "\033[100m"
#define BLACK   "\033[30m"

Ecosystem* crearEcosistema(int size) {
    Ecosystem *eco = malloc(sizeof(Ecosystem));
    eco->size = size;
    eco->tick = 0;
    eco->plant_count = eco->herbivore_count = eco->carnivore_count = 0;


    eco->grid = malloc(size * sizeof(Entity*));
    for (int i = 0; i < size; i++) {
        eco->grid[i] = calloc(size, sizeof(Entity));
    }

    return eco;
}

void liberarEcosistema(Ecosystem *eco) {
    for (int i = 0; i < eco->size; i++) {
        free(eco->grid[i]);
    }
    free(eco->grid);
    free(eco);
}


void colocarEntidades(Ecosystem *eco, int tipo, int cantidad) {
    int colocadas = 0;
    while (colocadas < cantidad) {
        int x = rand() % eco->size;
        int y = rand() % eco->size;

        if (!eco->grid[x][y].alive) { 
            eco->grid[x][y].type = tipo;
            eco->grid[x][y].energy = (rand() % 20) + 5;
            eco->grid[x][y].age = 0;
            eco->grid[x][y].alive = 1;
            colocadas++;
        }
    }
}

void iniciarEcosistema(Ecosystem *eco, int n_plantas, int n_herb, int n_carn) {
    srand(time(NULL));

    int total = n_plantas + n_herb + n_carn;
    if (total > eco->size * eco->size) {
        printf("Error: Demasiadas entidades para el tamaño del ecosistema.\n");
        return;
    }

    colocarEntidades(eco, 1, n_plantas);
    colocarEntidades(eco, 2, n_herb);
    colocarEntidades(eco, 3, n_carn);
}



void mostrarEcosistema(Ecosystem* eco) {
    eco->plant_count = eco->herbivore_count = eco->carnivore_count = 0;

    for (int i = 0; i < eco->size; i++) {
        for (int j = 0; j < eco->size; j++) {
            Entity e = eco->grid[i][j];
            if (e.alive) {
                switch (e.type) {
                    case 1: eco->plant_count++; break;
                    case 2: eco->herbivore_count++; break;
                    case 3: eco->carnivore_count++; break;
                }
            }
        }
    }

    printf("Tick: %d | Plantas: %d | Herbívoros: %d | Carnívoros: %d\n\n",
           eco->tick, eco->plant_count, eco->herbivore_count, eco->carnivore_count);

    for (int i = 0; i < eco->size; i++) {
        for (int j = 0; j < eco->size; j++) {
            Entity e = eco->grid[i][j];
            if (!e.alive) {
                printf(GRAY_BG BLACK " . " RESET);
            } else if (e.type == 1) {
                printf(GREEN " P " RESET);
            } else if (e.type == 2) {
                printf(YELLOW " H " RESET);
            } else if (e.type == 3) {
                printf(RED " C " RESET);
            }
        }
        printf("\n");
    }
}
