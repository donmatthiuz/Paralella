#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Math structures to replace GLM
typedef struct {
    float x, y, z;
} vec3;

typedef struct {
    float x, y, z, w;
} vec4;

typedef struct {
    float m[16];  // column-major order like OpenGL
} mat4;

// Global constants
static const double c = 299792458.0;
static const double G = 6.67430e-11;
static bool Gravity = false;
static double lastPrintTime = 0.0;
static int framesCount = 0;

// Math utility functions
vec3 vec3_create(float x, float y, float z) {
    vec3 v = {x, y, z};
    return v;
}

vec4 vec4_create(float x, float y, float z, float w) {
    vec4 v = {x, y, z, w};
    return v;
}

float vec3_length(vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 vec3_normalize(vec3 v) {
    float len = vec3_length(v);
    if (len > 0.0f) {
        v.x /= len;
        v.y /= len;
        v.z /= len;
    }
    return v;
}

vec3 vec3_subtract(vec3 a, vec3 b) {
    return vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec3 vec3_add(vec3 a, vec3 b) {
    return vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

vec3 vec3_cross(vec3 a, vec3 b) {
    return vec3_create(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float clamp_float(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Matrix functions
mat4 mat4_identity(void) {
    mat4 m = {{0}};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

mat4 mat4_perspective(float fov_radians, float aspect, float near_plane, float far_plane) {
    mat4 m = {{0}};
    float f = 1.0f / tanf(fov_radians * 0.5f);
    
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far_plane + near_plane) / (near_plane - far_plane);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * far_plane * near_plane) / (near_plane - far_plane);
    
    return m;
}

mat4 mat4_lookat(vec3 eye, vec3 center, vec3 up) {
    vec3 f = vec3_normalize(vec3_subtract(center, eye));
    vec3 s = vec3_normalize(vec3_cross(f, up));
    vec3 u = vec3_cross(s, f);
    
    mat4 m = mat4_identity();
    
    m.m[0] = s.x;
    m.m[4] = s.y;
    m.m[8] = s.z;
    m.m[1] = u.x;
    m.m[5] = u.y;
    m.m[9] = u.z;
    m.m[2] = -f.x;
    m.m[6] = -f.y;
    m.m[10] = -f.z;
    m.m[12] = -vec3_cross(s, eye).x - vec3_cross(s, eye).y - vec3_cross(s, eye).z;
    m.m[13] = -vec3_cross(u, eye).x - vec3_cross(u, eye).y - vec3_cross(u, eye).z;
    m.m[14] = vec3_cross(f, eye).x + vec3_cross(f, eye).y + vec3_cross(f, eye).z;
    
    return m;
}

mat4 mat4_multiply(mat4 a, mat4 b) {
    mat4 result = {{0}};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i * 4 + j] = 
                a.m[i * 4 + 0] * b.m[0 * 4 + j] +
                a.m[i * 4 + 1] * b.m[1 * 4 + j] +
                a.m[i * 4 + 2] * b.m[2 * 4 + j] +
                a.m[i * 4 + 3] * b.m[3 * 4 + j];
        }
    }
    return result;
}

// Camera structure and functions
typedef struct {
    vec3 target;
    float radius;
    float minRadius, maxRadius;
    float azimuth;
    float elevation;
    float orbitSpeed;
    float panSpeed;
    double zoomSpeed;
    bool dragging;
    bool panning;
    bool moving;
    double lastX, lastY;
} Camera;

Camera* camera_create(void) {
    Camera* cam = malloc(sizeof(Camera));
    cam->target = vec3_create(0.0f, 0.0f, 0.0f);
    cam->radius = 6.34194e10f;
    cam->minRadius = 1e10f;
    cam->maxRadius = 1e12f;
    cam->azimuth = 0.0f;
    cam->elevation = M_PI / 2.0f;
    cam->orbitSpeed = 0.01f;
    cam->panSpeed = 0.01f;
    cam->zoomSpeed = 25e9;
    cam->dragging = false;
    cam->panning = false;
    cam->moving = false;
    cam->lastX = 0.0;
    cam->lastY = 0.0;
    return cam;
}

vec3 camera_position(Camera* cam) {
    float clampedElevation = clamp_float(cam->elevation, 0.01f, (float)M_PI - 0.01f);
    return vec3_create(
        cam->radius * sinf(clampedElevation) * cosf(cam->azimuth),
        cam->radius * cosf(clampedElevation),
        cam->radius * sinf(clampedElevation) * sinf(cam->azimuth)
    );
}

void camera_update(Camera* cam) {
    cam->target = vec3_create(0.0f, 0.0f, 0.0f);
    cam->moving = cam->dragging || cam->panning;
}

void camera_process_mouse_move(Camera* cam, double x, double y) {
    float dx = (float)(x - cam->lastX);
    float dy = (float)(y - cam->lastY);
    
    if (cam->dragging && cam->panning) {
        // Panning disabled to keep camera centered on black hole
    }
    else if (cam->dragging && !cam->panning) {
        cam->azimuth += dx * cam->orbitSpeed;
        cam->elevation -= dy * cam->orbitSpeed;
        cam->elevation = clamp_float(cam->elevation, 0.01f, (float)M_PI - 0.01f);
    }
    
    cam->lastX = x;
    cam->lastY = y;
    camera_update(cam);
}

void camera_process_mouse_button(Camera* cam, int button, int action, int mods, GLFWwindow* win) {
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            cam->dragging = true;
            cam->panning = false; // Disable panning
            glfwGetCursorPos(win, &cam->lastX, &cam->lastY);
        } else if (action == GLFW_RELEASE) {
            cam->dragging = false;
            cam->panning = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            Gravity = true;
        } else if (action == GLFW_RELEASE) {
            Gravity = false;
        }
    }
}

