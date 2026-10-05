#include "Window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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
