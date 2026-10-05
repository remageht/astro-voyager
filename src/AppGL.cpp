#include "AppGL.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace astro {

AppGL::AppGL(int width, int height, bool visible) {
  m_window = std::make_unique<Window>(width, height, "astro-voyager v0.2.0", visible);
  m_sceneManager = std::make_unique<SceneManager>();

  // Initialize camera for initial station
  if (m_sceneManager->getCurrentStation()) {
    m_camera.position = Vec3(0.0, 0.0, m_sceneManager->getCurrentStation()->spawnDistanceRs);
  }

  // Setup Dear ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(m_window->getNativeWindow(), true);
  ImGui_ImplOpenGL3_Init("#version 330");
}

AppGL::~AppGL() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

int AppGL::run() {
  float lastTime = static_cast<float>(glfwGetTime());

  while (!m_window->shouldClose()) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    m_window->pollEvents();

    processInput(deltaTime);
    processMouse();

    if (m_sceneManager->getCurrentStation() &&
        m_sceneManager->getCurrentStation()->type == StationType::BlackHole) {
      m_camera.clampNearHorizon(m_sceneManager->getSettings().rs);
    }

    int fbWidth = 0, fbHeight = 0;
    m_window->getFramebufferSize(fbWidth, fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Render active scene
    m_sceneManager->render(m_camera, fbWidth, fbHeight, currentTime);

    // Render ImGui UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    drawImGui();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    m_window->swapBuffers();
  }

  return 0;
}

void AppGL::processInput(float deltaTime) {
  GLFWwindow* win = m_window->getNativeWindow();
  if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(win, GLFW_TRUE);
  }

  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) return;

  if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
    m_camera.moveForward(deltaTime, true);
  }
  if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) {
    m_camera.moveForward(deltaTime, false);
  }
  if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) {
    m_camera.moveRight(deltaTime, false);
  }
  if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) {
    m_camera.moveRight(deltaTime, true);
  }
  if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) {
    m_camera.position.y += m_camera.moveSpeed * deltaTime;
  }
  if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    m_camera.position.y -= m_camera.moveSpeed * deltaTime;
  }
}

void AppGL::processMouse() {
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();

  const bool rightDown = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
  const bool leftDown = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
                         !io.WantCaptureMouse);

  const bool isDragging = (rightDown || leftDown);

  double mouseX = 0.0, mouseY = 0.0;
  glfwGetCursorPos(win, &mouseX, &mouseY);

  if (!isDragging) {
    m_firstMouse = true;
    m_mouseDragging = false;
    return;
  }

  if (m_firstMouse) {
    m_lastMouseX = static_cast<float>(mouseX);
    m_lastMouseY = static_cast<float>(mouseY);
    m_firstMouse = false;
    m_mouseDragging = true;
    return;
  }

  float xoffset = static_cast<float>(mouseX) - m_lastMouseX;
  float yoffset = m_lastMouseY - static_cast<float>(mouseY);
  m_lastMouseX = static_cast<float>(mouseX);
  m_lastMouseY = static_cast<float>(mouseY);

  const float sensitivity = 0.003f;
  m_camera.yaw += xoffset * sensitivity;
  m_camera.pitch += yoffset * sensitivity;

  const float maxPitch = 1.55f;
  if (m_camera.pitch > maxPitch) m_camera.pitch = maxPitch;
  if (m_camera.pitch < -maxPitch) m_camera.pitch = -maxPitch;
}

