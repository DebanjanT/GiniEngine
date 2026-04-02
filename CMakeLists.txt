cmake_minimum_required(VERSION 3.20)

# Allow older CMake policies for dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(GiniEngine VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(GINI_BUILD_EDITOR "Build the Gini Editor" ON)
option(GINI_BUILD_SAMPLES "Build sample games" ON)
option(GINI_ENABLE_STEAM "Enable Steam SDK integration" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

# Platform detection
if(WIN32)
    add_definitions(-DGINI_PLATFORM_WINDOWS)
    set(GINI_PLATFORM "Windows")
elseif(APPLE)
    add_definitions(-DGINI_PLATFORM_MACOS)
    set(GINI_PLATFORM "macOS")
elseif(UNIX)
    add_definitions(-DGINI_PLATFORM_LINUX)
    set(GINI_PLATFORM "Linux")
endif()

message(STATUS "Gini Engine v${PROJECT_VERSION}")
message(STATUS "Platform: ${GINI_PLATFORM}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# Dependencies via FetchContent
include(FetchContent)

# GLFW - Windowing
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# GLM - Math
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

# EnTT - ECS
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.12.2
)

# stb - Image loading (header-only)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9
)

# OpenAL Soft - Audio
FetchContent_Declare(
    openal
    GIT_REPOSITORY https://github.com/kcat/openal-soft.git
    GIT_TAG 1.23.1
)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
add_definitions(-Dfdopen=fdopen)
FetchContent_MakeAvailable(assimp)
# Fetch all dependencies
FetchContent_MakeAvailable(glfw glm entt stb imgui openal assimp)

# GLAD - OpenGL loader (we'll include it directly)
add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glad/src/gl.c
)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glad/include)

# stb implementation library
add_library(stb_impl STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/stb/stb_impl.cpp
)
target_include_directories(stb_impl PUBLIC ${stb_SOURCE_DIR})

# ImGui library
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw glad)

# Engine library
add_subdirectory(Engine)

# Editor
if(GINI_BUILD_EDITOR)
    add_subdirectory(Editor)
endif()

# Sample games
if(GINI_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()
