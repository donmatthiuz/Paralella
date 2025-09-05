#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define MAX_PARTICLES 50000
#define M_PI 3.14159265359

// Estructuras matemáticas
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float m[16];
} Mat4;

// Estructura de partícula para anillos
typedef struct {
    Vec3 position;
    Vec3 velocity;
    float orbitRadius;    // Radio de órbita desde el centro
    float orbitAngle;     // Ángulo actual en la órbita
    float orbitSpeed;     // Velocidad angular de órbita
    float inclination;    // Inclinación del plano orbital
    float verticalOffset; // Desplazamiento vertical del anillo
    float size;           // Tamaño de la partícula
    float life;
    float maxLife;
    Vec3 color;
    int ringIndex;        // Índice del anillo al que pertenece
} RingParticle;

// Sistema de anillos
// Sistema de anillos
typedef struct {
    RingParticle* particles;
    size_t count;
    size_t capacity;
    int numRings;         // Número de anillos
    float minRadius;      // Radio mínimo de anillos
    float maxRadius;      // Radio máximo de anillos

    float* precomputed_cos;
    float* precomputed_sin;
    int    precomputed_size;
} RingSystem;



static float* g_precomputed_cos = NULL;
static float* g_precomputed_sin = NULL;
static int    g_precomputed_size = 0;

// Estructura para vértices de la esfera
typedef struct {
    float x, y, z;    // Posición
    float u, v;       // Coordenadas de textura
    float nx, ny, nz; // Normales
} Vertex;

// Variables globales para la esfera
GLuint VAO, VBO, EBO;
GLuint shaderProgram;
GLuint texture;
Vertex* sphereVertices;
unsigned int* sphereIndices;
int sphereVertexCount, sphereIndexCount;

// Shaders para la esfera
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "layout (location = 2) in vec3 aNormal;\n"
    "out vec2 TexCoord;\n"
    "out vec3 Normal;\n"
    "out vec3 FragPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "    Normal = mat3(transpose(inverse(model))) * aNormal;\n"
    "    TexCoord = aTexCoord;\n"
    "    gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "in vec3 Normal;\n"
    "in vec3 FragPos;\n"
    "uniform sampler2D ourTexture;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 lightColor;\n"
    "void main()\n"
    "{\n"
    "    float ambientStrength = 0.4;\n"
    "    vec3 ambient = ambientStrength * lightColor;\n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor;\n"
    "    float specularStrength = 0.3;\n"
    "    vec3 viewDir = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);\n"
    "    vec3 specular = specularStrength * spec * lightColor;\n"
    "    vec3 result = (ambient + diffuse + specular) * texture(ourTexture, TexCoord).rgb;\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\n\0";

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

