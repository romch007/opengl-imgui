#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

#include "rotatingcube.h"

static SDL_Window *window = nullptr;
static bool running = false;
static bool vsync_enabled = true;

void print_gl_infos() {
    const GLubyte *vendor     = glGetString(GL_VENDOR);
    const GLubyte *renderer   = glGetString(GL_RENDERER);
    const GLubyte *version    = glGetString(GL_VERSION);
    const GLubyte *glsl_ver   = glGetString(GL_SHADING_LANGUAGE_VERSION);

    SDL_Log("Vendor: %s", vendor);
    SDL_Log("Renderer: %s", renderer);
    SDL_Log("Version: %s", version);
    SDL_Log("GLSL: %s", glsl_ver);
}

void draw(int output_width, int output_height) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Controls");

    const bool paused = rotatingcube_is_paused();
    if (ImGui::Button(paused ? "Resume" : "Pause"))
        rotatingcube_pause(!paused);

    if (ImGui::Checkbox("VSync", &vsync_enabled)) {
        if (vsync_enabled)
            SDL_GL_SetSwapInterval(1);
        else
            SDL_GL_SetSwapInterval(0);
    }

    ImGui::End();

    static Uint32 last_time = SDL_GetTicks();
    static int frame_count = 0;
    static float fps = 0.0f;

    frame_count++;
    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_time >= 1000) {
        fps = frame_count * 1000.0f / (current_time - last_time);
        frame_count = 0;
        last_time = current_time;
    }

    ImGui::SetNextWindowPos(ImVec2(5, 5));
    ImGui::Begin("FPS", nullptr,
              ImGuiWindowFlags_NoTitleBar |
              ImGuiWindowFlags_NoResize |
              ImGuiWindowFlags_AlwaysAutoResize |
              ImGuiWindowFlags_NoMove |
              ImGuiWindowFlags_NoBackground);
    ImGui::Text("FPS: %1.f", fps);
    ImGui::End();

    ImGui::Render();

    glViewport(0, 0, output_width, output_height);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rotatingcube_draw(output_width, output_height);

    glDisable(GL_DEPTH_TEST);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#ifdef WIN32
#include <Windows.h>
int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
#endif
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("could not initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    window = SDL_CreateWindow("OpenGL + ImGui", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!window) {
        SDL_Log("could not create window/renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);

    print_gl_infos();

    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    rotatingcube_init();

    running = true;

    SDL_Event event;
    while (running) {
        int output_width, output_height;
        SDL_GetWindowSizeInPixels(window, &output_width, &output_height);

        float scale = SDL_GetWindowDisplayScale(window);

        float mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
                running = false;
            else if (!io.WantCaptureMouse)
                rotatingcube_handle_event(&event, scale);
        }

        draw(output_width, output_height);
        SDL_GL_SwapWindow(window);
    }

    rotatingcube_cleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
