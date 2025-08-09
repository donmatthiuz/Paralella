#ifndef ORGANISM_H
#define ORGANISM_H

typedef struct {
    char type; //1 es planta, 2 de herbiboro y 3 de carnivoro
    int energy;
    int age;
    int alive; // 1 vivo 0 requetemuerto
} Entity;

#endif
