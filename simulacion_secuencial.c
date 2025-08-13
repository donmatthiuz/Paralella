#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "entidades.h"


char estado_semaforo(int estado) {
    if (estado == 0) return 'R';
    if (estado == 1) return 'A';
    return 'V';
}

void mostrar_interfaz(Interseccion inter) {
    printf("\n=== INTERSECCIÓN ===\n");
    
 
    printf("       %c  \n", estado_semaforo(inter.semaforos[0].estado));
    printf("       |  \n");
    
 
    int autos_verticales_antes = 0;
    for (int i = 0; i < inter.cantidadAutos; i++) {
        if (inter.autos[i].carril == 0 && inter.autos[i].posicion == 0 && inter.autos[i].activo) {
            printf("       ↑ Auto%d\n", inter.autos[i].numero);
            autos_verticales_antes++;
        }
    }
    
 
    if (autos_verticales_antes == 0) {
        printf("       |\n");
    }
    
 
    printf(" %c-----+----- %c\n", 
           estado_semaforo(inter.semaforos[1].estado), 
           estado_semaforo(inter.semaforos[1].estado));
    
 
    printf("← ");
    int autos_horizontales = 0;
    for (int i = 0; i < inter.cantidadAutos; i++) {
        if (inter.autos[i].carril == 1 && inter.autos[i].activo) {
            if (inter.autos[i].posicion == 0) {
                printf("Auto%d ", inter.autos[i].numero);
                autos_horizontales++;
            }
        }
    }
    if (autos_horizontales == 0) {
        printf("      ");
    }
    printf("\n");
    
    // Línea inferior
    printf("       |\n");
    
    // Autos verticales, después del cruce
    for (int i = 0; i < inter.cantidadAutos; i++) {
        if (inter.autos[i].carril == 0 && inter.autos[i].posicion == 2 && inter.autos[i].activo) {
            printf("       ↑ Auto%d\n", inter.autos[i].numero);
        }
    }
    
    // Mostrar estado de semáforos
    printf("\nEstado Semáforos:\n");
    printf("Vertical: %c (tiempo: %d)\n", 
           estado_semaforo(inter.semaforos[0].estado), 
           inter.semaforos[0].tiempo_restante);
    printf("Horizontal: %c (tiempo: %d)\n", 
           estado_semaforo(inter.semaforos[1].estado), 
           inter.semaforos[1].tiempo_restante);
}

void actualizar_semaforos(Interseccion* inter) {
    for (int i = 0; i < inter->cantidadSemaforos; i++) {
        inter->semaforos[i].tiempo_restante--;
        
        if (inter->semaforos[i].tiempo_restante <= 0) {
            // Cambiar estado del semáforo
            if (inter->semaforos[i].estado == 2) { // Verde -> Amarillo
                inter->semaforos[i].estado = 1;
                inter->semaforos[i].tiempo_restante = 2; // 2 segundos en amarillo
            } else if (inter->semaforos[i].estado == 1) { // Amarillo -> Rojo
                inter->semaforos[i].estado = 0;
                inter->semaforos[i].tiempo_restante = 5; // 5 segundos en rojo
                
                // Activar el otro semáforo
                int otro_semaforo = (i == 0) ? 1 : 0;
                if (inter->semaforos[otro_semaforo].estado == 0) {
                    inter->semaforos[otro_semaforo].estado = 2; // Verde
                    inter->semaforos[otro_semaforo].tiempo_restante = 5;
                }
            } else { // Rojo -> Verde (solo si el otro está en rojo)
                int otro_semaforo = (i == 0) ? 1 : 0;
                if (inter->semaforos[otro_semaforo].estado == 0) {
                    inter->semaforos[i].estado = 2;
                    inter->semaforos[i].tiempo_restante = 5;
                }
            }
        }
    }
}

void mover_autos(Interseccion* inter) {
    for (int i = 0; i < inter->cantidadAutos; i++) {
        if (!inter->autos[i].activo) continue;
        
        Auto* auto_actual = &inter->autos[i];
        
        // Verificar si puede moverse según el semáforo
        int puede_moverse = 0;
        
        if (auto_actual->carril == 0) { // Carril vertical
            if (auto_actual->posicion == 0 && inter->semaforos[0].estado == 2) {
                puede_moverse = 1; // Verde, puede pasar
            } else if (auto_actual->posicion > 0) {
                puede_moverse = 1; // Ya pasó el semáforo
            }
        } else { // Carril horizontal
            if (auto_actual->posicion == 0 && inter->semaforos[1].estado == 2) {
                puede_moverse = 1; // Verde, puede pasar
            } else if (auto_actual->posicion > 0) {
                puede_moverse = 1; // Ya pasó el semáforo
            }
        }
        
        // Mover el auto si puede
        if (puede_moverse) {
            auto_actual->posicion++;
            
            // Si el auto sale de la intersección, desactivarlo
            if (auto_actual->posicion > 2) {
                auto_actual->activo = 0;
                printf("Auto%d salió de la intersección\n", auto_actual->numero);
            }
        }
    }
}

int hay_autos_activos(Interseccion inter) {
    for (int i = 0; i < inter.cantidadAutos; i++) {
        if (inter.autos[i].activo) return 1;
    }
    return 0;
}

int main() {
    int nAutos = 6;
    int nSemaforos = 2;
    Interseccion inter = crear_interseccion(nAutos, nSemaforos);
    
    // Crear autos verticales (carril 0)
    inter.autos[0] = crear_auto(0, 0, 0); // antes del cruce
    inter.autos[1] = crear_auto(1, 0, 0);
    inter.autos[2] = crear_auto(2, 0, 0);
    
    // Crear autos horizontales (carril 1)
    inter.autos[3] = crear_auto(3, 0, 1);
    inter.autos[4] = crear_auto(4, 0, 1);
    inter.autos[5] = crear_auto(5, 0, 1);
    
    // Semáforo vertical inicia en verde
    inter.semaforos[0] = crear_semaforo(0, 2, 0);
    
    // Semáforo horizontal inicia en rojo
    inter.semaforos[1] = crear_semaforo(1, 0, 1);
    
    printf("Iniciando simulación de intersección...\n");
    printf("Presiona Ctrl+C para detener\n\n");
    
    // Simulación principal
    while (hay_autos_activos(inter)) {
        // limpiar_pantalla();
        mostrar_interfaz(inter);
        
        sleep(1); // Esperar 1 segundo
        
        // Actualizar lógica
        actualizar_semaforos(&inter);
        mover_autos(&inter);
    }
    
    printf("\n¡Todos los autos han pasado por la intersección!\n");
    
    // Liberar memoria
    free(inter.autos);
    free(inter.semaforos);
    
    return 0;
}