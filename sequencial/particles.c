#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRAVITY_ACCEL 200.0f    
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

// flow-field vector (field centrado + rotacional)
void getFlowField(float x, float y, float* fx, float* fy) {
    float cx = WINDOW_WIDTH / 2.0f;
    float cy = WINDOW_HEIGHT / 2.0f;
    
    float dx = x - cx;
    float dy = y - cy;
    float dist = sqrtf(dx*dx + dy*dy) + 0.001f;

    float spiralStrength = 120.0f;   // fuerza rotacional (px/s^2 aproximado)
    float inwardStrength = 30.0f;    // atracción hacia el centro

    float fx_rot = -dy / dist * spiralStrength;
    float fy_rot = dx / dist * spiralStrength;

    float fx_in = -dx / dist * inwardStrength;
    float fy_in = -dy / dist * inwardStrength;

    *fx = fx_rot + fx_in;
    *fy = fy_rot + fy_in;
}

// Flags para fuerzas 
#define FORCE_GRAVITY    (1u<<0)
#define FORCE_FLOWFIELD  (1u<<1)
#define FORCE_WIND       (1u<<2)
#define FORCE_FIRE       (1u<<3)

uint32_t activeForces = FORCE_GRAVITY; 

int mouseX = 0, mouseY = 0;
int mouseWindActive = 0; 

typedef struct {
    float x, y;
    float vx, vy;
    float inv_mass;   
    float radius;
    float life;
    int pathIndex;
} Particle;

typedef struct {
    Particle* p;
    size_t count, capacity;
} ParticleSystem;

ParticleSystem* createParticleSystem(size_t capacity) {
    ParticleSystem* ps = malloc(sizeof(ParticleSystem));
    if (!ps) return NULL;
    ps->p = malloc(sizeof(Particle) * capacity);
    if (!ps->p) { free(ps); return NULL; }
    ps->count = 0;
    ps->capacity = capacity;
    return ps;
}

void destroyParticleSystem(ParticleSystem* ps) {
    if (ps) { free(ps->p); free(ps); }
}

void createRandomParticle(Particle* particle) {
    particle->x = (float)(rand() % WINDOW_WIDTH);
    particle->y = (float)(rand() % (WINDOW_HEIGHT / 2));
    particle->vx = ((float)rand() / RAND_MAX - 0.5f) * 60.0f;
    particle->vy = ((float)rand() / RAND_MAX) * 30.0f - 10.0f;
    particle->inv_mass = 0.5f + ((float)rand() / RAND_MAX) * 1.5f; // 0.5..2.0
    particle->radius = 2.0f + ((float)rand() / RAND_MAX) * 6.0f;
    particle->life = 1.0f;
    particle->pathIndex = rand() % (pathLength ? pathLength : 1);
}

// explosion
void triggerExplosion(ParticleSystem* ps, float ex, float ey, float strength) {
    for (size_t i = 0; i < ps->count; ++i) {
        float dx = ps->p[i].x - ex;
        float dy = ps->p[i].y - ey;
        float dist = sqrtf(dx*dx + dy*dy) + 0.0001f;
        if (dist < 300.0f) { 
            float impulse = strength * (1.0f - (dist / 300.0f)); // decae con distancia
            ps->p[i].vx += (dx / dist) * impulse * ps->p[i].inv_mass;
            ps->p[i].vy += (dy / dist) * impulse * ps->p[i].inv_mass;
            ps->p[i].life = 1.0f;
        }
    }
}

