#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc/des_crypt.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>

void decrypt(long key, char *ciph, int len){
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, len, DES_DECRYPT);  // usa len, no fijo 16
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

long brute_force_sequential(const char *cipher_file, const char *search_text,
                            uint64_t min_key, uint64_t max_key)
{
    FILE *fin = fopen(cipher_file, "rb");
    if (!fin) {
        fprintf(stderr, "Error abriendo archivo cifrado '%s': %s\n", cipher_file, strerror(errno));
        return -1;
    }

    long original_size = 0;
    if (fread(&original_size, sizeof(long), 1, fin) != 1) {
        fprintf(stderr, "Error leyendo tamaño original del archivo\n");
        fclose(fin);
        return -1;
    }

    size_t padded_size = ((original_size + 15) / 16) * 16;
    unsigned char *cipher = malloc(padded_size);
    if (!cipher) {
        fprintf(stderr, "Error al asignar memoria para cipher\n");
        fclose(fin);
        return -1;
    }

    if (fread(cipher, 1, padded_size, fin) != padded_size) {
        fprintf(stderr, "Error leyendo datos cifrados\n");
        free(cipher);
        fclose(fin);
        return -1;
    }
    fclose(fin);

    printf("Tamaño original: %ld bytes, tamaño cifrado (padded): %zu bytes\n", original_size, padded_size);struct timespec t_start, t_now;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    uint64_t keys_tried = 0;
    uint64_t total_keys = (uint64_t)max_key - (uint64_t)min_key + 1;
    const uint64_t PROGRESS_INTERVAL = 100000;

    long found = 0;
    for (long k = min_key; k <= max_key; ++k) {
        keys_tried++; 
        if (tryKey(k, (char *)cipher, padded_size, (char *)search_text)) {
            printf("LLAVE ENCONTRADA: %ld\n", k);
            found = 1;
            break;
        }
        if ((keys_tried % PROGRESS_INTERVAL) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &t_now);
            double elapsed = (t_now.tv_sec - t_start.tv_sec) +
                            (t_now.tv_nsec - t_start.tv_nsec) / 1e9;
            double keys_per_sec = keys_tried / elapsed;
            uint64_t remaining = total_keys - keys_tried;
            double eta_seconds = remaining / (keys_per_sec > 0 ? keys_per_sec : 1.0);

            int hours = (int)(eta_seconds / 3600);
            int mins = (int)((eta_seconds - hours*3600) / 60);
            int secs = (int)(eta_seconds - hours*3600 - mins*60);

            printf("[progreso] probadas=%" PRIu64 " / %" PRIu64 "  time=%.2fs  rate=%.0f k/s  ETA=%02d:%02d:%02d\n",
                keys_tried, total_keys, elapsed, keys_per_sec/1000.0, hours, mins, secs);
        }
        
        if (k == max_key) break;
    }
    free(cipher);
    return found;
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <cipher_file> <search_text> <min_key> <max_key>\n", argv[0]);
        return 1;
    }

    const char *cipher_file = argv[1];
    const char *search_text = argv[2];

    char *endptr = NULL;
    errno = 0;
    uint64_t min_key = strtoull(argv[3], &endptr, 0);
    if (errno || *endptr != '\0') {
        fprintf(stderr, "min_key inválida: %s\n", argv[3]);
        return 1;
    }
    errno = 0;
    uint64_t max_key = strtoull(argv[4], &endptr, 0);
    if (errno || *endptr != '\0') {
        fprintf(stderr, "max_key inválida: %s\n", argv[4]);
        return 1;
    }

    if (min_key > max_key) {
        fprintf(stderr, "min_key no puede ser mayor que max_key\n");
        return 1;
    }

    long res = brute_force_sequential(cipher_file, search_text, min_key, max_key);
    if (res < 0) {
        fprintf(stderr, "Error durante la búsqueda\n");
        return 1;
    } else if (res == 0) {
        printf("No se encontró la llave en el rango dado\n");
    }
    return 0;
}
