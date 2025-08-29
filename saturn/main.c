#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Incluir stb_image para cargar texturas
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Estructura para vértices
typedef struct {
    float x, y, z;    // Posición
    float u, v;       // Coordenadas de textura
    float nx, ny, nz; // Normales
} Vertex;

// Variables globales
GLuint VAO, VBO, EBO;
GLuint shaderProgram;
GLuint texture;
Vertex* vertices;
unsigned int* indices;
int vertexCount, indexCount;

// Código del vertex shader
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

// Código del fragment shader
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
    "    // Ambient lighting\n"
    "    float ambientStrength = 0.3;\n"
    "    vec3 ambient = ambientStrength * lightColor;\n"
    "    // Diffuse lighting\n"
    "    vec3 norm = normalize(Normal);\n"
    "    vec3 lightDir = normalize(lightPos - FragPos);\n"
    "    float diff = max(dot(norm, lightDir), 0.0);\n"
    "    vec3 diffuse = diff * lightColor;\n"
    "    // Specular lighting\n"
    "    float specularStrength = 0.5;\n"
    "    vec3 viewDir = normalize(viewPos - FragPos);\n"
    "    vec3 reflectDir = reflect(-lightDir, norm);\n"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
    "    vec3 specular = specularStrength * spec * lightColor;\n"
    "    vec3 result = (ambient + diffuse + specular) * texture(ourTexture, TexCoord).rgb;\n"
    "    FragColor = vec4(result, 1.0);\n"
    "}\n\0";

// Función para crear matriz de identidad 4x4
void mat4_identity(float* m) {
    for (int i = 0; i < 16; i++) {
        m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

// Función para multiplicar matrices 4x4
void mat4_multiply(float* result, const float* a, const float* b) {
    float temp[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                temp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    for (int i = 0; i < 16; i++) {
        result[i] = temp[i];
    }
}

// Función para crear matriz de perspectiva
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

// Función para crear matriz lookat
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

// Función para crear matriz de rotación
void mat4_rotate_y(float* m, float angle) {
    mat4_identity(m);
    float c = cosf(angle);
    float s = sinf(angle);
    m[0] = c; m[8] = s;
    m[2] = -s; m[10] = c;
}

// Generar esfera
void generateSphere(float radius, int sectors, int stacks) {
    vertexCount = (sectors + 1) * (stacks + 1);
    indexCount = sectors * stacks * 6;
    
    vertices = malloc(vertexCount * sizeof(Vertex));
    indices = malloc(indexCount * sizeof(unsigned int));
    
    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    
    // Generar vértices
    int vertIndex = 0;
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = M_PI / 2 - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * sectorStep;
            
            // Posición
            vertices[vertIndex].x = xy * cosf(sectorAngle);
            vertices[vertIndex].y = xy * sinf(sectorAngle);
            vertices[vertIndex].z = z;
            
            // Normales
            vertices[vertIndex].nx = vertices[vertIndex].x / radius;
            vertices[vertIndex].ny = vertices[vertIndex].y / radius;
            vertices[vertIndex].nz = vertices[vertIndex].z / radius;
            
            // Coordenadas de textura
            vertices[vertIndex].u = (float)j / sectors;
            vertices[vertIndex].v = (float)i / stacks;
            
            vertIndex++;
        }
    }
    
    // Generar índices
    int indexIndex = 0;
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices[indexIndex++] = k1;
                indices[indexIndex++] = k2;
                indices[indexIndex++] = k1 + 1;
            }
            
            if (i != (stacks - 1)) {
                indices[indexIndex++] = k1 + 1;
                indices[indexIndex++] = k2;
                indices[indexIndex++] = k2 + 1;
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

int main() {
    // Inicializar GLFW
    if (!glfwInit()) {
        printf("Error inicializando GLFW\n");
        return -1;
    }
    
    // Configurar GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Crear ventana
    GLFWwindow* window = glfwCreateWindow(800, 600, "Esfera de Urano", NULL, NULL);
    if (window == NULL) {
        printf("Error creando ventana\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Inicializar GLEW
    if (glewInit() != GLEW_OK) {
        printf("Error inicializando GLEW\n");
        return -1;
    }
    
    // Configurar viewport
    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);
    
    // Compilar shaders
    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    // Crear programa de shader
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Generar esfera
    generateSphere(0.5f, 50, 50);
    
    // Crear VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    
    // Configurar atributos de vértice
    // Posición
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // Coordenadas de textura
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Normales
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Cargar textura
    texture = loadTexture("2k_uranus.jpg");
    
    // Loop principal
    float angle = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // Input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);
        
        // Render
        glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        // Matrices de transformación
        float model[16], view[16], projection[16];
        
        mat4_rotate_y(model, angle);
        mat4_lookat(view, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        mat4_perspective(projection, M_PI/4.0f, 800.0f/600.0f, 0.1f, 100.0f);
        
        // Enviar matrices a los shaders
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection);
        
        // Configurar iluminación
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 0.0f, 0.0f, 3.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        
        // Activar textura
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
        
        // Dibujar esfera
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        
        // Incrementar ángulo para rotación
        angle += 0.01f;
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Limpiar recursos
    free(vertices);
    free(indices);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &texture);
    
    glfwTerminate();
    return 0;
}