// actualizar una partícula acumulando fuerzas
void updateParticle(Particle* particle, float deltaTime) {
    float fx = 0.0f;
    float fy = 0.0f;

    // GRAVEDAD
    if (activeForces & FORCE_GRAVITY) {
        fy += GRAVITY_ACCEL; // ya es aceleración en px/s^2
    }

    // FLOW FIELD 
    if (activeForces & FORCE_FLOWFIELD) {
        float fxf, fyf;
        getFlowField(particle->x, particle->y, &fxf, &fyf);
        fx += fxf;
        fy += fyf;
    }

    // WIND 
    if ((activeForces & FORCE_WIND) && mouseWindActive) {
        float dx = particle->x - (float)mouseX;
        float dy = particle->y - (float)mouseY;
        float dist = sqrtf(dx*dx + dy*dy) + 0.0001f;
        float maxInfluence = 250.0f;
        if (dist < maxInfluence) {
            float windStrength = 20000.0f; // ajusta intensidad
            float falloff = 1.0f - (dist / maxInfluence); // 1..0
            float w = windStrength * falloff;
            fx += (dx / dist) * w;
            fy += (dy / dist) * w;
        }
    }

    // FIRE
    if (activeForces & FORCE_FIRE) {
        // más fuerte en la parte baja de la pantalla
        float fireStrength = 500.0f * (1.0f - (particle->y / WINDOW_HEIGHT));
        float jitter = ((float)rand() / RAND_MAX - 0.5f) * 120.0f;
        fy -= fireStrength; // hacia arriba => restar en y
        fx += jitter * 0.3f;
        // vida puede crecer si queremos que "salgan" partículas
        particle->life += 0.01f;
        if (particle->life > 1.0f) particle->life = 1.0f;
    }

    particle->vx += fx * particle->inv_mass * deltaTime;
    particle->vy += fy * particle->inv_mass * deltaTime;

    // Posición
    particle->x += particle->vx * deltaTime;
    particle->y += particle->vy * deltaTime;

    // Si estamos usando gravedad -> rebotes en bordes
    if (activeForces & FORCE_GRAVITY) {
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

    // vida decrece )
    if (!(activeForces & FORCE_FIRE)) {
        particle->life -= 0.0025f;
        if (particle->life < 0) particle->life = 0;
    }
}

void drawParticle(SDL_Renderer* renderer, Particle* particle) {
    if (particle->life <= 0) return;
    int alpha = (int)(particle->life * 255);
    int red = 255;
    int green = (int)(particle->life * 160);
    int blue = (int)(particle->life * 80);

    // si FIRE activo, color más amarillento
    if (activeForces & FORCE_FIRE) {
        red = 255;
        green = 140 + (int)(particle->life * 80);
        blue = 30;
    }

    SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);

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

void updateParticleSystem(ParticleSystem* ps, float deltaTime) {
    for (size_t i = 0; i < ps->count; i++) {
        updateParticle(&ps->p[i], deltaTime);
        if (ps->p[i].life <= 0) {
            createRandomParticle(&ps->p[i]);
            // si FIRE está activo, spawn cerca del fondo
            if (activeForces & FORCE_FIRE) {
                ps->p[i].x = (float)(WINDOW_WIDTH/2 + (rand()%200 - 100));
                ps->p[i].y = (float)(WINDOW_HEIGHT - (rand()%50));
                ps->p[i].vx = ((float)rand()/RAND_MAX - 0.5f)*40.0f;
                ps->p[i].vy = -((float)rand()/RAND_MAX)*60.0f;
                ps->p[i].life = 0.8f;
            }
        }
    }
}

void drawParticleSystem(SDL_Renderer* renderer, ParticleSystem* ps) {
    for (size_t i = 0; i < ps->count; i++)
        drawParticle(renderer, &ps->p[i]);
}

int main(int argc, char* argv[]) {
    initPath();

    if (argc != 2) {
        printf("Uso: %s <numero_de_particulas>\n", argv[0]);
        return 1;
    }
    int numParticles = atoi(argv[1]);
    if (numParticles <= 0 || numParticles > 10000) {
        printf("Error: numero de partículas 1..10000\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Sistema de Particulas", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) { printf("Window error: %s\n", SDL_GetError()); SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) { printf("Renderer error: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    srand((unsigned int)time(NULL));

    ParticleSystem* ps = createParticleSystem(numParticles);
    if (!ps) { printf("No ps\n"); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    ps->count = numParticles;
    for (int i = 0; i < numParticles; ++i) createRandomParticle(&ps->p[i]);

    int running = 1;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();

    printf("Simulación iniciada. Teclas: 1,2,3, w,f,q(hold), e (explosion), r\n");

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f; // segundos reales
        lastTime = currentTime;

        // proceso eventos
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = 0;
            else if (event.type == SDL_MOUSEMOTION) {
                mouseX = event.motion.x;
                mouseY = event.motion.y;
            }
            else if (event.type == SDL_KEYDOWN) {
                SDL_Keycode k = event.key.keysym.sym;
                if (k == SDLK_ESCAPE) running = 0;
                else if (k == SDLK_1) { activeForces = FORCE_GRAVITY; printf("Modo exclusivo: GRAVEDAD\n"); }
                else if (k == SDLK_2) { activeForces = FORCE_FLOWFIELD; printf("Modo exclusivo: FLOWFIELD\n"); }
                else if (k == SDLK_3) { activeForces = FORCE_FIRE; printf("Modo exclusivo: FIRE\n"); }
                else if (k == SDLK_w) { activeForces ^= FORCE_GRAVITY; printf("Toggle GRAVITY -> %s\n", (activeForces & FORCE_GRAVITY) ? "ON" : "OFF"); }
                else if (k == SDLK_f) { activeForces ^= FORCE_FLOWFIELD; printf("Toggle FLOWFIELD -> %s\n", (activeForces & FORCE_FLOWFIELD) ? "ON" : "OFF"); }
                else if (k == SDLK_r) { activeForces ^= FORCE_FIRE; printf("Toggle FIRE -> %s\n", (activeForces & FORCE_FIRE) ? "ON" : "OFF"); }
                else if (k == SDLK_q) { // wind hold
                    mouseWindActive = 1;
                    activeForces |= FORCE_WIND;
                    printf("WIND ON (hold)\n");
                }
                else if (k == SDLK_e) {
                    // explosion instantánea en mouse
                    triggerExplosion(ps, (float)mouseX, (float)mouseY, 1200.0f);
                    printf("Explosion en (%d,%d)\n", mouseX, mouseY);
                }
            }
            else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_q) {
                    mouseWindActive = 0;
                    activeForces &= ~FORCE_WIND;
                    printf("WIND OFF\n");
                }
            }
        }

        float simDt = dt; 
        updateParticleSystem(ps, simDt);

        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        drawParticleSystem(renderer, ps);
        SDL_RenderPresent(renderer);

        SDL_Delay(16); // ~60fps
    }

    destroyParticleSystem(ps);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("Fin.\n");
    return 0;
}

