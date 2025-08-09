#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ecosystem.h"
#include "organisms.h"
#include <omp.h>
#include "entity.h" 

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

    // inicializar cuadricula y epecies
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

    printf("===================Ecosistema inicial=============\n ");
    mostrarEcosistema(eco);
    printf("================================================== \n");
    // Para cada tick de la simulación:
    for (; eco->tick <= tick_max; eco->tick++) {
        // Para cada celda en la cuadrícula:
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < eco->size; i++) {
             for (int j = 0; j < eco->size; j++) {

                
                if (eco->grid[i][j].type == 1){
                    actualizar_plantas(eco, i, j);

                }
                else if (eco->grid[i][j].type == 2){
                    actualizar_herbivoros(eco, i, j);

                }
                else if(eco->grid[i][j].type == 3){
                    actualizar_carnivoros(eco, i, j);

                }
                
               
            }
        }

        mostrarEcosistema(eco);

    }

    

    liberarEcosistema(eco);
    return 0;
}
