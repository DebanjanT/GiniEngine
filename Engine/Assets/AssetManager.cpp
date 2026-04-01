#include "AssetManager.h"
#include "Core/Logger.h"
#include <filesystem>
#include <algorithm>

namespace Gini {

void AssetManager::Init(const std::string& assetRootPath) {
    m_AssetRoot = assetRootPath;
    GINI_INFO("AssetManager initialized with root: ", m_AssetRoot);
}

void AssetManager::Shutdown() {
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

AssetType AssetManager::GetAssetType(const std::string& path) const {
    std::string ext = path.substr(path.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga") {
        return AssetType::Texture;
    }
    if (ext == "glsl" || ext == "vert" || ext == "frag" || ext == "shader") {
        return AssetType::Shader;
    }
    if (ext == "obj" || ext == "gltf" || ext == "glb" || ext == "fbx" || ext == "dae") {
        return AssetType::Model;
    }
    if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "flac") {
        return AssetType::Audio;
    }
    if (ext == "ttf" || ext == "otf") {
        return AssetType::Font;
    }
    
    return AssetType::Unknown;
}

TextureHandle AssetManager::LoadTexture(const std::string& path) {
    // Check cache first
    auto it = m_Textures.find(path);
    if (it != m_Textures.end() && it->second.IsValid()) {
        return it->second;
    }
    
    std::string fullPath = ResolvePath(path);
    
    TextureHandle handle;
    handle.metadata.path = path;
    handle.metadata.type = AssetType::Texture;
    handle.asset = Texture2D::Create(fullPath);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        m_Textures[path] = handle;
        GINI_INFO("Loaded texture: ", path);
    } else {
        GINI_ERROR("Failed to load texture: ", path);
    }
    
    return handle;
}

ShaderHandle AssetManager::LoadShader(const std::string& path) {
    auto it = m_Shaders.find(path);
    if (it != m_Shaders.end() && it->second.IsValid()) {
        return it->second;
    }
    
    std::string fullPath = ResolvePath(path);
    
    ShaderHandle handle;
    handle.metadata.path = path;
    handle.metadata.type = AssetType::Shader;
    handle.asset = Shader::CreateFromFile(fullPath);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        m_Shaders[path] = handle;
        GINI_INFO("Loaded shader: ", path);
    } else {
        GINI_ERROR("Failed to load shader: ", path);
    }
    
    return handle;
}

ShaderHandle AssetManager::LoadShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) {
    auto it = m_Shaders.find(name);
    if (it != m_Shaders.end() && it->second.IsValid()) {
        return it->second;
    }
    
    ShaderHandle handle;
    handle.metadata.path = name;
    handle.metadata.type = AssetType::Shader;
    handle.asset = Shader::Create(vertexSrc, fragmentSrc);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        m_Shaders[name] = handle;
        GINI_INFO("Created shader: ", name);
    } else {
        GINI_ERROR("Failed to create shader: ", name);
    }
    
    return handle;
}

ModelHandle AssetManager::LoadModel(const std::string& path) {
    auto it = m_Models.find(path);
    if (it != m_Models.end() && it->second.IsValid()) {
        return it->second;
    }
    
    std::string fullPath = ResolvePath(path);
    
    ModelHandle handle;
    handle.metadata.path = path;
    handle.metadata.type = AssetType::Model;
    handle.asset = Model::Create(fullPath);
    
    if (handle.asset) {
        handle.metadata.loaded = true;
        m_Models[path] = handle;
        GINI_INFO("Loaded model: ", path);
    } else {
        GINI_ERROR("Failed to load model: ", path);
    }
    
    return handle;
}

void AssetManager::LoadTextureAsync(const std::string& path, std::function<void(TextureHandle)> callback) {
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    
    m_AsyncFutures.push_back(std::async(std::launch::async, [this, path, callback]() {
        TextureHandle handle = LoadTexture(path);
        if (callback) {
            callback(handle);
        }
    }));
}

void AssetManager::LoadModelAsync(const std::string& path, std::function<void(ModelHandle)> callback) {
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    
    m_AsyncFutures.push_back(std::async(std::launch::async, [this, path, callback]() {
        ModelHandle handle = LoadModel(path);
        if (callback) {
            callback(handle);
        }
    }));
}

TextureHandle AssetManager::GetTexture(const std::string& path) {
    auto it = m_Textures.find(path);
    if (it != m_Textures.end()) {
        return it->second;
    }
    return TextureHandle();
}

ShaderHandle AssetManager::GetShader(const std::string& name) {
    auto it = m_Shaders.find(name);
    if (it != m_Shaders.end()) {
        return it->second;
    }
    return ShaderHandle();
}

ModelHandle AssetManager::GetModel(const std::string& path) {
    auto it = m_Models.find(path);
    if (it != m_Models.end()) {
        return it->second;
    }
    return ModelHandle();
}

bool AssetManager::IsTextureLoaded(const std::string& path) const {
    auto it = m_Textures.find(path);
    return it != m_Textures.end() && it->second.IsValid();
}

bool AssetManager::IsShaderLoaded(const std::string& name) const {
    auto it = m_Shaders.find(name);
    return it != m_Shaders.end() && it->second.IsValid();
}

bool AssetManager::IsModelLoaded(const std::string& path) const {
    auto it = m_Models.find(path);
    return it != m_Models.end() && it->second.IsValid();
}

void AssetManager::UnloadTexture(const std::string& path) {
    m_Textures.erase(path);
}

void AssetManager::UnloadShader(const std::string& name) {
    m_Shaders.erase(name);
}

void AssetManager::UnloadModel(const std::string& path) {
    m_Models.erase(path);
}

void AssetManager::UnloadAll() {
    m_Textures.clear();
    m_Shaders.clear();
    m_Models.clear();
    GINI_INFO("All assets unloaded");
}

void AssetManager::CheckForChanges() {
    // TODO: Implement file watching for hot reload
}

u64 AssetManager::GetTotalMemoryUsage() const {
    u64 total = 0;
    
    for (const auto& [path, handle] : m_Textures) {
        if (handle.asset) {
            total += handle.asset->GetWidth() * handle.asset->GetHeight() * 4;
        }
    }
    
    return total;
}

void AssetManager::ProcessAsyncQueue() {
    // Clean up completed futures
    m_AsyncFutures.erase(
        std::remove_if(m_AsyncFutures.begin(), m_AsyncFutures.end(),
            [](std::future<void>& f) {
                return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }),
        m_AsyncFutures.end()
    );
}

} // namespace Gini
