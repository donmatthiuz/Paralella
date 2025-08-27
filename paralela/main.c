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

#define MAX_OBJECTS 16
#define MAX_GRID_VERTICES 676  /* (25+1)*(25+1) */
#define MAX_GRID_INDICES 2500  /* approximate for grid lines */

/* GLOBAL VARIABLES */
double lastPrintTime = 0.0;
int    framesCount   = 0;
double c_speed = 299792458.0;
double G_const = 6.67430e-11;
bool Gravity = false;

/* VECTOR MATH STRUCTS */
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    float m[16];  /* 4x4 matrix stored column-major */
} Mat4;

/* HELPER FUNCTIONS FOR VECTOR/MATRIX MATH */
Vec3 vec3_create(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

Vec4 vec4_create(float x, float y, float z, float w) {
    Vec4 v = {x, y, z, w};
    return v;
}

Vec3 vec3_normalize(Vec3 v) {
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.0f) {
        v.x /= len;
        v.y /= len;
        v.z /= len;
    }
    return v;
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

Vec3 vec3_subtract(Vec3 a, Vec3 b) {
    Vec3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

float clamp_f(float value, float min_val, float max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0] = 1.0f; m.m[5] = 1.0f; m.m[10] = 1.0f; m.m[15] = 1.0f;
    return m;
}

Mat4 mat4_perspective(float fovy, float aspect, float near_z, float far_z) {
    Mat4 m = {0};
    float f = 1.0f / tan(fovy * 0.5f);
    
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far_z + near_z) / (near_z - far_z);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * far_z * near_z) / (near_z - far_z);
    
    return m;
}

Mat4 mat4_lookat(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_normalize(vec3_subtract(center, eye));
    Vec3 s = vec3_normalize(vec3_cross(f, up));
    Vec3 u = vec3_cross(s, f);
    
    Mat4 m = mat4_identity();
    
    m.m[0] = s.x;
    m.m[4] = s.y;
    m.m[8] = s.z;
    m.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    
    m.m[1] = u.x;
    m.m[5] = u.y;
    m.m[9] = u.z;
    m.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    
    m.m[2] = -f.x;
    m.m[6] = -f.y;
    m.m[10] = -f.z;
    m.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    
    m.m[3] = 0.0f;
    m.m[7] = 0.0f;
    m.m[11] = 0.0f;
    m.m[15] = 1.0f;
    
    return m;
}

Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
            }
        }
    }
    return result;
}

