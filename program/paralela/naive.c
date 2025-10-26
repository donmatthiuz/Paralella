//bruteforce_optimizado.c
//Programa para cifrar/descifrar con DES y hacer bruteforce
//Versión optimizada con reporte de progreso

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <rpc/des_crypt.h>
#include <sys/time.h>
#include <time.h>

void decrypt(long key, char *ciph, int len){
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_DECRYPT);
}

void encrypt(long key, char *ciph, int len){
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_ENCRYPT);
}

int tryKey(long key, char *ciph, int len, char *search){
  char *temp = malloc(len + 1);
  memcpy(temp, ciph, len);
  temp[len] = 0;
  
  // Descifrar todos los bloques de 16 bytes
  for(int i = 0; i < len; i += 16){
    decrypt(key, temp + i, 16);
  }
  
  int found = strstr((char *)temp, search) != NULL;
  free(temp);
  return found;
}

void encrypt_file(char *input_file, char *output_file, long key) {
  struct timeval start, end;
  gettimeofday(&start, NULL);
  
  FILE *fin = fopen(input_file, "r");
  FILE *fout = fopen(output_file, "wb");
  
  if(!fin || !fout){
    printf("Error abriendo archivos\n");
    return;
  }
  
  // Leer todo el archivo
  fseek(fin, 0, SEEK_END);
  long file_size = ftell(fin);
  fseek(fin, 0, SEEK_SET);
  
  // Calcular tamaño con padding (múltiplo de 16)
  long padded_size = ((file_size + 15) / 16) * 16;
  char *buffer = malloc(padded_size);
  memset(buffer, 0, padded_size);
  
  // Leer contenido
  fread(buffer, 1, file_size, fin);
  
  // Padding con espacios
  for(long i = file_size; i < padded_size; i++){
    buffer[i] = ' ';
  }
  
  // Cifrar en bloques de 16 bytes
  for(long i = 0; i < padded_size; i += 16){
    encrypt(key, buffer + i, 16);
  }
  
  // Escribir tamaño original y datos cifrados
  fwrite(&file_size, sizeof(long), 1, fout);
  fwrite(buffer, 1, padded_size, fout);
  
  free(buffer);
  fclose(fin);
  fclose(fout);
  
  gettimeofday(&end, NULL);
  double elapsed = (end.tv_sec - start.tv_sec) + 
                   (end.tv_usec - start.tv_usec) / 1000000.0;
  
  printf("\n========================================\n");
  printf("=== CIFRADO COMPLETADO ===\n");
  printf("========================================\n");
  printf("Archivo cifrado guardado en: %s\n", output_file);
  printf("Tamaño original: %ld bytes\n", file_size);
  printf("Tamaño cifrado: %ld bytes\n", padded_size);
  printf("Clave usada: %ld\n", key);
  printf("Tiempo de cifrado: %.6f segundos\n", elapsed);
  printf("========================================\n");
}

void decrypt_file(char *input_file, char *output_file, long key) {
  struct timeval start, end;
  gettimeofday(&start, NULL);
  
  FILE *fin = fopen(input_file, "rb");
  FILE *fout = fopen(output_file, "w");
  
  if(!fin || !fout){
    printf("Error abriendo archivos\n");
    return;
  }
  
  // Leer tamaño original
  long original_size;
  fread(&original_size, sizeof(long), 1, fin);
  
  // Leer datos cifrados
  long padded_size = ((original_size + 15) / 16) * 16;
  char *buffer = malloc(padded_size);
  fread(buffer, 1, padded_size, fin);
  
  // Descifrar en bloques de 16 bytes
  for(long i = 0; i < padded_size; i += 16){
    decrypt(key, buffer + i, 16);
  }
  
  // Escribir solo el contenido original (sin padding)
  fwrite(buffer, 1, original_size, fout);
  
  gettimeofday(&end, NULL);
  double elapsed = (end.tv_sec - start.tv_sec) + 
                   (end.tv_usec - start.tv_usec) / 1000000.0;
  
  printf("\n========================================\n");
  printf("=== DESCIFRADO COMPLETADO ===\n");
  printf("========================================\n");
  printf("Archivo descifrado guardado en: %s\n", output_file);
  printf("Clave usada: %ld\n", key);
  printf("Tamaño descifrado: %ld bytes\n", original_size);
  printf("Contenido: ");
  for(long i = 0; i < original_size; i++){
    printf("%c", buffer[i]);
  }
  printf("\n");
  printf("Tiempo de descifrado: %.6f segundos\n", elapsed);
  printf("========================================\n");
  
  free(buffer);
  fclose(fin);
  fclose(fout);
}

