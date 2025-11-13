#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>



__global__ void helloFromGalaxy(int brillo) {
    printf("Galaxia - %d, Estrella - %d << brillo: %d\n",  blockIdx.x,threadIdx.x, brillo);

};


int main() {

  srand(time(NULL)); 
  int numero = rand() % 10;
  helloFromGalaxy<<<2, 4>>>(numero);
  cudaDeviceSynchronize();
  return 0;

}