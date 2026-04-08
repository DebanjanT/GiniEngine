#include "Texture.h"
#include "Core/Logger.h"

#include <algorithm>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

namespace Gini {

Texture2D::Texture2D(u32 width, u32 height) : m_Width(width), m_Height(height) {
  m_InternalFormat = GL_RGBA8;
  m_DataFormat = GL_RGBA;

  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0,
               m_DataFormat, GL_UNSIGNED_BYTE, nullptr);
}

Texture2D::Texture2D(const std::string &path) : m_Path(path) {
  int width, height, channels;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc *data = stbi_load(path.c_str(), &width, &height, &channels, 0);

  if (!data) {
    GINI_ERROR("Failed to load texture: ", path);
    return;
  }

  m_Width = width;
  m_Height = height;

  if (channels == 4) {
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;
  } else if (channels == 3) {
    m_InternalFormat = GL_RGB8;
    m_DataFormat = GL_RGB;
  } else if (channels == 1) {
    m_InternalFormat = GL_R8;
    m_DataFormat = GL_RED;
  }

  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  GLfloat maxAniso = 1.0f;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
  if (maxAniso > 1.0f)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(maxAniso, 16.0f));

  glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0,
               m_DataFormat, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  GINI_DEBUG("Loaded texture: ", path, " (", width, "x", height, ", ", channels,
             " channels)");
}

Texture2D::Texture2D(unsigned char *data, int width, int height, int channels)
    : m_Width(static_cast<u32>(width)), m_Height(static_cast<u32>(height)),
      m_Path("<embedded>") {
  if (!data || channels < 1 || channels > 4) {
    if (data)
      stbi_image_free(data);
    GINI_ERROR("Texture2D: invalid embedded image data");
    return;
  }

  if (channels == 4) {
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;
  } else if (channels == 3) {
    m_InternalFormat = GL_RGB8;
    m_DataFormat = GL_RGB;
  } else {
    m_InternalFormat = GL_R8;
    m_DataFormat = GL_RED;
  }

  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  GLfloat maxAniso = 1.0f;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
  if (maxAniso > 1.0f)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
                    std::min(maxAniso, 16.0f));

  glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0,
               m_DataFormat, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);
}

Texture2D::Texture2D(const unsigned char *bgra, u32 width, u32 height)
    : m_Width(width), m_Height(height), m_Path("<embedded-bgra>") {
  m_InternalFormat = GL_RGBA8;
  m_DataFormat = GL_BGRA;

  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  GLfloat maxAniso = 1.0f;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
  if (maxAniso > 1.0f)
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
                    std::min(maxAniso, 16.0f));

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<i32>(width),
               static_cast<i32>(height), 0, GL_BGRA, GL_UNSIGNED_BYTE, bgra);
  glGenerateMipmap(GL_TEXTURE_2D);
}

Ref<Texture2D> Texture2D::CreateFromMemory(const unsigned char *buffer,
                                           size_t length) {
  if (!buffer || length == 0)
    return nullptr;

  int width, height, channels;
  stbi_set_flip_vertically_on_load(1);
  unsigned char *data = stbi_load_from_memory(
      buffer, static_cast<int>(length), &width, &height, &channels, 0);
  if (!data) {
    GINI_ERROR("CreateFromMemory: stbi could not decode embedded image");
    return nullptr;
  }
  return Ref<Texture2D>(new Texture2D(data, width, height, channels));
}

Ref<Texture2D> Texture2D::CreateFromBGRA(const unsigned char *bgra, u32 width,
                                         u32 height) {
  if (!bgra || width == 0 || height == 0)
    return nullptr;
  return Ref<Texture2D>(new Texture2D(bgra, width, height));
}

Texture2D::~Texture2D() { glDeleteTextures(1, &m_RendererID); }

void Texture2D::SetData(void *data, u32 size) {
  u32 bpp = m_DataFormat == GL_RGBA ? 4 : 3;
  GINI_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
  glBindTexture(GL_TEXTURE_2D, m_RendererID);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat,
                  GL_UNSIGNED_BYTE, data);
}

