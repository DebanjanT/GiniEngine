#pragma once

#include "Core/Types.h"
#include "Core/Threading.h"
#include "Renderer/Texture.h"
#include "Renderer/Shader.h"
#include "Renderer/MeshSource.h"
#include "Asset/AssimpMeshImporter.h"
#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <functional>

namespace Gini {

enum class LoadableAssetType {
    Unknown,
    Texture,
    Shader,
    Model,
    Audio,
    Font,
    Script
};

struct LoadableAssetInfo {
    std::string path;
    LoadableAssetType type = LoadableAssetType::Unknown;
    u64 size = 0;
    u64 lastModified = 0;
    bool loaded = false;
};

template<typename T>
struct AssetHandle {
    Ref<T> asset;
    LoadableAssetInfo metadata;
    
    bool IsValid() const { return asset != nullptr; }
    T* Get() { return asset.get(); }
    const T* Get() const { return asset.get(); }
    T* operator->() { return asset.get(); }
    const T* operator->() const { return asset.get(); }
};

using TextureHandle = AssetHandle<Texture2D>;
using ShaderHandle = AssetHandle<Shader>;
using MeshSourceHandle = AssetHandle<MeshSource>;

class AssetManager {
public:
    static AssetManager& Get() {
        static AssetManager instance;
        return instance;
    }
    
    void Init(const std::string& assetRootPath = "Assets/");
    void Init(const std::string& assetRootPath, bool enableLoaderThread);
    void Shutdown();
    void Update();
    
    // Synchronous loading
    TextureHandle LoadTexture(const std::string& path);
    ShaderHandle LoadShader(const std::string& path);
    ShaderHandle LoadShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
    MeshSourceHandle LoadMeshSource(const std::string& path);
    
    // Asynchronous loading
    void LoadTextureAsync(const std::string& path, std::function<void(TextureHandle)> callback);
    void LoadMeshSourceAsync(const std::string& path, std::function<void(MeshSourceHandle)> callback);
    
    // Get cached assets
    TextureHandle GetTexture(const std::string& path);
    ShaderHandle GetShader(const std::string& name);
    MeshSourceHandle GetMeshSource(const std::string& path);
    
    // Check if asset is loaded
    bool IsTextureLoaded(const std::string& path) const;
    bool IsShaderLoaded(const std::string& name) const;
    bool IsMeshSourceLoaded(const std::string& path) const;
    
    // Unload assets
    void UnloadTexture(const std::string& path);
    void UnloadShader(const std::string& name);
    void UnloadMeshSource(const std::string& path);
    void UnloadAll();
    
    // Hot reload
    void EnableHotReload(bool enable) { m_HotReloadEnabled = enable; }
    void CheckForChanges();
    
    // Stats
    u32 GetLoadedTextureCount() const { return static_cast<u32>(m_Textures.size()); }
    u32 GetLoadedShaderCount() const { return static_cast<u32>(m_Shaders.size()); }
    u32 GetLoadedMeshSourceCount() const { return static_cast<u32>(m_MeshSources.size()); }
    u64 GetTotalMemoryUsage() const;
    
    const std::string& GetAssetRoot() const { return m_AssetRoot; }
    std::string ResolvePath(const std::string& relativePath) const;
    
private:
    AssetManager() = default;
    ~AssetManager() = default;
    
    LoadableAssetType GetAssetType(const std::string& path) const;
    void ProcessAsyncQueue();
    
    std::string m_AssetRoot = "Assets/";
    bool m_HotReloadEnabled = false;
    bool m_EnableLoaderThread = true;
    
    std::unordered_map<std::string, TextureHandle> m_Textures;
    std::unordered_map<std::string, ShaderHandle> m_Shaders;
    std::unordered_map<std::string, MeshSourceHandle> m_MeshSources;
    mutable std::mutex m_AssetMutex;
    
    WorkerThread m_AssetLoaderThread;
    std::queue<std::function<void()>> m_CompletionQueue;
    std::mutex m_AsyncMutex;
};

} // namespace Gini
