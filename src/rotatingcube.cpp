#include "rotatingcube.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static unsigned int VAO = 0;
static unsigned int VBO = 0;
static unsigned int shader_program = 0;

static const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 MVP;
uniform mat4 Model;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(Model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(Model))) * aNormal;
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    // Ambient
    vec3 ambient = 0.2 * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Blinn)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfway), 0.0), 32);
    vec3 specular = 0.5 * spec * lightColor;

    vec3 color = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(color, 1.0);
}
)";

static bool paused = false;
static float angle = 0.0f;
static Uint32 last_ticks = 0;

static float cubeVertices[] = {
    // front
    -0.5,-0.5, 0.5,  0,0,1,  0.5,-0.5, 0.5,  0,0,1,  0.5,0.5,0.5, 0,0,1,
     0.5,0.5,0.5,  0,0,1, -0.5,0.5,0.5,  0,0,1, -0.5,-0.5,0.5, 0,0,1,
    // back
    -0.5,-0.5,-0.5, 0,0,-1, -0.5,0.5,-0.5, 0,0,-1,  0.5,0.5,-0.5, 0,0,-1,
     0.5,0.5,-0.5, 0,0,-1,  0.5,-0.5,-0.5, 0,0,-1, -0.5,-0.5,-0.5,0,0,-1,
    // left
    -0.5,0.5,0.5, -1,0,0, -0.5,0.5,-0.5,-1,0,0, -0.5,-0.5,-0.5,-1,0,0,
    -0.5,-0.5,-0.5,-1,0,0, -0.5,-0.5,0.5,-1,0,0, -0.5,0.5,0.5,-1,0,0,
    // right
     0.5,0.5,0.5, 1,0,0,  0.5,-0.5,-0.5,1,0,0,  0.5,0.5,-0.5,1,0,0,
     0.5,-0.5,-0.5,1,0,0,  0.5,0.5,0.5, 1,0,0,  0.5,-0.5,0.5,1,0,0,
    // top
    -0.5,0.5,-0.5,0,1,0, -0.5,0.5,0.5,0,1,0, 0.5,0.5,0.5,0,1,0,
     0.5,0.5,0.5,0,1,0,  0.5,0.5,-0.5,0,1,0, -0.5,0.5,-0.5,0,1,0,
    // bottom
    -0.5,-0.5,-0.5,0,-1,0, 0.5,-0.5,0.5,0,-1,0, -0.5,-0.5,0.5,0,-1,0,
     0.5,-0.5,0.5,0,-1,0, -0.5,-0.5,-0.5,0,-1,0, 0.5,-0.5,-0.5,0,-1,0
};


static unsigned int compile_shader(unsigned int type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

void check_shader(unsigned int shader, const char* name) {
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success)
        return;

    char infoLog[1024];
    glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
    SDL_Log("Shader compilation error (%s): %s", name, infoLog);
}

void rotatingcube_init() {
    glEnable(GL_DEPTH_TEST);

    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vs);
    glAttachShader(shader_program, fs);
    glLinkProgram(shader_program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void rotatingcube_draw(float width, float height) {

    Uint32 current = SDL_GetTicks();

    if (last_ticks == 0)
        last_ticks = current;

    float delta_time = (current - last_ticks) / 1000.0f;
    last_ticks = current;

    if (!paused)
        angle += delta_time * glm::radians(60.0f);

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle, {0.5f, 1.0f, 0.0f});
    glm::mat4 view = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -3.0f});
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                            width / height,
                                            0.1f, 100.0f);

    glm::mat4 mvp = projection * view * model;

    glUseProgram(shader_program);

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "Model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform3f(glGetUniformLocation(shader_program, "lightPos"), 2, 2, 2);
    glUniform3f(glGetUniformLocation(shader_program, "viewPos"), 0, 0, 3);
    glUniform3f(glGetUniformLocation(shader_program, "objectColor"), 1, 0, 0);
    glUniform3f(glGetUniformLocation(shader_program, "lightColor"), 1, 1, 1);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(cubeVertices) / sizeof(float) / 6);
    glBindVertexArray(0);
}

void rotatingcube_cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader_program);
}

void rotatingcube_pause(bool pause) {
    paused = pause;
}

bool rotatingcube_is_paused() {
    return paused;
}