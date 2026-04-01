#include "Gini.h"

using namespace Gini;

class RTSDemo : public Application {
public:
    RTSDemo() : Application(CreateConfig()) {
        m_World = CreateScope<World>();
        m_TileMap = CreateScope<TileMap>(64, 64, 32);
        m_Pathfinder = CreateScope<Pathfinder>(m_TileMap.get());
        m_Selection = CreateScope<UnitSelection>(m_World.get());
        m_CameraController = CreateScope<RTSCameraController>(GetWindow().GetAspectRatio(), 5.0f);
    }
    
    static EngineConfig CreateConfig() {
        EngineConfig config;
        config.windowTitle = "Gini Engine - RTS Demo";
        config.windowWidth = 1280;
        config.windowHeight = 720;
        config.vsync = true;
        return config;
    }
    
    void OnInit() override {
        GINI_INFO("RTS Demo initialized!");
        
        Renderer::Init();
        Renderer::SetClearColor(Color(0.1f, 0.15f, 0.2f));
        
        // Initialize tilemap with some terrain
        for (u32 y = 0; y < m_TileMap->GetHeight(); y++) {
            for (u32 x = 0; x < m_TileMap->GetWidth(); x++) {
                // Create some water and forests
                if ((x > 20 && x < 25 && y > 10 && y < 50) ||
                    (x > 40 && x < 45 && y > 20 && y < 40)) {
                    m_TileMap->SetTileType(x, y, TileType::Water);
                }
                else if ((x > 5 && x < 15 && y > 30 && y < 45)) {
                    m_TileMap->SetTileType(x, y, TileType::Forest);
                }
            }
        }
        
        // Create some test units
        for (int i = 0; i < 10; i++) {
            CreateUnit(Vec2(100 + i * 50, 100), 0);
        }
        
        // Create enemy units
        for (int i = 0; i < 5; i++) {
            CreateUnit(Vec2(800 + i * 40, 400), 1);
        }
        
        m_Selection->SetPlayerFilter(0); // Only select player 0's units
    }
    
    void OnShutdown() override {
        Renderer::Shutdown();
        GINI_INFO("RTS Demo shutdown!");
    }
    
    void OnUpdate(f32 dt) override {
        m_CameraController->OnUpdate(dt);
        
        // Handle input
        HandleInput();
        
        // Update units
        UpdateUnits(dt);
    }
    
    void OnFixedUpdate(f32 dt) override {
        // Deterministic game logic would go here
        // This runs at a fixed rate for multiplayer sync
    }
    
    void OnRender() override {
        Renderer::Clear();
        Renderer::BeginScene(m_CameraController->GetCamera());
        
        // Render tilemap
        RenderTileMap();
        
        // Render units
        RenderUnits();
        
        // Render selection box
        if (m_Selection->IsSelecting()) {
            Rect box = m_Selection->GetSelectionBox();
            Renderer::DrawRect(box, Color(0.0f, 1.0f, 0.0f, 0.5f), 2.0f);
        }
        
        Renderer::EndScene();
    }
    
    void OnEvent(Event& event) override {
        if (event.GetType() == EventType::WindowResize) {
            auto& e = static_cast<WindowResizeEvent&>(event);
            m_CameraController->OnResize(static_cast<f32>(e.GetWidth()), 
                                         static_cast<f32>(e.GetHeight()));
        }
    }
    
private:
    void HandleInput() {
        auto& input = Input::Get();
        
        // Selection
        if (input.IsMouseButtonPressed(MouseButton::Left)) {
            Vec2 worldPos = ScreenToWorld(input.GetMousePosition());
            
            if (input.IsShiftDown()) {
                m_Selection->SelectAt(worldPos, true);
            } else {
                m_Selection->BeginSelectionBox(worldPos);
            }
        }
        
        if (input.IsMouseButtonDown(MouseButton::Left) && m_Selection->IsSelecting()) {
            Vec2 worldPos = ScreenToWorld(input.GetMousePosition());
            m_Selection->UpdateSelectionBox(worldPos);
        }
        
        if (input.IsMouseButtonReleased(MouseButton::Left)) {
            m_Selection->EndSelectionBox(input.IsShiftDown());
        }
        
        // Right click to move
        if (input.IsMouseButtonPressed(MouseButton::Right)) {
            Vec2 worldPos = ScreenToWorld(input.GetMousePosition());
            IssueMovementCommand(worldPos);
        }
        
        // Deselect all with Escape
        if (input.IsKeyPressed(Key::Escape)) {
            m_Selection->DeselectAll();
        }
    }
    
    Vec2 ScreenToWorld(const Vec2& screenPos) {
        // Simple conversion - in real implementation would use inverse VP matrix
        auto& cam = m_CameraController->GetCamera();
        f32 zoom = m_CameraController->GetZoomLevel();
        Vec3 camPos = cam.GetPosition();
        
        f32 halfWidth = GetWindow().GetWidth() * 0.5f;
        f32 halfHeight = GetWindow().GetHeight() * 0.5f;
        
        return Vec2(
            (screenPos.x - halfWidth) / halfWidth * zoom * GetWindow().GetAspectRatio() + camPos.x,
            (halfHeight - screenPos.y) / halfHeight * zoom + camPos.y
        );
    }
    
