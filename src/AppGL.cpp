#include "AppGL.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Constellation.h"
#include "Interaction.h"
#include "Version.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {
static bool fuzzyMatch(const std::string& text, const std::string& query) {
  if (query.empty()) return true;
  size_t tIdx = 0;
  size_t qIdx = 0;
  while (tIdx < text.size() && qIdx < query.size()) {
    char cText = static_cast<char>(std::tolower(static_cast<unsigned char>(text[tIdx])));
    char cQuery = static_cast<char>(std::tolower(static_cast<unsigned char>(query[qIdx])));
    if (cText == cQuery) {
      ++qIdx;
    }
    ++tIdx;
  }
  return qIdx == query.size();
}
}  // namespace

namespace astro {

AppGL::AppGL(int width, int height, bool visible) {
  std::string title = std::string(kAppName) + " v" + kVersion;
  m_window = std::make_unique<Window>(width, height, title.c_str(), visible);
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

    // Auto-performance benchmark: 2-second FPS measurement on scene start
    if (m_benchmarking && !m_userOverrodePreset) {
      m_benchmarkTimer += deltaTime;
      m_benchmarkFrames++;
      if (m_benchmarkTimer >= 2.0f) {
        float avgFps = static_cast<float>(m_benchmarkFrames) / m_benchmarkTimer;
        if (avgFps >= 55.0f) {
          m_sceneManager->applyPreset(PerfPreset::Ultra);
        } else if (avgFps >= 30.0f) {
          m_sceneManager->applyPreset(PerfPreset::Med);
        } else {
          m_sceneManager->applyPreset(PerfPreset::Low);
        }
        m_benchmarking = false;
      }
    }

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
    drawWorldLabels();

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

  const bool slashDown = (glfwGetKey(win, GLFW_KEY_SLASH) == GLFW_PRESS);
  if (slashDown && !m_prevSlashPressed && !io.WantCaptureKeyboard) {
    m_searchFocusRequest = true;
  }
  m_prevSlashPressed = slashDown;

  if (!interaction::shouldProcessKeyboardMovement(io.WantCaptureKeyboard)) return;

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
  const bool leftDown = (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
  const bool rawMousePress = (rightDown || leftDown);

  const bool isDragging = interaction::shouldCaptureMouseForCamera(rawMousePress, io.WantCaptureMouse);

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
  ImGui::SetNextWindowSize(ImVec2(340.0f, 500.0f), ImGuiCond_FirstUseEver);

  ImGui::Begin("astro-voyager Navigator");

  const Station* curSt = m_sceneManager->getCurrentStation();
  std::string curId = curSt ? curSt->id : "";

  ImGui::Text("Active Station: %s", curSt ? curSt->name.c_str() : "None");
  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s",
                     curSt ? curSt->description.c_str() : "");

  // Station search box with hotkey '/' focus
  if (m_searchFocusRequest) {
    ImGui::SetKeyboardFocusHere();
  }
  float clearBtnWidth = 28.0f;
  float inputWidth = ImGui::GetContentRegionAvail().x - clearBtnWidth - ImGui::GetStyle().ItemSpacing.x;
  ImGui::SetNextItemWidth(inputWidth);
  bool enterPressed = ImGui::InputTextWithHint("##stationSearch", "Search stations... (press / to focus)",
                                               m_searchBuf, sizeof(m_searchBuf),
                                               ImGuiInputTextFlags_EnterReturnsTrue);
  m_searchFocusRequest = false;

  ImGui::SameLine();
  bool hasSearchText = (m_searchBuf[0] != '\0');
  if (!hasSearchText) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("X", ImVec2(clearBtnWidth, 0.0f))) {
    m_searchBuf[0] = '\0';
  }
  if (!hasSearchText) {
    ImGui::EndDisabled();
  }