void mat4_identity(float* m) {
    #pragma omp parallel for
    for (int i = 0; i < 16; i++) {
        m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

void mat4_perspective(float* m, float fovy, float aspect, float near, float far) {
    float f = 1.0f / tanf(fovy / 2.0f);
    mat4_identity(m);
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (far + near) / (near - far);
    m[11] = -1.0f;
    m[14] = (2.0f * far * near) / (near - far);
    m[15] = 0.0f;
}

void mat4_lookat(float* m, float eyeX, float eyeY, float eyeZ, 
                 float centerX, float centerY, float centerZ,
                 float upX, float upY, float upZ) {
    float f[3] = {centerX - eyeX, centerY - eyeY, centerZ - eyeZ};
    float len = sqrtf(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
    f[0] /= len; f[1] /= len; f[2] /= len;
    
    float s[3] = {f[1]*upZ - f[2]*upY, f[2]*upX - f[0]*upZ, f[0]*upY - f[1]*upX};
    len = sqrtf(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
    s[0] /= len; s[1] /= len; s[2] /= len;
    
    float u[3] = {s[1]*f[2] - s[2]*f[1], s[2]*f[0] - s[0]*f[2], s[0]*f[1] - s[1]*f[0]};
    
    mat4_identity(m);
    m[0] = s[0]; m[4] = s[1]; m[8] = s[2];
    m[1] = u[0]; m[5] = u[1]; m[9] = u[2];
    m[2] = -f[0]; m[6] = -f[1]; m[10] = -f[2];
    m[12] = -(s[0]*eyeX + s[1]*eyeY + s[2]*eyeZ);
    m[13] = -(u[0]*eyeX + u[1]*eyeY + u[2]*eyeZ);
    m[14] = f[0]*eyeX + f[1]*eyeY + f[2]*eyeZ;
}

void mat4_rotate_y(float* m, float angle) {
    mat4_identity(m);
    float c = cosf(angle);
    float s = sinf(angle);
    m[0] = c; m[8] = s;
    m[2] = -s; m[10] = c;
}

// Generar esfera
#include <omp.h>

// Generar esfera paralelizada
void generateSphere(float radius, int sectors, int stacks) {
    sphereVertexCount = (sectors + 1) * (stacks + 1);
    sphereIndexCount = sectors * stacks * 6;
    
    sphereVertices = malloc(sphereVertexCount * sizeof(Vertex));
    sphereIndices  = malloc(sphereIndexCount * sizeof(unsigned int));
    
    float sectorStep = 2 * M_PI / sectors;
    float stackStep  = M_PI / stacks;

    // === Paralelización de los vértices ===
    #pragma omp parallel for collapse(2)
    for (int i = 0; i <= stacks; ++i) {
        for (int j = 0; j <= sectors; ++j) {
            int vertIndex = i * (sectors + 1) + j; // índice único sin race condition

            float stackAngle = M_PI / 2 - i * stackStep;
            float xy = radius * cosf(stackAngle);
            float z  = radius * sinf(stackAngle);

            float sectorAngle = j * sectorStep;

            sphereVertices[vertIndex].x = xy * cosf(sectorAngle);
            sphereVertices[vertIndex].y = xy * sinf(sectorAngle);
            sphereVertices[vertIndex].z = z;
            
            sphereVertices[vertIndex].nx = sphereVertices[vertIndex].x / radius;
            sphereVertices[vertIndex].ny = sphereVertices[vertIndex].y / radius;
            sphereVertices[vertIndex].nz = sphereVertices[vertIndex].z / radius;

            sphereVertices[vertIndex].u = (float)j / sectors;
            sphereVertices[vertIndex].v = (float)i / stacks;
        }
    }

   
    int indexIndex = 0;
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                sphereIndices[indexIndex++] = k1;
                sphereIndices[indexIndex++] = k2;
                sphereIndices[indexIndex++] = k1 + 1;
            }
            
            if (i != (stacks - 1)) {
                sphereIndices[indexIndex++] = k1 + 1;
                sphereIndices[indexIndex++] = k2;
                sphereIndices[indexIndex++] = k2 + 1;
            }
        }
    }
}


// Compilar shader
unsigned int compileShader(const char* source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Error compilando shader: %s\n", infoLog);
    }
    
    return shader;
}

// Cargar textura
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    } else {
        printf("Error cargando textura: %s\n", path);
        stbi_image_free(data);
    }
    
    return textureID;
}

// Funciones del sistema de anillos
RingSystem* createRingSystem(size_t capacity, int numRings, float minRadius, float maxRadius) {
    RingSystem* rs = (RingSystem*)malloc(sizeof(RingSystem));
    if (!rs) return NULL;

    rs->particles = (RingParticle*)malloc(sizeof(RingParticle) * capacity);
    if (!rs->particles) {
        free(rs);
        return NULL;
    }

    rs->count      = 0;
    rs->capacity   = capacity;
    rs->numRings   = numRings;
    rs->minRadius  = minRadius;
    rs->maxRadius  = maxRadius;

    // ===== NUEVA OPTIMIZACIÓN: LUT de seno/coseno =====
    rs->precomputed_size = 3600; // 0.1 grados por muestra
    rs->precomputed_cos  = (float*)malloc(rs->precomputed_size * sizeof(float));
    rs->precomputed_sin  = (float*)malloc(rs->precomputed_size * sizeof(float));

    if (!rs->precomputed_cos || !rs->precomputed_sin) {
        free(rs->precomputed_cos);
        free(rs->precomputed_sin);
        free(rs->particles);
        free(rs);
        return NULL;
    }

    // Llenado del LUT
    #pragma omp parallel for
    for (int i = 0; i < rs->precomputed_size; i++) {
        float angle = (2.0f * (float)M_PI * (float)i) / (float)rs->precomputed_size;
        rs->precomputed_cos[i] = cosf(angle);
        rs->precomputed_sin[i] = sinf(angle);
    }

    // Hacer visible el LUT a updateRingParticle (sin cambiar su firma)
    g_precomputed_cos  = rs->precomputed_cos;
    g_precomputed_sin  = rs->precomputed_sin;
    g_precomputed_size = rs->precomputed_size;

    return rs;
}




