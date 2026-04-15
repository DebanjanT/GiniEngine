#pragma once

#include "Core/Types.h"
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations for Assimp types
struct aiNode;
struct aiScene;
struct aiAnimation;

namespace Gini {

struct BoneInfo {
  i32 id;
  Mat4 offsetMatrix;
};

struct KeyPosition {
  Vec3 position;
  f32 timeStamp;
};

struct KeyRotation {
  glm::quat orientation;
  f32 timeStamp;
};

struct KeyScale {
  Vec3 scale;
  f32 timeStamp;
};

class Bone {
public:
  Bone(const std::string &name, i32 id,
       const std::vector<KeyPosition> &positions,
       const std::vector<KeyRotation> &rotations,
       const std::vector<KeyScale> &scales);

  void Update(f32 animationTime);

  Mat4 GetLocalTransform() const { return m_LocalTransform; }
  const std::string &GetName() const { return m_Name; }
  i32 GetID() const { return m_ID; }

private:
  f32 GetScaleFactor(f32 lastTimeStamp, f32 nextTimeStamp, f32 animationTime);

  template <typename T>
  i32 GetIndex(const std::vector<T> &keys, f32 animationTime);

  Vec3 InterpolatePosition(f32 animationTime);
  glm::quat InterpolateRotation(f32 animationTime);
  Vec3 InterpolateScale(f32 animationTime);

  std::string m_Name;
  i32 m_ID;
  Mat4 m_LocalTransform = Mat4(1.0f);

  std::vector<KeyPosition> m_Positions;
  std::vector<KeyRotation> m_Rotations;
  std::vector<KeyScale> m_Scales;
};

struct AnimationNode {
  std::string name;
  Mat4 transformation;
  std::vector<AnimationNode> children;
};

class Animation {
public:
  Animation() = default;
  Animation(const std::string &filepath, class MeshSource *meshSource);

  Bone *FindBone(const std::string &name);

  f32 GetTicksPerSecond() const { return m_TicksPerSecond; }
  f32 GetDuration() const { return m_Duration; }
  const std::string &GetName() const { return m_Name; }
  const AnimationNode &GetRootNode() const { return m_RootNode; }
  const std::unordered_map<std::string, BoneInfo> &GetBoneInfoMap() const {
    return m_BoneInfoMap;
  }

  static Ref<Animation> Create(const std::string &filepath, class MeshSource *meshSource);

private:
  void ReadHierarchyData(AnimationNode &dest, const ::aiNode *src);
  void ReadMissingBones(const ::aiAnimation *animation, class MeshSource *meshSource);

  std::string m_Name;
  f32 m_Duration = 0.0f;
  f32 m_TicksPerSecond = 25.0f;

  std::vector<Bone> m_Bones;
  AnimationNode m_RootNode;
  std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
};

class Animator {
public:
  Animator() = default;
  Animator(Ref<Animation> animation);

  void UpdateAnimation(f32 deltaTime);
  void PlayAnimation(Ref<Animation> animation);
  void StopAnimation();
  void PauseAnimation();
  void ResumeAnimation();

  void SetSpeed(f32 speed) { m_Speed = speed; }
  void SetLooping(bool loop) { m_Looping = loop; }
  void SetCurrentTime(f32 time) { m_CurrentTime = time; }

  f32 GetCurrentTime() const { return m_CurrentTime; }
  f32 GetSpeed() const { return m_Speed; }
  bool IsPlaying() const { return m_Playing; }
  bool IsLooping() const { return m_Looping; }

  const std::vector<Mat4> &GetFinalBoneMatrices() const {
    return m_FinalBoneMatrices;
  }

  static constexpr u32 MAX_BONES = 100;

private:
  void CalculateBoneTransform(const AnimationNode *node,
                              const Mat4 &parentTransform);

  Ref<Animation> m_CurrentAnimation;
  std::vector<Mat4> m_FinalBoneMatrices;

  f32 m_CurrentTime = 0.0f;
  f32 m_Speed = 1.0f;
  bool m_Playing = false;
  bool m_Looping = true;
};

// Animation blending for smooth transitions
class AnimationBlender {
public:
  void SetAnimations(Ref<Animation> from, Ref<Animation> to);
  void SetBlendFactor(f32 factor) { m_BlendFactor = factor; }
  void SetBlendDuration(f32 duration) { m_BlendDuration = duration; }

  void Update(f32 deltaTime);
  const std::vector<Mat4> &GetBlendedMatrices() const {
    return m_BlendedMatrices;
  }

  bool IsBlending() const { return m_Blending; }

private:
  Ref<Animation> m_FromAnimation;
  Ref<Animation> m_ToAnimation;

  std::vector<Mat4> m_BlendedMatrices;

  f32 m_BlendFactor = 0.0f;
  f32 m_BlendDuration = 0.3f;
  f32 m_BlendTime = 0.0f;
  bool m_Blending = false;
};

} // namespace Gini