  if (curSt && curSt->type == StationType::Constellation) {
    const ConstellationInfo* cInfo = findConstellation(curSt->id);
    if (cInfo) {
      ImGui::Text("Constellation Stars (%zu Hipparcos):", cInfo->stars.size());
      if (ImGui::BeginChild("StarList", ImVec2(0.0f, 95.0f), true)) {
        for (const auto& s : cInfo->stars) {
          ImGui::BulletText("%s (%s, mag %.2f, HIP %d)",
                            s.name.c_str(), s.bayer.c_str(), s.mag, s.hip);
        }
        ImGui::EndChild();
      }
    }
  }

  ImGui::Separator();

  // Teleport Station List (fuzzy-filtered)
  std::string query = m_searchBuf;
  std::vector<const Station*> matchedStations;
  for (const auto& station : builtinCatalog()) {
    if (fuzzyMatch(station.name, query) || fuzzyMatch(station.id, query)) {
      matchedStations.push_back(&station);
    }
  }

  if (enterPressed && !query.empty() && !matchedStations.empty()) {
    m_sceneManager->teleportTo(matchedStations[0]->id, m_camera);
    m_searchBuf[0] = '\0';
  }

  ImGui::Text("Teleport Stations:");
  if (!query.empty()) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(%zu station(s) matched)", matchedStations.size());
  }

  if (matchedStations.empty()) {
    ImGui::TextColored(ImVec4(0.7f, 0.5f, 0.5f, 1.0f), "No matches");
  } else {
    for (const auto* stPtr : matchedStations) {
      const auto& station = *stPtr;
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
  }

  ImGui::Separator();
  ImGui::Text("Quality Preset:");
  if (m_benchmarking && !m_userOverrodePreset) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[Auto-calibrating 2s...]");
  }

  const char* presetNames[] = {"Low", "Med", "Ultra", "Custom"};
  int currentPreset = static_cast<int>(m_sceneManager->getPreset());
  if (ImGui::Combo("Preset", &currentPreset, presetNames, 4)) {
    m_userOverrodePreset = true;
    m_benchmarking = false;
    if (currentPreset >= 0 && currentPreset <= 2) {
      m_sceneManager->applyPreset(static_cast<PerfPreset>(currentPreset));
    } else {
      m_sceneManager->setPreset(PerfPreset::Custom);
    }
  }

  ImGui::Text("Parameters:");

  RenderSettings& settings = m_sceneManager->getSettings();
  bool sliderChanged = false;
  sliderChanged |= ImGui::SliderFloat("Shell radius", &settings.shellRadius, 5.0f, 80.0f, "%.1f Rs");
  sliderChanged |= ImGui::SliderFloat("Step size", &settings.stepSize, 0.005f, 0.1f, "%.4f");
  sliderChanged |= ImGui::SliderInt("Max steps", &settings.maxSteps, 50, 2000);
  sliderChanged |= ImGui::SliderFloat("FOV", &settings.fovDegrees, 30.0f, 110.0f, "%.1f deg");

  if (curSt && curSt->type == StationType::BlackHole) {
    sliderChanged |= ImGui::Checkbox("Disk", &settings.diskOn);
  }

  if (sliderChanged) {
    m_userOverrodePreset = true;
    m_benchmarking = false;
    m_sceneManager->setPreset(PerfPreset::Custom);
  }

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

