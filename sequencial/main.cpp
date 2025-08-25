#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>
#include "Hoyo.hpp"

using namespace glm;
using namespace std;

struct Motor {
    GLFWwindow* window;
    int WIDTH = 800, HEIGHT = 600;
    float width = 100.0f, height = 100.0f; // unidades de ventana

    Motor() {
        if (!glfwInit()) { cerr << "GLFW init failed\n"; exit(EXIT_FAILURE); }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Black Hole", nullptr, nullptr);
        if (!window) { cerr << "Failed to create GLFW window\n"; glfwTerminate(); exit(EXIT_FAILURE); }

        glfwMakeContextCurrent(window);
        glViewport(0, 0, WIDTH, HEIGHT);

        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 1);
    }

    void ejecutar_motor() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-width, width, -height, height, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
};

Motor mimotor;

int main() {
    double masa = 1.0e30; 
    Hoyo hoyo(glm::vec2(50.0f, 0.0f), masa);

    while (!glfwWindowShouldClose(mimotor.window)) {
        mimotor.ejecutar_motor();
        hoyo.drawHoyo(60); // más segmentos = círculo más suave
        glfwSwapBuffers(mimotor.window);
        glfwPollEvents();
    }

    glfwDestroyWindow(mimotor.window);
    glfwTerminate();
    return 0;
}
