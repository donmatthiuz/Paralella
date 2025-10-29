#include <stdio.h>
#include <mpi.h>


#define TAG_FOUND_KEY = 1
#define TAG_MORE_JOB = 2
#define TAG_FINISH = 3
#define TAG_SEND_JOB = 4
const long chunk_size = 10000000;


void decrypt(long key, char *ciph, int len){
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, len, DES_DECRYPT); 
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

void master(int size){
    long next_key_to_test = (size * (chunk_size -1 ));


    long found = 0;
    int active_workers = size -1;
    while (found == 0 && active_workers > 0){
        MPI_Status status;
        int mensaje;
            
        // Recibir cualquier mensaje de cualquier trabajador
        MPI_Recv(&mensaje, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, 
                    MPI_COMM_WORLD, &status);
        
        int worker = status.MPI_SOURCE;
        int tag = status.MPI_TAG;


        if (tag == TAG_FOUND_KEY){

            printf("¡Trabajador %d encontró la llave: %d!\n", trabajador, mensaje);
                found = 1;
                
            // Enviar señal de finalizar a todos
            for (int i = 1; i < size; i++) {
                int fin = -1;
                MPI_Send(&fin, 1, MPI_INT, i, TAG_FINALIZAR, MPI_COMM_WORLD);
            }

        }
        else {

            if (found) {
                    // Ya encontraron la llave, finalizar este trabajador
                    int fin = -1;
                    MPI_Send(&fin, 1, MPI_INT, worker, TAG_FINISH, MPI_COMM_WORLD);
                } else {
                    // Enviar más trabajo
                    next_key_to_test += chunk_size;
                    long new_job_limit = next_key_to_test // Ejemplo
                    MPI_Send(&nuevo_trabajo, 1, MPI_INT, trabajador, 
                                TAG_ENVIAR_TRABAJO, MPI_COMM_WORLD);
                    printf("Enviando trabajo %d al proceso %d\n", nuevo_trabajo, trabajador);
                }
                active_workers--;

        }

    }
    long next_keys_range = 0;
    return;
}

void worker(int rank){
    bool can_continue = 1;
    long max = rank * chunk_size;
    long min = max - chunk_size;
    FILE *fin = fopen(cipher_file, "rb");
    if (!fin) {
        fprintf(stderr, "Error abriendo archivo cifrado '%s': %s\n", cipher_file, strerror(errno));
        MPI_Finalize();
    }

    long original_size = 0;
    if (fread(&original_size, sizeof(long), 1, fin) != 1) {
        fprintf(stderr, "Error leyendo tamaño original del archivo\n");
        fclose(fin);
        MPI_Finalize();
    }

    size_t padded_size = ((original_size + 15) / 16) * 16;
    unsigned char *cipher = malloc(padded_size);
    if (!cipher) {
        fprintf(stderr, "Error al asignar memoria para cipher\n");
        fclose(fin);
        MPI_Finalize();
    }

    if (fread(cipher, 1, padded_size, fin) != padded_size) {
        fprintf(stderr, "Error leyendo datos cifrados\n");
        free(cipher);
        fclose(fin);
        MPI_Finalize();
    }
    fclose(fin);

    printf("Tamaño original: %ld bytes, tamaño cifrado (padded): %zu bytes\n", original_size, padded_size);struct timespec t_start, t_now;
    

    while (can_continue){

        for (long k = min; k<max; k++){
            if (tryKey(k, (char *)cipher, padded_size, (char *)search_text)) {
                printf("LLAVE ENCONTRADA: %ld\n", k);
                MPI_Send(&k, 1, MPI_INT, 0, TAG_ENCONTRE_LLAVE, MPI_COMM_WORLD);
                break;
            }
        }

        // No encontré nada, pedir más trabajo
                MPI_Send(&trabajo_actual, 1, MPI_INT, 0, TAG_PEDIR_TRABAJO, MPI_COMM_WORLD);
                
                // Esperar respuesta del maestro
                MPI_Status status;
                int respuesta;
                MPI_Recv(&respuesta, 1, MPI_INT, 0, MPI_ANY_TAG, 
                         MPI_COMM_WORLD, &status);
                
                if (status.MPI_TAG == TAG_FINALIZAR) {
                    printf("Trabajador %d: Recibí orden de finalizar\n", rank);
                    continuar = false;
                } else if (status.MPI_TAG == TAG_ENVIAR_TRABAJO) {
                    trabajo_actual = respuesta;
                    printf("Trabajador %d: Recibí nuevo trabajo %d\n", rank, trabajo_actual);
                }



    }
    return;
}
int main(int argc, char** argv){
    int process_Rank, size_Of_Cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_Of_Cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_Rank);



    if (process_Rank == 0){
        master();
    }
    else {
        worker();
    }

    MPI_Finalize();
    return 0;
}


    

