#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRAVITY 0.2f
#define BOUNCE_DAMPING 0.8f


typedef struct { float x, y; } Vec2;

#define MAX_PATH_POINTS 100

Vec2 path[MAX_PATH_POINTS];
int pathLength = 0;


void initPath() {
    pathLength = 4;
    path[0] = (Vec2){100, 100};
    path[1] = (Vec2){700, 100};
    path[2] = (Vec2){700, 500};
    path[3] = (Vec2){100, 500};
}



// Obtener vector del flow field en la posición (x, y)
// Obtener vector del flow field en la posición (x, y)
void getFlowField(float x, float y, float* fx, float* fy) {
    float cx = WINDOW_WIDTH / 2.0f;
    float cy = WINDOW_HEIGHT / 2.0f;
    
    float dx = x - cx;
    float dy = y - cy;
    float dist = sqrtf(dx*dx + dy*dy) + 0.001f; // evitar división por 0

    float spiralStrength = 1.0f;   // fuerza de rotación más suave
    float inwardStrength = 0.2f;   // atracción hacia el centro

    // Rotación perpendicular (efecto circular)
    float fx_rot = -dy / dist * spiralStrength;
    float fy_rot = dx / dist * spiralStrength;

    // Atracción hacia el centro (efecto caracol)
    float fx_in = -dx / dist * inwardStrength;
    float fy_in = -dy / dist * inwardStrength;

    // Combinar ambos efectos
    *fx = fx_rot + fx_in;
    *fy = fy_rot + fy_in;
}




typedef enum {
    MODE_GRAVITY,
    MODE_FLOWFIELD
} ParticleMode;

ParticleMode currentMode = MODE_GRAVITY;

typedef struct {
    float x, y;        
    float vx, vy;      
    float ax, ay;      
    float inv_mass;    
    float radius;      
    float life;
    int pathIndex;        
} Particle;

typedef struct {
    Particle* p;
    size_t count, capacity;
} ParticleSystem;

// Inicializar el sistema de partículas
ParticleSystem* createParticleSystem(size_t capacity) {
    ParticleSystem* ps = malloc(sizeof(ParticleSystem));
    if (!ps) return NULL;
    
    ps->p = malloc(sizeof(Particle) * capacity);
    if (!ps->p) {
        free(ps);
        return NULL;
    }
    
    ps->count = 0;
    ps->capacity = capacity;
    return ps;
}

// Liberar memoria del sistema de partículas
void destroyParticleSystem(ParticleSystem* ps) {
    if (ps) {
        free(ps->p);
        free(ps);
    }
}

// Crear una partícula aleatoria
void createRandomParticle(Particle* particle) {
    particle->x = (float)(rand() % WINDOW_WIDTH);
    particle->y = (float)(rand() % (WINDOW_HEIGHT / 2)); // Aparecen en la mitad superior
    particle->vx = ((float)rand() / RAND_MAX - 0.5f) * 10.0f; // Velocidad horizontal aleatoria
    particle->vy = ((float)rand() / RAND_MAX) * 5.0f - 2.0f;  // Velocidad vertical inicial
    particle->ax = 0.0f;
    particle->ay = GRAVITY;
    particle->inv_mass = 1.0f + ((float)rand() / RAND_MAX) * 2.0f; // Masa inversa aleatoria
    particle->radius = 2.0f + ((float)rand() / RAND_MAX) * 6.0f;   // Radio aleatorio
    particle->life = 1.0f; // Vida inicial máxima
}

// Actualizar una partícula
void updateParticle(Particle* particle, float deltaTime) {
    // Actualizar velocidad con aceleración


    if (currentMode == MODE_GRAVITY) {
        particle->vx += particle->ax * deltaTime * particle->inv_mass;
        particle->vy += particle->ay * deltaTime * particle->inv_mass;
    }
    else if (currentMode == MODE_FLOWFIELD) {
        // Obtener vector hacia el siguiente punto
        Vec2 target = path[particle->pathIndex];
        float dx = target.x - particle->x;
        float dy = target.y - particle->y;

        // Velocidad lenta hacia el objetivo
        float speed = 50.0f; // píxeles por segundo
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > 1.0f) {
            particle->vx = (dx / dist) * speed;
            particle->vy = (dy / dist) * speed;
        } else {
            // Pasó al siguiente punto del path
            particle->pathIndex = (particle->pathIndex + 1) % pathLength;
        }

    }
    
    
    // Actualizar posición con velocidad
    particle->x += particle->vx * deltaTime;
    particle->y += particle->vy * deltaTime;
    

    if (currentMode == MODE_GRAVITY) {
    
        if (particle->x - particle->radius < 0) {
            particle->x = particle->radius;
            particle->vx *= -BOUNCE_DAMPING;
        } else if (particle->x + particle->radius > WINDOW_WIDTH) {
            particle->x = WINDOW_WIDTH - particle->radius;
            particle->vx *= -BOUNCE_DAMPING;
        }

        if (particle->y - particle->radius < 0) {
            particle->y = particle->radius;
            particle->vy *= -BOUNCE_DAMPING;
        } else if (particle->y + particle->radius > WINDOW_HEIGHT) {
            particle->y = WINDOW_HEIGHT - particle->radius;
            particle->vy *= -BOUNCE_DAMPING;
        }

    }

    
    
    // Reducir vida gradualmente
    particle->life -= 0.002f;
    if (particle->life < 0) particle->life = 0;
}

