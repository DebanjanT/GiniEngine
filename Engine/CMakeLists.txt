cmake_minimum_required(VERSION 3.20)

# Gini Engine Library
set(ENGINE_SOURCES
    # Core
    Core/Application.cpp
    Core/Logger.cpp
    Core/Timer.cpp
    Core/Input.cpp
    Core/Threading.cpp
    Core/ThreadProfiler.cpp
    Core/LoadingIndicator.cpp

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
    Renderer/Camera3D.cpp
    Renderer/Light.cpp
    Renderer/Renderer3D.cpp
    Renderer/Skybox.cpp
    Renderer/AtmosphericSky.cpp

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

    # ImGui extensions
    ImGui/ImGuizmo.cpp

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

    # Project
    Project/Project.cpp

    # Asset
    Asset/AssetRegistry.cpp
    Asset/AssimpMeshImporter.cpp

    # Material
    Renderer/Material.cpp
    Renderer/MaterialAsset.cpp

    # MeshSource (Hazel-style mesh system)
    Renderer/MeshSource.cpp

    # Post-processing
    Renderer/PostProcess.cpp

    # Shadow Mapping
    Renderer/ShadowMap.cpp

    # IBL
    Renderer/IBL.cpp

    # SSAO
    Renderer/SSAO.cpp

)

set(ENGINE_HEADERS
    # Main include
    Gini.h

    # Core
    Core/Application.h
    Core/Logger.h
    Core/Timer.h
    Core/Input.h
    Core/Threading.h
    Core/ThreadProfiler.h
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
    Renderer/Camera3D.h
    Renderer/Light.h
    Renderer/Renderer3D.h
    Renderer/MeshSource.h
    Renderer/MeshNode.h
    Renderer/Submesh.h
    Renderer/MaterialAsset.h

    # Asset
    Asset/AssetRegistry.h
    Asset/AssimpMeshImporter.h

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
# add Vulkan library
find_package(Vulkan REQUIRED)


target_include_directories(GiniEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
    $<TARGET_PROPERTY:EnTT::EnTT,INTERFACE_INCLUDE_DIRECTORIES>


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
    # Link Vulkan
    Vulkan::Vulkan
)

# Precompiled header for faster builds (temporarily disabled due to compilation issues)
# target_precompile_headers(GiniEngine PRIVATE
#     <vector>
#     <string>
#     <memory>
#     <unordered_map>
#     <functional>
#     <algorithm>
#     <cmath>
#     <glm/glm.hpp>
#     <entt/entt.hpp>
# )

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
