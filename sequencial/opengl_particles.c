#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_PARTICLES 10000
#define GRAVITY 9.8f
#define BOUNCE_DAMPING 0.8f
#define M_PI 3.14159265359

// Estructuras matemáticas
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float m[16];
} Mat4;

// Estructura de partícula 3D
typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    float mass;
    float radius;
    float life;
    float maxLife;
    Vec3 color;
} Particle;

// Sistema de partículas
typedef struct {
    Particle* particles;
    size_t count;
    size_t capacity;
    Vec3 bounds;
} ParticleSystem;

// Funciones matemáticas
Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){v.x * s, v.y * s, v.z * s};
}

Vec3 vec3_normalize(Vec3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len > 0) return vec3_scale(v, 1.0f/len);
    return v;
}

Mat4 mat4_identity() {
    Mat4 m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
    Mat4 m = {0};
    float tanHalfFov = tanf(fov / 2.0f);
    
    m.m[0] = 1.0f / (aspect * tanHalfFov);
    m.m[5] = 1.0f / tanHalfFov;
    m.m[10] = -(far + near) / (far - near);
    m.m[11] = -1.0f;
    m.m[14] = -(2.0f * far * near) / (far - near);
    
    return m;
}

void mat4_load_identity(float m[16]) {
    memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void setupCamera(float rotationAngle) {
    // Configurar matriz de proyección
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float aspect = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
    float fov = 45.0f;
    float near = 0.1f;
    float far = 100.0f;
    
    // Crear matriz de perspectiva manualmente (gluPerspective alternativa)
    float top = near * tanf(fov * M_PI / 360.0f);
    float bottom = -top;
    float right = top * aspect;
    float left = -right;
    
    glFrustum(left, right, bottom, top, near, far);
    
    // Configurar matriz de vista
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Posición de la cámara orbitando
    float camX = 15.0f * cosf(rotationAngle);
    float camY = 10.0f;
    float camZ = 15.0f * sinf(rotationAngle);
    
    // LookAt manual (gluLookAt alternativa)
    Vec3 eye = {camX, camY, camZ};
    Vec3 center = {0.0f, 0.0f, 0.0f};
    Vec3 up = {0.0f, 1.0f, 0.0f};
    
    Vec3 f = vec3_normalize((Vec3){center.x - eye.x, center.y - eye.y, center.z - eye.z});
    Vec3 s = vec3_normalize((Vec3){f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x});
    Vec3 u = (Vec3){s.y*f.z - s.z*f.y, s.z*f.x - s.x*f.z, s.x*f.y - s.y*f.x};
    
    float m[16] = {
        s.x, u.x, -f.x, 0,
        s.y, u.y, -f.y, 0,
        s.z, u.z, -f.z, 0,
        -(s.x*eye.x + s.y*eye.y + s.z*eye.z),
        -(u.x*eye.x + u.y*eye.y + u.z*eye.z),
        f.x*eye.x + f.y*eye.y + f.z*eye.z,
        1
    };
    
    glMultMatrixf(m);
}

// Funciones del sistema de partículas
ParticleSystem* createParticleSystem(size_t capacity, Vec3 bounds) {
    ParticleSystem* ps = malloc(sizeof(ParticleSystem));
    if (!ps) return NULL;
    
    ps->particles = malloc(sizeof(Particle) * capacity);
    if (!ps->particles) {
        free(ps);
        return NULL;
    }
    
    ps->count = 0;
    ps->capacity = capacity;
    ps->bounds = bounds;
    return ps;
}

void destroyParticleSystem(ParticleSystem* ps) {
    if (ps) {
        free(ps->particles);
        free(ps);
    }
}

void createRandomParticle(Particle* p, Vec3 bounds) {
    // Posición inicial aleatoria
    p->position.x = ((float)rand() / RAND_MAX - 0.5f) * bounds.x * 0.5f;
    p->position.y = bounds.y * 0.3f + ((float)rand() / RAND_MAX) * bounds.y * 0.2f;
    p->position.z = ((float)rand() / RAND_MAX - 0.5f) * bounds.z * 0.5f;
    
    // Velocidad inicial aleatoria
    p->velocity.x = ((float)rand() / RAND_MAX - 0.5f) * 8.0f;
    p->velocity.y = ((float)rand() / RAND_MAX) * 3.0f + 1.0f;
    p->velocity.z = ((float)rand() / RAND_MAX - 0.5f) * 8.0f;
    
    // Aceleración (gravedad)
    p->acceleration.x = 0.0f;
    p->acceleration.y = -GRAVITY;
    p->acceleration.z = 0.0f;
    
    // Propiedades físicas
    p->mass = 0.5f + ((float)rand() / RAND_MAX) * 1.5f;
    p->radius = 0.1f + ((float)rand() / RAND_MAX) * 0.2f;
    p->life = p->maxLife = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
    
    // Color basado en la vida
    float lifeRatio = p->life / p->maxLife;
    p->color.x = 1.0f;                              // Rojo
    p->color.y = lifeRatio * 0.8f;                  // Verde
    p->color.z = lifeRatio * 0.4f;                  // Azul
}

void updateParticle(Particle* p, float deltaTime, Vec3 bounds) {
    // Integración de Euler
    p->velocity = vec3_add(p->velocity, vec3_scale(p->acceleration, deltaTime));
    p->position = vec3_add(p->position, vec3_scale(p->velocity, deltaTime));
    
    // Colisiones con límites (rebote en las 6 caras del cubo)
    if (p->position.x - p->radius < -bounds.x/2) {
        p->position.x = -bounds.x/2 + p->radius;
        p->velocity.x *= -BOUNCE_DAMPING;
    } else if (p->position.x + p->radius > bounds.x/2) {
        p->position.x = bounds.x/2 - p->radius;
        p->velocity.x *= -BOUNCE_DAMPING;
    }
    
    if (p->position.y - p->radius < -bounds.y/2) {
        p->position.y = -bounds.y/2 + p->radius;
        p->velocity.y *= -BOUNCE_DAMPING;
    } else if (p->position.y + p->radius > bounds.y/2) {
        p->position.y = bounds.y/2 - p->radius;
        p->velocity.y *= -BOUNCE_DAMPING;
    }
    
    if (p->position.z - p->radius < -bounds.z/2) {
        p->position.z = -bounds.z/2 + p->radius;
        p->velocity.z *= -BOUNCE_DAMPING;
    } else if (p->position.z + p->radius > bounds.z/2) {
        p->position.z = bounds.z/2 - p->radius;
        p->velocity.z *= -BOUNCE_DAMPING;
    }
    
    // Actualizar vida y color
    p->life -= deltaTime;
    if (p->life > 0) {
        float lifeRatio = p->life / p->maxLife;
        p->color.x = 1.0f;
        p->color.y = lifeRatio * 0.8f;
        p->color.z = lifeRatio * 0.4f;
    }
}

void updateParticleSystem(ParticleSystem* ps, float deltaTime) {
    for (size_t i = 0; i < ps->count; i++) {
        updateParticle(&ps->particles[i], deltaTime, ps->bounds);
        
        // Regenerar partícula si "murió"
        if (ps->particles[i].life <= 0) {
            createRandomParticle(&ps->particles[i], ps->bounds);
        }
    }
}

void drawBounds(Vec3 bounds) {
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINES);
    
    float hx = bounds.x / 2, hy = bounds.y / 2, hz = bounds.z / 2;
    
    // Líneas del cubo
    // Cara frontal
    glVertex3f(-hx, -hy, hz); glVertex3f(hx, -hy, hz);
    glVertex3f(hx, -hy, hz); glVertex3f(hx, hy, hz);
    glVertex3f(hx, hy, hz); glVertex3f(-hx, hy, hz);
    glVertex3f(-hx, hy, hz); glVertex3f(-hx, -hy, hz);
    
    // Cara trasera
    glVertex3f(-hx, -hy, -hz); glVertex3f(hx, -hy, -hz);
    glVertex3f(hx, -hy, -hz); glVertex3f(hx, hy, -hz);
    glVertex3f(hx, hy, -hz); glVertex3f(-hx, hy, -hz);
    glVertex3f(-hx, hy, -hz); glVertex3f(-hx, -hy, -hz);
    
    // Conexiones
    glVertex3f(-hx, -hy, hz); glVertex3f(-hx, -hy, -hz);
    glVertex3f(hx, -hy, hz); glVertex3f(hx, -hy, -hz);
    glVertex3f(hx, hy, hz); glVertex3f(hx, hy, -hz);
    glVertex3f(-hx, hy, hz); glVertex3f(-hx, hy, -hz);
    
    glEnd();
}

