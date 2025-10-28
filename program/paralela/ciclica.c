// ciclica.c
// Búsqueda por fuerza bruta con distribución cíclica de claves MPI

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <rpc/des_crypt.h>
#include <sys/time.h>
#include <time.h>

void decrypt(long key, char *ciph, int len){
  long k = 0;
  long tmp = key;
  for(int i=0; i<8; ++i){
    tmp <<= 1;
    k += (tmp & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_DECRYPT);
}

int tryKey(long key, char *ciph, int len, char *search){
  char *temp = malloc(len + 1);
  if(!temp) return 0;
  memcpy(temp, ciph, len);
  temp[len] = 0;
  
  for(int i = 0; i < len; i += 16){
    decrypt(key, temp + i, 16);
  }
  
  int found = (strstr((char *)temp, search) != NULL);
  free(temp);
  return found;
}

void bruteforce_ciclica(char *cipher_file, char *search_text) {
  int N, id;
  const long upper = (1L << 56); // espacio de búsqueda 2^56
  MPI_Status st;
  MPI_Request req;
  MPI_Comm comm = MPI_COMM_WORLD;
  
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  
  FILE *fin = fopen(cipher_file, "rb");
  if(!fin){
    if(id == 0) printf("Error abriendo archivo cifrado\n");
    MPI_Finalize();
    exit(1);
  }
  
  long original_size;
  fread(&original_size, sizeof(long), 1, fin);
  
  long padded_size = ((original_size + 15) / 16) * 16;
  unsigned char *cipher = malloc(padded_size);
  if(!cipher){
    if(id==0) fprintf(stderr, "Error al asignar memoria para cipher\n");
    fclose(fin);
    MPI_Finalize();
    exit(1);
  }
  fread(cipher, 1, padded_size, fin);
  fclose(fin);
  
  if(id == 0){
    printf("Tamaño original del archivo: %ld bytes\n", original_size);
    printf("Tamaño cifrado: %ld bytes\n", padded_size);
  }
  
  long found = 0; // la llave encontrada (0 = no encontrada)
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);
  
  long keys_checked = 0;
  long report_interval = 100000; 
  struct timeval last_report, process_start, global_start;
  gettimeofday(&last_report, NULL);
  gettimeofday(&process_start, NULL);
  MPI_Barrier(comm);
  if(id == 0){
    gettimeofday(&global_start, NULL);
    printf("\n========================================\n");
    printf("=== INICIANDO BRUTEFORCE (CICLICA) ===\n");
    printf("Fecha y hora de inicio: ");
    time_t now = time(NULL);
    printf("%s", ctime(&now));
    printf("Espacio de búsqueda: 0 a %ld (2^56)\n", upper);
    printf("Procesos: %d\n", N);
    printf("Distribución: cíclica (cada proceso prueba claves rank + k*N)\n");
    printf("Texto a buscar: '%s'\n", search_text);
    printf("========================================\n\n");
  }
  
  for(long key = id; key < upper && (found == 0); key += N){
    keys_checked++;
    
    if((keys_checked % report_interval) == 0){
      int flag = 0;
      MPI_Test(&req, &flag, &st);
      if(flag && found != 0){
        break;
      }
      
      struct timeval current_time;
      gettimeofday(&current_time, NULL);
      double elapsed = (current_time.tv_sec - last_report.tv_sec) + 
                       (current_time.tv_usec - last_report.tv_usec) / 1000000.0;
      if(elapsed >= 5.0){
        double total_time = (current_time.tv_sec - process_start.tv_sec) + 
                            (current_time.tv_usec - process_start.tv_usec) / 1000000.0;
        double rate = (total_time > 0) ? keys_checked / total_time : 0;
        double approx_progress = ((double)keys_checked * (double)N) / (double)upper * 100.0;
        printf("[Proceso %d] Progreso aproximado: %.6f%% | Claves probadas: %ld | Velocidad: %.0f claves/seg\n",
               id, approx_progress, keys_checked, rate);
        gettimeofday(&last_report, NULL);
      }
    }
    
    if(tryKey(key, (char *)cipher, padded_size, search_text)){
      found = key;
      printf("\n[Proceso %d] ¡CLAVE ENCONTRADA: %ld!\n", id, found);
      // Notificar a todos los procesos la llave encontrada
      for(int node=0; node < N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
      }
      break;
    }
  }
  
  // Esperar notificación si otro encontró la llave
  MPI_Wait(&req, &st);
  MPI_Barrier(comm);
  
  long total_checked = 0;
  MPI_Reduce(&keys_checked, &total_checked, 1, MPI_LONG, MPI_SUM, 0, comm);
  
  if(id == 0){
    struct timeval global_end;
    gettimeofday(&global_end, NULL);
    double elapsed = (global_end.tv_sec - global_start.tv_sec) + 
                     (global_end.tv_usec - global_start.tv_usec) / 1000000.0;
    
    if(found > 0){
      // Descifrar y mostrar
      unsigned char *temp = malloc(padded_size);
      if(temp){
        memcpy(temp, cipher, padded_size);
        for(long i = 0; i < padded_size; i += 16){
          decrypt(found, (char *)(temp + i), 16);
        }
        printf("\n========================================\n");
        printf("=== ¡CLAVE ENCONTRADA! (CICLICA) ===\n");
        printf("========================================\n");
        printf("Clave: %ld (0x%lX)\n", found, found);
        printf("Texto descifrado: ");
        for(long i = 0; i < original_size; i++){
          printf("%c", temp[i]);
        }
        printf("\n----------------------------------------\n");
        printf("TIEMPOS DE EJECUCIÓN:\n");
        printf("----------------------------------------\n");
        printf("  %.6f segundos\n", elapsed);
        printf("  %.2f minutos\n", elapsed/60.0);
        printf("  %.2f horas\n", elapsed/3600.0);
        if(elapsed >= 86400) printf("  %.2f días\n", elapsed/86400.0);
        printf("----------------------------------------\n");
        printf("ESTADÍSTICAS:\n");
        printf("----------------------------------------\n");
        printf("  Procesos usados: %d\n", N);
        printf("  Claves probadas (estimado total): %ld\n", total_checked);
        printf("  Velocidad promedio: %.2f millones de claves/seg\n", 
               (total_checked) / elapsed / 1000000.0);
        printf("  Porcentaje del espacio explorado: %.6f%%\n", 
               ((double)total_checked / (double)upper) * 100.0);
        printf("========================================\n");
        free(temp);
      } else {
        printf("No se pudo asignar memoria para mostrar el mensaje descifrado\n");
      }
    } else {
      printf("\n========================================\n");
      printf("=== BÚSQUEDA COMPLETADA (CICLICA) ===\n");
      printf("Clave NO encontrada en el espacio de búsqueda!\n");
      printf("Tiempo de ejecución: %.6f segundos (%.2f horas)\n", elapsed, elapsed/3600.0);
      printf("Claves probadas (estimado total): %ld\n", total_checked);
      printf("========================================\n");
    }
  }
  
  free(cipher);
}

int main(int argc, char *argv[]){
  MPI_Init(&argc, &argv);
  
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  if(argc < 3){
    if(rank == 0){
      printf("Uso:\n");
      printf("  mpirun -np N ./ciclica <archivo_cifrado> <texto_a_buscar>\n");
      printf("\nEjemplo:\n");
      printf("  mpirun -np 4 --allow-run-as-root --mca plm isolated ./ciclica texto.txt.enc \" es una prueba de\"\n");
      printf("\nNota: El texto a buscar debe estar entre comillas si contiene espacios\n");
    }
    MPI_Finalize();
    return 1;
  }
  
  bruteforce_ciclica(argv[1], argv[2]);
  
  MPI_Finalize();
  return 0;
}

