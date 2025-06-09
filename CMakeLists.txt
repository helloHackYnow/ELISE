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

function(setup_ffmpeg_simple)
    if(WIN32)
        set(FFMPEG_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")

        # Download FFmpeg if not present
        if(NOT EXISTS "${FFMPEG_DIR}")
            message(STATUS "Downloading FFmpeg...")
            set(FFMPEG_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip")
            set(FFMPEG_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg.zip")

            file(DOWNLOAD "${FFMPEG_URL}" "${FFMPEG_ARCHIVE}" SHOW_PROGRESS)
            execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${FFMPEG_ARCHIVE}"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

            # Find extracted directory and rename
            file(GLOB EXTRACTED_DIRS "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-*")
            if(EXTRACTED_DIRS)
                list(GET EXTRACTED_DIRS 0 EXTRACTED_DIR)
                file(RENAME "${EXTRACTED_DIR}" "${FFMPEG_DIR}")
            endif()

            file(REMOVE "${FFMPEG_ARCHIVE}")
        endif()

        # Find include and lib directories
        file(GLOB_RECURSE AVCODEC_HEADER "${FFMPEG_DIR}/*/libavcodec/avcodec.h")
        if(AVCODEC_HEADER)
            get_filename_component(FFMPEG_INCLUDE_DIR "${AVCODEC_HEADER}" DIRECTORY)
            get_filename_component(FFMPEG_INCLUDE_DIR "${FFMPEG_INCLUDE_DIR}" DIRECTORY)
            message(STATUS "Found FFmpeg headers: ${FFMPEG_INCLUDE_DIR}")
        else()
            message(FATAL_ERROR "Could not find FFmpeg headers")
        endif()

        file(GLOB_RECURSE AVCODEC_LIB "${FFMPEG_DIR}/*/avcodec.lib")
        if(AVCODEC_LIB)
            get_filename_component(FFMPEG_LIB_DIR "${AVCODEC_LIB}" DIRECTORY)
            message(STATUS "Found FFmpeg libraries: ${FFMPEG_LIB_DIR}")
        else()
            message(FATAL_ERROR "Could not find FFmpeg libraries")
        endif()

        # Set global variables that can be used later
        set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INCLUDE_DIR}" PARENT_SCOPE)
        set(FFMPEG_LIBRARIES
                "${FFMPEG_LIB_DIR}/avcodec.lib"
                "${FFMPEG_LIB_DIR}/avformat.lib"
                "${FFMPEG_LIB_DIR}/avutil.lib"
                "${FFMPEG_LIB_DIR}/swscale.lib"
                # Windows system libraries
                ws2_32 secur32 bcrypt mfplat mfuuid strmiids ole32 user32 kernel32 psapi advapi32
                PARENT_SCOPE)

    else()
        # For Linux/macOS, use pkg-config
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)

        set(FFMPEG_INCLUDE_DIRS "${FFMPEG_INCLUDE_DIRS}" PARENT_SCOPE)
        set(FFMPEG_LIBRARIES "${FFMPEG_LIBRARIES}" PARENT_SCOPE)
    endif()
endfunction()


# Force static linking for the entire project
if(WIN32)
    # Use static runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    # Static linking flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

elseif(UNIX)
    # Linux/macOS static linking
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

    # For completely static binary (optional, can cause issues)
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

setup_ffmpeg_simple()



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
        src/2D\ renderer/Texture.cpp
        src/2D\ renderer/Framebuffer.cpp
        src/2D\ renderer/Shader/Shader.cpp
        src/2D\ renderer/Shader/Graphics.cpp
        src/2D\ renderer/PostProcessing/Effects/Effect.cpp
        src/2D\ renderer/PostProcessing/Effects/Bloom/Bloom.cpp
        src/2D\ renderer/Renderer.cpp
        src/VideoEncoder.cpp
        src/VideoEncoder.h
)

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        imgui
        glfw
        ${CMAKE_DL_LIBS}
        OpenGL::GL
        ${FFMPEG_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_SEARCH_START_STATIC ON
        LINK_SEARCH_END_STATIC ON
)