
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct {
    float x, y;        
    float vx, vy;      
    float ax, ay;      
    float inv_mass;    
    float radius;      
    float life;        
} Particle;

typedef struct {
    Particle* p;
    size_t count, capacity;
} ParticleSystem;




int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error al iniciar SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Hello SDL", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        800, 600, 
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("Error al crear ventana: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Error al crear renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    SDL_Event event;
    int x = 0; 

    while (running) {
        // Manejo de eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Actualizar lógica (mover rectángulo)
        x += 2;
        if (x > 800) x = -50; // vuelve a empezar

        // Dibujar fondo
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // negro
        SDL_RenderClear(renderer);

        // Dibujar rectángulo rojo
        SDL_Rect rect = { x, 250, 50, 50 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);

        // Mostrar en pantalla
        SDL_RenderPresent(renderer);

        // Controlar FPS (delay ~16 ms ≈ 60 FPS)
        SDL_Delay(16);
    }

    // 5. Liberar recursos
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}