#ifndef HOYO_HPP
#define HOYO_HPP

#include <glm/glm.hpp> 
#include <GLFW/glfw3.h>
#include <iostream>
double c = 299792458.0;
double G = 6.67430e-11;

struct Hoyo {
    glm::vec2 posicion;
    double masa;
    double r_s;
    Hoyo(glm::vec2 pos, float m) : posicion(pos), masa(m) {r_s = 2.0 * G * masa / (c*c);}

};

#endif
