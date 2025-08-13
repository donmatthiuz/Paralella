#include <stdio.h>
#include <stdlib.h>
#include "entidades.h"

Auto crear_auto(int numero, int posicion, int carril, int dir_fila, int dir_col){
    Auto auto_nuevo;
    auto_nuevo.numero = numero;
    auto_nuevo.posicion = posicion;
    auto_nuevo.carril = carril;
    auto_nuevo.activo = 1;
    auto_nuevo.dir_fila = dir_fila;
    auto_nuevo.dir_col = dir_col;
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

Interseccion  crear_interseccion(int nAutos, int nSemaforos, int longitud) {
    Interseccion inter;
    inter.autos = (Auto*)malloc(nAutos * sizeof(Auto));
    inter.semaforos = (Semaforo*)malloc(nSemaforos * sizeof(Semaforo));
    inter.cantidadAutos = nAutos;
    inter.cantidadSemaforos = nSemaforos;
    inter.longitud =  longitud;
    return inter;
}