void bruteforce_crack(char *cipher_file, char *search_text) {
  int N, id;
  long upper = (1L << 56);
  long mylower, myupper;
  MPI_Status st;
  MPI_Request req;
  MPI_Comm comm = MPI_COMM_WORLD;
  
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);
  
  // Leer archivo cifrado
  FILE *fin = fopen(cipher_file, "rb");
  if(!fin){
    if(id == 0) printf("Error abriendo archivo cifrado\n");
    MPI_Finalize();
    exit(1);
  }
  
  // Leer tamaño original
  long original_size;
  fread(&original_size, sizeof(long), 1, fin);
  
  // Leer datos cifrados
  long padded_size = ((original_size + 15) / 16) * 16;
  unsigned char *cipher = malloc(padded_size);
  fread(cipher, 1, padded_size, fin);
  fclose(fin);
  
  if(id == 0){
    printf("Tamaño original del archivo: %ld bytes\n", original_size);
    printf("Tamaño cifrado: %ld bytes\n", padded_size);
  }
  
  // Dividir trabajo
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) - 1;
  if(id == N-1){
    myupper = upper;
  }
  
  long found = 0;
  struct timeval start, end;
  
  // Sincronizar inicio
  MPI_Barrier(comm);
  
  if(id == 0){
    gettimeofday(&start, NULL);
    printf("\n========================================\n");
    printf("=== INICIANDO BRUTEFORCE ===\n");
    printf("========================================\n");
    printf("Fecha y hora de inicio: ");
    time_t now = time(NULL);
    printf("%s", ctime(&now));
    printf("Espacio de búsqueda: 0 a %ld (2^56)\n", upper);
    printf("Procesos: %d\n", N);
    printf("Rango por proceso: ~%ld claves\n", range_per_node);
    printf("Texto a buscar: '%s'\n", search_text);
    printf("========================================\n\n");
  }
  
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);
  
  // Variables para reporte de progreso
  long keys_checked = 0;
  long report_interval = 100000; // Reportar cada 100K claves
  struct timeval last_report, current_time, process_start;
  gettimeofday(&last_report, NULL);
  gettimeofday(&process_start, NULL);
  
  // Buscar clave
  for(long i = mylower; i < myupper && (found == 0); ++i){
    keys_checked++;
    
    // Verificar si otro proceso encontró la clave (cada 100K iteraciones)
    if(keys_checked % report_interval == 0){
      int flag = 0;
      MPI_Test(&req, &flag, &st);
      if(flag && found != 0){
        break;
      }
      
      // Reporte de progreso cada 5 segundos
      gettimeofday(&current_time, NULL);
      double elapsed = (current_time.tv_sec - last_report.tv_sec);
      if(elapsed >= 5.0){
        double progress = ((double)(i - mylower) / (double)(myupper - mylower)) * 100.0;
        double total_time = (current_time.tv_sec - process_start.tv_sec) + 
                           (current_time.tv_usec - process_start.tv_usec) / 1000000.0;
        double rate = (total_time > 0) ? keys_checked / total_time : 0;
        printf("[Proceso %d] Progreso: %.4f%% | Claves probadas: %ld | Velocidad: %.0f claves/seg\n", 
               id, progress, keys_checked, rate);
        gettimeofday(&last_report, NULL);
      }
    }
    
    if(tryKey(i, (char *)cipher, padded_size, search_text)){
      found = i;
      printf("\n[Proceso %d] ¡CLAVE ENCONTRADA: %ld!\n", id, found);
      for(int node=0; node<N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
      }
      break;
    }
  }
  
  // Esperar a que todos terminen
  MPI_Wait(&req, &st);
  MPI_Barrier(comm);
  
  if(id == 0){
    gettimeofday(&end, NULL);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_usec - start.tv_usec) / 1000000.0;
    
    if(found > 0){
      // Descifrar todo el contenido
      unsigned char *temp = malloc(padded_size);
      memcpy(temp, cipher, padded_size);
      
      for(long i = 0; i < padded_size; i += 16){
        decrypt(found, (char *)(temp + i), 16);
      }
      
      printf("\n========================================\n");
      printf("=== ¡CLAVE ENCONTRADA! ===\n");
      printf("========================================\n");
      printf("Clave: %ld (0x%lX)\n", found, found);
      printf("Texto descifrado: ");
      for(long i = 0; i < original_size; i++){
        printf("%c", temp[i]);
      }
      printf("\n");
      printf("----------------------------------------\n");
      printf("TIEMPOS DE EJECUCIÓN:\n");
      printf("----------------------------------------\n");
      printf("  %.6f segundos\n", elapsed);
      printf("  %.2f minutos\n", elapsed/60.0);
      printf("  %.2f horas\n", elapsed/3600.0);
      if(elapsed >= 86400){
        printf("  %.2f días\n", elapsed/86400.0);
      }
      printf("----------------------------------------\n");
      printf("ESTADÍSTICAS:\n");
      printf("----------------------------------------\n");
      printf("  Procesos usados: %d\n", N);
      printf("  Claves probadas: ~%ld\n", keys_checked * N);
      printf("  Velocidad promedio: %.2f millones de claves/seg\n", 
             (keys_checked * N) / elapsed / 1000000.0);
      printf("  Porcentaje del espacio explorado: %.6f%%\n", 
             ((double)(keys_checked * N) / (double)upper) * 100.0);
      printf("========================================\n");
      
      free(temp);
    } else {
      printf("\n========================================\n");
      printf("=== BÚSQUEDA COMPLETADA ===\n");
      printf("========================================\n");
      printf("¡Clave NO encontrada en el espacio de búsqueda!\n");
      printf("Tiempo de ejecución: %.6f segundos (%.2f horas)\n", elapsed, elapsed/3600.0);
      printf("Claves probadas: ~%ld\n", keys_checked * N);
      printf("========================================\n");
    }
  }
  
  free(cipher);
}


