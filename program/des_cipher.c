
// Programa para cifrar y descifrar archivos usando DES

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc/des_crypt.h>
#include <sys/time.h>

void encrypt(long key, char *ciph, int len){
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_ENCRYPT);
}

void decrypt(long key, char *ciph, int len){
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_DECRYPT);
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

int main(int argc, char *argv[]){
  if(argc < 2){
    printf("Uso:\n");
    printf("  Cifrar:     ./des_cipher encrypt <archivo_entrada> <clave>\n");
    printf("  Descifrar:  ./des_cipher decrypt <archivo_cifrado> <clave>\n");
    printf("\nEjemplos:\n");
    printf("  ./des_cipher encrypt texto.txt 123456\n");
    printf("  ./des_cipher decrypt texto.txt.enc 123456\n");
    return 1;
  }
  
  if(strcmp(argv[1], "encrypt") == 0 && argc >= 4){
    long key = atol(argv[3]);
    char output[256];
    snprintf(output, 256, "%s.enc", argv[2]);
    encrypt_file(argv[2], output, key);
  }
  else if(strcmp(argv[1], "decrypt") == 0 && argc >= 4){
    long key = atol(argv[3]);
    char output[256];
    snprintf(output, 256, "%s.dec", argv[2]);
    decrypt_file(argv[2], output, key);
  }
  else {
    printf("Comando no reconocido o faltan parámetros\n");
  }
  
  return 0;
}