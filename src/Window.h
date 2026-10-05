#pragma once

struct GLFWwindow;

namespace astro {

class Window {
 public:
  Window(int width, int height, const char* title, bool visible = true);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool shouldClose() const;
  void swapBuffers() const;
  void pollEvents() const;

  GLFWwindow* getNativeWindow() const { return m_window; }
  void getFramebufferSize(int& width, int& height) const;

 private:
  GLFWwindow* m_window = nullptr;
};

}  // namespace astro