/* CAMERA STRUCT */
typedef struct {
    Vec3 target;
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

/* BLACKHOLE STRUCT */
typedef struct {
    Vec3 position;
    double mass;
    double radius;
    double r_s;
} BlackHole;

/* OBJECT DATA STRUCT */
typedef struct {
    Vec4 posRadius;  /* xyz = position, w = radius */
    Vec4 color;      /* rgb = color, a = unused */
    float mass;
    Vec3 velocity;
} ObjectData;

/* ENGINE STRUCT */
typedef struct {
    GLFWwindow* window;
    GLuint gridShaderProgram;
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
    int WIDTH;
    int HEIGHT;
    int COMPUTE_WIDTH;
    int COMPUTE_HEIGHT;
    float width;
    float height;
} Engine;

/* GLOBAL INSTANCES */
Camera camera;
BlackHole SagA;
ObjectData objects[MAX_OBJECTS];
int objectCount = 0;
Engine engine;

/* CAMERA FUNCTIONS */
Vec3 camera_position(const Camera* cam) {
    float clampedElevation = clamp_f(cam->elevation, 0.01f, (float)M_PI - 0.01f);
    Vec3 pos;
    pos.x = cam->radius * sin(clampedElevation) * cos(cam->azimuth);
    pos.y = cam->radius * cos(clampedElevation);
    pos.z = cam->radius * sin(clampedElevation) * sin(cam->azimuth);
    return pos;
}

void camera_update(Camera* cam) {
    cam->target = vec3_create(0.0f, 0.0f, 0.0f);
    cam->moving = cam->dragging || cam->panning;
}

void camera_processMouseMove(Camera* cam, double x, double y) {
    float dx = (float)(x - cam->lastX);
    float dy = (float)(y - cam->lastY);

    if (cam->dragging && !cam->panning) {
        cam->azimuth += dx * cam->orbitSpeed;
        cam->elevation -= dy * cam->orbitSpeed;
        cam->elevation = clamp_f(cam->elevation, 0.01f, (float)M_PI - 0.01f);
    }

    cam->lastX = x;
    cam->lastY = y;
    camera_update(cam);
}

void camera_processMouseButton(Camera* cam, int button, int action, int mods, GLFWwindow* win) {
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            cam->dragging = true;
            cam->panning = false;
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

void camera_processScroll(Camera* cam, double xoffset, double yoffset) {
    cam->radius -= (float)(yoffset * cam->zoomSpeed);
    cam->radius = clamp_f(cam->radius, cam->minRadius, cam->maxRadius);
    camera_update(cam);
}

void camera_processKey(Camera* cam, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_G) {
        Gravity = !Gravity;
        printf("[INFO] Gravity turned %s\n", Gravity ? "ON" : "OFF");
    }
}

/* BLACKHOLE FUNCTIONS */
BlackHole blackhole_create(Vec3 pos, float m) {
    BlackHole bh;
    bh.position = pos;
    bh.mass = m;
    bh.r_s = 2.0 * G_const * m / (c_speed * c_speed);
    return bh;
}

bool blackhole_intercept(const BlackHole* bh, float px, float py, float pz) {
    double dx = (double)px - (double)bh->position.x;
    double dy = (double)py - (double)bh->position.y;
    double dz = (double)pz - (double)bh->position.z;
    double dist2 = dx * dx + dy * dy + dz * dz;
    return dist2 < bh->r_s * bh->r_s;
}

/* SHADER LOADING FUNCTIONS */
char* load_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
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
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
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

GLuint create_shader_program_from_files(const char* vertPath, const char* fragPath) {
    char* vertSource = load_file(vertPath);
    char* fragSource = load_file(fragPath);
    
    if (!vertSource || !fragSource) {
        exit(EXIT_FAILURE);
    }
    
    GLuint program = create_shader_program_from_strings(vertSource, fragSource);
    
    free(vertSource);
    free(fragSource);
    
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
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
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

/* ENGINE FUNCTIONS */
void engine_init(Engine* eng) {
    if (!glfwInit()) {
        fprintf(stderr, "GLFW init failed\n");
        exit(EXIT_FAILURE);
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    eng->WIDTH = 800;
    eng->HEIGHT = 600;
    eng->COMPUTE_WIDTH = 200;
    eng->COMPUTE_HEIGHT = 150;
    eng->width = 100000000000.0f;
    eng->height = 75000000000.0f;
    eng->gridIndexCount = 0;
    
    eng->window = glfwCreateWindow(eng->WIDTH, eng->HEIGHT, "Black Hole", NULL, NULL);
    if (!eng->window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    glfwMakeContextCurrent(eng->window);
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW: %s\n", 
                (const char*)glewGetErrorString(glewErr));
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    printf("OpenGL %s\n", (const char*)glGetString(GL_VERSION));
    
    /* Create default shader program */
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
    
    eng->shaderProgram = create_shader_program_from_strings(vertexShaderSource, fragmentShaderSource);
    eng->gridShaderProgram = create_shader_program_from_files("grid.vert", "grid.frag");
    eng->computeProgram = create_compute_program("geodesic.comp");
    
    /* Create UBOs */
    glGenBuffers(1, &eng->cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, eng->cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, 128, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, eng->cameraUBO);
    
    glGenBuffers(1, &eng->diskUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, eng->diskUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, eng->diskUBO);
    
    glGenBuffers(1, &eng->objectsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, eng->objectsUBO);
    GLsizeiptr objUBOSize = sizeof(int) + 3 * sizeof(float) + 
                           16 * (sizeof(Vec4) + sizeof(Vec4)) + 
                           16 * sizeof(float);
    glBufferData(GL_UNIFORM_BUFFER, objUBOSize, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 3, eng->objectsUBO);
    
    /* Create quad VAO and texture */
    float quadVertices[] = {
        /* positions    texCoords */
        -1.0f,  1.0f,  0.0f, 1.0f,  /* top left */
        -1.0f, -1.0f,  0.0f, 0.0f,  /* bottom left */
         1.0f, -1.0f,  1.0f, 0.0f,  /* bottom right */
        -1.0f,  1.0f,  0.0f, 1.0f,  /* top left */
         1.0f, -1.0f,  1.0f, 0.0f,  /* bottom right */
         1.0f,  1.0f,  1.0f, 1.0f   /* top right */
    };
    
    GLuint VBO;
    glGenVertexArrays(1, &eng->quadVAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(eng->quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glGenTextures(1, &eng->texture);
    glBindTexture(GL_TEXTURE_2D, eng->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, eng->COMPUTE_WIDTH, eng->COMPUTE_HEIGHT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void generate_grid(Engine* eng, const ObjectData* objects, int objCount) {
    const int gridSize = 25;
    const float spacing = 1e10f;
    
    static Vec3 vertices[MAX_GRID_VERTICES];
    static GLuint indices[MAX_GRID_INDICES];
    int vertexCount = 0;
    int indexCount = 0;
    
    /* Generate vertices */
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            float worldX = (x - gridSize / 2) * spacing;
            float worldZ = (z - gridSize / 2) * spacing;
            float y = 0.0f;
            
            /* Warp grid using Schwarzschild geometry */
            for (int i = 0; i < objCount; ++i) {
                Vec3 objPos = vec3_create(objects[i].posRadius.x, objects[i].posRadius.y, objects[i].posRadius.z);
                double mass = objects[i].mass;
                double r_s = 2.0 * G_const * mass / (c_speed * c_speed);
                
                double dx = worldX - objPos.x;
                double dz = worldZ - objPos.z;
                double dist = sqrt(dx * dx + dz * dz);
                
                if (dist > r_s) {
                    double deltaY = 2.0 * sqrt(r_s * (dist - r_s));
                    y += (float)deltaY - 3e10f;
                } else {
                    y += 2.0f * (float)sqrt(r_s * r_s) - 3e10f;
                }
            }
            
            vertices[vertexCount] = vec3_create(worldX, y, worldZ);
            vertexCount++;
        }
    }
    
    /* Generate indices for line rendering */
    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int i = z * (gridSize + 1) + x;
            
            /* Horizontal line */
            if (indexCount < MAX_GRID_INDICES - 1) {
                indices[indexCount++] = i;
                indices[indexCount++] = i + 1;
            }
            
            /* Vertical line */
            if (indexCount < MAX_GRID_INDICES - 1) {
                indices[indexCount++] = i;
                indices[indexCount++] = i + gridSize + 1;
            }
        }
    }
    
    /* Upload to GPU */
    if (eng->gridVAO == 0) {
        glGenVertexArrays(1, &eng->gridVAO);
        glGenBuffers(1, &eng->gridVBO);
        glGenBuffers(1, &eng->gridEBO);
    }
    
    glBindVertexArray(eng->gridVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, eng->gridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vec3), vertices, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eng->gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), indices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);
    
    eng->gridIndexCount = indexCount;
    glBindVertexArray(0);
}

void draw_grid(Engine* eng, Mat4 viewProj) {
    glUseProgram(eng->gridShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(eng->gridShaderProgram, "viewProj"),
                       1, GL_FALSE, viewProj.m);
    glBindVertexArray(eng->gridVAO);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glDrawElements(GL_LINES, eng->gridIndexCount, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void upload_camera_ubo(Engine* eng, const Camera* cam) {
    typedef struct {
        Vec3 pos; float _pad0;
        Vec3 right; float _pad1;
        Vec3 up; float _pad2;
        Vec3 forward; float _pad3;
        float tanHalfFov;
        float aspect;
        int moving;  /* using int instead of bool for alignment */
        int _pad4;
    } UBOData;
    
    UBOData data;
    Vec3 camPos = camera_position(cam);
    Vec3 fwd = vec3_normalize(vec3_subtract(cam->target, camPos));
    Vec3 up = vec3_create(0, 1, 0);
    Vec3 right = vec3_normalize(vec3_cross(fwd, up));
    up = vec3_cross(right, fwd);
    
    data.pos = camPos;
    data.right = right;
    data.up = up;
    data.forward = fwd;
    data.tanHalfFov = tan((60.0f * M_PI / 180.0f) * 0.5f);
    data.aspect = (float)eng->WIDTH / (float)eng->HEIGHT;
    data.moving = cam->dragging || cam->panning ? 1 : 0;
    
    glBindBuffer(GL_UNIFORM_BUFFER, eng->cameraUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBOData), &data);
}

void upload_objects_ubo(Engine* eng, const ObjectData* objs, int objCount) {
    typedef struct {
        int numObjects;
        float _pad0, _pad1, _pad2;
        Vec4 posRadius[16];
        Vec4 color[16];
        float mass[16];
    } UBOData;
    
    UBOData data = {0};
    int count = objCount < 16 ? objCount : 16;
    data.numObjects = count;
    
    for (int i = 0; i < count; ++i) {
        data.posRadius[i] = objs[i].posRadius;
        data.color[i] = objs[i].color;
        data.mass[i] = objs[i].mass;
    }
    
    glBindBuffer(GL_UNIFORM_BUFFER, eng->objectsUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
}

void upload_disk_ubo(Engine* eng) {
    float r1 = (float)(SagA.r_s * 2.2);
    float r2 = (float)(SagA.r_s * 5.2);
    float num = 2.0f;
    float thickness = 1e9f;
    float diskData[4] = {r1, r2, num, thickness};
    
    glBindBuffer(GL_UNIFORM_BUFFER, eng->diskUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(diskData), diskData);
}

void dispatch_compute(Engine* eng, const Camera* cam) {
    int cw = cam->moving ? eng->COMPUTE_WIDTH : 200;
    int ch = cam->moving ? eng->COMPUTE_HEIGHT : 150;
    
    glBindTexture(GL_TEXTURE_2D, eng->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cw, ch, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    
    glUseProgram(eng->computeProgram);
    upload_camera_ubo(eng, cam);
    upload_disk_ubo(eng);
    upload_objects_ubo(eng, objects, objectCount);
    
    glBindImageTexture(0, eng->texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    
    GLuint groupsX = (GLuint)ceil(cw / 16.0f);
    GLuint groupsY = (GLuint)ceil(ch / 16.0f);
    glDispatchCompute(groupsX, groupsY, 1);
    
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void draw_fullscreen_quad(Engine* eng) {
    glUseProgram(eng->shaderProgram);
    glBindVertexArray(eng->quadVAO);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, eng->texture);
    glUniform1i(glGetUniformLocation(eng->shaderProgram, "screenTexture"), 0);
    
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    glEnable(GL_DEPTH_TEST);
}

/* CALLBACK FUNCTIONS */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    camera_processMouseButton(&camera, button, action, mods, window);
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    camera_processMouseMove(&camera, xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera_processScroll(&camera, xoffset, yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    camera_processKey(&camera, key, scancode, action, mods);
}

void setup_camera_callbacks(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, &camera);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
}

/* PHYSICS UPDATE FUNCTION */
void update_gravity(ObjectData* objects, int objCount, double dt) {
    if (!Gravity) return;
    
    for (int i = 0; i < objCount; i++) {
        for (int j = 0; j < objCount; j++) {
            if (i == j) continue; /* skip self-interaction */
            
            float dx = objects[j].posRadius.x - objects[i].posRadius.x;
            float dy = objects[j].posRadius.y - objects[i].posRadius.y;
            float dz = objects[j].posRadius.z - objects[i].posRadius.z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);
            
            if (distance > 0) {
                Vec3 direction = {dx / distance, dy / distance, dz / distance};
                double Gforce = (G_const * objects[i].mass * objects[j].mass) / (distance * distance);
                double acc1 = Gforce / objects[i].mass;
                
                Vec3 acc = {
                    direction.x * acc1,
                    direction.y * acc1,
                    direction.z * acc1
                };
                
                objects[i].velocity.x += acc.x;
                objects[i].velocity.y += acc.y;
                objects[i].velocity.z += acc.z;
                
                objects[i].posRadius.x += objects[i].velocity.x;
                objects[i].posRadius.y += objects[i].velocity.y;
                objects[i].posRadius.z += objects[i].velocity.z;
                
                printf("velocity: %f, %f, %f\n", 
                       objects[i].velocity.x, objects[i].velocity.y, objects[i].velocity.z);
            }
        }
    }
}

/* INITIALIZATION FUNCTIONS */
void init_camera(Camera* cam) {
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
}

void init_objects(void) {
    /* Initialize Sagittarius A black hole */
    SagA = blackhole_create(vec3_create(0.0f, 0.0f, 0.0f), 8.54e36f);
    
    /* Initialize objects array */
    objectCount = 3;
    
    /* Object 1: Yellow star */
    objects[0].posRadius = vec4_create(4e11f, 0.0f, 0.0f, 4e10f);
    objects[0].color = vec4_create(1.0f, 1.0f, 0.0f, 1.0f);
    objects[0].mass = 1.98892e30f;
    objects[0].velocity = vec3_create(0.0f, 0.0f, 0.0f);
    
    /* Object 2: Red star */
    objects[1].posRadius = vec4_create(0.0f, 0.0f, 4e11f, 4e10f);
    objects[1].color = vec4_create(1.0f, 0.0f, 0.0f, 1.0f);
    objects[1].mass = 1.98892e30f;
    objects[1].velocity = vec3_create(0.0f, 0.0f, 0.0f);
    
    /* Object 3: Black hole */
    objects[2].posRadius = vec4_create(0.0f, 0.0f, 0.0f, (float)SagA.r_s);
    objects[2].color = vec4_create(0.0f, 0.0f, 0.0f, 1.0f);
    objects[2].mass = (float)SagA.mass;
    objects[2].velocity = vec3_create(0.0f, 0.0f, 0.0f);
}

/* MAIN FUNCTION */
int main(void) {
    /* Initialize components */
    init_camera(&camera);
    init_objects();
    engine_init(&engine);
    setup_camera_callbacks(engine.window);
    
    /* Timing variables */
    clock_t t0 = clock();
    lastPrintTime = (double)t0 / CLOCKS_PER_SEC;
    
    double lastTime = glfwGetTime();
    
    /* Main render loop */
    while (!glfwWindowShouldClose(engine.window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        double now = glfwGetTime();
        double dt = now - lastTime;
        lastTime = now;
        
        /* Update gravity physics */
        update_gravity(objects, objectCount, dt);
        
        /* Generate and draw grid */
        generate_grid(&engine, objects, objectCount);
        
        Vec3 camPos = camera_position(&camera);
        Mat4 view = mat4_lookat(camPos, camera.target, vec3_create(0, 1, 0));
        Mat4 proj = mat4_perspective(
            60.0f * M_PI / 180.0f,  /* 60 degrees in radians */
            (float)engine.COMPUTE_WIDTH / (float)engine.COMPUTE_HEIGHT,
            1e9f,
            1e14f
        );
        Mat4 viewProj = mat4_multiply(proj, view);
        
        draw_grid(&engine, viewProj);
        
        /* Run raytracer */
        glViewport(0, 0, engine.WIDTH, engine.HEIGHT);
        dispatch_compute(&engine, &camera);
        draw_fullscreen_quad(&engine);
        
        /* Present to screen */
        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }
    
    /* Cleanup */
    glfwDestroyWindow(engine.window);
    glfwTerminate();
    return 0;
}