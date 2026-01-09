cmake_minimum_required(VERSION 4.0)
project(opengl-imgui C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
find_package(glm REQUIRED)

set(IMGUI_SOURCES
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_tables.cpp
        imgui/backends/imgui_impl_sdl3.cpp
        imgui/backends/imgui_impl_sdl3.h
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/backends/imgui_impl_opengl3.h
)

include_directories(imgui)
include_directories(imgui/backends)

add_executable(opengl-imgui src/main.cpp src/glad.c src/rotatingcube.h src/rotatingcube.cpp include/glad/glad.h include/KHR/khrplatform.h ${IMGUI_SOURCES})
target_link_libraries(opengl-imgui PRIVATE SDL3::SDL3 glm::glm)