void drawParticleSystem(ParticleSystem* ps) {
    glEnable(GL_POINT_SMOOTH);
    glPointSize(4.0f);
    
    glBegin(GL_POINTS);
    for (size_t i = 0; i < ps->count; i++) {
        Particle* p = &ps->particles[i];
        if (p->life > 0) {
            float alpha = p->life / p->maxLife;
            glColor4f(p->color.x, p->color.y, p->color.z, alpha);
            glVertex3f(p->position.x, p->position.y, p->position.z);
        }
    }
    glEnd();
    
    glDisable(GL_POINT_SMOOTH);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <numero_de_particulas>\n", argv[0]);
        printf("Ejemplo: %s 1000\n", argv[0]);
        return 1;
    }
    
    int numParticles = atoi(argv[1]);
    // if (numParticles <= 0 || numParticles > MAX_PARTICLES) {
    //     printf("Error: Número de partículas debe estar entre 1 y %d\n", MAX_PARTICLES);
    //     return 1;
    // }
    
    printf("Creando sistema de partículas 3D con %d partículas...\n", numParticles);
    
    // Inicializar GLFW
    if (!glfwInit()) {
        printf("Error al inicializar GLFW\n");
        return 1;
    }
    
    // Configuración de ventana OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "Sistema de Partículas 3D OpenGL", NULL, NULL);
    if (!window) {
        printf("Error al crear ventana\n");
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1); // VSync
    
    // Inicializar GLEW
    if (glewInit() != GLEW_OK) {
        printf("Error al inicializar GLEW\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    
    // Configurar OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Configurar viewport
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Inicializar generador de números aleatorios
    srand((unsigned int)time(NULL));
    
    // Crear sistema de partículas
    Vec3 bounds = {20.0f, 20.0f, 20.0f};
    ParticleSystem* ps = createParticleSystem(numParticles, bounds);
    if (!ps) {
        printf("Error al crear sistema de partículas\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    
    // Crear partículas iniciales
    ps->count = numParticles;
    for (int i = 0; i < numParticles; i++) {
        createRandomParticle(&ps->particles[i], bounds);
    }
    
    printf("Sistema iniciado. Usa ESC para salir.\n");
    printf("Controles: La cámara rota automáticamente alrededor del sistema\n");
    
    // Variables de tiempo
    double lastTime = glfwGetTime();
    double rotationAngle = 0.0;
    int frameCount = 0;
    
    // Bucle principal
    while (!glfwWindowShouldClose(window)) {
        // Calcular deltaTime
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;
        
        // Actualizar sistema de partículas
        updateParticleSystem(ps, deltaTime);
        
        // Rotar cámara
        rotationAngle += deltaTime * 0.3; // Rotación lenta
        
        // Limpiar buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Configurar cámara
        setupCamera(rotationAngle);
        
        // Dibujar límites del espacio
        drawBounds(bounds);
        
        // Dibujar partículas
        drawParticleSystem(ps);
        
        // Intercambiar buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Información de debug
        frameCount++;
        if (frameCount % 60 == 0) {
            printf("Frame %d - FPS: %.1f - Partículas: %zu\n", 
                   frameCount, 1.0f / deltaTime, ps->count);
        }
    }
    
    // Limpieza
    destroyParticleSystem(ps);
    glfwDestroyWindow(window);
    glfwTerminate();
    
    printf("Simulación terminada.\n");
    return 0;
}