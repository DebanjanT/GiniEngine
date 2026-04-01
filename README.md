# Gini Engine

A cross-platform 2D/2.5D RTS game engine built with C++20, OpenGL, and modern game development practices.

## Features

- **Cross-Platform**: Windows, macOS, Linux support
- **OpenGL Rendering**: Modern OpenGL 4.1+ with sprite batching
- **Entity Component System**: Powered by EnTT for efficient entity management
- **RTS-Specific Systems**:
  - Tile-based world with pathfinding (A* and Flow Fields)
  - Unit selection (click and drag box)
  - Command system for unit orders
  - Deterministic fixed-timestep simulation for multiplayer readiness
- **Audio System**: OpenAL-based 3D positional audio
- **Input System**: Keyboard, mouse, and gamepad support
- **Debug UI**: Dear ImGui integration (coming soon)

## Tech Stack

- **Language**: C++20
- **Build System**: CMake 3.20+
- **Graphics**: OpenGL 4.1+ (GLAD loader)
- **Windowing**: GLFW 3.3+
- **Math**: GLM
- **ECS**: EnTT
- **Image Loading**: stb_image
- **Audio**: OpenAL Soft
- **UI**: Dear ImGui

## Building

### Prerequisites

- CMake 3.20 or higher
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Python 3 (for GLAD setup script)

### Setup Dependencies

First, run the setup script to download GLAD:

```bash
chmod +x scripts/setup_dependencies.sh
./scripts/setup_dependencies.sh
```

### Build Commands

```bash
# Create build directory
mkdir build && cd build

# Configure (Debug)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Configure (Release)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release -j$(nproc)

# Run the RTS Demo
./bin/RTSDemo
```

### Windows (Visual Studio)

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

## Project Structure

```
Gini Engine/
├── CMakeLists.txt          # Root CMake configuration
├── Engine/                 # Engine source code
│   ├── CMakeLists.txt
│   ├── Gini.h              # Main include header
│   ├── Core/               # Core utilities (types, logging, events)
│   ├── Window/             # GLFW window management
│   ├── Renderer/           # OpenGL rendering (sprites, shaders, batching)
│   ├── ECS/                # Entity Component System
│   ├── RTS/                # RTS-specific systems (tilemap, pathfinding)
│   ├── Audio/              # OpenAL audio system
│   └── Assets/             # Asset loading (textures, maps)
├── Samples/                # Sample games
│   └── RTSDemo/            # RTS demonstration
├── Editor/                 # Map editor (coming soon)
├── ThirdParty/             # Third-party libraries
│   └── stb/                # stb_image implementation
└── scripts/                # Build and setup scripts
```

## Usage Example

```cpp
#include "Gini.h"

class MyGame : public Gini::Application {
public:
    MyGame() : Application(GetConfig()) {}
    
    static Gini::EngineConfig GetConfig() {
        Gini::EngineConfig config;
        config.windowTitle = "My RTS Game";
        config.windowWidth = 1280;
        config.windowHeight = 720;
        return config;
    }
    
    void OnInit() override {
        Gini::Renderer::Init();
        // Initialize your game
    }
    
    void OnUpdate(float dt) override {
        // Update game logic
    }
    
    void OnFixedUpdate(float dt) override {
        // Deterministic simulation (for multiplayer)
    }
    
    void OnRender() override {
        Gini::Renderer::Clear();
        Gini::Renderer::BeginScene(m_Camera);
        // Render your game
        Gini::Renderer::EndScene();
    }
};

Gini::Application* Gini::CreateApplication() {
    return new MyGame();
}

int main() {
    auto app = Gini::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
```

## RTS Features

### Pathfinding

```cpp
// A* pathfinding
Gini::Pathfinder pathfinder(&tileMap);
auto path = pathfinder.FindPath(startPos, endPos);

// Flow field for large unit counts
Gini::FlowField flowField(&tileMap);
flowField.Generate(targetPos);
Vec2 direction = flowField.GetDirection(unitPos);
```

### Unit Selection

```cpp
Gini::UnitSelection selection(&world);
selection.SetPlayerFilter(0); // Only select player 0's units

// Click selection
selection.SelectAt(worldPos);

// Box selection
selection.BeginSelectionBox(startPos);
selection.UpdateSelectionBox(currentPos);
selection.EndSelectionBox();
```

### Tile Map

```cpp
Gini::TileMap tileMap(64, 64, 32); // 64x64 tiles, 32px each
tileMap.SetTileType(x, y, Gini::TileType::Water);

// Coordinate conversion
IVec2 tilePos = tileMap.WorldToTile(worldPos);
Vec2 worldPos = tileMap.TileToWorldCenter(tilePos);
```

## Roadmap

- [x] Core engine framework
- [x] OpenGL rendering with sprite batching
- [x] ECS with EnTT
- [x] Tile map system
- [x] A* pathfinding
- [x] Unit selection system
- [x] Fixed timestep for deterministic simulation
- [ ] Dear ImGui debug UI
- [ ] Map editor
- [ ] Fog of war
- [ ] Networking (lockstep multiplayer)
- [ ] Steam SDK integration
- [ ] Particle system
- [ ] Animation system

## License

MIT License - See LICENSE file for details.

## Contributing

Contributions are welcome! Please read CONTRIBUTING.md for guidelines.
