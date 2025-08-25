#ifndef HOYO_HPP
#define HOYO_HPP

#include <glm/glm.hpp> 
#include <GLFW/glfw3.h>
#include <iostream>

extern const double c;
extern const double G;

// Escala: cuántos metros equivalen a 1 unidad de ventana
extern double METROS_POR_UNIDAD;

struct Hoyo {
    glm::vec2 posicion; // en unidades de ventana
    double masa;        // en kg
    double r_s;         // en metros (real)

    Hoyo(glm::vec2 pos, double m);

    // Dibuja usando el radio real (m) escalado a unidades de ventana
    void drawHoyo(int segmentos = 30) const;
};

#endif