int main(int argc, char *argv[]){
  MPI_Init(&argc, &argv);
  
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  if(argc < 2){
    if(rank == 0){
      printf("Uso:\n");
      printf("  Cifrar:     mpirun -np N ./programa encrypt <archivo_entrada> <clave>\n");
      printf("  Descifrar:  mpirun -np N ./programa decrypt <archivo_cifrado> <clave>\n");
      printf("  Bruteforce: mpirun -np N ./programa crack <archivo_cifrado> <texto_a_buscar>\n");
      printf("\nEjemplos:\n");
      printf("  mpirun -np 1 --allow-run-as-root --mca plm isolated ./bruteforce encrypt texto.txt 123456\n");
      printf("  mpirun -np 1 --allow-run-as-root --mca plm isolated ./bruteforce decrypt texto.enc 123456\n");
      printf("  mpirun -np 4 --allow-run-as-root --mca plm isolated ./bruteforce crack texto.enc \" es una prueba de\"\n");
    }
    MPI_Finalize();
    return 1;
  }
  
  if(strcmp(argv[1], "encrypt") == 0 && argc >= 4){
    if(rank == 0){
      long key = atol(argv[3]);
      char output[256];
      snprintf(output, 256, "%s.enc", argv[2]);
      encrypt_file(argv[2], output, key);
    }
  }
  else if(strcmp(argv[1], "decrypt") == 0 && argc >= 4){
    if(rank == 0){
      long key = atol(argv[3]);
      char output[256];
      snprintf(output, 256, "%s.dec", argv[2]);
      decrypt_file(argv[2], output, key);
    }
  }
  else if(strcmp(argv[1], "crack") == 0 && argc >= 4){
    bruteforce_crack(argv[2], argv[3]);
  }
  else {
    if(rank == 0){
      printf("Comando no reconocido\n");
    }
  }
  
  MPI_Finalize();
  return 0;
}