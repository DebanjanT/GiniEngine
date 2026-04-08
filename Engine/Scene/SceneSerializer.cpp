#include "SceneSerializer.h"
#include "Core/Logger.h"
#include "ECS/Components.h"
#include "Renderer/ModelCache.h"

#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace YAML {

// Vec3 conversion
template <> struct convert<glm::vec3> {
  static Node encode(const glm::vec3 &rhs) {
    Node node;
    node.push_back(rhs.x);
    node.push_back(rhs.y);
    node.push_back(rhs.z);
    node.SetStyle(EmitterStyle::Flow);
    return node;
  }

  static bool decode(const Node &node, glm::vec3 &rhs) {
    if (!node.IsSequence() || node.size() != 3)
      return false;
    rhs.x = node[0].as<float>();
    rhs.y = node[1].as<float>();
    rhs.z = node[2].as<float>();
    return true;
  }
};

// Vec4 conversion
template <> struct convert<glm::vec4> {
  static Node encode(const glm::vec4 &rhs) {
    Node node;
    node.push_back(rhs.x);
    node.push_back(rhs.y);
    node.push_back(rhs.z);
    node.push_back(rhs.w);
    node.SetStyle(EmitterStyle::Flow);
    return node;
  }

  static bool decode(const Node &node, glm::vec4 &rhs) {
    if (!node.IsSequence() || node.size() != 4)
      return false;
    rhs.x = node[0].as<float>();
    rhs.y = node[1].as<float>();
    rhs.z = node[2].as<float>();
    rhs.w = node[3].as<float>();
    return true;
  }
};

} // namespace YAML

namespace Gini {

// YAML emitter operators
static YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &v) {
  out << YAML::Flow;
  out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
  return out;
}

static YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &v) {
  out << YAML::Flow;
  out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
  return out;
}

SceneSerializer::SceneSerializer(const Ref<Scene> &scene) : m_Scene(scene) {}

