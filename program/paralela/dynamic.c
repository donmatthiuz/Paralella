// des_bruteforce_dynamic.c
// Bruteforce DES distribuido con asignación dinámica de trabajo (Master-Worker)
// Para clusters MPI en red local

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <rpc/des_crypt.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MASTER 0
#define TAG_WORK_REQUEST 1
#define TAG_WORK_ASSIGN 2
#define TAG_FOUND 3
#define TAG_TERMINATE 4

// Tamaño de chunk dinámico (ajustable según rendimiento)
#define CHUNK_SIZE 100000L

typedef struct {
    long start;
    long end;
} WorkChunk;

void decrypt(long key, char *ciph, int len){
    long k = 0;
    for(int i=0; i<8; ++i){
        key <<= 1;
        k += (key & (0xFE << i*8));
    }
    des_setparity((char *)&k);
    ecb_crypt((char *)&k, (char *) ciph, 16, DES_DECRYPT);
}

int tryKey(long key, char *ciph, int len, char *search){
    char *temp = malloc(len + 1);
    memcpy(temp, ciph, len);
    temp[len] = 0;
    
    for(int i = 0; i < len; i += 16){
        decrypt(key, temp + i, 16);
    }
    
    int found = strstr((char *)temp, search) != NULL;
    free(temp);
    return found;
}

