

### 1️⃣ Instalar dependencias

#### **En Linux (Ubuntu/Debian)**

```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libglfw3-dev libglew-dev libglm-dev libx11-dev libxcursor-dev libxi-dev libxrandr-dev libxinerama-dev libopenal-dev
```

#### **En Windows**

* Descarga e instala:

  * [GLEW](http://glew.sourceforge.net/)
  * [GLFW](https://www.glfw.org/download.html)
* Configura tu IDE (Visual Studio, Code::Blocks, etc.) para linkear las librerías y agregar los `.dll` al ejecutable o al PATH.



---

### 2️⃣ Compilar el programa

```bash
gcc main.c -o particle_sim -lGL -lGLEW -lglfw -lm
```

* `-lGL` → OpenGL
* `-lGLEW` → GLEW
* `-lglfw` → GLFW
* `-lm` → math (`sin`, `cos`, `sqrt`, etc.)

---

### 4️⃣ Ejecutar Serial

Después de compilar:

#### Linux/macOS

```bash
./particle_sim  1000
```

El 1000 es el numero de particulas, ve variando


### 5️⃣ Ejecutar Paralella


```bash

gcc paralel.c -o particle_sim_parallel -lGL -lGLEW -lglfw -lm -fopenmp

```