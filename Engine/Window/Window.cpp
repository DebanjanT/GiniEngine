#include "Window.h"
#include "Core/Input.h"
#include "Core/Logger.h"

// Prevent GLFW from including OpenGL headers - we use GLAD
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

namespace Gini {

static bool s_GLFWInitialized = false;
static u32 s_WindowCount = 0;

static void GLFWErrorCallback(int error, const char *description) {
  GINI_ERROR("GLFW Error (", error, "): ", description);
}

Window::Window(const WindowProps &props) { Init(props); }

Window::~Window() { Shutdown(); }

void Window::Init(const WindowProps &props) {
  m_Data.title = props.title;
  m_Data.width = props.width;
  m_Data.height = props.height;
  m_Data.vsync = props.vsync;
  m_Data.fullscreen = props.fullscreen;

  GINI_INFO("Creating window: ", props.title, " (", props.width, "x",
            props.height, ")");

  if (!s_GLFWInitialized) {
    int success = glfwInit();
    GINI_ASSERT(success, "Failed to initialize GLFW!");
    glfwSetErrorCallback(GLFWErrorCallback);
    s_GLFWInitialized = true;
  }

  // OpenGL hints
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_RESIZABLE, props.resizable ? GLFW_TRUE : GLFW_FALSE);

  GLFWmonitor *monitor = nullptr;
  if (props.fullscreen) {
    monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    m_Data.width = mode->width;
    m_Data.height = mode->height;
  }

  m_Window = glfwCreateWindow(static_cast<int>(m_Data.width),
                              static_cast<int>(m_Data.height),
                              m_Data.title.c_str(), monitor, nullptr);
  GINI_ASSERT(m_Window, "Failed to create GLFW window!");

  s_WindowCount++;

  glfwMakeContextCurrent(m_Window);

  // Initialize GLAD (GLAD2 API)
  int gladStatus = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
  GINI_ASSERT(gladStatus, "Failed to initialize GLAD!");

  GINI_INFO("OpenGL Info:");
  GINI_INFO("  Vendor: ",
            reinterpret_cast<const char *>(glGetString(GL_VENDOR)));
  GINI_INFO("  Renderer: ",
            reinterpret_cast<const char *>(glGetString(GL_RENDERER)));
  GINI_INFO("  Version: ",
            reinterpret_cast<const char *>(glGetString(GL_VERSION)));

  glfwSetWindowUserPointer(m_Window, &m_Data);
  SetVSync(props.vsync);

  // Store windowed position/size for fullscreen toggle
  glfwGetWindowPos(m_Window, &m_WindowedX, &m_WindowedY);
  m_WindowedWidth = props.width;
  m_WindowedHeight = props.height;

  SetupCallbacks();
}

void Window::SetupCallbacks() {
  glfwSetWindowSizeCallback(
      m_Window, [](GLFWwindow *window, int width, int height) {
        WindowData &data =
            *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        data.width = width;
        data.height = height;

        WindowResizeEvent event(width, height);
        if (data.eventCallback)
          data.eventCallback(event);
      });

  glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window) {
    WindowData &data =
        *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
    WindowCloseEvent event;
    if (data.eventCallback)
      data.eventCallback(event);
  });

  glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode,
                                  int action, int mods) {
    WindowData &data =
        *static_cast<WindowData *>(glfwGetWindowUserPointer(window));

    Input::Get().SetModifiers(static_cast<KeyMod>(mods));

    switch (action) {
    case GLFW_PRESS: {
      Input::Get().SetKeyState(static_cast<Key>(key), true);
      KeyPressedEvent event(key, false);
      if (data.eventCallback)
        data.eventCallback(event);
      break;
    }
    case GLFW_RELEASE: {
      Input::Get().SetKeyState(static_cast<Key>(key), false);
      KeyReleasedEvent event(key);
      if (data.eventCallback)
        data.eventCallback(event);
      break;
    }
    case GLFW_REPEAT: {
      KeyPressedEvent event(key, true);
      if (data.eventCallback)
        data.eventCallback(event);
      break;
    }
    }
  });

  glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button,
                                          int action, int mods) {
    WindowData &data =
        *static_cast<WindowData *>(glfwGetWindowUserPointer(window));

    switch (action) {
    case GLFW_PRESS: {
      Input::Get().SetMouseButtonState(static_cast<MouseButton>(button), true);
      MouseButtonPressedEvent event(button);
      if (data.eventCallback)
        data.eventCallback(event);
      break;
    }
    case GLFW_RELEASE: {
      Input::Get().SetMouseButtonState(static_cast<MouseButton>(button), false);
      MouseButtonReleasedEvent event(button);
      if (data.eventCallback)
        data.eventCallback(event);
      break;
    }
    }
  });

  glfwSetScrollCallback(
      m_Window, [](GLFWwindow *window, double xOffset, double yOffset) {
        WindowData &data =
            *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        Input::Get().SetScrollDelta(static_cast<f32>(xOffset),
                                    static_cast<f32>(yOffset));

        MouseScrolledEvent event(static_cast<f32>(xOffset),
                                 static_cast<f32>(yOffset));
        if (data.eventCallback)
          data.eventCallback(event);
      });

  glfwSetCursorPosCallback(
      m_Window, [](GLFWwindow *window, double xPos, double yPos) {
        WindowData &data =
            *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
        Input::Get().SetMousePosition(static_cast<f32>(xPos),
                                      static_cast<f32>(yPos));

        MouseMovedEvent event(static_cast<f32>(xPos), static_cast<f32>(yPos));
        if (data.eventCallback)
          data.eventCallback(event);
      });
}

