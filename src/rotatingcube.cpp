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

static const char* vertex_shader_src = R"(
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

static const char* fragment_shader_src = R"(
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
static Uint32 last_ticks = 0;
static bool dragging = false;
static int last_mouse_x = 0;
static int last_mouse_y = 0;

static glm::quat rotation = {1.0f, 0, 0, 0};

static float cube_vertices[] = {
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

static void check_shader(unsigned int shader, const char* name) {
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success)
        return;

    char info_log[1024] = {};
    glGetShaderInfoLog(shader, 1024, nullptr, info_log);
    SDL_Log("Shader compilation error (%s): %s", name, info_log);
}

static glm::mat4 model_matrix() {
    return glm::mat4_cast(rotation);
}

static glm::mat4 view_matrix() {
    return glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -3.0f});
}

static glm::mat4 projection_matrix(float width, float height) {
    return glm::perspective(glm::radians(45.0f),
                                            width / height,
                                            0.1f, 100.0f);
}

void rotatingcube_init() {
    glEnable(GL_DEPTH_TEST);

    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

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

    if (!paused) {
        float angular_speed = glm::radians(60.0f) * delta_time;
        glm::quat spin = glm::angleAxis(angular_speed, glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::normalize(spin * rotation);
    }

    glm::mat4 model = model_matrix();
    glm::mat4 view = view_matrix();
    glm::mat4 projection = projection_matrix(width, height);

    glm::mat4 mvp = projection * view * model;

    glUseProgram(shader_program);

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "MVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "Model"), 1, GL_FALSE, glm::value_ptr(model));

    glUniform3f(glGetUniformLocation(shader_program, "lightPos"), 2, 2, 2);
    glUniform3f(glGetUniformLocation(shader_program, "viewPos"), 0, 0, 3);
    glUniform3f(glGetUniformLocation(shader_program, "objectColor"), 1, 0, 0);
    glUniform3f(glGetUniformLocation(shader_program, "lightColor"), 1, 1, 1);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, sizeof(cube_vertices) / sizeof(float) / 6);
    glBindVertexArray(0);
}

void rotatingcube_cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader_program);
}

void rotatingcube_handle_event(const SDL_Event *event, float scale) {
    if (!paused)
        return;

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        dragging = true;
        last_mouse_x = event->button.x * scale;
        last_mouse_y = event->button.y * scale;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT)
        dragging = false;

    if (event->type == SDL_EVENT_MOUSE_MOTION && dragging) {
        float dx = event->motion.x * scale - last_mouse_x;
        float dy = event->motion.y * scale - last_mouse_y;

        last_mouse_x = event->motion.x * scale;
        last_mouse_y = event->motion.y * scale;

        float sensitivity = 0.005f;

        glm::quat qx = glm::angleAxis(dy * sensitivity, glm::vec3(1, 0, 0));
        glm::quat qy = glm::angleAxis(dx * sensitivity, glm::vec3(0, 1, 0));

        rotation = glm::normalize(qy * qx * rotation);
    }
}

void rotatingcube_pause(bool pause) {
    paused = pause;
}

bool rotatingcube_is_paused() {
    return paused;
}