#include "Hoyo.hpp"
#include <GL/gl.h>
#include <cmath>

const double c = 299792458.0;
const double G = 6.67430e-11;

// Por ejemplo: 100 m por unidad de ventana
double METROS_POR_UNIDAD = 100.0;

Hoyo::Hoyo(glm::vec2 pos, double m) : posicion(pos), masa(m) {
    r_s = 2.0 * G * masa / (c * c); // metros (real)
}

void Hoyo::drawHoyo(int segmentos) const {
    // Escalar metros -> unidades de ventana
    float radio_unidades = static_cast<float>(r_s / METROS_POR_UNIDAD);
    float x = posicion.x;
    float y = posicion.y;

    glColor3f(1.0f, 0.0f, 0.0f); // rojo
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y); // centro
        for (int i = 0; i <= segmentos; i++) {
            float ang = i * 2.0f * static_cast<float>(M_PI) / segmentos;
            float dx = radio_unidades * std::cos(ang);
            float dy = radio_unidades * std::sin(ang);
            glVertex2f(x + dx, y + dy);
        }
    glEnd();
}
