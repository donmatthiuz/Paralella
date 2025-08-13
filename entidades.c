#include <stdio.h>
#include <stdlib.h>
#include "entidades.h"

Auto crear_auto(int numero, int posicion, int carril) {
    Auto auto_nuevo;
    auto_nuevo.numero = numero;
    auto_nuevo.posicion = posicion;
    auto_nuevo.carril = carril;
    auto_nuevo.activo = 1;
    return auto_nuevo;
}

Semaforo crear_semaforo(int id, int estado, int carril) {
    Semaforo sem;
    sem.id = id;
    sem.estado = estado;
    sem.carril = carril;
    sem.tiempo_restante = 5; // 5 segundos por estado
    return sem;
}

Interseccion crear_interseccion(int nAutos, int nSemaforos) {
    Interseccion inter;
    inter.autos = (Auto*)malloc(nAutos * sizeof(Auto));
    inter.semaforos = (Semaforo*)malloc(nSemaforos * sizeof(Semaforo));
    inter.cantidadAutos = nAutos;
    inter.cantidadSemaforos = nSemaforos;
    return inter;
}
