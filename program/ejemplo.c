#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);  // Inicializa MPI

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);  // ID del proceso

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);  // Total de procesos

    printf("Hola desde proceso %d de %d\n", world_rank, world_size);

    MPI_Finalize();  // Finaliza MPI
    return 0;
}