void camera_process_scroll(Camera* cam, double xoffset, double yoffset) {
    cam->radius -= (float)(yoffset * cam->zoomSpeed);
    cam->radius = clamp_float(cam->radius, cam->minRadius, cam->maxRadius);
    camera_update(cam);
}

void camera_process_key(Camera* cam, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_G) {
        Gravity = !Gravity;
        printf("[INFO] Gravity turned %s\n", Gravity ? "ON" : "OFF");
    }
}

// Black hole structure
typedef struct {
    vec3 position;
    double mass;
    double radius;
    double r_s;
} BlackHole;

BlackHole* blackhole_create(vec3 pos, float m) {
    BlackHole* bh = malloc(sizeof(BlackHole));
    bh->position = pos;
    bh->mass = m;
    bh->r_s = 2.0 * G * m / (c * c);
    return bh;
}

bool blackhole_intercept(BlackHole* bh, float px, float py, float pz) {
    double dx = (double)px - (double)bh->position.x;
    double dy = (double)py - (double)bh->position.y;
    double dz = (double)pz - (double)bh->position.z;
    double dist2 = dx * dx + dy * dy + dz * dz;
    return dist2 < bh->r_s * bh->r_s;
}

// Object data structure
typedef struct {
    vec4 posRadius; // xyz = position, w = radius
    vec4 color;     // rgb = color, a = unused
    float mass;
    vec3 velocity;
} ObjectData;

// Dynamic array for objects
typedef struct {
    ObjectData* data;
    size_t size;
    size_t capacity;
} ObjectArray;

ObjectArray* object_array_create(void) {
    ObjectArray* arr = malloc(sizeof(ObjectArray));
    arr->capacity = 10;
    arr->size = 0;
    arr->data = malloc(sizeof(ObjectData) * arr->capacity);
    return arr;
}

void object_array_push(ObjectArray* arr, ObjectData obj) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, sizeof(ObjectData) * arr->capacity);
    }
    arr->data[arr->size++] = obj;
}

void object_array_free(ObjectArray* arr) {
    free(arr->data);
    free(arr);
}

// Engine structure
typedef struct {
    GLuint gridShaderProgram;
    GLFWwindow* window;
    GLuint quadVAO;
    GLuint texture;
    GLuint shaderProgram;
    GLuint computeProgram;
    GLuint cameraUBO;
    GLuint diskUBO;
    GLuint objectsUBO;
    GLuint gridVAO;
    GLuint gridVBO;
    GLuint gridEBO;
    int gridIndexCount;
    int WIDTH, HEIGHT;
    int COMPUTE_WIDTH, COMPUTE_HEIGHT;
    float width, height;
} Engine;

