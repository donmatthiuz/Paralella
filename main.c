#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ecosystem.h"


void generar_cantidades(int size, int *p, int *h, int *c) {
    int plantas, herbivoros, carnivoros;

    do {
        carnivoros = rand() % (size + 1);          
        herbivoros = carnivoros + 1 + rand() % (size - carnivoros + 1); // h > c, hasta size
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



    for (; eco->tick <= tick_max; eco->tick++) {

        
        mostrarEcosistema(eco);

    }

    liberarEcosistema(eco);
    return 0;
}
