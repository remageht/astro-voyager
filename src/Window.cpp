#include "Window.h"

#include "GLCompat.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <stdexcept>
#include <iostream>

namespace astro {

Window::Window(int width, int height, const char* title, bool visible) {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_window) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  // Window/taskbar icon. Windows takes the icon resource already embedded in
  // the exe (packaging/app.ico, resource id 1), so the window icon matches the
  // file icon exactly. Other platforms load the AppImage/desktop icon from
  // res/icon.png; a missing file degrades to the toolkit default.
#ifdef _WIN32
  {
    HMODULE module = GetModuleHandleW(nullptr);
    HICON big = static_cast<HICON>(LoadImageW(
        module, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    HICON small = static_cast<HICON>(LoadImageW(
        module, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    HWND hwnd = glfwGetWin32Window(m_window);
    if (hwnd && big) {
      SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big));
      m_iconBig = big;
    }
    if (hwnd && small) {
      SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small));
      m_iconSmall = small;
    }
  }
#else
  {
    int iconW = 0, iconH = 0, channels = 0;
    unsigned char* pixels =
        stbi_load("res/icon.png", &iconW, &iconH, &channels, 4);
    if (pixels) {
      GLFWimage image{iconW, iconH, pixels};
      glfwSetWindowIcon(m_window, 1, &image);
      stbi_image_free(pixels);
    }
  }
#endif

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(1);

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK) {
    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_window = nullptr;
    throw std::runtime_error(std::string("Failed to initialize GLEW: ") +
                             reinterpret_cast<const char*>(glewGetErrorString(err)));
  }

  // Clear any benign GLEW error
  glGetError();

  glViewport(0, 0, width, height);
}

Window::~Window() {
  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
#ifdef _WIN32
  if (m_iconBig) {
    DestroyIcon(static_cast<HICON>(m_iconBig));
    m_iconBig = nullptr;
  }
  if (m_iconSmall) {
    DestroyIcon(static_cast<HICON>(m_iconSmall));
    m_iconSmall = nullptr;
  }
#endif
  glfwTerminate();
}

bool Window::shouldClose() const {
  return m_window ? glfwWindowShouldClose(m_window) : true;
}

void Window::swapBuffers() const {
  if (m_window) {
    glfwSwapBuffers(m_window);
  }
}

void Window::pollEvents() const {
  glfwPollEvents();
}

void Window::getFramebufferSize(int& width, int& height) const {
  if (m_window) {
    glfwGetFramebufferSize(m_window, &width, &height);
  } else {
    width = 0;
    height = 0;
  }
}

}  // namespace astro
