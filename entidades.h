#ifndef ENTIDADES_H
#define ENTIDADES_H

#include "entidades.h"


typedef struct {
    int numero;
    int posicion; // el numero de la interseccion
    int carril;
} Auto;


typedef struct {
    int numero;
    int estado;  // 0 rojo, 1 amarllo, 2 verde         
    int carril;
} Semaforo;

typedef struct {
    Auto *autos;
    int cantidadAutos;
    Semaforo *semaforos;
    int cantidadSemaforos;
} Interseccion;



#endif