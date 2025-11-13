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

__global__ void helloFromGalaxy(curandState *state) {

    __shared__ float avg [64];

    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    float uniform_float = curand_uniform(&state[idx]);

    int random_int = (int)(uniform_float * (RANGE + 1));
    if (random_int > RANGE) random_int = RANGE;  
    
    printf("Galaxia: %d, Estrella - %d -> Brillo promedio: %d\n", blockIdx.x, threadIdx.x, random_int);

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
    

    helloFromGalaxy<<<numBlocks, threadsPerBlock>>>(d_state);
    cudaDeviceSynchronize();
    cudaFree(d_state);
    
    return 0;
}