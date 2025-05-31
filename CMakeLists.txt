cmake_minimum_required(VERSION 3.16)
project(ELISE VERSION 0.1 LANGUAGES C CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add GLFW submodule
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)   # Builds libglfw and exports target glfw

# Add ImGui submodule (core + GLFW backend)
add_library(imgui
imgui/imgui.cpp
imgui/imgui_draw.cpp
imgui/imgui_widgets.cpp
imgui/imgui_tables.cpp
imgui/imgui_demo.cpp
imgui/backends/imgui_impl_glfw.cpp
imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
imgui
imgui/backends
)

target_link_libraries(imgui PUBLIC glfw)

add_executable(${PROJECT_NAME} src/main.cpp
        src/WaveformViewer.cpp
        src/WaveformViewer.h
        src/EliseApp.cpp
        src/EliseApp.h
        src/AudioManager.cpp
        src/AudioManager.h
        src/AudioUtils.cpp
        src/AudioUtils.h
        libs/glad/src/glad.c
        src/Viewport.cpp
        src/Viewport.h
        src/shaders.h
        src/InterpolationUtils.h
        src/LightManager.h
        src/LightManager.cpp
        src/ImGui_themes.h
        src/JsonHandler.cpp
        src/JsonHandler.h
        src/Exporter.cpp
        src/Exporter.h
        libs/portable_file_dialog.h
        src/file_utils.h
        libs/stb_image.h
)

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        imgui
        glfw
        ${CMAKE_DL_LIBS}
        OpenGL::GL
)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        ${CMAKE_BINARY_DIR}/resources
)