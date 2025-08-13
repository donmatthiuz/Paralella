#include <stdio.h>
#include <stdlib.h>
#include "entidades.h"


int main() {
  int nAutos = 8;
  int nSemaforos = 4;
  Interseccion inter = crear_interseccion(nAutos, nSemaforos);


  int autoId = 0;
  for (int carril = 0; carril < 4; carril++) {
      for (int j = 0; j < 2; j++) {
          inter.autos[autoId] = crear_auto(autoId, 0, carril);
          autoId++;
      }
  }


  // Inicial: Norte-Sur verde (carriles 0 y 1), Este-Oeste rojo (carriles 2 y 3)
  inter.semaforos[0] = crear_semaforo(0, 2, 0); // Verde
  inter.semaforos[1] = crear_semaforo(1, 2, 1); // Verde
  inter.semaforos[2] = crear_semaforo(2, 0, 2); // Rojo
  inter.semaforos[3] = crear_semaforo(3, 0, 3); // Rojo

  // Mostrar estado inicial
  printf("=== ESTADO INICIAL ===\n");
  printf("Autos:\n");
  for (int i = 0; i < inter.cantidadAutos; i++) {
      printf("Auto %d -> Posicion: %d, Carril: %d\n",
              inter.autos[i].numero, inter.autos[i].posicion, inter.autos[i].carril);
  }

  printf("\nSemáforos:\n");
  for (int i = 0; i < inter.cantidadSemaforos; i++) {
      printf("Semaforo %d -> Estado: %d, Carril: %d\n",
              inter.semaforos[i].numero, inter.semaforos[i].estado, inter.semaforos[i].carril);
  }

  // Liberar memoria
  free(inter.autos);
  free(inter.semaforos);
  
  return 0;
}