void Texture2D::Bind(u32 slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void Texture2D::Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

Ref<Texture2D> Texture2D::Create(u32 width, u32 height) {
  return CreateRef<Texture2D>(width, height);
}

Ref<Texture2D> Texture2D::Create(const std::string &path) {
  return CreateRef<Texture2D>(path);
}

// TextureAtlas
TextureAtlas::TextureAtlas(Ref<Texture2D> texture, u32 tileWidth,
                           u32 tileHeight)
    : m_Texture(texture), m_TileWidth(tileWidth), m_TileHeight(tileHeight) {
  m_TilesPerRow = texture->GetWidth() / tileWidth;
  m_TilesPerColumn = texture->GetHeight() / tileHeight;
}

Rect TextureAtlas::GetTileUV(u32 tileIndex) const {
  u32 col = tileIndex % m_TilesPerRow;
  u32 row = tileIndex / m_TilesPerRow;
  return GetTileUV(col, row);
}

Rect TextureAtlas::GetTileUV(u32 col, u32 row) const {
  f32 texWidth = static_cast<f32>(m_Texture->GetWidth());
  f32 texHeight = static_cast<f32>(m_Texture->GetHeight());

  f32 u = (col * m_TileWidth) / texWidth;
  f32 v = (row * m_TileHeight) / texHeight;
  f32 uWidth = m_TileWidth / texWidth;
  f32 vHeight = m_TileHeight / texHeight;

  return Rect(u, v, uWidth, vHeight);
}

// TextureCube implementation
TextureCube::TextureCube(const std::vector<std::string> &facePaths) {
  if (facePaths.size() != 6) {
    GINI_ERROR("TextureCube requires exactly 6 face images");
    return;
  }

  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

  stbi_set_flip_vertically_on_load(0); // Don't flip cubemap faces

  for (u32 i = 0; i < 6; i++) {
    int width, height, channels;
    stbi_uc *data =
        stbi_load(facePaths[i].c_str(), &width, &height, &channels, 0);

    if (data) {
      if (i == 0)
        m_Size = width;

      GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
      GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;

      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width,
                   height, 0, format, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      GINI_ERROR("Failed to load cubemap face: ", facePaths[i]);
    }
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  m_IsHDR = false;
  GINI_DEBUG("Loaded cubemap from 6 faces, size: ", m_Size);
}

TextureCube::TextureCube(const std::string &hdrPath, bool isHDR)
    : m_IsHDR(isHDR) {
  if (isHDR) {
    CreateFromEquirectangular(hdrPath);
  } else {
    GINI_ERROR("Non-HDR single image cubemap not supported, use 6 faces");
  }
}

TextureCube::TextureCube(u32 size, bool hdr) : m_Size(size), m_IsHDR(hdr) {
  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

  GLenum internalFormat = hdr ? GL_RGB16F : GL_RGB8;
  GLenum format = GL_RGB;
  GLenum type = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;

  for (u32 i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, size,
                 size, 0, format, type, nullptr);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

TextureCube::~TextureCube() { glDeleteTextures(1, &m_RendererID); }

void TextureCube::CreateFromEquirectangular(const std::string &hdrPath) {
  // Load HDR image
  stbi_set_flip_vertically_on_load(1);
  int width, height, channels;
  float *data = stbi_loadf(hdrPath.c_str(), &width, &height, &channels, 0);

  if (!data) {
    GINI_ERROR("Failed to load HDR image: ", hdrPath);
    return;
  }

  GINI_DEBUG("Loaded HDR image: ", hdrPath, " (", width, "x", height, ")");

  // Create HDR texture from equirectangular image
  u32 hdrTexture;
  glGenTextures(1, &hdrTexture);
  glBindTexture(GL_TEXTURE_2D, hdrTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT,
               data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  stbi_image_free(data);

  // Create cubemap
  m_Size = 1024; // Default cubemap resolution
  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);

  for (u32 i = 0; i < 6; i++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, m_Size,
                 m_Size, 0, GL_RGB, GL_FLOAT, nullptr);
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // Create framebuffer for rendering to cubemap
  u32 captureFBO, captureRBO;
  glGenFramebuffers(1, &captureFBO);
  glGenRenderbuffers(1, &captureRBO);

  glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
  glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_Size, m_Size);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, captureRBO);

  // Equirectangular to cubemap shader
  const char *vertexShader = R"(
        #version 410 core
        layout (location = 0) in vec3 a_Position;
        out vec3 v_LocalPos;
        uniform mat4 u_Projection;
        uniform mat4 u_View;
        void main() {
            v_LocalPos = a_Position;
            gl_Position = u_Projection * u_View * vec4(a_Position, 1.0);
        }
    )";

  const char *fragmentShader = R"(
        #version 410 core
        out vec4 FragColor;
        in vec3 v_LocalPos;
        uniform sampler2D u_EquirectangularMap;
        const vec2 invAtan = vec2(0.1591, 0.3183);
        vec2 SampleSphericalMap(vec3 v) {
            vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
            uv *= invAtan;
            uv += 0.5;
            return uv;
        }
        void main() {
            vec2 uv = SampleSphericalMap(normalize(v_LocalPos));
            vec3 color = texture(u_EquirectangularMap, uv).rgb;
            FragColor = vec4(color, 1.0);
        }
    )";

  // Compile shader
  u32 vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertexShader, nullptr);
  glCompileShader(vs);

  u32 fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragmentShader, nullptr);
  glCompileShader(fs);

  u32 program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  // Create cube VAO for rendering
  float cubeVertices[] = {
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

  u32 cubeVAO, cubeVBO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glBindVertexArray(cubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  // Capture projection and view matrices for each face
  glm::mat4 captureProjection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  glm::mat4 captureViews[] = {
      glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f))};

  // Render to cubemap
  glUseProgram(program);
  glUniformMatrix4fv(glGetUniformLocation(program, "u_Projection"), 1, GL_FALSE,
                     glm::value_ptr(captureProjection));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hdrTexture);
  glUniform1i(glGetUniformLocation(program, "u_EquirectangularMap"), 0);

  glViewport(0, 0, m_Size, m_Size);
  glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
  glDisable(GL_CULL_FACE);

  for (u32 i = 0; i < 6; i++) {
    glUniformMatrix4fv(glGetUniformLocation(program, "u_View"), 1, GL_FALSE,
                       glm::value_ptr(captureViews[i]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_RendererID, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
  }

  glEnable(GL_CULL_FACE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Generate mipmaps
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  // Cleanup
  glDeleteTextures(1, &hdrTexture);
  glDeleteFramebuffers(1, &captureFBO);
  glDeleteRenderbuffers(1, &captureRBO);
  glDeleteVertexArrays(1, &cubeVAO);
  glDeleteBuffers(1, &cubeVBO);
  glDeleteProgram(program);

  m_IsHDR = true;
  GINI_DEBUG("Created HDR cubemap from equirectangular image, size: ", m_Size);
}

void TextureCube::Bind(u32 slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID);
}

void TextureCube::Unbind() const { glBindTexture(GL_TEXTURE_CUBE_MAP, 0); }

Ref<TextureCube>
TextureCube::Create(const std::vector<std::string> &facePaths) {
  return CreateRef<TextureCube>(facePaths);
}

Ref<TextureCube> TextureCube::CreateFromHDR(const std::string &hdrPath) {
  return CreateRef<TextureCube>(hdrPath, true);
}

Ref<TextureCube> TextureCube::Create(u32 size, bool hdr) {
  return CreateRef<TextureCube>(size, hdr);
}

// TextureHDR implementation
TextureHDR::TextureHDR(const std::string &path) : m_Path(path) {
  stbi_set_flip_vertically_on_load(1);
  int width, height, channels;
  float *data = stbi_loadf(path.c_str(), &width, &height, &channels, 0);

  if (!data) {
    GINI_ERROR("Failed to load HDR texture: ", path);
    return;
  }

  m_Width = width;
  m_Height = height;

  glGenTextures(1, &m_RendererID);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT,
               data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  stbi_image_free(data);
  GINI_DEBUG("Loaded HDR texture: ", path, " (", width, "x", height, ")");
}

TextureHDR::~TextureHDR() { glDeleteTextures(1, &m_RendererID); }

void TextureHDR::Bind(u32 slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void TextureHDR::Unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

Ref<TextureHDR> TextureHDR::Create(const std::string &path) {
  return CreateRef<TextureHDR>(path);
}

} // namespace Gini