void SceneSerializer::SerializeEntity(void *emitterPtr, Entity entity) {
  YAML::Emitter &out = *static_cast<YAML::Emitter *>(emitterPtr);
  auto &world = m_Scene->GetWorld();

  out << YAML::BeginMap; // Entity
  out << YAML::Key << "Entity" << YAML::Value << static_cast<uint32_t>(entity);

  // TagComponent
  if (world.HasComponent<TagComponent>(entity)) {
    out << YAML::Key << "TagComponent";
    out << YAML::BeginMap;
    auto &tag = world.GetComponent<TagComponent>(entity);
    out << YAML::Key << "Tag" << YAML::Value << tag.tag;
    out << YAML::EndMap;
  }

  // TransformComponent
  if (world.HasComponent<TransformComponent>(entity)) {
    out << YAML::Key << "TransformComponent";
    out << YAML::BeginMap;
    auto &tc = world.GetComponent<TransformComponent>(entity);
    out << YAML::Key << "Position" << YAML::Value << tc.position;
    out << YAML::Key << "Rotation" << YAML::Value << tc.rotation;
    out << YAML::Key << "Scale" << YAML::Value << tc.scale;
    out << YAML::EndMap;
  }

  // MeshComponent
  if (world.HasComponent<MeshComponent>(entity)) {
    out << YAML::Key << "MeshComponent";
    out << YAML::BeginMap;
    auto &mc = world.GetComponent<MeshComponent>(entity);
    out << YAML::Key << "MeshType" << YAML::Value
        << static_cast<int>(mc.meshType);
    out << YAML::Key << "ModelPath" << YAML::Value << mc.modelPath;
    out << YAML::Key << "CastShadows" << YAML::Value << mc.castShadows;
    out << YAML::Key << "ReceiveShadows" << YAML::Value << mc.receiveShadows;
    out << YAML::EndMap;
  }

  // MaterialComponent
  if (world.HasComponent<MaterialComponent>(entity)) {
    out << YAML::Key << "MaterialComponent";
    out << YAML::BeginMap;
    auto &mat = world.GetComponent<MaterialComponent>(entity);
    out << YAML::Key << "Albedo" << YAML::Value << mat.albedo;
    out << YAML::Key << "Metallic" << YAML::Value << mat.metallic;
    out << YAML::Key << "Roughness" << YAML::Value << mat.roughness;
    out << YAML::Key << "AO" << YAML::Value << mat.ao;
    out << YAML::Key << "Emissive" << YAML::Value << mat.emissive;
    out << YAML::Key << "AlbedoTexturePath" << YAML::Value
        << mat.albedoTexturePath;
    out << YAML::Key << "NormalTexturePath" << YAML::Value
        << mat.normalTexturePath;
    out << YAML::Key << "MetallicTexturePath" << YAML::Value
        << mat.metallicTexturePath;
    out << YAML::Key << "RoughnessTexturePath" << YAML::Value
        << mat.roughnessTexturePath;
    out << YAML::Key << "AOTexturePath" << YAML::Value << mat.aoTexturePath;
    out << YAML::EndMap;
  }

  // LightComponent
  if (world.HasComponent<LightComponent>(entity)) {
    out << YAML::Key << "LightComponent";
    out << YAML::BeginMap;
    auto &lc = world.GetComponent<LightComponent>(entity);
    out << YAML::Key << "Type" << YAML::Value << lc.type;
    out << YAML::Key << "Color" << YAML::Value << lc.color;
    out << YAML::Key << "Intensity" << YAML::Value << lc.intensity;
    out << YAML::Key << "Range" << YAML::Value << lc.range;
    out << YAML::Key << "InnerConeAngle" << YAML::Value << lc.innerConeAngle;
    out << YAML::Key << "OuterConeAngle" << YAML::Value << lc.outerConeAngle;
    out << YAML::Key << "CastShadows" << YAML::Value << lc.castShadows;
    out << YAML::EndMap;
  }

  // CameraComponent
  if (world.HasComponent<CameraComponent>(entity)) {
    out << YAML::Key << "CameraComponent";
    out << YAML::BeginMap;
    auto &cc = world.GetComponent<CameraComponent>(entity);
    out << YAML::Key << "IsPrimary" << YAML::Value << cc.isPrimary;
    out << YAML::Key << "FOV" << YAML::Value << cc.fov;
    out << YAML::Key << "NearClip" << YAML::Value << cc.nearClip;
    out << YAML::Key << "FarClip" << YAML::Value << cc.farClip;
    out << YAML::EndMap;
  }

  // SkyboxComponent
  if (world.HasComponent<SkyboxComponent>(entity)) {
    out << YAML::Key << "SkyboxComponent";
    out << YAML::BeginMap;
    auto &sb = world.GetComponent<SkyboxComponent>(entity);
    out << YAML::Key << "HDRPath" << YAML::Value << sb.hdrPath;
    out << YAML::Key << "Intensity" << YAML::Value << sb.intensity;
    out << YAML::Key << "LOD" << YAML::Value << sb.lod;
    out << YAML::Key << "UseHDR" << YAML::Value << sb.useHDR;
    out << YAML::EndMap;
  }

  // AnimatorComponent3D
  if (world.HasComponent<AnimatorComponent3D>(entity)) {
    out << YAML::Key << "AnimatorComponent3D";
    out << YAML::BeginMap;
    auto &ac = world.GetComponent<AnimatorComponent3D>(entity);
    out << YAML::Key << "AnimationPath" << YAML::Value << ac.animationPath;
    out << YAML::Key << "Playing" << YAML::Value << ac.playing;
    out << YAML::Key << "Speed" << YAML::Value << ac.speed;
    out << YAML::Key << "Loop" << YAML::Value << ac.loop;
    out << YAML::EndMap;
  }

  out << YAML::EndMap; // Entity
}