void AppGL::drawImGui() {
  ImGui::SetNextWindowPos(ImVec2(15.0f, 15.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(340.0f, 480.0f), ImGuiCond_FirstUseEver);

  ImGui::Begin("astro-voyager Navigator");

  const Station* curSt = m_sceneManager->getCurrentStation();
  std::string curId = curSt ? curSt->id : "";

  ImGui::Text("Active Station: %s", curSt ? curSt->name.c_str() : "None");
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s",
                     curSt ? curSt->description.c_str() : "");
  ImGui::Separator();

  // Teleport Station List
  ImGui::Text("Teleport Stations:");
  for (const auto& station : builtinCatalog()) {
    bool isCurrent = (station.id == curId);
    std::string label = station.name + " (" + station.id + ")";
    if (isCurrent) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
    }

    if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 0.0f))) {
      m_sceneManager->teleportTo(station.id, m_camera);
    }

    if (isCurrent) {
      ImGui::PopStyleColor();
    }
  }

  ImGui::Separator();
  ImGui::Text("Parameters:");

  RenderSettings& settings = m_sceneManager->getSettings();
  ImGui::SliderFloat("Shell radius", &settings.shellRadius, 5.0f, 80.0f, "%.1f Rs");
  ImGui::SliderFloat("Step size", &settings.stepSize, 0.005f, 0.1f, "%.4f");
  ImGui::SliderInt("Max steps", &settings.maxSteps, 50, 2000);
  ImGui::SliderFloat("FOV", &settings.fovDegrees, 30.0f, 110.0f, "%.1f deg");

  ImGui::Separator();
  ImGui::Text("Camera & Performance:");
  const double dist = std::sqrt(m_camera.position.x * m_camera.position.x +
                                m_camera.position.y * m_camera.position.y +
                                m_camera.position.z * m_camera.position.z);
  ImGui::Text("Distance: %.2f Rs", dist);
  ImGui::Text("Pos: (%.1f, %.1f, %.1f)", m_camera.position.x, m_camera.position.y,
              m_camera.position.z);
  ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate,
              1000.0f / (ImGui::GetIO().Framerate > 0 ? ImGui::GetIO().Framerate : 1.0f));

  ImGui::Separator();
  if (ImGui::Button("Reset Camera", ImVec2(-1.0f, 0.0f))) {
    if (curSt) {
      m_camera.position = Vec3(0.0, 0.0, curSt->spawnDistanceRs);
      m_camera.yaw = 0.0;
      m_camera.pitch = 0.0;
    }
  }

  if (ImGui::Button("Save Screenshot (F12)", ImVec2(-1.0f, 0.0f))) {
    std::filesystem::create_directories("docs/screens");
    std::string path = "docs/screens/" + (curSt ? curSt->id : "screen") + ".png";
    if (saveScreenshot(path)) {
      m_lastScreenshotStatus = "Saved: " + path;
    } else {
      m_lastScreenshotStatus = "Failed to save screenshot";
    }
  }

  if (!m_lastScreenshotStatus.empty()) {
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s",
                       m_lastScreenshotStatus.c_str());
  }

  ImGui::End();
}

bool AppGL::saveScreenshot(const std::string& filepath) {
  int width = 0, height = 0;
  m_window->getFramebufferSize(width, height);
  if (width <= 0 || height <= 0) return false;

  std::vector<unsigned char> pixels(width * height * 4);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  // Flip vertically
  std::vector<unsigned char> flipped(width * height * 4);
  const int rowBytes = width * 4;
  for (int y = 0; y < height; ++y) {
    memcpy(&flipped[y * rowBytes], &pixels[(height - 1 - y) * rowBytes], rowBytes);
  }

  // Ensure directory exists
  std::filesystem::path p(filepath);
  if (p.has_parent_path()) {
    std::filesystem::create_directories(p.parent_path());
  }

  int success = stbi_write_png(filepath.c_str(), width, height, 4, flipped.data(), rowBytes);
  return (success != 0);
}

int AppGL::captureAllScreenshots(const std::string& outDir) {
  std::filesystem::create_directories(outDir);

  // Create AppGL (with window 1280x720)
  AppGL app(1280, 720, false);

  const std::vector<std::string> targetStations = {"sgra", "ori", "qso-3c273"};

  for (const auto& stId : targetStations) {
    app.m_sceneManager->teleportTo(stId, app.m_camera);

    // Warm-up and render
    int fbWidth = 1280, fbHeight = 720;
    app.m_window->getFramebufferSize(fbWidth, fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    for (int frame = 0; frame < 5; ++frame) {
      app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
      app.m_window->swapBuffers();
      app.m_window->pollEvents();
    }

    std::string outFile = outDir + "/" + stId + ".png";
    if (app.saveScreenshot(outFile)) {
      std::cout << "Captured screenshot: " << outFile << "\n";
    } else {
      std::cerr << "Failed to capture screenshot: " << outFile << "\n";
    }
  }

  // Also capture full UI with ImGui overlay
  app.m_sceneManager->teleportTo("sgra", app.m_camera);
  int fbWidth = 1280, fbHeight = 720;
  app.m_window->getFramebufferSize(fbWidth, fbHeight);
  glViewport(0, 0, fbWidth, fbHeight);

  for (int frame = 0; frame < 5; ++frame) {
    app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    app.drawImGui();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  std::string guiFile = outDir + "/gui.png";
  if (app.saveScreenshot(guiFile)) {
    std::cout << "Captured screenshot: " << guiFile << "\n";
  }

  return 0;
}

}  // namespace astro
