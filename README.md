# Proyecto 2


# Usar contenedor Docker C con OpenMP/OpenMPI

## Ejecutar el contenedor en segundo plano
```bash
docker compose up -d
````

## Acceder al contenedor

```bash
docker exec -it dev-c bash
```

## Dentro del contenedor prueba el ejemplo de open mpi

```bash
mpicc ejemplo.c -o ejemplo
```


## Para compilar bruteforce ve a la carpeta de paralela

```bash
mpicc -o bruteforce.o bruteforce.c
```

## Para ver que funciona bruteforce ejecuta
```bash
mpirun -np 4 --allow-run-as-root --mca plm isolated ./bruteforce.o
```

## Naive

```bash
mpicc -o naived.o naive.c -lcrypt
```

### Cifrar clave

```bash
mpirun -np 1 --allow-run-as-root --mca plm isolated ./naived.o encrypt ./data/texto.txt 123456
```

### Verificar cifrado 

mpirun -np 1 --allow-run-as-root --mca plm isolated ./naived.o decrypt ./data/texto.txt.enc 123456

### Decifrar clave

```bash
mpirun -np 4 --allow-run-as-root --mca plm isolated ./naived.o  crack ./data/texto.txt.enc " es una prueba"
```