void SceneSerializer::Serialize(const std::string &filepath) {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();

  // Save terrain path if scene has terrain
  if (m_Scene->HasTerrain() && !m_Scene->GetTerrainPath().empty()) {
    out << YAML::Key << "TerrainPath" << YAML::Value
        << m_Scene->GetTerrainPath();
  }

  out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

  auto &world = m_Scene->GetWorld();
  auto view = world.GetRegistry().view<TagComponent>();

  for (auto entity : view) {
    SerializeEntity(&out, entity);
  }

  out << YAML::EndSeq;
  out << YAML::EndMap;

  std::ofstream fout(filepath);
  fout << out.c_str();

  GINI_INFO("Scene serialized to: ", filepath);
}

std::string SceneSerializer::SerializeToString() {
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
  out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

  auto &world = m_Scene->GetWorld();
  auto view = world.GetRegistry().view<TagComponent>();

  for (auto entity : view) {
    SerializeEntity(&out, entity);
  }

  out << YAML::EndSeq;
  out << YAML::EndMap;

  return std::string(out.c_str());
}

bool SceneSerializer::Deserialize(const std::string &filepath) {
  std::ifstream stream(filepath);
  if (!stream.is_open()) {
    GINI_ERROR("Failed to open scene file: ", filepath);
    return false;
  }

  std::stringstream strStream;
  strStream << stream.rdbuf();

  return DeserializeFromString(strStream.str());
}

