#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

#include "rotatingcube.h"

struct AppState {
    SDL_Window *window;
    SDL_GLContext gl_context;
    bool vsync_enabled;
};

void print_gl_infos() {
    SDL_Log("OpenGL vendor:    %s", glGetString(GL_VENDOR));
    SDL_Log("OpenGL renderer:  %s", glGetString(GL_RENDERER));
    SDL_Log("OpenGL version:   %s", glGetString(GL_VERSION));
    SDL_Log("GLSL version:     %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void draw(AppState *app, int output_width, int output_height) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Controls");

    const bool paused = rotatingcube_is_paused();
    if (ImGui::Button(paused ? "Resume" : "Pause"))
        rotatingcube_pause(!paused);

    if (ImGui::Checkbox("VSync", &app->vsync_enabled)) {
        SDL_GL_SetSwapInterval(app->vsync_enabled ? 1 : 0);
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

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("could not initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    AppState *app = new AppState();
    app->vsync_enabled = true;

    app->window = SDL_CreateWindow("OpenGL + ImGui", 1280, 720,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!app->window) {
        SDL_Log("could not create window: %s", SDL_GetError());
        delete app;
        return SDL_APP_FAILURE;
    }

    app->gl_context = SDL_GL_CreateContext(app->window);
    gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress);

    print_gl_infos();

    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ChildRounding     = 4.0f;
    style.ScrollbarRounding = 8.0f;
    style.TabRounding       = 4.0f;

    ImGui_ImplSDL3_InitForOpenGL(app->window, app->gl_context);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    rotatingcube_init();

    *appstate = app;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *app = (AppState *) appstate;
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDL3_ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    if (!io.WantCaptureMouse) {
        float scale = SDL_GetWindowDisplayScale(app->window);
        rotatingcube_handle_event(event, scale);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *app = (AppState *) appstate;

    int output_width, output_height;
    SDL_GetWindowSizeInPixels(app->window, &output_width, &output_height);

    draw(app, output_width, output_height);
    SDL_GL_SwapWindow(app->window);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppState *app = (AppState *) appstate;

    rotatingcube_cleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(app->gl_context);
    SDL_DestroyWindow(app->window);
    delete app;

    SDL_Quit();
}