void destroyRingSystem(RingSystem* rs) {
    if (rs) {
        free(rs->particles);
        free(rs);
    }
}

void createRingParticle(RingParticle* p, int ringIndex, int numRings, float minRadius, float maxRadius, 
                      int particleIndex, int particlesInRing, int totalParticlesInRing) {
    float ringSpacing = (maxRadius - minRadius) / (numRings > 1 ? numRings - 1 : 1);
    float baseRadius = minRadius + ringIndex * ringSpacing;
    
    // Crear anillos más delgados y densos
    float ringThickness = ringSpacing * 0.15f; // Anillos muy delgados
    
    // Distribución más densa: múltiples "capas" por anillo
    int layersPerRing = 5; // Más capas para mayor densidad
    int particlesPerLayer = totalParticlesInRing / layersPerRing;
    
    int layerIndex = particleIndex / particlesPerLayer;
    if (layerIndex >= layersPerRing) layerIndex = layersPerRing - 1;
    
    int positionInLayer = particleIndex % particlesPerLayer;
    
    // Radio específico dentro del anillo (distribución más compacta)
    float layerOffset = (ringThickness / layersPerRing) * layerIndex - ringThickness * 0.5f;
    p->orbitRadius = baseRadius + layerOffset;
    
    // Variación mínima para mantener orden
    p->orbitRadius += ((float)rand() / RAND_MAX - 0.5f) * (ringThickness * 0.1f);
    
    // Distribución angular completamente uniforme
    if (particlesPerLayer > 1) {
        p->orbitAngle = (2.0f * M_PI * positionInLayer) / particlesPerLayer;
        // Desplazamiento angular por capa para llenar huecos
        p->orbitAngle += (2.0f * M_PI * layerIndex) / (layersPerRing * particlesPerLayer);
    } else {
        p->orbitAngle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
    }
    
    // Variación angular mínima
    p->orbitAngle += ((float)rand() / RAND_MAX - 0.5f) * 0.02f;
    
    // Velocidad orbital uniforme por anillo
    p->orbitSpeed = 0.3f / sqrtf(p->orbitRadius + 1.0f);
    // Pequeña variación para evitar sincronización perfecta
    p->orbitSpeed *= (0.98f + ((float)rand() / RAND_MAX) * 0.04f);
    
    // Anillos completamente planos
    p->inclination = 0.0f; // Sin inclinación
    p->verticalOffset = ((float)rand() / RAND_MAX - 0.5f) * 0.005f; // Muy plano
    
    // Calcular posición inicial
    float cosAngle = cosf(p->orbitAngle);
    float sinAngle = sinf(p->orbitAngle);
    
    p->position.x = p->orbitRadius * cosAngle;
    p->position.y = p->verticalOffset;
    p->position.z = p->orbitRadius * sinAngle;
    
    // Velocidad tangencial
    p->velocity.x = -p->orbitRadius * p->orbitSpeed * sinAngle;
    p->velocity.y = 0.0f;
    p->velocity.z = p->orbitRadius * p->orbitSpeed * cosAngle;
    
    p->size = 0.02f + ((float)rand() / RAND_MAX) * 0.01f;
    p->life = p->maxLife = 100.0f; 
    p->ringIndex = ringIndex;
    
    float brightness = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
    float variation = ((float)rand() / RAND_MAX - 0.5f) * 0.2f;

    // Generar diferentes tonos de café según la distancia y posición
    float distanceFactor = (p->orbitRadius - minRadius) / (maxRadius - minRadius);

    switch (ringIndex % 6) {
        case 0: // Anillo A - café dorado claro
            p->color = (Vec3){
                (0.85f + variation * 0.5f) * brightness,
                (0.65f + variation * 0.3f) * brightness,
                (0.35f + variation * 0.2f) * brightness
            };
            break;
        case 1: // Anillo B - café medio
            p->color = (Vec3){
                (0.7f + variation) * brightness,
                (0.5f + variation) * brightness,
                (0.28f + variation) * brightness
            };
            break;
        case 2: // Anillo C (Crepe) - café muy claro, casi transparente
            p->color = (Vec3){
                (0.9f + variation * 0.3f) * brightness * 0.6f,
                (0.7f + variation * 0.2f) * brightness * 0.6f,
                (0.45f + variation * 0.1f) * brightness * 0.6f
            };
            break;
        case 3: // División Cassini - café oscuro
            p->color = (Vec3){
                (0.4f + variation) * brightness,
                (0.3f + variation) * brightness,
                (0.2f + variation) * brightness
            };
            break;
        case 4: // Anillo F - café rojizo
            p->color = (Vec3){
                (0.75f + variation) * brightness,
                (0.45f + variation) * brightness,
                (0.32f + variation) * brightness
            };
            break;
        case 5: // Anillos externos - café grisáceo
            p->color = (Vec3){
                (0.6f + variation) * brightness,
                (0.55f + variation) * brightness,
                (0.4f + variation) * brightness
            };
            break;
    }

    // Ajustar brillo según la densidad del anillo (más oscuro = más denso)
    float densityDarkening = 1.0f - (distanceFactor * 0.3f);
    p->color.x *= densityDarkening;
    p->color.y *= densityDarkening;
    p->color.z *= densityDarkening;
}


