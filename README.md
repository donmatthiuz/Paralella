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