// Shader loading utility
char* load_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(length + 1);
    fread(content, 1, length, file);
    content[length] = '\0';
    
    fclose(file);
    return content;
}

GLuint compile_shader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        char* log = malloc(logLen);
        glGetShaderInfoLog(shader, logLen, NULL, log);
        fprintf(stderr, "Shader compile error:\n%s\n", log);
        free(log);
        exit(EXIT_FAILURE);
    }
    
    return shader;
}

GLuint create_shader_program_from_strings(const char* vertSource, const char* fragSource) {
    GLuint vertShader = compile_shader(vertSource, GL_VERTEX_SHADER);
    GLuint fragShader = compile_shader(fragSource, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    
    GLint linkSuccess;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        char* log = malloc(logLen);
        glGetProgramInfoLog(program, logLen, NULL, log);
        fprintf(stderr, "Shader link error:\n%s\n", log);
        free(log);
        exit(EXIT_FAILURE);
    }
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    
    return program;
}

GLuint create_compute_program(const char* path) {
    char* source = load_file(path);
    if (!source) {
        exit(EXIT_FAILURE);
    }
    
    GLuint cs = compile_shader(source, GL_COMPUTE_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, cs);
    glLinkProgram(program);
    
    GLint linkSuccess;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        GLint logLen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        char* log = malloc(logLen);
        glGetProgramInfoLog(program, logLen, NULL, log);
        fprintf(stderr, "Compute shader link error:\n%s\n", log);
        free(log);
        exit(EXIT_FAILURE);
    }
    
    glDeleteShader(cs);
    free(source);
    return program;
}

