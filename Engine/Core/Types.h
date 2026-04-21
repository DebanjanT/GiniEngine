#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// Bit manipulation macro
#define BIT(x) (1u << (x))

namespace Gini {

// Type aliases
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

// Smart pointer aliases
template <typename T> using Scope = std::unique_ptr<T>;

template <typename T, typename... Args>
constexpr Scope<T> CreateScope(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T> using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
constexpr Ref<T> CreateRef(Args &&...args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

// Math types (prefixed with G to avoid conflicts with ImGui types)
using GVec2 = glm::vec2;
using GVec3 = glm::vec3;
using GVec4 = glm::vec4;
using GMat3 = glm::mat3;
using GMat4 = glm::mat4;
using GIVec2 = glm::ivec2;
using GIVec3 = glm::ivec3;
using GIVec4 = glm::ivec4;

// Backwards compatibility aliases (deprecated - use G-prefixed versions)
using Vec2 = GVec2;
using Vec3 = GVec3;
using Vec4 = GVec4;
using Mat3 = GMat3;
using Mat4 = GMat4;
using IVec2 = GIVec2;
using IVec3 = GIVec3;
using IVec4 = GIVec4;

// Entity type from EnTT
using Entity = entt::entity;
constexpr Entity NullEntity = entt::null;

// Asset Handle type
using AssetHandle = u64;
constexpr AssetHandle NullAssetHandle = 0;

// Color
struct Color {
  f32 r = 1.0f;
  f32 g = 1.0f;
  f32 b = 1.0f;
  f32 a = 1.0f;

  Color() = default;
  Color(f32 r, f32 g, f32 b, f32 a = 1.0f) : r(r), g(g), b(b), a(a) {}

  static Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
  static Color Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
  static Color Red() { return {1.0f, 0.0f, 0.0f, 1.0f}; }
  static Color Green() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
  static Color Blue() { return {0.0f, 0.0f, 1.0f, 1.0f}; }
  static Color Yellow() { return {1.0f, 1.0f, 0.0f, 1.0f}; }
  static Color Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

  GVec4 ToVec4() const { return {r, g, b, a}; }
};

// Rectangle
struct Rect {
  f32 x = 0.0f;
  f32 y = 0.0f;
  f32 width = 0.0f;
  f32 height = 0.0f;

  Rect() = default;
  Rect(f32 x, f32 y, f32 w, f32 h) : x(x), y(y), width(w), height(h) {}

  bool Contains(Vec2 point) const {
    return point.x >= x && point.x <= x + width && point.y >= y &&
           point.y <= y + height;
  }

  bool Intersects(const Rect &other) const {
    return x < other.x + other.width && x + width > other.x &&
           y < other.y + other.height && y + height > other.y;
  }

  Vec2 Center() const { return {x + width * 0.5f, y + height * 0.5f}; }
};

// Integer Rectangle (for tile coordinates)
struct IRect {
  i32 x = 0;
  i32 y = 0;
  i32 width = 0;
  i32 height = 0;

  IRect() = default;
  IRect(i32 x, i32 y, i32 w, i32 h) : x(x), y(y), width(w), height(h) {}
};

// Result type for error handling
template <typename T> struct Result {
  T value;
  bool success = false;
  std::string error;

  static Result<T> Ok(T val) {
    Result<T> r;
    r.value = std::move(val);
    r.success = true;
    return r;
  }

  static Result<T> Err(const std::string &err) {
    Result<T> r;
    r.success = false;
    r.error = err;
    return r;
  }

  operator bool() const { return success; }
};

// Engine configuration
struct EngineConfig {
  std::string windowTitle = "Gini Engine";
  u32 windowWidth = 1280;
  u32 windowHeight = 720;
  bool vsync = true;
  bool fullscreen = false;
  bool resizable = true;
  u32 targetFPS = 60;
  std::string assetPath = "Assets/";
  bool enableRenderThread = false;
  bool enableAssetLoadingThread = true;
  bool enableNetworkThread = false;
};

} // namespace Gini
