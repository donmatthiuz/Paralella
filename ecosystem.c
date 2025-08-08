#include <stdio.h>
#include "ecosystem.h"

// Códigos ANSI
#define RESET   "\033[0m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define RED     "\033[0;31m"
#define GRAY_BG "\033[100m"
#define BLACK   "\033[30m"

void mostrarEcosistema(Ecosystem* eco) {
    // Recalcular conteos reales:
    eco->plant_count = 0;
    eco->herbivore_count = 0;
    eco->carnivore_count = 0;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int tipo = eco->grid[i][j].type;
            switch (tipo) {
                case 1: eco->plant_count++; break;
                case 2: eco->herbivore_count++; break;
                case 3: eco->carnivore_count++; break;
            }
        }
    }

    printf("Tick: %d | Plantas: %d | Herbívoros: %d | Carnívoros: %d\n\n",
           eco->tick, eco->plant_count, eco->herbivore_count, eco->carnivore_count);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int tipo = eco->grid[i][j].type;

            switch (tipo) {
                case 1: // planta
                    printf(GREEN " P " RESET);
                    break;
                case 2: // herbívoro
                    printf(YELLOW " H " RESET);
                    break;
                case 3: // carnívoro
                    printf(RED " C " RESET);
                    break;
                default: // vacío
                    printf(GRAY_BG BLACK " . " RESET);
                    break;
            }
        }
        printf("\n");
    }
}
