#pragma once

#include "Core/Event.h"
#include "Core/Types.h"
#include "Renderer/Camera3D.h"
#include "Renderer/Framebuffer.h"
#include "Scene/Scene.h"

#include <string>

namespace Gini {

class RuntimeLayer {
public:
  RuntimeLayer();
  ~RuntimeLayer();

  void OnAttach();
  void OnDetach();
  void OnUpdate(f32 deltaTime);
  void OnRender();
  void OnEvent(Event &event);

  void LoadScene(const std::string &scenePath);
  void SetScene(Ref<Scene> scene);

private:
  void OnScenePlay();
  void OnSceneStop();

private:
  Ref<Scene> m_RuntimeScene;
  Ref<Framebuffer> m_Framebuffer;
  Ref<Camera3D> m_Camera;

  bool m_IsPlaying = false;
  u32 m_ViewportWidth = 1280;
  u32 m_ViewportHeight = 720;
};

} // namespace Gini