void AppGL::drawWorldLabels() {
  const Station* curSt = m_sceneManager->getCurrentStation();
  if (!curSt) return;

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  if (!drawList) return;

  ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) return;

  ImFont* font = ImGui::GetFont();
  if (!font) return;

  if (curSt->type == StationType::Constellation) {
    const ConstellationInfo* cInfo = findConstellation(curSt->id);
    if (!cInfo) return;

    float aspect = displaySize.x / displaySize.y;
    float fov = m_sceneManager->getSettings().fovDegrees;
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);

    Vec3 cp = m_camera.position;
    Vec3 cf = m_camera.forward();
    Vec3 cu = m_camera.up();

    glm::vec3 eye(cp.x, cp.y, cp.z);
    glm::vec3 fwd(cf.x, cf.y, cf.z);
    glm::vec3 up(cu.x, cu.y, cu.z);
    glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
    glm::mat4 mvp = proj * view;

    for (const auto& s : cInfo->stars) {
      glm::vec4 clip = mvp * glm::vec4(s.x, s.y, s.z, 1.0f);
      if (clip.w <= 0.001f) continue;

      glm::vec3 ndc = glm::vec3(clip) / clip.w;
      if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;

      // Distance from camera to star
      double dx = cp.x - static_cast<double>(s.x);
      double dy = cp.y - static_cast<double>(s.y);
      double dz = cp.z - static_cast<double>(s.z);
      double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

      // Threshold 1: d <= 45
      if (dist > 45.0) continue;

      // Screen coordinates (displaySize)
      float sx = (ndc.x * 0.5f + 0.5f) * displaySize.x;
      float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;

      // Closeness factor t in [0, 1] (0 at dist=45, 1 at dist<=6)
      float t = static_cast<float>(std::clamp((45.0 - dist) / (45.0 - 6.0), 0.0, 1.0));

      // Font size: 13 -> 17
      float nameFontSize = 13.0f + t * 4.0f;
      // Circle radius: 3 -> 13
      float circleRadius = 3.0f + t * 10.0f;
      // Alpha: 0.35 -> 0.9
      float alpha = 0.35f + t * 0.55f;
      int alphaByte = static_cast<int>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));

      // Warm white circle: RGB (255, 245, 225)
      ImU32 circleColor = IM_COL32(255, 245, 225, alphaByte);
      drawList->AddCircle(ImVec2(sx, sy), circleRadius, circleColor, 16, 1.5f);

      // Name text
      ImVec2 textPos(sx + circleRadius + 5.0f, sy - nameFontSize * 0.55f);
      ImU32 nameColor = IM_COL32(255, 255, 255, alphaByte);
      drawList->AddText(font, nameFontSize, textPos, nameColor, s.name.c_str());

      float currentY = textPos.y + nameFontSize + 2.0f;

      // Threshold 2: d <= 15
      if (dist <= 15.0) {
        char detailBuf[128];
        std::snprintf(detailBuf, sizeof(detailBuf), "%s - mag %.2f - HIP %d",
                      s.bayer.c_str(), s.mag, s.hip);
        // Gray-blue color: RGB (170, 200, 230)
        ImU32 detailColor = IM_COL32(170, 200, 230, alphaByte);
        drawList->AddText(font, 12.0f, ImVec2(textPos.x, currentY), detailColor, detailBuf);
        currentY += 14.0f;
      }

      // Threshold 3: d <= 6
      if (dist <= 6.0) {
        char distBuf[64];
        std::snprintf(distBuf, sizeof(distBuf), "distance: %.1f units", dist);
        // Green color: RGB (100, 230, 120)
        ImU32 distColor = IM_COL32(100, 230, 120, alphaByte);
        drawList->AddText(font, 12.0f, ImVec2(textPos.x, currentY), distColor, distBuf);
      }
    }
  } else if (curSt->type == StationType::Galaxy) {
    Vec3 cp = m_camera.position;
    double dist = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
    double spawn = curSt->spawnDistanceRs > 0.0 ? curSt->spawnDistanceRs : 80.0;
    double ratio = dist / spawn;

    // Card lines
    struct CardLine {
      std::string text;
      float size;
      ImU32 color;
    };
    std::vector<CardLine> lines;

    // Always: Station name (20px, white) + subtitle (13.5px, gray-blue)
    lines.push_back({curSt->name, 20.0f, IM_COL32(255, 255, 255, 255)});
    lines.push_back({"Spiral galaxy - type SA(s)b", 13.5f, IM_COL32(170, 200, 230, 230)});

    // ratio <= 0.65
    if (ratio <= 0.65) {
      lines.push_back({"Distance 2.54 Mly - diameter ~220,000 ly", 13.0f, IM_COL32(210, 225, 245, 240)});
    }

    // ratio <= 0.35
    if (ratio <= 0.35) {
      lines.push_back({"Mass ~1.5e12 solar masses - approaching at ~300 km/s", 12.5f, IM_COL32(200, 220, 240, 240)});
      // Amber: RGB (255, 190, 70)
      lines.push_back({"Individual star clouds resolving in the disk", 12.5f, IM_COL32(255, 190, 70, 255)});
    }

    // Compute dimensions
    float padX = 24.0f;
    float padY = 14.0f;
    float lineSpacing = 4.0f;
    float maxLineWidth = 0.0f;
    float totalTextHeight = 0.0f;

    for (size_t i = 0; i < lines.size(); ++i) {
      ImVec2 sz = font->CalcTextSizeA(lines[i].size, FLT_MAX, 0.0f, lines[i].text.c_str());
      if (sz.x > maxLineWidth) maxLineWidth = sz.x;
      totalTextHeight += sz.y;
      if (i + 1 < lines.size()) totalTextHeight += lineSpacing;
    }

    float cardWidth = maxLineWidth + padX * 2.0f;
    float cardHeight = totalTextHeight + padY * 2.0f;

    float cardX = (displaySize.x - cardWidth) * 0.5f;
    float cardY = 20.0f;

    ImVec2 minPos(cardX, cardY);
    ImVec2 maxPos(cardX + cardWidth, cardY + cardHeight);

    // Background: semi-transparent dark rounded rectangle
    ImU32 bgCol = IM_COL32(10, 15, 25, 200);
    ImU32 borderCol = IM_COL32(80, 120, 170, 120);
    drawList->AddRectFilled(minPos, maxPos, bgCol, 8.0f);
    drawList->AddRect(minPos, maxPos, borderCol, 8.0f, 0, 1.0f);

    float curY = cardY + padY;
    for (const auto& l : lines) {
      ImVec2 sz = font->CalcTextSizeA(l.size, FLT_MAX, 0.0f, l.text.c_str());
      float textX = cardX + (cardWidth - sz.x) * 0.5f;
      drawList->AddText(font, l.size, ImVec2(textX, curY), l.color, l.text.c_str());
      curY += sz.y + lineSpacing;
    }
  }
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

