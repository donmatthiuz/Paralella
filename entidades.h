#ifndef ENTIDADES_H
#define ENTIDADES_H

#include "entidades.h"


typedef struct {
    int numero;
    int posicion;
    int carril;
    int activo;
    int dir_fila; 
    int dir_col;  
} Auto;


typedef struct {
    int id;
    int estado;
    int carril;
    int tiempo_restante;
} Semaforo;

typedef struct {
    Auto* autos;
    Semaforo* semaforos;
    int cantidadAutos;
    int cantidadSemaforos;
    int longitud; 
} Interseccion;


Auto crear_auto(int numero, int posicion, int carril, int dir_fila, int dir_col);
Semaforo crear_semaforo(int id, int estado, int carril);
Interseccion crear_interseccion(int nAutos, int nSemaforos, int longitud);



#endif