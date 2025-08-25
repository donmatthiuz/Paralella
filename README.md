
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

Puedes compilar tu programa usando el comando estándar:

```bash
g++ hoyo_negrote.cpp -o hoyo_negrote -lGLEW -lGL -lGLU -lglut -lglfw
```

Si tienes varios archivos `.cpp` (por ejemplo, separando `Motor.cpp` y `main.cpp`):

```bash
g++ main.cpp Motor.cpp -o hoyo_negrote -lGLEW -lGL -lGLU -lglut -lglfw
```

> **Nota:** El orden importa: primero los archivos `.cpp`, después las librerías (`-lGLEW -lGL ... -lglfw`).

---

## Ejecución

Después de compilar:

```bash
./hoyo_negrote
```

* Se abrirá una ventana de 800x600 con un fondo negro.
* Para cerrar, usa la X de la ventana o presiona **ESC** (si implementas la captura de teclas).

---

## Personalización

* **Tamaño de la ventana:** cambia `WIDTH` y `HEIGHT` en `Motor`.
* **Escala de coordenadas:** cambia `width` y `height` en `Motor`.
* **Color de fondo:** modifica `glClearColor(r, g, b, a)` en el constructor de `Motor`.

---

## Función `buildgl` (opcional)

Si quieres usar un alias para compilar fácilmente:

```bash
buildgl() {
    if [ $# -eq 0 ]; then
        echo "Uso: buildgl archivo1.cpp [archivo2.cpp ...]"
        return 1
    fi
    exe="${1%.*}"
    g++ "$@" -o "$exe" -lGLEW -lGL -lGLU -lglut -lglfw
    echo "Compilación completada. Ejecutable: $exe"
}
```

Luego, en la terminal:

```bash
buildgl hoyo_negrote.cpp
./hoyo_negrote
```

---
