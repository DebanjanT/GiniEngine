#pragma once

#include "Core/Types.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/Model.h"
#include <string>
#include <unordered_map>
#include <future>
#include <queue>
#include <mutex>
#include <functional>

namespace Gini {

enum class AssetType {
    Unknown,
    Texture,
    Shader,
    Model,
    Audio,
    Font,
    Script
};

struct AssetMetadata {
    std::string path;
    AssetType type = AssetType::Unknown;
    u64 size = 0;
    u64 lastModified = 0;
    bool loaded = false;
};

template<typename T>
struct AssetHandle {
    Ref<T> asset;
    AssetMetadata metadata;
    
    bool IsValid() const { return asset != nullptr; }
    T* Get() { return asset.get(); }
    const T* Get() const { return asset.get(); }
    T* operator->() { return asset.get(); }
    const T* operator->() const { return asset.get(); }
};

using TextureHandle = AssetHandle<Texture2D>;
using ShaderHandle = AssetHandle<Shader>;
using ModelHandle = AssetHandle<Model>;

class AssetManager {
public:
    static AssetManager& Get() {
        static AssetManager instance;
        return instance;
    }
    
    void Init(const std::string& assetRootPath = "Assets/");
    void Shutdown();
    void Update();
    
    // Synchronous loading
    TextureHandle LoadTexture(const std::string& path);
    ShaderHandle LoadShader(const std::string& path);
    ShaderHandle LoadShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    ModelHandle LoadModel(const std::string& path);
    
    // Asynchronous loading
    void LoadTextureAsync(const std::string& path, std::function<void(TextureHandle)> callback);
    void LoadModelAsync(const std::string& path, std::function<void(ModelHandle)> callback);
    
    // Get cached assets
    TextureHandle GetTexture(const std::string& path);
    ShaderHandle GetShader(const std::string& name);
    ModelHandle GetModel(const std::string& path);
    
    // Check if asset is loaded
    bool IsTextureLoaded(const std::string& path) const;
    bool IsShaderLoaded(const std::string& name) const;
    bool IsModelLoaded(const std::string& path) const;
    
    // Unload assets
    void UnloadTexture(const std::string& path);
    void UnloadShader(const std::string& name);
    void UnloadModel(const std::string& path);
    void UnloadAll();
    
    // Hot reload
    void EnableHotReload(bool enable) { m_HotReloadEnabled = enable; }
    void CheckForChanges();
    
    // Stats
    u32 GetLoadedTextureCount() const { return static_cast<u32>(m_Textures.size()); }
    u32 GetLoadedShaderCount() const { return static_cast<u32>(m_Shaders.size()); }
    u32 GetLoadedModelCount() const { return static_cast<u32>(m_Models.size()); }
    u64 GetTotalMemoryUsage() const;
    
    const std::string& GetAssetRoot() const { return m_AssetRoot; }
    std::string ResolvePath(const std::string& relativePath) const;
    
private:
    AssetManager() = default;
    ~AssetManager() = default;
    
    AssetType GetAssetType(const std::string& path) const;
    void ProcessAsyncQueue();
    
    std::string m_AssetRoot = "Assets/";
    bool m_HotReloadEnabled = false;
    
    std::unordered_map<std::string, TextureHandle> m_Textures;
    std::unordered_map<std::string, ShaderHandle> m_Shaders;
    std::unordered_map<std::string, ModelHandle> m_Models;
    
    // Async loading
    struct AsyncTask {
        std::string path;
        AssetType type;
        std::function<void()> callback;
    };
    
    std::queue<AsyncTask> m_AsyncQueue;
    std::mutex m_AsyncMutex;
    std::vector<std::future<void>> m_AsyncFutures;
};

} // namespace Gini