void updateRingParticle(RingParticle* p, float deltaTime) {
    const float TWO_PI = 2.0f * (float)M_PI;

    // Actualizar ángulo orbital
    float speedMultiplier = 3.0f;
    p->orbitAngle += p->orbitSpeed * deltaTime * speedMultiplier;
    if (p->orbitAngle >= TWO_PI) {
        p->orbitAngle -= TWO_PI;
    } else if (p->orbitAngle < 0.0f) {
        p->orbitAngle += TWO_PI;
    }

    float cosAngle, sinAngle;

    // === Usar LUT si está disponible; si no, caer a cosf/sinf ===
    if (g_precomputed_cos && g_precomputed_sin && g_precomputed_size > 0) {
        // Mapear ángulo a índice fraccional en [0, precomputed_size)
        float fidx = (p->orbitAngle * (float)g_precomputed_size) / TWO_PI;
        int   idx  = (int)fidx;                     // parte entera
        float t    = fidx - (float)idx;             // parte fraccional
        if (idx >= g_precomputed_size) idx -= g_precomputed_size;
        int next = idx + 1; if (next >= g_precomputed_size) next = 0;

        // Interpolación lineal para mayor precisión/suavidad
        cosAngle = g_precomputed_cos[idx] + t * (g_precomputed_cos[next] - g_precomputed_cos[idx]);
        sinAngle = g_precomputed_sin[idx] + t * (g_precomputed_sin[next] - g_precomputed_sin[idx]);
    } else {
        // Fallback seguro
        cosAngle = cosf(p->orbitAngle);
        sinAngle = sinf(p->orbitAngle);
    }

    // Calcular nueva posición orbital
    p->position.x = p->orbitRadius * cosAngle;
    p->position.y = p->verticalOffset + p->orbitRadius * p->inclination * sinAngle;
    p->position.z = p->orbitRadius * sinAngle;

    // Actualizar velocidad tangencial
    p->velocity.x = -p->orbitRadius * p->orbitSpeed * sinAngle;
    p->velocity.y =  p->orbitRadius * p->orbitSpeed * p->inclination * cosAngle;
    p->velocity.z =  p->orbitRadius * p->orbitSpeed * cosAngle;

    // Actualizar vida
    p->life -= deltaTime;
}


