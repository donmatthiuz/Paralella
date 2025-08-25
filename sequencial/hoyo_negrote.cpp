#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace glm;
using namespace std;
using Clock = std::chrono::high_resolution_clock;


struct Motor {
    GLFWwindow* window;
    int WIDTH = 800;
    int HEIGHT = 600;
    float width = 1e11f;
    float height = 7.5e10f;

    // Constructor
    Motor() {
        if (!glfwInit()) {
            cerr << "GLFW init failed\n";
            exit(EXIT_FAILURE);
        }

        window = glfwCreateWindow(WIDTH, HEIGHT, "Black Hole", nullptr, nullptr);
        if (!window) {
            cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(window);
        glViewport(0, 0, WIDTH, HEIGHT);

        // Si quieres usar GLEW, inicialízalo aquí
        // glewExperimental = GL_TRUE;
        // if (glewInit() != GLEW_OK) { cerr << "GLEW init failed\n"; exit(EXIT_FAILURE); }

        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 1);
    }

    void ejecutar_motor() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        double left = -width;
        double right = width;
        double bottom = -height;
        double top = height;
        glOrtho(left, right, bottom, top, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
    }
};


Motor mimotor;


int main(){
    while (!glfwWindowShouldClose(mimotor.window)) {
        mimotor.ejecutar_motor();
        glfwSwapBuffers(mimotor.window);
        glfwPollEvents();
    }

    glfwDestroyWindow(mimotor.window);
    glfwTerminate();

    return 0;


}