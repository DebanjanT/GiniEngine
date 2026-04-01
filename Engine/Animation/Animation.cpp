#include "Animation.h"
#include "Core/Logger.h"
#include "Renderer/Model.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Gini {

// Bone implementation
Bone::Bone(const std::string &name, i32 id,
           const std::vector<KeyPosition> &positions,
           const std::vector<KeyRotation> &rotations,
           const std::vector<KeyScale> &scales)
    : m_Name(name), m_ID(id), m_Positions(positions), m_Rotations(rotations),
      m_Scales(scales) {}

void Bone::Update(f32 animationTime) {
  Mat4 translation =
      glm::translate(Mat4(1.0f), InterpolatePosition(animationTime));
  Mat4 rotation = glm::toMat4(InterpolateRotation(animationTime));
  Mat4 scale = glm::scale(Mat4(1.0f), InterpolateScale(animationTime));
  m_LocalTransform = translation * rotation * scale;
}

f32 Bone::GetScaleFactor(f32 lastTimeStamp, f32 nextTimeStamp,
                         f32 animationTime) {
  f32 midWayLength = animationTime - lastTimeStamp;
  f32 framesDiff = nextTimeStamp - lastTimeStamp;
  return midWayLength / framesDiff;
}

template <typename T>
i32 Bone::GetIndex(const std::vector<T> &keys, f32 animationTime) {
  for (i32 i = 0; i < static_cast<i32>(keys.size()) - 1; i++) {
    if (animationTime < keys[i + 1].timeStamp) {
      return i;
    }
  }
  return 0;
}

Vec3 Bone::InterpolatePosition(f32 animationTime) {
  if (m_Positions.size() == 1) {
    return m_Positions[0].position;
  }

  i32 p0Index = GetIndex(m_Positions, animationTime);
  i32 p1Index = p0Index + 1;

  f32 scaleFactor =
      GetScaleFactor(m_Positions[p0Index].timeStamp,
                     m_Positions[p1Index].timeStamp, animationTime);

  return glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position,
                  scaleFactor);
}

glm::quat Bone::InterpolateRotation(f32 animationTime) {
  if (m_Rotations.size() == 1) {
    return glm::normalize(m_Rotations[0].orientation);
  }

  i32 p0Index = GetIndex(m_Rotations, animationTime);
  i32 p1Index = p0Index + 1;

  f32 scaleFactor =
      GetScaleFactor(m_Rotations[p0Index].timeStamp,
                     m_Rotations[p1Index].timeStamp, animationTime);

  glm::quat finalRotation =
      glm::slerp(m_Rotations[p0Index].orientation,
                 m_Rotations[p1Index].orientation, scaleFactor);
  return glm::normalize(finalRotation);
}

Vec3 Bone::InterpolateScale(f32 animationTime) {
  if (m_Scales.size() == 1) {
    return m_Scales[0].scale;
  }

  i32 p0Index = GetIndex(m_Scales, animationTime);
  i32 p1Index = p0Index + 1;

  f32 scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
                                   m_Scales[p1Index].timeStamp, animationTime);

  return glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale,
                  scaleFactor);
}

// Animation implementation
Animation::Animation(const std::string &filepath, Model *model) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate);

  if (!scene || !scene->mRootNode || !scene->mNumAnimations) {
    GINI_ERROR("Failed to load animation: ", filepath);
    return;
  }

  aiAnimation *animation = scene->mAnimations[0];
  m_Duration = static_cast<f32>(animation->mDuration);
  m_TicksPerSecond = static_cast<f32>(animation->mTicksPerSecond);
  m_Name = animation->mName.C_Str();

  ReadHierarchyData(m_RootNode, scene->mRootNode);
  ReadMissingBones(animation, model);

  GINI_INFO("Loaded animation: ", m_Name, " (", m_Duration, " ticks, ",
            m_Bones.size(), " bones)");
}

Ref<Animation> Animation::Create(const std::string &filepath, Model *model) {
  return CreateRef<Animation>(filepath, model);
}

Bone *Animation::FindBone(const std::string &name) {
  for (auto &bone : m_Bones) {
    if (bone.GetName() == name) {
      return &bone;
    }
  }
  return nullptr;
}

void Animation::ReadHierarchyData(AnimationNode &dest, const aiNode *src) {
  dest.name = src->mName.C_Str();

  // Convert aiMatrix4x4 to glm::mat4
  aiMatrix4x4 m = src->mTransformation;
  dest.transformation = Mat4(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2,
                             m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);

  for (u32 i = 0; i < src->mNumChildren; i++) {
    AnimationNode child;
    ReadHierarchyData(child, src->mChildren[i]);
    dest.children.push_back(child);
  }
}

