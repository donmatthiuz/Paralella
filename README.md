
# Screen Savor de Hoyo Negro

Este proyecto es un motor gráfico básico que utiliza **OpenGL**, **GLFW**, **GLEW** y **GLM** para representar un “hoyo negro” real como screen saver.

## Requisitos

Para compilar y ejecutar este proyecto necesitas:

1. **Compilador C++** compatible con C++11 o superior (GCC/Clang).
2. **GLFW** – Para crear ventanas y manejar input.
3. **GLEW** – Para manejar extensiones de OpenGL.
4. **GLM** – Biblioteca de matemáticas para gráficos (header-only).
5. **OpenGL** y **GLUT** – Librerías gráficas básicas.

---

## Instalación en Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential g++ cmake
sudo apt install libglew-dev libglm-dev libglfw3-dev freeglut3-dev
```

* `build-essential` → herramientas de compilación básicas (gcc/g++/make).
* `libglew-dev` → GLEW.
* `libglm-dev` → GLM (header-only).
* `libglfw3-dev` → GLFW.
* `freeglut3-dev` → GLUT y OpenGL utilities.

---

## Compilación

- Ve al directorio de sequencial.
- Compila tu programa usando el comando estándar:

```bash
gcc main.c -o blackhole  -lGL -lGLU -lglut -lGLEW -lglfw -lm
```