// ==================== PROCESO MAESTRO ====================
void master_process(int num_workers, long total_keys, char *cipher_file, char *search_text) {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║          BRUTEFORCE DES - MODO CLUSTER MPI            ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    // Leer archivo cifrado
    FILE *fin = fopen(cipher_file, "rb");
    if(!fin){
        printf("❌ Error abriendo archivo cifrado: %s\n", cipher_file);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    long original_size;
    fread(&original_size, sizeof(long), 1, fin);
    long padded_size = ((original_size + 15) / 16) * 16;
    unsigned char *cipher = malloc(padded_size);
    fread(cipher, 1, padded_size, fin);
    fclose(fin);
    
    printf(" Archivo: %s\n", cipher_file);
    printf(" Tamaño original: %ld bytes\n", original_size);
    printf(" Tamaño cifrado: %ld bytes\n", padded_size);
    printf(" Texto a buscar: '%s'\n", search_text);
    printf(" Workers disponibles: %d\n", num_workers);
    printf(" Tamaño de chunk: %ld claves\n", CHUNK_SIZE);
    printf(" Espacio total: 2^56 = %ld claves\n\n", total_keys);
    
    // Broadcast de datos cifrados a todos los workers
    printf("Enviando datos cifrados a los workers...\n");
    MPI_Bcast(&original_size, 1, MPI_LONG, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&padded_size, 1, MPI_LONG, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(cipher, padded_size, MPI_UNSIGNED_CHAR, MASTER, MPI_COMM_WORLD);
    
    int search_len = strlen(search_text) + 1;
    MPI_Bcast(&search_len, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(search_text, search_len, MPI_CHAR, MASTER, MPI_COMM_WORLD);
    
    printf("✅ Datos distribuidos correctamente\n\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf(" INICIANDO BÚSQUEDA DISTRIBUIDA\n");
    printf("═══════════════════════════════════════════════════════\n\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // Estado del trabajo
    long current_key = 0;
    long chunks_sent = 0;
    long chunks_completed = 0;
    long total_chunks = (total_keys + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int active_workers = 0;
    long found_key = 0;
    
    // Estadísticas por worker
    long *keys_per_worker = calloc(num_workers + 1, sizeof(long));
    
    struct timeval last_report;
    gettimeofday(&last_report, NULL);
    
    // Loop principal del maestro
    while(chunks_completed < total_chunks && found_key == 0) {
        MPI_Status status;
        int message[2]; // [0] = tipo de mensaje, [1] = worker_id o resultado
        
        // Recibir solicitud de trabajo o resultado
        MPI_Recv(message, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        int source = status.MPI_SOURCE;
        int tag = status.MPI_TAG;
        
        if(tag == TAG_FOUND) {
            // ¡Clave encontrada!
            long key_found;
            MPI_Recv(&key_found, 1, MPI_LONG, source, TAG_FOUND, MPI_COMM_WORLD, &status);
            found_key = key_found;
            printf("\n ¡CLAVE ENCONTRADA POR WORKER %d!\n", source);
            printf(" Clave: %ld (0x%lX)\n\n", found_key, found_key);
            break;
            
        } else if(tag == TAG_WORK_REQUEST) {
            chunks_completed++;
            
            if(message[1] >= 0) { // Worker completó un chunk
                keys_per_worker[source] += CHUNK_SIZE;
            }
            
            // Asignar nuevo chunk si hay trabajo disponible
            if(current_key < total_keys && found_key == 0) {
                WorkChunk chunk;
                chunk.start = current_key;
                chunk.end = (current_key + CHUNK_SIZE < total_keys) ? 
                            current_key + CHUNK_SIZE : total_keys;
                
                MPI_Send(&chunk, sizeof(WorkChunk), MPI_BYTE, source, 
                         TAG_WORK_ASSIGN, MPI_COMM_WORLD);
                
                current_key += CHUNK_SIZE;
                chunks_sent++;
                active_workers++;
                
            } else {
                // No hay más trabajo, enviar señal de terminación
                WorkChunk terminate = {-1, -1};
                MPI_Send(&terminate, sizeof(WorkChunk), MPI_BYTE, source, 
                         TAG_TERMINATE, MPI_COMM_WORLD);
                active_workers--;
            }
            
            // Reporte de progreso cada 3 segundos
            struct timeval now;
            gettimeofday(&now, NULL);
            double elapsed_report = (now.tv_sec - last_report.tv_sec);
            
            if(elapsed_report >= 3.0) {
                double progress = ((double)chunks_completed / (double)total_chunks) * 100.0;
                double total_elapsed = (now.tv_sec - start.tv_sec) + 
                                      (now.tv_usec - start.tv_usec) / 1000000.0;
                long total_keys_checked = chunks_completed * CHUNK_SIZE;
                double rate = (total_elapsed > 0) ? total_keys_checked / total_elapsed : 0;
                
                printf(" Progreso: %.4f%% | Chunks: %ld/%ld | Workers activos: %d | Velocidad: %.2f M claves/seg\n",
                       progress, chunks_completed, total_chunks, active_workers, rate / 1000000.0);
                
                gettimeofday(&last_report, NULL);
            }
        }
    }
    
    // Terminar todos los workers
    printf("\n Enviando señal de terminación a todos los workers...\n");
    for(int i = 1; i <= num_workers; i++) {
        WorkChunk terminate = {-1, -1};
        MPI_Send(&terminate, sizeof(WorkChunk), MPI_BYTE, i, 
                 TAG_TERMINATE, MPI_COMM_WORLD);
    }
    
    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_usec - start.tv_usec) / 1000000.0;
    
    // Resultados finales
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    
    if(found_key > 0) {
        printf("║               ✅ BÚSQUEDA EXITOSA                      ║\n");
        printf("╚════════════════════════════════════════════════════════╝\n\n");
        
        // Descifrar mensaje completo
        unsigned char *temp = malloc(padded_size);
        memcpy(temp, cipher, padded_size);
        for(long i = 0; i < padded_size; i += 16){
            decrypt(found_key, (char *)(temp + i), 16);
        }
        
        printf(" Clave encontrada: %ld (0x%lX)\n", found_key, found_key);
        printf("\n Texto descifrado:\n");
        printf("─────────────────────────────────────────────────────────\n");
        for(long i = 0; i < original_size; i++){
            printf("%c", temp[i]);
        }
        printf("\n─────────────────────────────────────────────────────────\n\n");
        free(temp);
        
    } else {
        printf("║               CLAVE NO ENCONTRADA                   ║\n");
        printf("╚════════════════════════════════════════════════════════╝\n\n");
    }
    
    // Estadísticas
    printf(" ESTADÍSTICAS DE EJECUCIÓN:\n");
    printf("─────────────────────────────────────────────────────────\n");
    printf("  Tiempo total: %.2f segundos (%.2f minutos)\n", elapsed, elapsed/60.0);
    if(elapsed >= 3600) printf("               %.2f horas\n", elapsed/3600.0);
    printf("  Chunks procesados: %ld de %ld\n", chunks_completed, total_chunks);
    printf("  Claves probadas: ~%ld\n", chunks_completed * CHUNK_SIZE);
    printf("  Velocidad promedio: %.2f millones de claves/seg\n", 
           (chunks_completed * CHUNK_SIZE) / elapsed / 1000000.0);
    printf("  Workers utilizados: %d\n", num_workers);
    
    printf("\n DISTRIBUCIÓN DE TRABAJO:\n");
    printf("─────────────────────────────────────────────────────────\n");
    for(int i = 1; i <= num_workers; i++) {
        printf("  Worker %2d: %ld claves (%.2f%%)\n", i, keys_per_worker[i],
               (double)keys_per_worker[i] / (chunks_completed * CHUNK_SIZE) * 100.0);
    }
    printf("═════════════════════════════════════════════════════════\n\n");
    
    free(cipher);
    free(keys_per_worker);
}

// ==================== PROCESO WORKER ====================
void worker_process(int rank) {
    // Recibir datos cifrados del maestro
    long original_size, padded_size;
    MPI_Bcast(&original_size, 1, MPI_LONG, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&padded_size, 1, MPI_LONG, MASTER, MPI_COMM_WORLD);
    
    unsigned char *cipher = malloc(padded_size);
    MPI_Bcast(cipher, padded_size, MPI_UNSIGNED_CHAR, MASTER, MPI_COMM_WORLD);
    
    int search_len;
    MPI_Bcast(&search_len, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    char *search_text = malloc(search_len);
    MPI_Bcast(search_text, search_len, MPI_CHAR, MASTER, MPI_COMM_WORLD);
    
    printf(" Worker %d: Datos recibidos, listo para trabajar\n", rank);
    
    // Solicitar trabajo inicial
    int request[2] = {0, -1}; // -1 indica primera solicitud
    MPI_Send(request, 2, MPI_INT, MASTER, TAG_WORK_REQUEST, MPI_COMM_WORLD);
    
    while(1) {
        WorkChunk chunk;
        MPI_Status status;
        
        MPI_Recv(&chunk, sizeof(WorkChunk), MPI_BYTE, MASTER, 
                 MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        if(status.MPI_TAG == TAG_TERMINATE || chunk.start == -1) {
            printf(" Worker %d: Terminando\n", rank);
            break;
        }
        
        // Procesar chunk
        long found = 0;
        for(long key = chunk.start; key < chunk.end && found == 0; key++) {
            if(tryKey(key, (char *)cipher, padded_size, search_text)) {
                found = key;
                
                // Notificar al maestro
                int notify[2] = {1, rank};
                MPI_Send(notify, 2, MPI_INT, MASTER, TAG_FOUND, MPI_COMM_WORLD);
                MPI_Send(&found, 1, MPI_LONG, MASTER, TAG_FOUND, MPI_COMM_WORLD);
                break;
            }
        }
        
        if(found == 0) {
            // Solicitar más trabajo
            int req[2] = {0, rank};
            MPI_Send(req, 2, MPI_INT, MASTER, TAG_WORK_REQUEST, MPI_COMM_WORLD);
        } else {
            break;
        }
    }
    
    free(cipher);
    free(search_text);
}

// ==================== MAIN ====================
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if(size < 2) {
        if(rank == MASTER) {
            printf(" Error: Se requieren al menos 2 procesos (1 maestro + 1 worker)\n");
            printf("Uso: mpirun -np N ./des_bruteforce_dynamic <archivo> <texto>\n");
            printf("Ejemplo con hostfile:\n");
            printf("  mpirun -np 8 --hostfile hosts.txt ./des_bruteforce_dynamic texto.txt.enc \" es una prueba\"\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    if(argc < 3) {
        if(rank == MASTER) {
            printf("\n╔════════════════════════════════════════════════════════╗\n");
            printf("║      BRUTEFORCE DES - CLUSTER MPI DINÁMICO            ║\n");
            printf("╚════════════════════════════════════════════════════════╝\n\n");
            printf("Uso:\n");
            printf("  mpirun -np N ./des_bruteforce_dynamic <archivo_cifrado> <texto_buscar>\n\n");
            printf("Ejemplo con cluster local:\n");
            printf("  mpirun -np 8 --hostfile hosts.txt ./des_bruteforce_dynamic texto.enc \"prueba\"\n\n");
            printf("Ejemplo con nodos específicos:\n");
            printf("  mpirun -H node1:4,node2:4 ./des_bruteforce_dynamic texto.enc \"prueba\"\n\n");
            printf("Archivo hosts.txt ejemplo:\n");
            printf("  192.168.1.10 slots=4\n");
            printf("  192.168.1.11 slots=4\n");
            printf("  192.168.1.12 slots=2\n\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    long total_keys = (1L << 56);
    int num_workers = size - 1;
    
    if(rank == MASTER) {
        master_process(num_workers, total_keys, argv[1], argv[2]);
    } else {
        worker_process(rank);
    }
    
    MPI_Finalize();
    return 0;
}