void Window::Shutdown() {
  if (m_Window) {
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;
    s_WindowCount--;

    if (s_WindowCount == 0) {
      glfwTerminate();
      s_GLFWInitialized = false;
    }
  }
}

void Window::Update() {
  glfwPollEvents();
  Input::Get().Update();
}

void Window::SwapBuffers() { glfwSwapBuffers(m_Window); }

void Window::MakeContextCurrent() { glfwMakeContextCurrent(m_Window); }

void Window::DetachContext() { glfwMakeContextCurrent(nullptr); }

bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); }

void Window::Close() { glfwSetWindowShouldClose(m_Window, GLFW_TRUE); }

void Window::SetVSync(bool enabled) {
  glfwSwapInterval(enabled ? 1 : 0);
  m_Data.vsync = enabled;
}

void Window::SetTitle(const std::string &title) {
  m_Data.title = title;
  glfwSetWindowTitle(m_Window, title.c_str());
}

void Window::SetFullscreen(bool fullscreen) {
  if (m_Data.fullscreen == fullscreen)
    return;

  if (fullscreen) {
    glfwGetWindowPos(m_Window, &m_WindowedX, &m_WindowedY);
    m_WindowedWidth = m_Data.width;
    m_WindowedHeight = m_Data.height;

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height,
                         mode->refreshRate);
  } else {
    glfwSetWindowMonitor(m_Window, nullptr, m_WindowedX, m_WindowedY,
                         m_WindowedWidth, m_WindowedHeight, 0);
  }

  m_Data.fullscreen = fullscreen;
  SetVSync(m_Data.vsync);
}

void Window::Maximize() { glfwMaximizeWindow(m_Window); }

void Window::Minimize() { glfwIconifyWindow(m_Window); }

void Window::Restore() { glfwRestoreWindow(m_Window); }

void Window::Focus() { glfwFocusWindow(m_Window); }

void Window::SetSize(u32 width, u32 height) {
  glfwSetWindowSize(m_Window, static_cast<int>(width),
                    static_cast<int>(height));
}

void Window::SetPosition(i32 x, i32 y) { glfwSetWindowPos(m_Window, x, y); }

Vec2 Window::GetPosition() const {
  int x, y;
  glfwGetWindowPos(m_Window, &x, &y);
  return Vec2(static_cast<f32>(x), static_cast<f32>(y));
}

void Window::SetCursorMode(bool visible, bool locked) {
  if (locked) {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  } else if (!visible) {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  } else {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}

void Window::SetCursor(i32 cursorType) {
  GLFWcursor *cursor = glfwCreateStandardCursor(cursorType);
  glfwSetCursor(m_Window, cursor);
}

Scope<Window> Window::Create(const WindowProps &props) {
  return CreateScope<Window>(props);
}

} // namespace Gini