bool SceneSerializer::DeserializeFromString(const std::string &yamlString) {
  YAML::Node data;
  try {
    data = YAML::Load(yamlString);
  } catch (const YAML::ParserException &e) {
    GINI_ERROR("Failed to parse scene YAML: ", e.what());
    return false;
  }

  if (!data["Scene"]) {
    GINI_ERROR("Invalid scene file: missing 'Scene' node");
    return false;
  }

  std::string sceneName = data["Scene"].as<std::string>();
  GINI_INFO("Deserializing scene: ", sceneName);

  // Clear existing entities
  m_Scene->Clear();
  m_Scene->SetName(sceneName);

  // Load terrain if path is specified
  if (data["TerrainPath"]) {
    std::string terrainPath = data["TerrainPath"].as<std::string>();
    m_Scene->LoadTerrainFromFile(terrainPath);
  }

  auto entities = data["Entities"];
  if (entities) {
    for (auto entityNode : entities) {
      std::string name = "Entity";

      auto tagComponent = entityNode["TagComponent"];
      if (tagComponent) {
        name = tagComponent["Tag"].as<std::string>();
      }

      Entity entity = m_Scene->CreateEntity(name);
      auto &world = m_Scene->GetWorld();

      // TransformComponent
      auto transformComponent = entityNode["TransformComponent"];
      if (transformComponent) {
        auto &tc = world.GetComponent<TransformComponent>(entity);
        tc.position = transformComponent["Position"].as<glm::vec3>();
        tc.rotation = transformComponent["Rotation"].as<glm::vec3>();
        tc.scale = transformComponent["Scale"].as<glm::vec3>();
      }

      // MeshComponent
      auto meshComponent = entityNode["MeshComponent"];
      if (meshComponent) {
        auto &mc = world.AddComponent<MeshComponent>(entity);
        mc.meshType =
            static_cast<MeshType>(meshComponent["MeshType"].as<int>());
        mc.modelPath = meshComponent["ModelPath"].as<std::string>();
        if (meshComponent["CastShadows"])
          mc.castShadows = meshComponent["CastShadows"].as<bool>();
        if (meshComponent["ReceiveShadows"])
          mc.receiveShadows = meshComponent["ReceiveShadows"].as<bool>();

        if (mc.meshType == MeshType::Custom && !mc.modelPath.empty()) {
          ModelCache::Get().Load(mc.modelPath);
        }
      }

      // MaterialComponent
      auto materialComponent = entityNode["MaterialComponent"];
      if (materialComponent) {
        auto &mat = world.AddComponent<MaterialComponent>(entity);
        mat.albedo = materialComponent["Albedo"].as<glm::vec3>();
        mat.metallic = materialComponent["Metallic"].as<float>();
        mat.roughness = materialComponent["Roughness"].as<float>();
        mat.ao = materialComponent["AO"].as<float>();
        mat.emissive = materialComponent["Emissive"].as<glm::vec3>();
        if (materialComponent["AlbedoTexturePath"])
          mat.albedoTexturePath =
              materialComponent["AlbedoTexturePath"].as<std::string>();
        if (materialComponent["NormalTexturePath"])
          mat.normalTexturePath =
              materialComponent["NormalTexturePath"].as<std::string>();
        if (materialComponent["MetallicTexturePath"])
          mat.metallicTexturePath =
              materialComponent["MetallicTexturePath"].as<std::string>();
        if (materialComponent["RoughnessTexturePath"])
          mat.roughnessTexturePath =
              materialComponent["RoughnessTexturePath"].as<std::string>();
        if (materialComponent["AOTexturePath"])
          mat.aoTexturePath =
              materialComponent["AOTexturePath"].as<std::string>();
      }

      // LightComponent
      auto lightComponent = entityNode["LightComponent"];
      if (lightComponent) {
        auto &lc = world.AddComponent<LightComponent>(entity);
        lc.type = lightComponent["Type"].as<int>();
        lc.color = lightComponent["Color"].as<glm::vec3>();
        lc.intensity = lightComponent["Intensity"].as<float>();
        lc.range = lightComponent["Range"].as<float>();
        lc.innerConeAngle = lightComponent["InnerConeAngle"].as<float>();
        lc.outerConeAngle = lightComponent["OuterConeAngle"].as<float>();
        lc.castShadows = lightComponent["CastShadows"].as<bool>();
      }

      // CameraComponent
      auto cameraComponent = entityNode["CameraComponent"];
      if (cameraComponent) {
        auto &cc = world.AddComponent<CameraComponent>(entity);
        cc.isPrimary = cameraComponent["IsPrimary"].as<bool>();
        cc.fov = cameraComponent["FOV"].as<float>();
        cc.nearClip = cameraComponent["NearClip"].as<float>();
        cc.farClip = cameraComponent["FarClip"].as<float>();
      }

      // SkyboxComponent
      auto skyboxComponent = entityNode["SkyboxComponent"];
      if (skyboxComponent) {
        auto &sb = world.AddComponent<SkyboxComponent>(entity);
        if (skyboxComponent["HDRPath"])
          sb.hdrPath = skyboxComponent["HDRPath"].as<std::string>();
        if (skyboxComponent["Intensity"])
          sb.intensity = skyboxComponent["Intensity"].as<float>();
        if (skyboxComponent["LOD"])
          sb.lod = skyboxComponent["LOD"].as<float>();
        if (skyboxComponent["UseHDR"])
          sb.useHDR = skyboxComponent["UseHDR"].as<bool>();
      }

      // AnimatorComponent3D
      auto animatorComponent = entityNode["AnimatorComponent3D"];
      if (animatorComponent) {
        auto &ac = world.AddComponent<AnimatorComponent3D>(entity);
        if (animatorComponent["AnimationPath"])
          ac.animationPath =
              animatorComponent["AnimationPath"].as<std::string>();
        if (animatorComponent["Playing"])
          ac.playing = animatorComponent["Playing"].as<bool>();
        if (animatorComponent["Speed"])
          ac.speed = animatorComponent["Speed"].as<float>();
        if (animatorComponent["Loop"])
          ac.loop = animatorComponent["Loop"].as<bool>();
      }
    }
  }

  GINI_INFO("Scene deserialized successfully: ", sceneName);
  return true;
}

} // namespace Gini
