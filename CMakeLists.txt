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

function(setup_ffmpeg_static)
    set(FFMPEG_DIR "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg")

    if(WIN32)
        # Use BtbN's static builds for Windows
        set(FFMPEG_URL "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip")
        set(FFMPEG_ARCHIVE "ffmpeg-win64-static.zip")

    elseif(APPLE)
        # For macOS, we'll need to build static or find static builds
        # Using a static build source (you may need to adjust this URL)
        set(FFMPEG_URL "https://github.com/eugeneware/ffmpeg-static/releases/download/b4.4.0/darwin-x64")
        set(FFMPEG_ARCHIVE "ffmpeg-macos-static")

    else()
        # Linux - we'll download static builds or use system static libs
        set(FFMPEG_URL "https://github.com/eugeneware/ffmpeg-static/releases/download/b4.4.0/linux-x64")
        set(FFMPEG_ARCHIVE "ffmpeg-linux-static")
    endif()

    # Download and extract if not already present
    if(NOT EXISTS "${FFMPEG_DIR}")
        message(STATUS "Downloading static FFmpeg for ${CMAKE_SYSTEM_NAME}...")

        if(WIN32)
            # Windows: Download ZIP archive
            file(DOWNLOAD
                    "${FFMPEG_URL}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${FFMPEG_ARCHIVE}"
                    SHOW_PROGRESS
                    TIMEOUT 300
            )

            message(STATUS "Extracting FFmpeg...")
            execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xf "${FFMPEG_ARCHIVE}"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                    RESULT_VARIABLE EXTRACT_RESULT
            )

            if(NOT EXTRACT_RESULT EQUAL 0)
                message(FATAL_ERROR "Failed to extract FFmpeg archive")
            endif()

            # Find the extracted directory and rename it to ffmpeg
            file(GLOB EXTRACTED_DIRS "${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-*")
            if(EXTRACTED_DIRS)
                list(GET EXTRACTED_DIRS 0 EXTRACTED_DIR)
                message(STATUS "Found extracted directory: ${EXTRACTED_DIR}")
                file(RENAME "${EXTRACTED_DIR}" "${FFMPEG_DIR}")
            else()
                message(FATAL_ERROR "Could not find extracted FFmpeg directory")
            endif()

            # Clean up archive
            file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/${FFMPEG_ARCHIVE}")

        else()
            # Linux/macOS: Direct binary download (for executables)
            # For libraries, we'll use a different approach
            message(STATUS "Setting up static FFmpeg for ${CMAKE_SYSTEM_NAME}...")
            file(MAKE_DIRECTORY "${FFMPEG_DIR}")

            # We'll need to handle this differently for static libraries
            # Let's use system static packages instead
            find_package(PkgConfig REQUIRED)
            pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
                    libavcodec
                    libavformat
                    libavutil
                    libswscale
            )

            # Try to find static versions
            set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

            find_library(AVCODEC_STATIC_LIB avcodec REQUIRED)
            find_library(AVFORMAT_STATIC_LIB avformat REQUIRED)
            find_library(AVUTIL_STATIC_LIB avutil REQUIRED)
            find_library(SWSCALE_STATIC_LIB swscale REQUIRED)

            # Create static imported targets
            add_library(ffmpeg::avcodec STATIC IMPORTED)
            add_library(ffmpeg::avformat STATIC IMPORTED)
            add_library(ffmpeg::avutil STATIC IMPORTED)
            add_library(ffmpeg::swscale STATIC IMPORTED)

            set_target_properties(ffmpeg::avcodec PROPERTIES
                    IMPORTED_LOCATION "${AVCODEC_STATIC_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            )

            set_target_properties(ffmpeg::avformat PROPERTIES
                    IMPORTED_LOCATION "${AVFORMAT_STATIC_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            )

            set_target_properties(ffmpeg::avutil PROPERTIES
                    IMPORTED_LOCATION "${AVUTIL_STATIC_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            )

            set_target_properties(ffmpeg::swscale PROPERTIES
                    IMPORTED_LOCATION "${SWSCALE_STATIC_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            )

            # Create convenience target
            add_library(ffmpeg INTERFACE)
            target_link_libraries(ffmpeg INTERFACE
                    ffmpeg::avcodec
                    ffmpeg::avformat
                    ffmpeg::avutil
                    ffmpeg::swscale
                    # Add system dependencies for static linking
                    ${FFMPEG_STATIC_LIBRARIES}
                    -static-libgcc
                    -static-libstdc++
            )

            return()
        endif()
    endif()

    # Windows static library setup
    if(WIN32)
        # Debug: List contents of the ffmpeg directory
        message(STATUS "Contents of ${FFMPEG_DIR}:")
        file(GLOB_RECURSE FFMPEG_CONTENTS "${FFMPEG_DIR}/*")
        foreach(item ${FFMPEG_CONTENTS})
            message(STATUS "  ${item}")
        endforeach()

        # Look for headers in common locations
        set(POSSIBLE_INCLUDE_DIRS
                "${FFMPEG_DIR}/include"
                "${FFMPEG_DIR}/include/ffmpeg"
                "${FFMPEG_DIR}/dev/include"
        )

        set(FFMPEG_INCLUDE_DIR "")
        foreach(dir ${POSSIBLE_INCLUDE_DIRS})
            if(EXISTS "${dir}/libavcodec/avcodec.h")
                set(FFMPEG_INCLUDE_DIR "${dir}")
                break()
            endif()
        endforeach()

        if(NOT FFMPEG_INCLUDE_DIR)
            message(FATAL_ERROR "FFmpeg headers not found. Searched in: ${POSSIBLE_INCLUDE_DIRS}")
        endif()

        message(STATUS "Found FFmpeg headers in: ${FFMPEG_INCLUDE_DIR}")

        # Look for libraries in common locations
        set(POSSIBLE_LIB_DIRS
                "${FFMPEG_DIR}/lib"
                "${FFMPEG_DIR}/lib/x64"
                "${FFMPEG_DIR}/dev/lib"
        )

        set(FFMPEG_LIB_DIR "")
        foreach(dir ${POSSIBLE_LIB_DIRS})
            if(EXISTS "${dir}/avcodec.lib")
                set(FFMPEG_LIB_DIR "${dir}")
                break()
            endif()
        endforeach()

        if(NOT FFMPEG_LIB_DIR)
            message(FATAL_ERROR "FFmpeg libraries not found. Searched in: ${POSSIBLE_LIB_DIRS}")
        endif()

        message(STATUS "Found FFmpeg libraries in: ${FFMPEG_LIB_DIR}")

        # Set up static imported targets for Windows
        set(FFMPEG_LIBS avcodec avformat avutil swscale)

        foreach(lib IN LISTS FFMPEG_LIBS)
            add_library(ffmpeg::${lib} STATIC IMPORTED)

            set_target_properties(ffmpeg::${lib} PROPERTIES
                    IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/${lib}.lib"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
            )

            # Verify static library exists
            if(NOT EXISTS "${FFMPEG_LIB_DIR}/${lib}.lib")
                message(FATAL_ERROR "FFmpeg static library not found: ${FFMPEG_LIB_DIR}/${lib}.lib")
            endif()
        endforeach()

        # Create convenience target with Windows dependencies
        add_library(ffmpeg INTERFACE)
        target_link_libraries(ffmpeg INTERFACE
                ffmpeg::avcodec
                ffmpeg::avformat
                ffmpeg::avutil
                ffmpeg::swscale
                # Windows system libraries needed for static FFmpeg
                ws2_32      # Winsock
                secur32     # Security
                bcrypt      # Cryptography
                mfplat      # Media Foundation
                mfuuid      # Media Foundation UUIDs
                strmiids    # DirectShow
                ole32       # OLE
                user32      # User interface
                kernel32    # Kernel
                psapi       # Process API
                advapi32    # Advanced API
        )

        # Force static linking
        set_target_properties(ffmpeg PROPERTIES
                INTERFACE_LINK_OPTIONS "/WHOLEARCHIVE"
        )
    endif()

    message(STATUS "Static FFmpeg setup complete: ${FFMPEG_DIR}")
endfunction()


# Force static linking for the entire project
if(WIN32)
    # Use static runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # Static linking flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

elseif(UNIX)
    # Linux/macOS static linking
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

    # For completely static binary (optional, can cause issues)
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

setup_ffmpeg_static()

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

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        imgui
        glfw
        ${CMAKE_DL_LIBS}
        OpenGL::GL
        avcodec
        avformat
        avutil
        swscale
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_SEARCH_START_STATIC ON
        LINK_SEARCH_END_STATIC ON
)