void Animation::ReadMissingBones(const aiAnimation *animation, Model *model) {
  i32 boneCount = 0;

  for (u32 i = 0; i < animation->mNumChannels; i++) {
    aiNodeAnim *channel = animation->mChannels[i];
    std::string boneName = channel->mNodeName.C_Str();

    if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
      BoneInfo info;
      info.id = boneCount++;
      info.offsetMatrix = Mat4(1.0f);
      m_BoneInfoMap[boneName] = info;
    }

    // Extract keyframes
    std::vector<KeyPosition> positions;
    for (u32 j = 0; j < channel->mNumPositionKeys; j++) {
      KeyPosition key;
      key.position = Vec3(channel->mPositionKeys[j].mValue.x,
                          channel->mPositionKeys[j].mValue.y,
                          channel->mPositionKeys[j].mValue.z);
      key.timeStamp = static_cast<f32>(channel->mPositionKeys[j].mTime);
      positions.push_back(key);
    }

    std::vector<KeyRotation> rotations;
    for (u32 j = 0; j < channel->mNumRotationKeys; j++) {
      KeyRotation key;
      key.orientation = glm::quat(channel->mRotationKeys[j].mValue.w,
                                  channel->mRotationKeys[j].mValue.x,
                                  channel->mRotationKeys[j].mValue.y,
                                  channel->mRotationKeys[j].mValue.z);
      key.timeStamp = static_cast<f32>(channel->mRotationKeys[j].mTime);
      rotations.push_back(key);
    }

    std::vector<KeyScale> scales;
    for (u32 j = 0; j < channel->mNumScalingKeys; j++) {
      KeyScale key;
      key.scale = Vec3(channel->mScalingKeys[j].mValue.x,
                       channel->mScalingKeys[j].mValue.y,
                       channel->mScalingKeys[j].mValue.z);
      key.timeStamp = static_cast<f32>(channel->mScalingKeys[j].mTime);
      scales.push_back(key);
    }

    m_Bones.emplace_back(boneName, m_BoneInfoMap[boneName].id, positions,
                         rotations, scales);
  }
}

// Animator implementation
Animator::Animator(Ref<Animation> animation) : m_CurrentAnimation(animation) {
  m_FinalBoneMatrices.resize(MAX_BONES, Mat4(1.0f));
}

void Animator::UpdateAnimation(f32 deltaTime) {
  if (!m_Playing || !m_CurrentAnimation)
    return;

  m_CurrentTime +=
      m_CurrentAnimation->GetTicksPerSecond() * deltaTime * m_Speed;

  if (m_CurrentTime >= m_CurrentAnimation->GetDuration()) {
    if (m_Looping) {
      m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
    } else {
      m_CurrentTime = m_CurrentAnimation->GetDuration();
      m_Playing = false;
    }
  }

  CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), Mat4(1.0f));
}

void Animator::PlayAnimation(Ref<Animation> animation) {
  m_CurrentAnimation = animation;
  m_CurrentTime = 0.0f;
  m_Playing = true;
  m_FinalBoneMatrices.resize(MAX_BONES, Mat4(1.0f));
}

void Animator::StopAnimation() {
  m_Playing = false;
  m_CurrentTime = 0.0f;
}

void Animator::PauseAnimation() { m_Playing = false; }

void Animator::ResumeAnimation() { m_Playing = true; }

void Animator::CalculateBoneTransform(const AnimationNode *node,
                                      const Mat4 &parentTransform) {
  if (!node || !m_CurrentAnimation)
    return;

  std::string nodeName = node->name;
  Mat4 nodeTransform = node->transformation;

  Bone *bone = m_CurrentAnimation->FindBone(nodeName);
  if (bone) {
    bone->Update(m_CurrentTime);
    nodeTransform = bone->GetLocalTransform();
  }

  Mat4 globalTransform = parentTransform * nodeTransform;

  const auto &boneInfoMap = m_CurrentAnimation->GetBoneInfoMap();
  auto it = boneInfoMap.find(nodeName);
  if (it != boneInfoMap.end()) {
    i32 index = it->second.id;
    if (index >= 0 && index < static_cast<i32>(m_FinalBoneMatrices.size())) {
      m_FinalBoneMatrices[index] = globalTransform * it->second.offsetMatrix;
    }
  }

  for (const auto &child : node->children) {
    CalculateBoneTransform(&child, globalTransform);
  }
}

// AnimationBlender implementation
void AnimationBlender::SetAnimations(Ref<Animation> from, Ref<Animation> to) {
  m_FromAnimation = from;
  m_ToAnimation = to;
  m_BlendTime = 0.0f;
  m_Blending = true;
}

void AnimationBlender::Update(f32 deltaTime) {
  if (!m_Blending)
    return;

  m_BlendTime += deltaTime;
  m_BlendFactor = glm::clamp(m_BlendTime / m_BlendDuration, 0.0f, 1.0f);

  if (m_BlendFactor >= 1.0f) {
    m_Blending = false;
  }

  // TODO: Implement actual bone matrix blending between animations
}

} // namespace Gini