void updateRingSystem(RingSystem* rs, float deltaTime) {
    // Usar schedule dynamic para mejor balance de carga
    #pragma omp parallel for schedule(dynamic, 128) 
    for (size_t i = 0; i < rs->count; i++) {
        updateRingParticle(&rs->particles[i], deltaTime);
        
        if (rs->particles[i].life <= 0) {
            // Mover regeneración fuera del bucle crítico
            int ringIndex = rs->particles[i].ringIndex;
            int particlesPerRing = rs->count / rs->numRings;
            int particleIndexInRing = i % particlesPerRing;
            
            createRingParticle(&rs->particles[i], ringIndex, rs->numRings, 
                             rs->minRadius, rs->maxRadius, 
                             particleIndexInRing, particlesPerRing, particlesPerRing);
        }
    }
}

void drawRingSystem(RingSystem* rs) {
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Blend aditivo para mayor brillo
    
    // === Pre-asignar memoria solo una vez ===
    static float* alphas = NULL;
    static float* vertices = NULL;  // Buffer para vértices
    static float* colors = NULL;    // Buffer para colores
    static int* valid_indices = NULL; // Índices de partículas válidas
    static size_t buffer_capacity = 0;
    
    if (buffer_capacity < rs->count) {
        alphas = realloc(alphas, rs->count * sizeof(float));
        vertices = realloc(vertices, rs->count * 3 * sizeof(float)); // x,y,z por partícula
        colors = realloc(colors, rs->count * 4 * sizeof(float));     // r,g,b,a por partícula
        valid_indices = realloc(valid_indices, rs->count * sizeof(int));
        buffer_capacity = rs->count;
    }
    
    for (int ring = 0; ring < rs->numRings; ring++) {
        float pointSize = 3.0f + (float)ring * 0.5f;
        
        // === FASE 1: Cálculo paralelo de alpha ===
        #pragma omp parallel for schedule(dynamic, 64)
        for (size_t i = 0; i < rs->count; i++) {
            RingParticle* p = &rs->particles[i];
            if (p->life > 0 && p->ringIndex == ring) {
                float alpha = 0.8f;
                int nearbyCount = 0;
                
                // Buscar vecinos con optimización
                for (size_t j = i + 1; j < rs->count; j++) {
                    if (rs->particles[j].ringIndex == ring) {
                        float dx = p->position.x - rs->particles[j].position.x;
                        float dz = p->position.z - rs->particles[j].position.z;
                        float dist_sq = dx * dx + dz * dz;
                        if (dist_sq < 0.04f) {
                            nearbyCount++;
                        }
                    }
                }
                
                float densityFactor = 1.0f + (float)nearbyCount * 0.1f;
                alphas[i] = alpha * fminf(densityFactor, 2.0f);
            } else {
                alphas[i] = 0.0f;
            }
        }
 
        int valid_count = 0;
        
        #pragma omp parallel
        {
            int local_count = 0;
            int thread_start_index = 0;
            
            #pragma omp for schedule(static)
            for (size_t i = 0; i < rs->count; i++) {
                if (alphas[i] > 0.0f) {
                    local_count++;
                }
            }
            
            // ← Barrera implícita del for
            
            // FASE 2a: Calcular offset para cada thread (reduction)
            #pragma omp critical
            {
                thread_start_index = valid_count;
                valid_count += local_count;
            }
            
            #pragma omp barrier  
            
            // FASE 2b: Llenar buffers con offset correcto
            int local_index = thread_start_index;
            #pragma omp for schedule(static)
            for (size_t i = 0; i < rs->count; i++) {
                if (alphas[i] > 0.0f) {
                    RingParticle* p = &rs->particles[i];
                    
                    // Guardar índice y datos
                    valid_indices[local_index] = (int)i;
                    
                    vertices[local_index * 3 + 0] = p->position.x;
                    vertices[local_index * 3 + 1] = p->position.y;
                    vertices[local_index * 3 + 2] = p->position.z;
                    
                    colors[local_index * 4 + 0] = p->color.x;
                    colors[local_index * 4 + 1] = p->color.y;
                    colors[local_index * 4 + 2] = p->color.z;
                    colors[local_index * 4 + 3] = alphas[i];
                    
                    local_index++;
                }
            }
        }
        
        
        glPointSize(pointSize);
        glBegin(GL_POINTS);
        for (int k = 0; k < valid_count; k++) {
            glColor4f(colors[k*4 + 0], colors[k*4 + 1], colors[k*4 + 2], colors[k*4 + 3]);
            glVertex3f(vertices[k*3 + 0], vertices[k*3 + 1], vertices[k*3 + 2]);
        }
        glEnd();
        
        glPointSize(pointSize * 0.6f);
        glBegin(GL_POINTS);
        for (int k = 0; k < valid_count; k++) {
            glColor4f(colors[k*4 + 0], colors[k*4 + 1], colors[k*4 + 2], 0.3f);
            glVertex3f(vertices[k*3 + 0], vertices[k*3 + 1], vertices[k*3 + 2]);
        }
        glEnd();
    }
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_POINT_SMOOTH);
}
void initializeRingSystem(RingSystem* rs, int numParticles, int numRings) {
    rs->count = numParticles;

    int particlesPerRing = numParticles / numRings;
    int remainder = numParticles % numRings;
    int maxParticlesPerRing = particlesPerRing + 1; // el máximo posible

    printf("Distribuyendo %d partículas en %d anillos:\n", numParticles, numRings);

    #pragma omp parallel for collapse(2)
    for (int ring = 0; ring < numRings; ring++) {
        for (int p = 0; p < maxParticlesPerRing; p++) {
            int particlesInThisRing = particlesPerRing + (ring < remainder ? 1 : 0);

            // Saltar iteraciones sobrantes
            if (p >= particlesInThisRing) continue;

            float baseRadius = rs->minRadius + ring * ((rs->maxRadius - rs->minRadius) / (numRings - 1));
            float maxRadius  = baseRadius + ((rs->maxRadius - rs->minRadius) / (numRings - 1)) * 0.15f;

            // Calcular índice global
            int globalIndex = 0;
            for (int r = 0; r < ring; r++) {
                globalIndex += particlesPerRing + (r < remainder ? 1 : 0);
            }
            globalIndex += p;

            createRingParticle(&rs->particles[globalIndex], ring, numRings,
                               rs->minRadius, rs->maxRadius, p,
                               particlesInThisRing, particlesInThisRing);
        }
    }
}




