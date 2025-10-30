


# 🧩 Proyecto 2

## 🐋 Usar contenedor Docker C con OpenMP/OpenMPI

### Ejecutar el contenedor en segundo plano

```bash
docker compose up -d
```

**Explicación:**

* `docker compose up`: levanta y ejecuta los servicios definidos en el archivo `docker-compose.yml`.
* `-d`: indica que el contenedor se ejecute en modo *detached*, es decir, en segundo plano.

---

### Acceder al contenedor

```bash
docker exec -it dev-c bash
```

**Explicación:**

* `docker exec`: ejecuta un comando dentro de un contenedor en ejecución.
* `-it`: abre una sesión interactiva con terminal (modo consola).
* `dev-c`: nombre del contenedor (debe coincidir con el definido en tu `docker-compose.yml`).
* `bash`: abre una terminal bash dentro del contenedor.



---

## 🚀 Para compilar el programa de fuerza bruta


Accede a la carpeta /program/paralela y ejecuta

```bash
mpicc -o bruteforce.o bruteforce.c
```

**Explicación:**

* `mpicc`: compilador compatible con MPI.
* `-o bruteforce.o`: nombre del ejecutable resultante.
* `bruteforce.c`: archivo fuente del programa de fuerza bruta.

---

## ▶️ Para ejecutar el programa de fuerza bruta

```bash
mpirun -np 4 --allow-run-as-root --mca plm isolated ./bruteforce.o
```

**Explicación:**

* `mpirun`: ejecuta un programa MPI en paralelo.
* `-np 4`: usa 4 procesos en paralelo.
* `--allow-run-as-root`: permite ejecutar el programa incluso si el usuario es `root` (necesario dentro de algunos contenedores Docker).
* `--mca plm isolated`: desactiva el gestor de procesos externo de MPI (recomendado para contenedores).
* `./bruteforce.o`: ejecutable que se va a correr.

---

# 💻 Programa

Dentro del proyecto, en la carpeta `program`.

---

## 🔐 CIFRADO / DECIFRADO

### Compilación

```bash
gcc des_cipher.c -o des_cipher.o -lcrypt
```

**Explicación:**

* `gcc`: compilador de C estándar.
* `des_cipher.c`: archivo fuente del programa de cifrado/descifrado.
* `-o des_cipher.o`: genera el ejecutable con ese nombre.
* `-lcrypt`: enlaza la librería del sistema `crypt` (usada para funciones de cifrado DES).

---

### Cifrar un texto con una clave

```bash
./des_cipher.o encrypt ./data/texto.txt 123456
```

**Explicación:**

* `./des_cipher.o`: ejecutable del programa.
* `encrypt`: modo de operación → indica que se va a cifrar.
* `./data/texto.txt`: archivo de texto de entrada que se desea cifrar.
* `123456`: clave utilizada para el cifrado.

---

### Descifrar un texto con una clave

```bash
./des_cipher.o decrypt ./data/texto.txt.enc 123456
```

**Explicación:**

* `decrypt`: modo de operación → indica que se va a descifrar.
* `./data/texto.txt.enc`: archivo previamente cifrado (extensión `.enc`).
* `123456`: clave usada originalmente para cifrar (necesaria para recuperar el texto original).

---

## 🧠 FUERZA BRUTA

### Compilación

#### Naive Paralelo

```bash
mpicc ./paralela/naive.c -o ./paralela/naived.o -lcrypt
```

#### Ciclico 

```bash
mpicc ./paralela/ciclico.c -o ./paralela/ciclico.o -lcrypt
```


**Explicación:**

* `mpicc`: compilador para programas MPI.
* `./paralela/naive.c`: código fuente del ataque de fuerza bruta.
* `-o ./paralela/naived.o`: nombre del ejecutable.
* `-lcrypt`: librería de cifrado necesaria para comparar hashes DES.

#### Secuencial

```bash
gcc -./sequencial/sequencial.c -o ./sequencial/sequencial.o
```

---

### Ejecutar el ataque de fuerza bruta secuencial

```bash
./sequencial/sequencial ./data/texto.txt.enc "frase_o_palabra_del_texto" 0 999999

```

**Explicación:**

* `mpirun`: ejecuta el programa en paralelo con MPI.
* `-np 4`: usa 4 procesos de ejecución.
* `--allow-run-as-root`: permite correrlo como usuario root (requerido en Docker).
* `--mca plm isolated`: ejecuta los procesos MPI sin un gestor externo (modo aislado).
* `./paralela/naived.o`: ejecutable del ataque de fuerza bruta.
* `./data/texto.txt.enc`: archivo cifrado a descifrar.
* `" es una prueba"`: texto esperado dentro del mensaje descifrado (se usa como validación para saber si la clave encontrada es correcta).

---

### Ejecutar el ataque de fuerza bruta paralelo

```bash
mpirun -np 4 --allow-run-as-root --mca plm isolated ./paralela/naived.o ./data/texto.txt.enc " es una prueba"
```

#### Ciclico

```sh
mpirun -np 4 --allow-run-as-root --mca plm isolated ./paralela/ciclica.o ./data/texto.txt.enc " es una prueba de"
```
**Explicación:**

* `mpirun`: ejecuta el programa en paralelo con MPI.
* `-np 4`: usa 4 procesos de ejecución.
* `--allow-run-as-root`: permite correrlo como usuario root (requerido en Docker).
* `--mca plm isolated`: ejecuta los procesos MPI sin un gestor externo (modo aislado).
* `./paralela/naived.o`: ejecutable del ataque de fuerza bruta.
* `./data/texto.txt.enc`: archivo cifrado a descifrar.
* `" es una prueba"`: texto esperado dentro del mensaje descifrado (se usa como validación para saber si la clave encontrada es correcta).

---

## 🧠 Dinamica con clusters


### Prueba en el docker de master
```sh
mpirun -np 12 --allow-run-as-root \
    --mca plm_rsh_agent /usr/local/bin/mpi-docker-exec \
    --hostfile hosts.txt \
    hostname
```

### Ejecutar la prueba

```sh
mpirun -np 12 --allow-run-as-root \
    --mca plm_rsh_agent /usr/local/bin/mpi-docker-exec \
    --mca btl_tcp_if_include 10.20.1.0/24 \
    --mca oob_tcp_if_include 10.20.1.0/24 \
    --hostfile hosts.txt \
    /usr/src/app/program/paralela/Dinamica_PL/des_bruteforce_dynamic \
    /usr/src/app/program/data/texto.txt.enc " es una prueba"
```