int AppGL::captureAllScreenshots(const std::string& outDir, bool diskOn) {
  std::filesystem::create_directories(outDir);

  // Create AppGL (with window 1280x720)
  AppGL app(1280, 720, false);

  const std::vector<std::string> targetStations = {
      "sgra", "qso-3c273", "ori", "uma", "m31", "m1"
  };

  for (const auto& stId : targetStations) {
    app.m_sceneManager->teleportTo(stId, app.m_camera);
    app.m_sceneManager->getSettings().diskOn = diskOn;

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
    app.drawWorldLabels();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  std::string guiFile = outDir + "/gui.png";
  if (app.saveScreenshot(guiFile)) {
    std::cout << "Captured screenshot: " << guiFile << "\n";
  }

  // Capture close-up progressive detail shots:
  // 1. M31 Andromeda Galaxy close-up (z = 28)
  app.m_sceneManager->teleportTo("m31", app.m_camera);
  app.m_camera.position.z = 28.0;
  for (int frame = 0; frame < 5; ++frame) {
    app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    app.drawImGui();
    app.drawWorldLabels();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  std::string m31CloseFile = outDir + "/m31_close.png";
  if (app.saveScreenshot(m31CloseFile)) {
    std::cout << "Captured screenshot: " << m31CloseFile << "\n";
  }

  // 2. Orion Constellation close-up (z = 4.5)
  app.m_sceneManager->teleportTo("ori", app.m_camera);
  app.m_camera.position.z = 4.5;
  for (int frame = 0; frame < 5; ++frame) {
    app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    app.drawImGui();
    app.drawWorldLabels();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  std::string oriCloseFile = outDir + "/ori_close.png";
  if (app.saveScreenshot(oriCloseFile)) {
    std::cout << "Captured screenshot: " << oriCloseFile << "\n";
  }

  return 0;
}

}  // namespace astro