void setupCamera(float cameraAngle, float cameraHeight, float cameraDistance) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    float aspect = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
    float fov = 45.0f;
    float near = 0.1f;
    float far = 100.0f;
    
    float top = near * tanf(fov * M_PI / 360.0f);
    float bottom = -top;
    float right = top * aspect;
    float left = -right;
    
    glFrustum(left, right, bottom, top, near, far);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    float camX = cameraDistance * cosf(cameraAngle);
    float camY = cameraHeight;
    float camZ = cameraDistance * sinf(cameraAngle);
    
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: ejecutable num_particulas");
        return 1;
    }
        
    
    int numParticles = atoi(argv[1]);
    int numRings = 1;
    if (numParticles <= 0) {
        printf("Error: Número de partículas debe ser mayor que 0\n");
        return 1;
    }

    omp_set_num_threads(omp_get_max_threads());
    omp_set_dynamic(0); // deshabilitar ajuste dinámico
    printf("Usando %d threads\n", omp_get_max_threads());
    
    printf("Creando sistema de anillos de Urano con %d partículas...\n", numParticles);
    
    // Inicializar GLFW
    if (!glfwInit()) {
        printf("Error al inicializar GLFW\n");
        return 1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "Anillos de Urano - Sistema de Partículas", NULL, NULL);
    if (!window) {
        printf("Error al crear ventana\n");
        glfwTerminate();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);
    
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
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Configurar esfera
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    float sphereRadius = 0.5f;
    generateSphere(sphereRadius, 50, 50);
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertexCount * sizeof(Vertex), sphereVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndexCount * sizeof(unsigned int), sphereIndices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    texture = loadTexture("87614c592cfb5e9d369fdde536263e2b.jpg");
    
    srand((unsigned int)time(NULL));
    
    // Crear sistema de anillos
    float minRadius = 1.0f;  // Radio mínimo (después de la superficie del planeta)
    float maxRadius = 4.0f;  // Radio máximo
    
    RingSystem* rs = createRingSystem(numParticles, numRings, minRadius, maxRadius);
    if (!rs) {
        printf("Error al crear sistema de anillos\n");
        return 1;
    }
    
    int numStars = 2000;
    RingSystem* stars = createRingSystem(numStars, 1, 0.0f, 0.0f); // radio no importa porque no usamos órbitas
    stars->count = numStars;

    #pragma omp parallel for
    for (int i = 0; i < stars->count; i++) {
        // Distribución aleatoria en un cubo grande
        float range = 50.0f; // cuánto se alejan del centro
        stars->particles[i].position.x = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * range;
        stars->particles[i].position.y = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * range;
        stars->particles[i].position.z = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * range;

        stars->particles[i].orbitSpeed = 0.0f;        // no se mueven
        stars->particles[i].color = (Vec3){1.0f, 1.0f, 1.0f}; // blancas
        stars->particles[i].size = 1.0f + ((float)rand() / RAND_MAX) * 1.5f; // variar tamaño
        stars->particles[i].life = stars->particles[i].maxLife = 1.0f; // opcional, no cambia
    }


    // Distribuir partículas entre anillos
    rs->count = numParticles;
    int particlesPerRing = numParticles / numRings;
    
    initializeRingSystem(rs, numParticles, numRings);

    printf("Sistema iniciado con %d anillos. Usa ESC para salir.\n", numRings);
    
    // Variables de control
    double lastTime = glfwGetTime();
    double cameraAngle = 2.0;
    double planetRotation = 0.0;
    int frameCount = 0;
    float cameraHeight = 2.0f;
    float cameraDistance = 4.0f;
    
    // Bucle principal
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;
        
        // Actualizar sistema de anillos
        updateRingSystem(rs, deltaTime);
        
        // Rotar cámara y planeta
        cameraAngle += deltaTime * 0.2;
        planetRotation += deltaTime * 0.5;
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Dibujar esfera de Urano
        glUseProgram(shaderProgram);
        
        float model[16], view[16], projection[16];
        
        mat4_rotate_y(model, planetRotation);
        mat4_lookat(view, cameraDistance * cosf(cameraAngle), cameraHeight, cameraDistance * sinf(cameraAngle),
                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

                    
        mat4_perspective(projection, M_PI/4.0f, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 100.0f);
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection);
        
        // Configurar iluminación
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 5.0f, 5.0f, 5.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 
                    cameraDistance * cosf(cameraAngle), cameraHeight, cameraDistance * sinf(cameraAngle));
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        
        // Activar textura
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
        
        // Dibujar esfera
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
       
        

        glUseProgram(0);
        setupCamera(cameraAngle, cameraHeight, cameraDistance);
        drawRingSystem(stars);

        // Dibujar anillos de partículas
        drawRingSystem(rs);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Debug info cada 60 frames
        frameCount++;
        if (frameCount % 60 == 0) {
            printf("Frame %d - FPS: %.1f - Particulas: %zu - Anillos: %d\n", 
                   frameCount, 1.0f / deltaTime, rs->count, numRings);
        }
    }
    
    // Limpieza
    destroyRingSystem(rs);
    free(sphereVertices);
    free(sphereIndices);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &texture);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    printf("Simulación terminada.\n");
    return 0;
}