    void CreateUnit(const Vec2& position, i32 playerId) {
        Entity entity = m_World->CreateEntity("Unit");
        
        auto& transform = m_World->GetComponent<TransformComponent>(entity);
        transform.position = Vec3(position, 0.0f);
        transform.scale = Vec3(32.0f, 32.0f, 1.0f);
        
        auto& sprite = m_World->AddComponent<SpriteComponent>(entity);
        sprite.color = (playerId == 0) ? Color(0.2f, 0.6f, 1.0f) : Color(1.0f, 0.3f, 0.2f);
        
        auto& selectable = m_World->AddComponent<SelectableComponent>(entity);
        selectable.selectionRadius = 20.0f;
        
        auto& unit = m_World->AddComponent<UnitComponent>(entity);
        unit.playerId = playerId;
        unit.moveSpeed = 150.0f;
        
        auto& movement = m_World->AddComponent<MovementComponent>(entity);
    }
    
    void IssueMovementCommand(const Vec2& target) {
        for (auto entity : m_Selection->GetSelectedEntities()) {
            if (m_World->HasComponent<MovementComponent>(entity)) {
                auto& transform = m_World->GetComponent<TransformComponent>(entity);
                auto& movement = m_World->GetComponent<MovementComponent>(entity);
                
                Vec2 start(transform.position.x, transform.position.y);
                movement.path = m_Pathfinder->FindPath(start, target);
                movement.currentPathIndex = 0;
                movement.isMoving = !movement.path.empty();
            }
        }
    }
    
    void UpdateUnits(f32 dt) {
        auto view = m_World->View<TransformComponent, MovementComponent, UnitComponent>();
        
        for (auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            auto& movement = view.get<MovementComponent>(entity);
            auto& unit = view.get<UnitComponent>(entity);
            
            if (movement.isMoving && movement.currentPathIndex < movement.path.size()) {
                Vec2 currentPos(transform.position.x, transform.position.y);
                Vec2 targetPos = movement.path[movement.currentPathIndex];
                
                Vec2 dir = targetPos - currentPos;
                f32 dist = glm::length(dir);
                
                if (dist < movement.arrivalThreshold) {
                    movement.currentPathIndex++;
                    if (movement.currentPathIndex >= movement.path.size()) {
                        movement.isMoving = false;
                    }
                } else {
                    dir = glm::normalize(dir);
                    Vec2 velocity = dir * unit.moveSpeed * dt;
                    transform.position.x += velocity.x;
                    transform.position.y += velocity.y;
                }
            }
        }
    }
    
    void RenderTileMap() {
        for (u32 y = 0; y < m_TileMap->GetHeight(); y++) {
            for (u32 x = 0; x < m_TileMap->GetWidth(); x++) {
                const Tile& tile = m_TileMap->GetTile(x, y);
                Vec2 pos = m_TileMap->TileToWorld(IVec2(x, y));
                Vec2 size(static_cast<f32>(m_TileMap->GetTileSize()));
                
                Color color;
                switch (tile.type) {
                    case TileType::Ground: color = Color(0.3f, 0.5f, 0.2f); break;
                    case TileType::Water: color = Color(0.2f, 0.4f, 0.8f); break;
                    case TileType::Forest: color = Color(0.1f, 0.4f, 0.15f); break;
                    default: color = Color(0.4f, 0.4f, 0.4f); break;
                }
                
                Renderer::DrawQuad(pos, size, color);
            }
        }
    }
    
    void RenderUnits() {
        auto view = m_World->View<TransformComponent, SpriteComponent>();
        
        for (auto entity : view) {
            auto& transform = view.get<TransformComponent>(entity);
            auto& sprite = view.get<SpriteComponent>(entity);
            
            Vec2 pos(transform.position.x - transform.scale.x * 0.5f,
                     transform.position.y - transform.scale.y * 0.5f);
            Vec2 size(transform.scale.x, transform.scale.y);
            
            Renderer::DrawQuad(pos, size, sprite.color);
            
            // Draw selection indicator
            if (m_World->HasComponent<SelectableComponent>(entity)) {
                auto& selectable = m_World->GetComponent<SelectableComponent>(entity);
                if (selectable.selected) {
                    Renderer::DrawCircle(Vec2(transform.position.x, transform.position.y),
                                        selectable.selectionRadius, Color(0.0f, 1.0f, 0.0f), 2.0f);
                }
            }
        }
    }
    
    Scope<World> m_World;
    Scope<TileMap> m_TileMap;
    Scope<Pathfinder> m_Pathfinder;
    Scope<UnitSelection> m_Selection;
    Scope<RTSCameraController> m_CameraController;
};

Gini::Application* Gini::CreateApplication() {
    return new RTSDemo();
}

int main(int argc, char** argv) {
    auto app = Gini::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
