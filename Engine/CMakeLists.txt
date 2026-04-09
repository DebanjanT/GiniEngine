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
    Core/Profiler.cpp

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
    Renderer/AtmosphericSky.cpp
    Renderer/DiligentRendererMinimal.cpp
    Renderer/DiligentTexture.cpp
    Renderer/DiligentPipeline.cpp
    Renderer/RendererAdapter.cpp
    Renderer/Material3D.cpp
    Renderer/DiligentMaterial.cpp

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
    Particles/RainSystem.cpp

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

    # Material
    Renderer/Material.cpp

    # Post-processing
    Renderer/PostProcess.cpp

    # Shadow Mapping
    Renderer/ShadowMap.cpp

    # IBL
    Renderer/IBL.cpp

    # SSAO
    Renderer/SSAO.cpp

    # Model Cache
    Renderer/ModelCache.cpp

    # Improved Model System
    Renderer/Model3D.cpp
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

    # Improved Model System
    Renderer/Vertex.h
    Renderer/Material3D.h
    Renderer/Model3D.h
)

add_library(GiniEngine SHARED ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(GiniEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/glad/include
    ${CMAKE_BINARY_DIR}/_deps/stb-src
    ${CMAKE_BINARY_DIR}/_deps/imgui-src
    ${CMAKE_BINARY_DIR}/_deps/imgui-src/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Common/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Primitives/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Graphics/GraphicsEngine/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Graphics/GraphicsTools/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Graphics/GraphicsAccessories/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Platforms/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/Platforms/Apple/interface
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
    # Diligent Engine
    Diligent-Common
    Diligent-Primitives
    Diligent-GraphicsEngineVk-static
    Diligent-GraphicsTools
    Diligent-GraphicsAccessories
    Diligent-ApplePlatform
    Diligent-ShaderTools
    Diligent-HLSL2GLSLConverterLib
    Diligent-Archiver-static
)

# Add Diligent Engine library directory
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DiligentEngine_v2.5.6/build/${CMAKE_BUILD_TYPE})

# Set Vulkan SDK path for Diligent Engine
set(VULKAN_SDK "/opt/homebrew/opt/vulkan-headers" CACHE PATH "Vulkan SDK path")

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
    POSITION_INDEPENDENT_CODE ON
)

# Set symbol visibility for shared library
if(WIN32)
    set_target_properties(GiniEngine PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
else()
    # For Unix systems, export all symbols for now (can be refined later)
    target_compile_options(GiniEngine PRIVATE -fvisibility=default)
    target_compile_definitions(GiniEngine PRIVATE GINI_ENGINE_EXPORTS)
endif()

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