// Engine functions
Engine* engine_create(void) {
    Engine* engine = malloc(sizeof(Engine));
    
    engine->WIDTH = 800;
    engine->HEIGHT = 600;
    engine->COMPUTE_WIDTH = 200;
    engine->COMPUTE_HEIGHT = 150;
    engine->width = 100000000000.0f;
    engine->height = 75000000000.0f;
    engine->gridIndexCount = 0;
    engine->gridVAO = 0;
    engine->gridVBO = 0;
    engine->gridEBO = 0;
    
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "GLFW init failed\n");
        exit(EXIT_FAILURE);
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    engine->window = glfwCreateWindow(engine->WIDTH, engine->HEIGHT, "Black Hole", NULL, NULL);
    if (!engine->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    glfwMakeContextCurrent(engine->window);
    
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n", 
                (const char*)glewGetErrorString(glewErr));
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    printf("OpenGL %s\n", (const char*)glGetString(GL_VERSION));
    
    // Create shader programs
    const char* vertexShaderSource = 
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "    TexCoord = aTexCoord;\n"
        "}\n";
    
    const char* fragmentShaderSource = 
        "#version 330 core\n"
        "in vec2 TexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D screenTexture;\n"
        "void main() {\n"
        "    FragColor = texture(screenTexture, TexCoord);\n"
        "}\n";
    
    engine->shaderProgram = create_shader_program_from_strings(vertexShaderSource, fragmentShaderSource);
    
    // Create compute program (assuming shader files exist)
    engine->computeProgram = create_compute_program("geodesic.comp");
    
    // Create UBOs
    glGenBuffers(1, &engine->cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, engine->cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, 128, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, engine->cameraUBO);
    
    glGenBuffers(1, &engine->diskUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, engine->diskUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, engine->diskUBO);
    
    glGenBuffers(1, &engine->objectsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, engine->objectsUBO);
    GLsizeiptr objUBOSize = sizeof(int) + 3 * sizeof(float) + 
                           16 * (sizeof(vec4) + sizeof(vec4)) + 
                           16 * sizeof(float);
    glBufferData(GL_UNIFORM_BUFFER, objUBOSize, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, engine->objectsUBO);
    
    // Create quad VAO and texture
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    GLuint VBO;
    glGenVertexArrays(1, &engine->quadVAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(engine->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glGenTextures(1, &engine->texture);
    glBindTexture(GL_TEXTURE_2D, engine->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, engine->COMPUTE_WIDTH, engine->COMPUTE_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
    return engine;
}

// Global variables (replacing C++ static members)
static Camera* g_camera = NULL;
static Engine* g_engine = NULL;
static BlackHole* g_sagA = NULL;
static ObjectArray* g_objects = NULL;

// Callback functions
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    camera_process_mouse_button(g_camera, button, action, mods, window);
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    camera_process_mouse_move(g_camera, xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera_process_scroll(g_camera, xoffset, yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    camera_process_key(g_camera, key, scancode, action, mods);
}

void setup_callbacks(GLFWwindow* window) {
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
}

// Main function
int main(void) {
    // Initialize global objects
    g_camera = camera_create();
    g_engine = engine_create();
    g_sagA = blackhole_create(vec3_create(0.0f, 0.0f, 0.0f), 8.54e36f);
    g_objects = object_array_create();
    
    // Add initial objects
    ObjectData obj1 = {
        vec4_create(4e11f, 0.0f, 0.0f, 4e10f),
        vec4_create(1.0f, 1.0f, 0.0f, 1.0f),
        1.98892e30f,
        vec3_create(0.0f, 0.0f, 0.0f)
    };
    object_array_push(g_objects, obj1);
    
    ObjectData obj2 = {
        vec4_create(0.0f, 0.0f, 4e11f, 4e10f),
        vec4_create(1.0f, 0.0f, 0.0f, 1.0f),
        1.98892e30f,
        vec3_create(0.0f, 0.0f, 0.0f)
    };
    object_array_push(g_objects, obj2);
    
    ObjectData obj3 = {
        vec4_create(0.0f, 0.0f, 0.0f, (float)g_sagA->r_s),
        vec4_create(0.0f, 0.0f, 0.0f, 1.0f),
        (float)g_sagA->mass,
        vec3_create(0.0f, 0.0f, 0.0f)
    };
    object_array_push(g_objects, obj3);
    
    setup_callbacks(g_engine->window);
    
    double lastTime = glfwGetTime();
    
    // Main loop
    while (!glfwWindowShouldClose(g_engine->window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        double now = glfwGetTime();
        double dt = now - lastTime;
        lastTime = now;
        
        // Gravity simulation (simplified)
        if (Gravity) {
            for (size_t i = 0; i < g_objects->size; i++) {
                for (size_t j = 0; j < g_objects->size; j++) {
                    if (i == j) continue;
                    
                    ObjectData* obj1 = &g_objects->data[i];
                    ObjectData* obj2 = &g_objects->data[j];
                    
                    float dx = obj2->posRadius.x - obj1->posRadius.x;
                    float dy = obj2->posRadius.y - obj1->posRadius.y;
                    float dz = obj2->posRadius.z - obj1->posRadius.z;
                    float distance = sqrtf(dx*dx + dy*dy + dz*dz);
                    
                    if (distance > 0) {
                        vec3 direction = vec3_create(dx/distance, dy/distance, dz/distance);
                        double Gforce = (G * obj1->mass * obj2->mass) / (distance * distance);
                        double acc1 = Gforce / obj1->mass;
                        
                        vec3 acc = vec3_create(
                            direction.x * acc1,
                            direction.y * acc1, 
                            direction.z * acc1
                        );
                        
                        obj1->velocity = vec3_add(obj1->velocity, acc);
                        obj1->posRadius.x += obj1->velocity.x;
                        obj1->posRadius.y += obj1->velocity.y;
                        obj1->posRadius.z += obj1->velocity.z;
                    }
                }
            }
        }
        
        // Render fullscreen quad
        glUseProgram(g_engine->shaderProgram);
        glBindVertexArray(g_engine->quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_engine->texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glfwSwapBuffers(g_engine->window);
        glfwPollEvents();
    }
    
    // Cleanup
    object_array_free(g_objects);
    free(g_sagA);
    free(g_camera);
    free(g_engine);
    
    glfwDestroyWindow(g_engine->window);
    glfwTerminate();
    
    return 0;
}