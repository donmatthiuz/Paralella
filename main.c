// main.c
#include <stdio.h>
#include "ecosystem.h"

int main() {
    Ecosystem eco = {0};
    eco.tick = 1;

    
    eco.grid[0][0].type = 1;
    eco.grid[0][1].type = 1;
    eco.grid[0][2].type = 1;
    eco.grid[0][3].type = 1;
    eco.grid[0][4].type = 1;
    eco.grid[1][0].type = 1;
    eco.grid[1][1].type = 1;
    eco.grid[1][2].type = 1;
    eco.grid[1][3].type = 1;
    eco.grid[1][4].type = 1;

    
    eco.grid[2][0].type = 2;
    eco.grid[2][1].type = 2;

    
    eco.grid[2][3].type = 3;
    eco.grid[2][4].type = 3;

    mostrarEcosistema(&eco);

    return 0;
}
