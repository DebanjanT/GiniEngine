cmake_minimum_required(VERSION 3.20)

# Gini Engine Library
set(ENGINE_SOURCES
    # Core
    Core/Application.cpp
    Core/Logger.cpp
    Core/Timer.cpp
    Core/Input.cpp

    # Window
    Window/Window.cpp

    # Renderer 2D
    Renderer/Renderer.cpp
    Renderer/Shader.cpp
    Renderer/Texture.cpp
    Renderer/SpriteBatch.cpp
    Renderer/Camera.cpp
    Renderer/Framebuffer.cpp

    # Renderer 3D
    Renderer/Mesh.cpp
    Renderer/Model.cpp
    Renderer/Camera3D.cpp
    Renderer/Light.cpp
    Renderer/Renderer3D.cpp
    Renderer/Skybox.cpp

    # ECS
    ECS/World.cpp

    # Audio
    Audio/AudioSystem.cpp

    # RTS
    RTS/TileMap.cpp
    RTS/Pathfinding.cpp
    RTS/UnitSelection.cpp

    # UI
    UI/ImGuiLayer.cpp

    # Assets
    Assets/AssetManager.cpp

    # Animation
    Animation/Animation.cpp

    # Particles
    Particles/ParticleSystem.cpp

    # Network
    Network/Network.cpp

    # Platform
    Platform/Steam.cpp

    # Scene
    Scene/Scene.cpp
    Scene/SceneSerializer.cpp

    # Terrain
    Terrain/Terrain.cpp
    Terrain/Vegetation.cpp
)

set(ENGINE_HEADERS
    # Main include
    Gini.h

    # Core
    Core/Application.h
    Core/Logger.h
    Core/Timer.h
    Core/Input.h
    Core/Types.h
    Core/Event.h

    # Window
    Window/Window.h

    # Renderer 2D
    Renderer/Renderer.h
    Renderer/Shader.h
    Renderer/Texture.h
    Renderer/SpriteBatch.h
    Renderer/Camera.h
    Renderer/Framebuffer.h

    # Renderer 3D
    Renderer/Mesh.h
    Renderer/Model.h
    Renderer/Camera3D.h
    Renderer/Light.h
    Renderer/Renderer3D.h

    # ECS
    ECS/World.h
    ECS/Components.h

    # Audio
    Audio/AudioSystem.h

    # RTS
    RTS/TileMap.h
    RTS/Pathfinding.h
    RTS/UnitSelection.h

    # UI
    UI/ImGuiLayer.h

    # Assets
    Assets/AssetManager.h

    # Animation
    Animation/Animation.h

    # Particles
    Particles/ParticleSystem.h

    # Network
    Network/Network.h

    # Platform
    Platform/Steam.h

    # Scene
    Scene/Scene.h
)

add_library(GiniEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(GiniEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(GiniEngine PUBLIC
    glfw
    glad
    glm::glm
    EnTT::EnTT
    stb_impl
    imgui_lib
    OpenAL::OpenAL
    assimp::assimp
    yaml-cpp::yaml-cpp
)

# Precompiled header for faster builds
target_precompile_headers(GiniEngine PRIVATE
    <vector>
    <string>
    <memory>
    <unordered_map>
    <functional>
    <algorithm>
    <cmath>
    <glm/glm.hpp>
    <entt/entt.hpp>
)

# Set properties
set_target_properties(GiniEngine PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Platform-specific libraries
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(GiniEngine PUBLIC
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
elseif(WIN32)
    target_link_libraries(GiniEngine PUBLIC
        opengl32
    )
endif()
