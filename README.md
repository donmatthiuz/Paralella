# Proyecto 2

````markdown
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
