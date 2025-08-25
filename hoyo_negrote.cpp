#include <GL/glut.h>   // Librería GLUT

void display() {
    glClear(GL_COLOR_BUFFER_BIT);       // Limpia el buffer de color

    // Dibujar un triángulo
    glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0); glVertex2f(-0.5f, -0.5f); // Rojo
        glColor3f(0.0, 1.0, 0.0); glVertex2f( 0.5f, -0.5f); // Verde
        glColor3f(0.0, 0.0, 1.0); glVertex2f( 0.0f,  0.5f); // Azul
    glEnd();

    glFlush();  // Forzar el renderizado
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);                          // Inicializa GLUT
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);    // Buffer simple y color RGB
    glutInitWindowSize(500, 500);                   // Tamaño ventana
    glutInitWindowPosition(100, 100);               // Posición en pantalla
    glutCreateWindow("Ventana OpenGL");             // Título ventana
    glutDisplayFunc(display);                       // Función de render
    glutMainLoop();                                 // Loop principal
    return 0;
}
