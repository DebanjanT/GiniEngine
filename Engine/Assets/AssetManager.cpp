#include "AssetManager.h"
#include "Core/Logger.h"
#include <filesystem>
#include <algorithm>

namespace Gini {

void AssetManager::Init(const std::string& assetRootPath) {
    Init(assetRootPath, true);
}

void AssetManager::Init(const std::string& assetRootPath, bool enableLoaderThread) {
    m_AssetRoot = assetRootPath;
    m_EnableLoaderThread = enableLoaderThread;
    if (m_EnableLoaderThread) {
        m_AssetLoaderThread.Start("AssetLoadingThread");
    }
    GINI_INFO("AssetManager initialized with root: ", m_AssetRoot);
}

void AssetManager::Shutdown() {
    m_AssetLoaderThread.Stop();
    UnloadAll();
    GINI_INFO("AssetManager shutdown");
}

void AssetManager::Update() {
    ProcessAsyncQueue();
    
    if (m_HotReloadEnabled) {
        CheckForChanges();
    }
}

std::string AssetManager::ResolvePath(const std::string& relativePath) const {
    if (relativePath.empty()) return "";
    
    // If already absolute or starts with asset root, return as-is
    if (relativePath[0] == '/' || relativePath.find(m_AssetRoot) == 0) {
        return relativePath;
    }
    
    return m_AssetRoot + relativePath;
}

LoadableAssetType AssetManager::GetAssetType(const std::string& path) const {
    std::string ext = path.substr(path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga") {
        return LoadableAssetType::Texture;
    }
    if (ext == "glsl" || ext == "vert" || ext == "frag" || ext == "shader") {
        return LoadableAssetType::Shader;
    }
    if (ext == "obj" || ext == "gltf" || ext == "glb" || ext == "fbx" || ext == "dae") {
        return LoadableAssetType::Model;
    }
    if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "flac") {
        return LoadableAssetType::Audio;
    }
    if (ext == "ttf" || ext == "otf") {
        return LoadableAssetType::Font;
    }
    
    return LoadableAssetType::Unknown;
}

TextureAsset AssetManager::LoadTexture(const std::string& path) {
    {
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        auto it = m_Textures.find(path);
        if (it != m_Textures.end() && it->second.IsValid()) {
            return it->second;
        }
    }
    
    std::string fullPath = ResolvePath(path);
    
    TextureAsset handle;
    handle.metadata.path = path;
    handle.metadata.type = LoadableAssetType::Texture;
    handle.asset = Texture2D::Create(fullPath);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        m_Textures[path] = handle;
        GINI_INFO("Loaded texture: ", path);
    } else {
        GINI_ERROR("Failed to load texture: ", path);
    }
    
    return handle;
}

ShaderAsset AssetManager::LoadShader(const std::string& path) {
    {
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        auto it = m_Shaders.find(path);
        if (it != m_Shaders.end() && it->second.IsValid()) {
            return it->second;
        }
    }
    
    std::string fullPath = ResolvePath(path);
    
    ShaderAsset handle;
    handle.metadata.path = path;
    handle.metadata.type = LoadableAssetType::Shader;
    handle.asset = Shader::CreateFromFile(fullPath);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        m_Shaders[path] = handle;
        GINI_INFO("Loaded shader: ", path);
    } else {
        GINI_ERROR("Failed to load shader: ", path);
    }
    
    return handle;
}

ShaderAsset AssetManager::LoadShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) {
    {
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        auto it = m_Shaders.find(name);
        if (it != m_Shaders.end() && it->second.IsValid()) {
            return it->second;
        }
    }
    
    ShaderAsset handle;
    handle.metadata.path = name;
    handle.metadata.type = LoadableAssetType::Shader;
    handle.asset = Shader::Create(vertexSrc, fragmentSrc);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        m_Shaders[name] = handle;
        GINI_INFO("Created shader: ", name);
    } else {
        GINI_ERROR("Failed to create shader: ", name);
    }
    
    return handle;
}

