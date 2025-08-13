#include <stdio.h>
#include <stdlib.h>
#include "entidades.h"

Auto crear_auto(int numero, int posicion, int carril) {
    Auto a;
    a.numero = numero;
    a.posicion = posicion;
    a.carril = carril;
    return a;
}

Semaforo crear_semaforo(int numero, int estado, int carril) {
    Semaforo s;
    s.numero = numero;
    s.estado = estado;
    s.carril = carril;
    return s;
}

Interseccion crear_interseccion(int cantidadAutos, int cantidadSemaforos) {
    Interseccion inter;
    inter.cantidadAutos = cantidadAutos;
    inter.autos = malloc(sizeof(Auto) * cantidadAutos);
    inter.cantidadSemaforos = cantidadSemaforos;
    inter.semaforos = malloc(sizeof(Semaforo) * cantidadSemaforos);
    return inter;
}