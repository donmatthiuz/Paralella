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

#define WINDOW_WIDTH 800 
#define WINDOW_HEIGHT 600
#define MAX_PARTICLES 50000
#define M_PI 3.14159265359

// Estructuras matemáticas
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float m[16];
} Mat4;

// Aqui empieza el primer cambio para paralelizar, separamos los datos que mas cambian [lo nombrare como hot data] y los cambios que cambian muy
// poco [cold data] por lo que separo la estructura anterior en 2 estructuras mas pequenas

typedef struct {
    Vec3 position;
    Vec3 velocity;
    float orbitAngle;
    float life;
} HotParticleData;

typedef struct {
    float orbitRadius;
    float orbitSpeed;
    float inclination;
    float verticalOffset;
    float size;
    float maxLife;
    Vec3 color;
    int ringIndex;
} ColdParticleData;


// Sistema de anillos
typedef struct {
	// Aqui solo agrego esa separacion de estructuras de arriba 
	HotParticleData* hotData;
    ColdParticleData* coldData;
    size_t count;
    size_t capacity;
    int numRings;         // Número de anillos
    float minRadius;      // Radio mínimo de anillos
    float maxRadius;      // Radio máximo de anillos
} RingSystem;

// Estructura para vértices de la esfera
typedef struct {
    float x, y, z;    // Posición
    float u, v;       // Coordenadas de textura
    float nx, ny, nz; // Normales
} Vertex;

// Tambien voy a definir una estructura que me ayudara a paralelizar los calculos trigonometricos
typedef struct {
    float* cosTable;
    float* sinTable;
    int tableSize;
    float angleStep;
} TrigTable;

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

// Funcion que usa la estructura TrigTable para paralelizar y calcular trigonometria
TrigTable* createTrigTable(int size) {
    TrigTable* table = malloc(sizeof(TrigTable));
    table->tableSize = size;
    table->angleStep = 2.0f * M_PI / size;
    table->cosTable = malloc(size * sizeof(float));
    table->sinTable = malloc(size * sizeof(float));
    
	// OMP for para crear las tablas trigonometricas
    #pragma omp parallel for
    for (int i = 0; i < size; i++) {
        float angle = i * table->angleStep;
        table->cosTable[i] = cosf(angle);
        table->sinTable[i] = sinf(angle);
    }
    
    return table;
}