// Dibujar una partícula como un círculo
void drawParticle(SDL_Renderer* renderer, Particle* particle) {
    if (particle->life <= 0) return;
    
    // Color basado en la vida de la partícula
    int alpha = (int)(particle->life * 255);
    int red = 255;
    int green = (int)(particle->life * 100);
    int blue = (int)(particle->life * 50);
    
    SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
    
    // Dibujar círculo (aproximado con rectángulos)
    int centerX = (int)particle->x;
    int centerY = (int)particle->y;
    int radius = (int)particle->radius;
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
}

// Actualizar todo el sistema de partículas
void updateParticleSystem(ParticleSystem* ps, float deltaTime) {
    for (size_t i = 0; i < ps->count; i++) {
        updateParticle(&ps->p[i], deltaTime);
        
        // Recrear partícula si ha "muerto"
        if (ps->p[i].life <= 0) {
            createRandomParticle(&ps->p[i]);
        }
    }
}

// Dibujar todo el sistema de partículas
void drawParticleSystem(SDL_Renderer* renderer, ParticleSystem* ps) {
    for (size_t i = 0; i < ps->count; i++) {
        drawParticle(renderer, &ps->p[i]);
    }
}

int main(int argc, char* argv[]) {
    // Verificar argumentos
    initPath();
    if (argc != 2) {
        printf("Uso: %s <numero_de_particulas>\n", argv[0]);
        printf("Ejemplo: %s 100\n", argv[0]);
        return 1;
    }
    
    int numParticles = atoi(argv[1]);
    if (numParticles <= 0 || numParticles > 10000) {
        printf("Error: El número de partículas debe estar entre 1 y 10000\n");
        return 1;
    }
    
    printf("Creando simulación con %d partículas...\n", numParticles);
    
    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error al iniciar SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow(
        "Sistema de Partículas SDL", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        WINDOW_WIDTH, WINDOW_HEIGHT, 
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
    
    // Inicializar generador de números aleatorios
    srand((unsigned int)time(NULL));
    
    // Crear sistema de partículas
    ParticleSystem* ps = createParticleSystem(numParticles);
    if (!ps) {
        printf("Error al crear sistema de partículas\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Crear todas las partículas iniciales
    ps->count = numParticles;
    for (int i = 0; i < numParticles; i++) {
        createRandomParticle(&ps->p[i]);
    }
    
    // Variables para el bucle principal
    int running = 1;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    
    printf("Simulación iniciada. Presiona ESC o cierra la ventana para salir.\n");
    
    while (running) {
        // Calcular delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // Manejo de eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                } else if (event.key.keysym.sym == SDLK_1) {
                    currentMode = MODE_GRAVITY;    // Presiona 1 → gravedad
                } else if (event.key.keysym.sym == SDLK_2) {
                    currentMode = MODE_FLOWFIELD;  // Presiona 2 → flow field
                }
            }
        }
        
        // Actualizar sistema de partículas
        updateParticleSystem(ps, deltaTime * 60.0f); // Escalar para mejor visual
        
        // Limpiar pantalla
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Fondo negro
        SDL_RenderClear(renderer);
        
        // Dibujar partículas
        drawParticleSystem(renderer, ps);
        
        // Mostrar en pantalla
        SDL_RenderPresent(renderer);
        
        // Controlar FPS
        SDL_Delay(16); // ~60 FPS
    }
    
    // Liberar recursos
    destroyParticleSystem(ps);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("Simulación terminada.\n");
    return 0;
}