MeshSourceAsset AssetManager::LoadMeshSource(const std::string& path) {
    {
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        auto it = m_MeshSources.find(path);
        if (it != m_MeshSources.end() && it->second.IsValid()) {
            return it->second;
        }
    }
    
    std::string fullPath = ResolvePath(path);
    
    MeshSourceAsset handle;
    handle.metadata.path = path;
    handle.metadata.type = LoadableAssetType::Model;
    AssimpMeshImporter importer(fullPath);
    auto result = importer.Import();
    handle.asset = result.meshSource;
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        std::lock_guard<std::mutex> lock(m_AssetMutex);
        m_MeshSources[path] = handle;
        GINI_INFO("Loaded mesh source: ", path);
    } else {
        GINI_ERROR("Failed to load mesh source: ", path);
    }
    
    return handle;
}

void AssetManager::LoadTextureAsync(const std::string& path, std::function<void(TextureAsset)> callback) {
    auto loadTask = [this, path, callback]() {
        TextureAsset handle = LoadTexture(path);
        if (callback) { 
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            m_CompletionQueue.push([callback, handle]() { callback(handle); });
        }
    };

    if (m_EnableLoaderThread && m_AssetLoaderThread.IsRunning()) {
        m_AssetLoaderThread.Submit(loadTask);
    } else {
        loadTask();
    }
}

void AssetManager::LoadMeshSourceAsync(const std::string& path, std::function<void(MeshSourceAsset)> callback) {
    auto loadTask = [this, path, callback]() {
        MeshSourceAsset handle = LoadMeshSource(path);
        if (callback) {
            std::lock_guard<std::mutex> lock(m_AsyncMutex);
            m_CompletionQueue.push([callback, handle]() { callback(handle); });
        }
    };

    if (m_EnableLoaderThread && m_AssetLoaderThread.IsRunning()) {
        m_AssetLoaderThread.Submit(loadTask);
    } else {
        loadTask();
    }
}

TextureAsset AssetManager::GetTexture(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    auto it = m_Textures.find(path);
    if (it != m_Textures.end()) {
        return it->second;
    }
    return TextureAsset();
}

ShaderAsset AssetManager::GetShader(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    auto it = m_Shaders.find(name);
    if (it != m_Shaders.end()) {
        return it->second;
    }
    return ShaderAsset();
}

MeshSourceAsset AssetManager::GetMeshSource(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    auto it = m_MeshSources.find(path);
    if (it != m_MeshSources.end()) {
        return it->second;
    }
    return MeshSourceAsset();
}

bool AssetManager::IsTextureLoaded(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    auto it = m_Textures.find(path);
    return it != m_Textures.end() && it->second.IsValid();
}

bool AssetManager::IsShaderLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    auto it = m_Shaders.find(name);
    return it != m_Shaders.end() && it->second.IsValid();
}

bool AssetManager::IsMeshSourceLoaded(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    auto it = m_MeshSources.find(path);
    return it != m_MeshSources.end() && it->second.IsValid();
}

void AssetManager::UnloadTexture(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    m_Textures.erase(path);
}

void AssetManager::UnloadShader(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    m_Shaders.erase(name);
}

void AssetManager::UnloadMeshSource(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    m_MeshSources.erase(path);
}

void AssetManager::UnloadAll() {
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    m_Textures.clear();
    m_Shaders.clear();
    m_MeshSources.clear();
    GINI_INFO("All assets unloaded");
}

void AssetManager::CheckForChanges() {
    // TODO: Implement file watching for hot reload
}

u64 AssetManager::GetTotalMemoryUsage() const {
    u64 total = 0;
    std::lock_guard<std::mutex> lock(m_AssetMutex);
    
    for (const auto& [path, handle] : m_Textures) {
        if (handle.asset) {
            total += handle.asset->GetWidth() * handle.asset->GetHeight() * 4;
        }
    }
    
    return total;
}

void AssetManager::ProcessAsyncQueue() {
    std::queue<std::function<void()>> localQueue;
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        std::swap(localQueue, m_CompletionQueue);
    }

    while (!localQueue.empty()) {
        localQueue.front()();
        localQueue.pop();
    }
}

} // namespace Gini