unsigned int rand_r(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed / 65536) % 32768;
}
// Generar esfera
void generateSphere(float radius, int sectors, int stacks) {
    sphereVertexCount = (sectors + 1) * (stacks + 1);
    sphereIndexCount = sectors * stacks * 6;
    
    sphereVertices = malloc(sphereVertexCount * sizeof(Vertex));
    sphereIndices = malloc(sphereIndexCount * sizeof(unsigned int));
    
    const float sectorStep = 2 * M_PI / sectors;
    const float stackStep = M_PI / stacks;
    
    #pragma omp parallel for collapse(2)
    for (int i = 0; i <= stacks; ++i) {
        for (int j = 0; j <= sectors; ++j) {
            const float stackAngle = M_PI / 2 - i * stackStep;
            const float sectorAngle = j * sectorStep;
            const int vertIndex = i * (sectors + 1) + j;
            
            const float xy = radius * cosf(stackAngle);
            const float z = radius * sinf(stackAngle);
            
            sphereVertices[vertIndex].x = xy * cosf(sectorAngle);
            sphereVertices[vertIndex].y = xy * sinf(sectorAngle);
            sphereVertices[vertIndex].z = z;
            
            const float invRadius = 1.0f / radius;
            sphereVertices[vertIndex].nx = sphereVertices[vertIndex].x * invRadius;
            sphereVertices[vertIndex].ny = sphereVertices[vertIndex].y * invRadius;
            sphereVertices[vertIndex].nz = sphereVertices[vertIndex].z * invRadius;
            
            sphereVertices[vertIndex].u = (float)j / sectors;
            sphereVertices[vertIndex].v = (float)i / stacks;
        }
    }
    
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            const int k1 = i * (sectors + 1) + j;
            const int k2 = k1 + sectors + 1;
            const int indexBase = (i * sectors + j) * 6;
            
            if (i != 0) {
                sphereIndices[indexBase] = k1;
                sphereIndices[indexBase + 1] = k2;
                sphereIndices[indexBase + 2] = k1 + 1;
            }
            
            if (i != (stacks - 1)) {
                sphereIndices[indexBase + 3] = k1 + 1;
                sphereIndices[indexBase + 4] = k2;
                sphereIndices[indexBase + 5] = k2 + 1;
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
    RingSystem* rs = malloc(sizeof(RingSystem));
    if (!rs) return NULL;

    rs->hotData = malloc(sizeof(HotParticleData) * capacity);
    if (!rs->hotData) {
        free(rs);
        return NULL;
    }

    rs->coldData = malloc(sizeof(ColdParticleData) * capacity);
    if (!rs->coldData) {
        free(rs->hotData);
        free(rs);
        return NULL;
    }

    rs->count = 0;
    rs->capacity = capacity;
    rs->numRings = numRings;
    rs->minRadius = minRadius;
    rs->maxRadius = maxRadius;
    return rs;
}

void destroyRingSystem(RingSystem* rs) {
    if (!rs) return;

	if (rs->hotData){
        free(rs->hotData);
		rs->hotData = NULL;
    }
	if (rs->coldData){
        free(rs->coldData);
		rs->coldData = NULL;
    }
	rs->count = 0;
	rs->capacity= 0;

}

void createRingParticle(HotParticleData* hot, ColdParticleData* cold, int ringIndex, int numRings, float minRadius, float maxRadius, TrigTable* trigTable){
    
    unsigned int seed = omp_get_thread_num() + time(NULL);
    
    float ringSpacing = (maxRadius - minRadius) / (numRings > 1 ? numRings - 1 : 1);
    float baseRadius = minRadius + ringIndex * ringSpacing;
    
    cold->orbitRadius = baseRadius + (rand_r(&seed) / (float)RAND_MAX - 0.5f) * 
                        ringSpacing * 0.3f;
    
    hot->orbitAngle = (rand_r(&seed) / (float)RAND_MAX) * 2.0f * M_PI;
    
    cold->orbitSpeed = 0.5f * sqrtf(cold->orbitRadius) * 
                       (0.8f + (rand_r(&seed) / (float)RAND_MAX) * 0.4f);
    
    int angleIndex = (int)(hot->orbitAngle / trigTable->angleStep) % trigTable->tableSize;
    float cosAngle = trigTable->cosTable[angleIndex];
    float sinAngle = trigTable->sinTable[angleIndex];
    
    cold->inclination = (rand_r(&seed) / (float)RAND_MAX - 0.5f) * 0.2f;
    cold->verticalOffset = (rand_r(&seed) / (float)RAND_MAX - 0.5f) * 0.1f;
    
    hot->position.x = cold->orbitRadius * cosAngle;
    hot->position.y = cold->verticalOffset + cold->orbitRadius * cold->inclination * sinAngle;
    hot->position.z = cold->orbitRadius * sinAngle;
    
    cold->size = 0.02f + (rand_r(&seed) / (float)RAND_MAX) * 0.03f;
    hot->life = cold->maxLife = 5.0f + (rand_r(&seed) / (float)RAND_MAX) * 10.0f;
    cold->ringIndex = ringIndex;

    switch (ringIndex % 4){
        case 0:
            cold->color = (Vec3){1.0f, 1.0f, 1.0f};
            break;
        case 1:
            cold->color = (Vec3){0.7f, 1.0f, 0.8f};
            break;
        case 2:
            cold->color = (Vec3){1.0f, 1.0f, 0.7f};
            break;
        case 3:
            cold->color = (Vec3){1.0f, 0.8f, 0.6f};
            break;
    }
}

void updateRingParticle(HotParticleData* hot, ColdParticleData* cold, float deltaTime, TrigTable* trigTable) {
    hot->orbitAngle += cold->orbitSpeed * deltaTime;
    if (hot->orbitAngle > 2.0f * M_PI) {
        hot->orbitAngle -= 2.0f * M_PI;
    }

    int angleIndex = (int)(hot->orbitAngle / trigTable->angleStep) % trigTable->tableSize;
    float cosAngle = trigTable->cosTable[angleIndex];
    float sinAngle = trigTable->sinTable[angleIndex];

    hot->position.x = cold->orbitRadius * cosAngle;
    hot->position.y = cold->verticalOffset + cold->orbitRadius * cold->inclination * sinAngle;
    hot->position.z = cold->orbitRadius * sinAngle;

    hot->velocity.x = -cold->orbitRadius * cold->orbitSpeed * sinAngle;
    hot->velocity.y = cold->orbitRadius * cold->orbitSpeed * cold->inclination * cosAngle;
    hot->velocity.z = cold->orbitRadius * cold->orbitSpeed * cosAngle;

    hot->life -= deltaTime;
}

// si alguien lo limpia, mueva las funciones con el orden requerido
void regenerateDeadParticles(RingSystem* rs, TrigTable* trigTable);
void updateRingParticle(HotParticleData* hot, ColdParticleData* cold, float deltaTime, TrigTable* trigTable);

// se aplica la funcion trigtable [la que esta paralelizada] al sistema de actualizacion de anillos
void updateRingSystem(RingSystem* rs, float deltaTime, TrigTable* trigTable) {
    const int numThreads = omp_get_max_threads();

    #pragma omp parallel for schedule(static) num_threads(numThreads)
    for (size_t i = 0; i < rs->count; i++) {
        HotParticleData* hot = &rs->hotData[i];
        ColdParticleData* cold = &rs->coldData[i];

        hot->orbitAngle += cold->orbitSpeed * deltaTime;
        if (hot->orbitAngle > 2.0f * M_PI) {
            hot->orbitAngle -= 2.0f * M_PI;
        }

        int angleIndex = (int)(hot->orbitAngle / trigTable->angleStep) % trigTable->tableSize;
        float cosAngle = trigTable->cosTable[angleIndex];
        float sinAngle = trigTable->sinTable[angleIndex];

        hot->position.x = cold->orbitRadius * cosAngle;
        hot->position.y = cold->verticalOffset + cold->orbitRadius * cold->inclination * sinAngle;
        hot->position.z = cold->orbitRadius * sinAngle;

        hot->velocity.x = -cold->orbitRadius * cold->orbitSpeed * sinAngle;
        hot->velocity.y = cold->orbitRadius * cold->orbitSpeed * cold->inclination * cosAngle;
        hot->velocity.z = cold->orbitRadius * cold->orbitSpeed * cosAngle;

        hot->life -= deltaTime;
    }

    regenerateDeadParticles(rs, trigTable);
}

void regenerateDeadParticles(RingSystem* rs, TrigTable* trigTable) {
    size_t* deadIndices = malloc(rs->count * sizeof(size_t));
    size_t deadCount = 0;

    #pragma omp parallel
    {
        size_t localDeadIndices[1024];
        size_t localDeadCount = 0;

        #pragma omp for nowait
        for (size_t i = 0; i < rs->count; i++) {
            if (rs->hotData[i].life <= 0) {
                localDeadIndices[localDeadCount++] = i;

                if (localDeadCount >= 1024) {
                    #pragma omp critical
                    {
                        memcpy(&deadIndices[deadCount], localDeadIndices,
                               localDeadCount * sizeof(size_t));
                        deadCount += localDeadCount;
                    }
                    localDeadCount = 0;
                }
            }
        }
        if (localDeadCount > 0) {
            #pragma omp critical
            {
                memcpy(&deadIndices[deadCount], localDeadIndices,
                       localDeadCount * sizeof(size_t));
                deadCount += localDeadCount;
            }
        }
    }

    #pragma omp parallel for
    for (size_t i = 0; i < deadCount; i++) {
        size_t idx = deadIndices[i];
        createRingParticle(&rs->hotData[idx], &rs->coldData[idx],
                                    rs->coldData[idx].ringIndex, rs->numRings,
                                    rs->minRadius, rs->maxRadius, trigTable);
    }

    free(deadIndices);
}

void drawRingSystem(RingSystem* rs) {
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(2.0f);

    glBegin(GL_POINTS);
    for (size_t i = 0; i < rs->count; i++) {
        HotParticleData* hot = &rs->hotData[i];
        ColdParticleData* cold = &rs->coldData[i];
        if (hot->life > 0.0f) {
            float alpha = (hot->life / cold->maxLife) * 0.8f;
            glColor4f(cold->color.x, cold->color.y, cold->color.z, alpha);
            glVertex3f(hot->position.x, hot->position.y, hot->position.z);
        }
    }
    glEnd();

    glDisable(GL_POINT_SMOOTH);
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
        printf("Uso: %s <numero_de_particulas>\n", argv[0]);
        printf("Ejemplo: %s 5000\n", argv[0]);
        return 1;
    }
    
    int numParticles = atoi(argv[1]);
    if (numParticles <= 0) {
        printf("Error: Número de partículas debe ser mayor que 0\n");
        return 1;
    }
    
    printf("Creando sistema de anillos de Urano con %d partículas...\n", numParticles);
    
    // Inicializar GLFW
    if (!glfwInit()) {
        printf("Error al inicializar GLFW\n");
        return 1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height,
    	"Anillos de Urano - Sistema de Partículas", monitor, NULL);
    
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
    
    texture = loadTexture("2k_uranus.jpg");
    
    srand((unsigned int)time(NULL));
    
    // Crear tabla trigonométrica para acelerar seno/coseno (usa la función ya definida)
    TrigTable* trigTable = createTrigTable(8192);

    // Crear sistema de anillos (usando las nuevas hotData / coldData directamente)
    int numRings = 1;  // Número de anillos
    float minRadius = 1.0f;  // Radio mínimo (después de la superficie del planeta)
    float maxRadius = 4.0f;  // Radio máximo

    RingSystem* rs = malloc(sizeof(RingSystem));
    if (!rs) {
        printf("Error: malloc RingSystem\n");
        return 1;
    }
    rs->hotData = malloc(sizeof(HotParticleData) * (size_t)numParticles);
    rs->coldData = malloc(sizeof(ColdParticleData) * (size_t)numParticles);
    if (!rs->hotData || !rs->coldData) {
        printf("Error: malloc datos de partículas\n");
        free(rs->hotData); free(rs->coldData); free(rs);
        return 1;
    }
    rs->count = (size_t)numParticles;
    rs->capacity = (size_t)numParticles;
    rs->numRings = numRings;
    rs->minRadius = minRadius;
    rs->maxRadius = maxRadius;

    // Crear "estrellas" (otro RingSystem, pero sin órbitas activas)
    int numStars = 2000;
    RingSystem* stars = malloc(sizeof(RingSystem));
    if (!stars) {
        printf("Error: malloc stars RingSystem\n");
        free(rs->hotData); free(rs->coldData); free(rs);
        return 1;
    }
    stars->hotData = malloc(sizeof(HotParticleData) * (size_t)numStars);
    stars->coldData = malloc(sizeof(ColdParticleData) * (size_t)numStars);
    if (!stars->hotData || !stars->coldData) {
        printf("Error: malloc datos de estrellas\n");
        free(stars->hotData); free(stars->coldData); free(stars);
        free(rs->hotData); free(rs->coldData); free(rs);
        return 1;
    }
    stars->count = (size_t)numStars;
    stars->capacity = (size_t)numStars;
    stars->numRings = 1;
    stars->minRadius = 0.0f;
    stars->maxRadius = 0.0f;

    for (size_t i = 0; i < stars->count; i++) {
        // Distribución aleatoria en un cubo grande
        float range = 50.0f; // cuánto se alejan del centro
        stars->hotData[i].position.x = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * range;
        stars->hotData[i].position.y = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * range;
        stars->hotData[i].position.z = ((float)rand() / RAND_MAX - 0.5f) * 2.0f * range;

        stars->coldData[i].orbitSpeed = 0.0f;        // no se mueven
        stars->coldData[i].color = (Vec3){1.0f, 1.0f, 1.0f}; // blancas
        stars->coldData[i].size = 1.0f + ((float)rand() / RAND_MAX) * 1.5f; // variar tamaño
        stars->hotData[i].life = stars->coldData[i].maxLife = 1.0f; // opcional, no cambia

        // Inicializar velocidad a cero
        stars->hotData[i].velocity = (Vec3){0.0f, 0.0f, 0.0f};
        stars->hotData[i].orbitAngle = 0.0f;
    }

    // Distribuir partículas entre anillos - inicializar hot/cold arrays
    int particlesPerRing = numRings > 0 ? numParticles / numRings : numParticles;
    for (int i = 0; i < numParticles; i++) {
        int ringIndex = i / particlesPerRing;
        if (ringIndex >= numRings) ringIndex = numRings - 1;

        // Llamar a la función refactorizada que inicializa hot y cold para una partícula
        createRingParticle(&rs->hotData[i], &rs->coldData[i], ringIndex, numRings, minRadius, maxRadius, trigTable);
    }
    
    printf("Sistema iniciado con %d anillos. Usa ESC para salir.\n", numRings);
    
    // Variables de control
    double lastTime = glfwGetTime();
    double cameraAngle = 0.0;
    double planetRotation = 0.0;
    int frameCount = 0;
    float cameraHeight = 2.0f;
    float cameraDistance = maxRadius * 1.5f;
    
    // Bucle principal
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        if (deltaTime <= 0.0f) deltaTime = 1.0f/60.0f;
        lastTime = currentTime;
        
        // Actualizar sistema de anillos (usa la versión que acepta trigTable)
        updateRingSystem(rs, deltaTime, trigTable);
        
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

        // Usamos la vieja función setupCamera para la vista en fixed pipeline (mantener)
        setupCamera(cameraAngle, cameraHeight, cameraDistance);

        //drawRingSystem(stars);

        //drawRingSystem(rs);

        
        // Dibujar estrellas (puntos) directamente usando hotData (evitamos dependencias de estructuras antiguas)
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < stars->count; i++) {
            float alpha = 1.0f;
            Vec3 col = stars->coldData[i].color;
            glColor4f(col.x, col.y, col.z, alpha);
            Vec3 pos = stars->hotData[i].position;
            glVertex3f(pos.x, pos.y, pos.z);
        }
        glEnd();
        glDisable(GL_POINT_SMOOTH);

        // Dibujar anillos de partículas (usando hotData/coldData)
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < rs->count; i++) {
            if (rs->hotData[i].life > 0) {
                float alpha = (rs->hotData[i].life / rs->coldData[i].maxLife) * 0.8f;
                Vec3 c = rs->coldData[i].color;
                glColor4f(c.x, c.y, c.z, alpha);
                Vec3 p = rs->hotData[i].position;
                glVertex3f(p.x, p.y, p.z);
            }
        }
        glEnd();
        glDisable(GL_POINT_SMOOTH);
        
        glfwSwapBuffers(window);
        glfwPollEvents();

        

        //glfwSwapBuffers(window);
        //glfwPollEvents();
        
        // Debug info cada 60 frames
        frameCount++;
        if (frameCount % 60 == 0) {
            printf("Frame %d - FPS: %.1f - Partículas: %zu - Anillos: %d\n", 
                   frameCount, 1.0f / deltaTime, rs->count, numRings);
        }
    }
    
    // Limpieza
    // Liberar memoria asignada manualmente (no llamamos a funciones antiguas que usan tipos viejos)
    if (rs) {
        free(rs->hotData);
        free(rs->coldData);
        free(rs);
    }
    if (stars) {
        free(stars->hotData);
        free(stars->coldData);
        free(stars);
    }

    // liberar trigTable
    if (trigTable) {
        free(trigTable->cosTable);
        free(trigTable->sinTable);
        free(trigTable);
    }

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

