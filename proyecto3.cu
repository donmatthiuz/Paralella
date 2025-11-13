#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cuda_runtime.h>
#include <curand_kernel.h>



#define RANGE 10

__global__ void setup_kernel(curandState *state, unsigned long long seed) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    curand_init(seed, idx, 0, &state[idx]);
}

__global__ void helloFromGalaxy(curandState *state, int hilosBloque) {
     
     __shared__ int suma;
     if (threadIdx.x == 0) {
        suma = 0;
    }

    __syncthreads();
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    __shared__ int brillosCompartidos[256];
   

    float uniform_float = curand_uniform(&state[idx]);

    int random_int = (int)(uniform_float * (RANGE + 1));
    if (random_int > RANGE) random_int = RANGE;  

     brillosCompartidos[threadIdx.x] = random_int;
     atomicAdd(&suma, random_int);
     __syncthreads();
  
    if (threadIdx.x == 0) {
        float promedio = (float)suma / hilosBloque;
        printf(">>> Galaxia %d completa Brillo Promedio: %.2f:\n", blockIdx.x, promedio);
    }
	 __syncthreads();

  
    
}


int main() {

    srand(time(NULL)); 
    

    int numBlocks = 2;
    int threadsPerBlock = 4;
    int totalThreads = numBlocks * threadsPerBlock;

    curandState *d_state;
    cudaMalloc(&d_state, totalThreads * sizeof(curandState));
    

    unsigned long long seed = time(NULL);
    setup_kernel<<<numBlocks, threadsPerBlock>>>(d_state, seed);
    cudaDeviceSynchronize();
    

    helloFromGalaxy<<<numBlocks, threadsPerBlock>>>(d_state, threadsPerBlock );
    cudaDeviceSynchronize();
    cudaFree(d_state);
    
    return 0;
}