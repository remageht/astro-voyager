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
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Constellation.h"
#include "Interaction.h"
#include "Journey.h"
#include "Achievements.h"
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
    m_route.push_back(m_sceneManager->getCurrentStation()->id);
    m_visited.push_back(m_sceneManager->getCurrentStation()->id);
  }
  checkAchievements();

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
  static float menuAngle = 0.0f;

  while (!m_window->shouldClose()) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    if (m_inMenu) {
      // Main menu: showcase orbit around the current station, world renders behind.
      // ESC in menu does nothing (consume the edge only).
      GLFWwindow* menuWin = m_window->getNativeWindow();
      bool menuEscDown = (glfwGetKey(menuWin, GLFW_KEY_ESCAPE) == GLFW_PRESS);
      m_prevEscPressed = menuEscDown;

      m_window->pollEvents();

      const Station* menuSt = m_sceneManager->getCurrentStation();
      double menuSpawn = (menuSt ? menuSt->spawnDistanceRs : 20.0);
      float menuRadius = static_cast<float>(menuSpawn * 1.25 + 4.0);
      menuAngle += deltaTime * 0.05f;
      m_camera.position = Vec3(static_cast<double>(std::cos(menuAngle)) * menuRadius,
                               static_cast<double>(menuRadius) * 0.35,
                               static_cast<double>(std::sin(menuAngle)) * menuRadius);
      Vec3 menuOrigin(0.0, 0.0, 0.0);
      computeLookYawPitch(m_camera.position, menuOrigin, m_camera.yaw, m_camera.pitch);

      int menuFbW = 0, menuFbH = 0;
      m_window->getFramebufferSize(menuFbW, menuFbH);
      glViewport(0, 0, menuFbW, menuFbH);

      // Render active scene
      m_sceneManager->render(m_camera, menuFbW, menuFbH, currentTime);

      // Render ImGui UI
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      drawMenu();
      drawWorldLabels();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      m_window->swapBuffers();
      continue;
    }

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
      double d = std::sqrt(m_camera.position.x * m_camera.position.x +
                           m_camera.position.y * m_camera.position.y +
                           m_camera.position.z * m_camera.position.z);
      if (d < m_minBhDistance) {
        m_minBhDistance = d;
      }
    }
    if (m_sceneManager->getCurrentStation() &&
        m_sceneManager->getCurrentStation()->type == StationType::Constellation) {
      const ConstellationInfo* cInfo =
          findConstellation(m_sceneManager->getCurrentStation()->id);
      if (cInfo) {
        for (const auto& s : cInfo->stars) {
          m_camera.position = interaction::pushOutOfSphere(
              m_camera.position, s.x, s.y, s.z, 2.0);
        }
      }
    }
    if (m_sceneManager->getCurrentStation() &&
        m_sceneManager->getCurrentStation()->type == StationType::SolarSystem) {
      for (const auto& body : m_sceneManager->getBodies()) {
        if (!body.def) continue;
        double minDist = static_cast<double>(body.def->radius) * 1.3;
        if (minDist < 0.8) minDist = 0.8;
        m_camera.position = interaction::pushOutOfSphere(
            m_camera.position, body.pos.x, body.pos.y, body.pos.z, minDist);
      }
    }
    checkAchievements();

    int fbWidth = 0, fbHeight = 0;
    m_window->getFramebufferSize(fbWidth, fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Render active scene
    m_sceneManager->render(m_camera, fbWidth, fbHeight, currentTime);

    // Render ImGui UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (!m_hideUI) {
      drawImGui();
    }
    if (m_helpOverlay) {
      drawHelpOverlay();
    }
    drawWorldLabels();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    m_window->swapBuffers();
  }

  return 0;
}

void AppGL::processInput(float deltaTime) {
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();

  const bool escDown = (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS);
  if (escDown && !m_prevEscPressed && !io.WantCaptureKeyboard) {
    m_inMenu = true;
  }
  m_prevEscPressed = escDown;

  const bool slashDown = (glfwGetKey(win, GLFW_KEY_SLASH) == GLFW_PRESS);
  if (slashDown && !m_prevSlashPressed && !io.WantCaptureKeyboard) {
    m_searchFocusRequest = true;
  }
  m_prevSlashPressed = slashDown;

  const bool f5Down = (glfwGetKey(win, GLFW_KEY_F5) == GLFW_PRESS);
  if (f5Down && !m_prevF5Pressed && !io.WantCaptureKeyboard) {
    exportJourney();
  }
  m_prevF5Pressed = f5Down;

  const bool f6Down = (glfwGetKey(win, GLFW_KEY_F6) == GLFW_PRESS);
  if (f6Down && !m_prevF6Pressed && !io.WantCaptureKeyboard) {
    importJourney();
  }
  m_prevF6Pressed = f6Down;

  const bool pDown = (glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS);
  if (pDown && !m_prevPPressed && !io.WantCaptureKeyboard) {
    m_sceneManager->togglePause();
  }
  m_prevPPressed = pDown;

  // Time pace: - steps down, = (or +) steps up the preset list.
  // Slash is reserved for station search focus, so it is not reused here.
  const bool minusDown = (glfwGetKey(win, GLFW_KEY_MINUS) == GLFW_PRESS);
  if (minusDown && !m_prevMinusPressed && !io.WantCaptureKeyboard) {
    stepTimePace(-1);
  }
  m_prevMinusPressed = minusDown;

  const bool equalDown = (glfwGetKey(win, GLFW_KEY_EQUAL) == GLFW_PRESS);
  if (equalDown && !m_prevEqualPressed && !io.WantCaptureKeyboard) {
    stepTimePace(+1);
  }
  m_prevEqualPressed = equalDown;

  const bool delDown = (glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
  if (delDown && !m_prevDeletePressed && !io.WantCaptureKeyboard) {
    m_sceneManager->deleteSelected();
  }
  m_prevDeletePressed = delDown;

  const bool hDown = (glfwGetKey(win, GLFW_KEY_H) == GLFW_PRESS);
  if (hDown && !m_prevHPressed && !io.WantCaptureKeyboard) {
    m_hideUI = !m_hideUI;
  }
  m_prevHPressed = hDown;

  const bool f1Down = (glfwGetKey(win, GLFW_KEY_F1) == GLFW_PRESS);
  if (f1Down && !m_prevF1Pressed && !io.WantCaptureKeyboard) {
    m_helpOverlay = !m_helpOverlay;
  }
  m_prevF1Pressed = f1Down;

  // Smooth fly-to flight update (runs every frame).
  if (m_flying) {
    updateFlight(deltaTime);
    if (m_flying) {
      return;
    }
    // If flight just finished or was cancelled, fall through to normal input.
  }

  if (!interaction::shouldProcessKeyboardMovement(io.WantCaptureKeyboard)) return;

  // M31 approach: distance-scaled WASD speed so the 80 -> 2 flight takes
  // seconds, not minutes, while staying precise near the core. Other
  // stations keep the default moveSpeed untouched.
  double savedSpeed = m_camera.moveSpeed;
  const Station* mvSt = m_sceneManager->getCurrentStation();
  if (mvSt && mvSt->id == "m31") {
    double m31D = std::sqrt(m_camera.position.x * m_camera.position.x +
                            m_camera.position.y * m_camera.position.y +
                            m_camera.position.z * m_camera.position.z);
    double want = 4.0 + m31D * 0.25;
    if (want < 4.0) want = 4.0;
    if (want > 30.0) want = 30.0;
    m_camera.moveSpeed = want;
  }

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
  m_camera.moveSpeed = savedSpeed;
}

void AppGL::stepTimePace(int dir) {
  // Walk the pace preset list {1, 10, 60, 300, 1000} from the current
  // scale: +1 takes the next higher preset, -1 the next lower one.
  // Changing pace resumes a paused clock so the effect is visible.
  float cur = m_sceneManager->getTimeScale();
  float next = cur;
  if (dir > 0) {
    for (int i = 0; i < kTimePacePresetCount; ++i) {
      if (kTimePacePresets[i] > cur * 1.001f) {
        next = kTimePacePresets[i];
        break;
      }
      next = kTimePacePresets[i];
    }
  } else if (dir < 0) {
    for (int i = kTimePacePresetCount - 1; i >= 0; --i) {
      if (kTimePacePresets[i] < cur * 0.999f) {
        next = kTimePacePresets[i];
        break;
      }
      next = kTimePacePresets[i];
    }
  }
  if (m_sceneManager->isPaused()) {
    m_sceneManager->togglePause();
  }
  m_sceneManager->setTimeScale(next);
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

  // Double-click detection on left-button press edges.
  if (leftDown && !m_prevLeftDown) {
    double now = glfwGetTime();
    if (!io.WantCaptureMouse) {
      double dx = mouseX - m_lastPressX;
      double dy = mouseY - m_lastPressY;
      double moved = std::sqrt(dx * dx + dy * dy);
      if ((now - m_lastPressTime) < 0.45 && moved < 8.0) {
        flyToObjectAt(mouseX, mouseY);
      } else {
        m_lastPressTime = now;
        m_lastPressX = mouseX;
        m_lastPressY = mouseY;
        const Station* pressSt = m_sceneManager->getCurrentStation();
        if (pressSt && pressSt->type == StationType::SolarSystem) {
          m_sceneManager->setSelected(pickCustomAt(mouseX, mouseY));
        }
      }
    }
  }
  m_prevLeftDown = leftDown;

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

void AppGL::computeLookYawPitch(const Vec3& fromPos, const Vec3& target,
                                double& outYaw, double& outPitch) {
  double dx = target.x - fromPos.x;
  double dy = target.y - fromPos.y;
  double dz = target.z - fromPos.z;
  double len = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (len < 1e-9) {
    outYaw = m_camera.yaw;
    outPitch = m_camera.pitch;
    return;
  }
  dx /= len;
  dy /= len;
  dz /= len;
  // Inverse of Camera::forward():
  // forward = (-sin(yaw)*cos(pitch), sin(pitch), -cos(yaw)*cos(pitch)).
  // So pitch = asin(dy), yaw = atan2(-dx, -dz).
  double horiz = std::sqrt(dx * dx + dz * dz);
  if (horiz < 1e-6) {
    outYaw = m_camera.yaw;
  } else {
    outYaw = std::atan2(-dx, -dz);
  }
  if (dy > 1.0) dy = 1.0;
  if (dy < -1.0) dy = -1.0;
  outPitch = std::asin(dy);
  const double kMaxPitch = 1.55;
  if (outPitch > kMaxPitch) outPitch = kMaxPitch;
  if (outPitch < -kMaxPitch) outPitch = -kMaxPitch;
}

void AppGL::startObjectFlight(const Vec3& objPos, double viewDist,
                              const std::string& name,
                              const Vec3& lookTarget) {
  Vec3 cp = m_camera.position;
  double dx = cp.x - objPos.x;
  double dy = cp.y - objPos.y;
  double dz = cp.z - objPos.z;
  double len = std::sqrt(dx * dx + dy * dy + dz * dz);
  double nx = 0.0, ny = 0.0, nz = 1.0;
  if (len < 1e-9) {
    nx = 0.0;
    ny = 0.0;
    nz = 1.0;
  } else {
    nx = dx / len;
    ny = dy / len;
    nz = dz / len;
  }
  if (viewDist < 0.5) viewDist = 0.5;
  Vec3 to;
  to.x = objPos.x + nx * viewDist;
  to.y = objPos.y + ny * viewDist + viewDist * 0.25;
  to.z = objPos.z + nz * viewDist;
  m_flyFrom = cp;
  m_flyTo = to;
  m_yawFrom = m_camera.yaw;
  m_pitchFrom = m_camera.pitch;
  computeLookYawPitch(to, lookTarget, m_yawTo, m_pitchTo);
  m_flyT = 0.0f;
  m_flyDur = 1.2f;
  m_flying = true;
  m_flyTargetName = name;
}

void AppGL::startDollyFlight() {
  const Station* curSt = m_sceneManager->getCurrentStation();
  Vec3 cp = m_camera.position;
  double r = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
  Vec3 to;
  if (curSt && curSt->type == StationType::BlackHole) {
    double rs = static_cast<double>(m_sceneManager->getSettings().rs);
    if (rs < 0.1) rs = 1.0;
    double want = r * 0.62;
    double minR = 6.5 * rs;
    if (want < minR) want = minR;
    if (r < 1e-9) {
      to = Vec3(0.0, 0.0, want);
    } else {
      double k = want / r;
      to = Vec3(cp.x * k, cp.y * k, cp.z * k);
    }
  } else {
    to = Vec3(cp.x * 0.62, cp.y * 0.62, cp.z * 0.62);
  }
  m_flyFrom = cp;
  m_flyTo = to;
  m_yawFrom = m_camera.yaw;
  m_pitchFrom = m_camera.pitch;
  Vec3 origin(0.0, 0.0, 0.0);
  computeLookYawPitch(to, origin, m_yawTo, m_pitchTo);
  m_flyT = 0.0f;
  m_flyDur = 1.2f;
  m_flying = true;
  m_flyTargetName.clear();
}

void AppGL::updateFlight(float deltaTime) {
  if (!m_flying) return;
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();
  if (!io.WantCaptureKeyboard) {
    bool wDown = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
    bool aDown = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
    bool sDown = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
    bool dDown = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
    if (wDown || aDown || sDown || dDown) {
      m_flying = false;
      return;
    }
  }
  m_flyT += deltaTime / m_flyDur;
  float t = m_flyT;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  float e = t * t * (3.0f - 2.0f * t);
  const double kPi = 3.14159265358979323846;
  double yawDelta = m_yawTo - m_yawFrom;
  while (yawDelta > kPi) yawDelta -= 2.0 * kPi;
  while (yawDelta < -kPi) yawDelta += 2.0 * kPi;
  m_camera.position.x = m_flyFrom.x + (m_flyTo.x - m_flyFrom.x) * e;
  m_camera.position.y = m_flyFrom.y + (m_flyTo.y - m_flyFrom.y) * e;
  m_camera.position.z = m_flyFrom.z + (m_flyTo.z - m_flyFrom.z) * e;
  m_camera.yaw = m_yawFrom + yawDelta * e;
  m_camera.pitch = m_pitchFrom + (m_pitchTo - m_pitchFrom) * e;
  if (m_flyT >= 1.0f) {
    m_flying = false;
  }
}

std::string AppGL::pickCustomAt(double px, double py) {
  const Station* curSt = m_sceneManager->getCurrentStation();
  if (!curSt || curSt->type != StationType::SolarSystem) return "";
  const std::vector<CustomBody>& customs = m_sceneManager->getCustoms();
  if (customs.empty()) return "";
  int fbW = 0, fbH = 0;
  m_window->getFramebufferSize(fbW, fbH);
  if (fbW <= 0 || fbH <= 0) return "";
  GLFWwindow* win = m_window->getNativeWindow();
  int winW = 0, winH = 0;
  glfwGetWindowSize(win, &winW, &winH);
  double fpx = px;
  double fpy = py;
  if (winW > 0 && winH > 0) {
    fpx = px * static_cast<double>(fbW) / static_cast<double>(winW);
    fpy = py * static_cast<double>(fbH) / static_cast<double>(winH);
  }
  float fov = m_sceneManager->getSettings().fovDegrees;
  float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
  glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
  Vec3 cp = m_camera.position;
  Vec3 cf = m_camera.forward();
  Vec3 cu = m_camera.up();
  glm::vec3 eye(static_cast<float>(cp.x), static_cast<float>(cp.y),
                static_cast<float>(cp.z));
  glm::vec3 fwd(static_cast<float>(cf.x), static_cast<float>(cf.y),
                static_cast<float>(cf.z));
  glm::vec3 up(static_cast<float>(cu.x), static_cast<float>(cu.y),
               static_cast<float>(cu.z));
  glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
  glm::mat4 mvp = proj * view;
  std::string bestId;
  double bestScreen = 20.0;
  for (const auto& c : customs) {
    glm::vec4 clip = mvp * glm::vec4(static_cast<float>(c.pos.x),
                                     static_cast<float>(c.pos.y),
                                     static_cast<float>(c.pos.z), 1.0f);
    if (clip.w <= 0.001f) continue;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;
    float sx = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fbW);
    float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(fbH);
    double sdx = static_cast<double>(sx) - fpx;
    double sdy = static_cast<double>(sy) - fpy;
    double screenDist = std::sqrt(sdx * sdx + sdy * sdy);
    if (screenDist < bestScreen) {
      bestScreen = screenDist;
      bestId = c.id;
    }
  }
  return bestId;
}

void AppGL::flyToObjectAt(double px, double py) {
  const Station* curSt = m_sceneManager->getCurrentStation();
  if (!curSt) return;
  int fbW = 0, fbH = 0;
  m_window->getFramebufferSize(fbW, fbH);
  if (fbW <= 0 || fbH <= 0) return;
  GLFWwindow* win = m_window->getNativeWindow();
  int winW = 0, winH = 0;
  glfwGetWindowSize(win, &winW, &winH);
  double fpx = px;
  double fpy = py;
  if (winW > 0 && winH > 0) {
    fpx = px * static_cast<double>(fbW) / static_cast<double>(winW);
    fpy = py * static_cast<double>(fbH) / static_cast<double>(winH);
  }

  float fov = m_sceneManager->getSettings().fovDegrees;
  float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
  glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
  Vec3 cp = m_camera.position;
  Vec3 cf = m_camera.forward();
  Vec3 cu = m_camera.up();
  glm::vec3 eye(static_cast<float>(cp.x), static_cast<float>(cp.y),
                static_cast<float>(cp.z));
  glm::vec3 fwd(static_cast<float>(cf.x), static_cast<float>(cf.y),
                static_cast<float>(cf.z));
  glm::vec3 up(static_cast<float>(cu.x), static_cast<float>(cu.y),
               static_cast<float>(cu.z));
  glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
  glm::mat4 mvp = proj * view;

  const double kPi = 3.14159265358979323846;
  double halfFovRad = static_cast<double>(fov) * 0.5 * kPi / 180.0;
  double tanHalf = std::tan(halfFovRad);
  if (tanHalf < 1e-6) tanHalf = 1e-6;
  double focal = static_cast<double>(fbH) / (2.0 * tanHalf);

  if (curSt->type == StationType::SolarSystem) {
    const std::vector<BodyState>& bodies = m_sceneManager->getBodies();
    bool haveHit = false;
    double bestScreen = 1e30;
    Vec3 bestPos;
    double bestRadius = 1.0;
    std::string bestName;
    for (const auto& body : bodies) {
      if (!body.def) continue;
        glm::vec3 pos(static_cast<float>(body.pos.x),
                          static_cast<float>(body.pos.y),
                          static_cast<float>(body.pos.z));
      glm::vec4 clip = mvp * glm::vec4(pos, 1.0f);
      if (clip.w <= 0.001f) continue;
      glm::vec3 ndc = glm::vec3(clip) / clip.w;
      if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;
      float sx = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fbW);
      float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(fbH);
      double dx = cp.x - static_cast<double>(pos.x);
      double dy = cp.y - static_cast<double>(pos.y);
      double dz = cp.z - static_cast<double>(pos.z);
      double distToCam = std::sqrt(dx * dx + dy * dy + dz * dz);
      if (distToCam < 1e-6) distToCam = 1e-6;
      double radius = static_cast<double>(body.def->radius);
      if (radius < 0.05) radius = 0.05;
      double projR = radius * focal / distToCam;
      double hitR = projR + 10.0;
      if (hitR < 24.0) hitR = 24.0;
      double sdx = static_cast<double>(sx) - fpx;
      double sdy = static_cast<double>(sy) - fpy;
      double screenDist = std::sqrt(sdx * sdx + sdy * sdy);
      if (screenDist < hitR && screenDist < bestScreen) {
        haveHit = true;
        bestScreen = screenDist;
        bestPos = Vec3(static_cast<double>(pos.x), static_cast<double>(pos.y),
                       static_cast<double>(pos.z));
        bestRadius = radius;
        bestName = body.def->name;
      }
    }
    if (haveHit) {
      startObjectFlight(bestPos, bestRadius * 6.0, bestName, bestPos);
      return;
    }
    startDollyFlight();
    return;
  }

  if (curSt->type == StationType::Constellation) {
    const ConstellationInfo* cInfo = findConstellation(curSt->id);
    if (cInfo) {
      bool haveHit = false;
      double bestScreen = 1e30;
      Vec3 bestPos;
      std::string bestName;
      for (const auto& s : cInfo->stars) {
        glm::vec4 clip = mvp * glm::vec4(s.x, s.y, s.z, 1.0f);
        if (clip.w <= 0.001f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;
        float sx = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fbW);
        float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(fbH);
        double sdx = static_cast<double>(sx) - fpx;
        double sdy = static_cast<double>(sy) - fpy;
        double screenDist = std::sqrt(sdx * sdx + sdy * sdy);
        if (screenDist < 24.0 && screenDist < bestScreen) {
          haveHit = true;
          bestScreen = screenDist;
          bestPos = Vec3(static_cast<double>(s.x), static_cast<double>(s.y),
                         static_cast<double>(s.z));
          bestName = s.name;
        }
      }
      if (haveHit) {
        startObjectFlight(bestPos, 3.0, bestName, bestPos);
        return;
      }
    }
    startDollyFlight();
    return;
  }

  if (curSt->type == StationType::BlackHole) {
    glm::vec4 clip = mvp * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (clip.w > 0.001f) {
      glm::vec3 ndc = glm::vec3(clip) / clip.w;
      if (ndc.x >= -1.02f && ndc.x <= 1.02f && ndc.y >= -1.02f && ndc.y <= 1.02f) {
        float sx = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fbW);
        float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(fbH);
        double rs = static_cast<double>(m_sceneManager->getSettings().rs);
        if (rs < 0.1) rs = 1.0;
        double distCam = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
        if (distCam < 1e-6) distCam = 1e-6;
        double projRs = rs * focal / distCam;
        double hitR = projRs * 3.0 + 12.0;
        double sdx = static_cast<double>(sx) - fpx;
        double sdy = static_cast<double>(sy) - fpy;
        double screenDist = std::sqrt(sdx * sdx + sdy * sdy);
        if (screenDist < hitR) {
          Vec3 origin(0.0, 0.0, 0.0);
          startObjectFlight(origin, rs * 6.5, curSt->name, origin);
          return;
        }
      }
    }
    startDollyFlight();
    return;
  }

  // M31 only: double-click a bright named star flies to it, else dolly in.
  if (curSt->type == StationType::Galaxy && curSt->id == "m31") {
    bool haveHit = false;
    double bestScreen = 1e30;
    Vec3 bestPos;
    std::string bestName;
    for (const auto& bs : m_sceneManager->m31BrightStars()) {
      glm::vec4 clip = mvp * glm::vec4(bs.pos, 1.0f);
      if (clip.w <= 0.001f) continue;
      glm::vec3 ndc = glm::vec3(clip) / clip.w;
      if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;
      float sx = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fbW);
      float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(fbH);
      double sdx = static_cast<double>(sx) - fpx;
      double sdy = static_cast<double>(sy) - fpy;
      double screenDist = std::sqrt(sdx * sdx + sdy * sdy);
      if (screenDist < 28.0 && screenDist < bestScreen) {
        haveHit = true;
        bestScreen = screenDist;
        bestPos = Vec3(static_cast<double>(bs.pos.x), static_cast<double>(bs.pos.y),
                       static_cast<double>(bs.pos.z));
        bestName = bs.name;
      }
    }
    if (haveHit) {
      startObjectFlight(bestPos, 2.0, bestName, bestPos);
      return;
    }
    startDollyFlight();
    return;
  }

  startDollyFlight();
}

void AppGL::drawImGui() {
  ImGui::SetNextWindowPos(ImVec2(15.0f, 15.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(340.0f, 500.0f), ImGuiCond_FirstUseEver);

  ImGui::Begin("astro-voyager Navigator");

  const Station* curSt = m_sceneManager->getCurrentStation();
  std::string curId = curSt ? curSt->id : "";

  ImGui::Text("Active Station:");
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", curSt ? curSt->name.c_str() : "None");
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
    teleportToStation(matchedStations[0]->id);
    m_searchBuf[0] = '\0';
  }

  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Teleport Stations:");
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
        teleportToStation(station.id);
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", station.description.c_str());
      }

      if (isCurrent) {
        ImGui::PopStyleColor();
      }
    }
  }
  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                     "(Double-click an object to fly to it)");

  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Quality Preset:");
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

  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Parameters:");

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
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Camera & Performance:");
  const double dist = std::sqrt(m_camera.position.x * m_camera.position.x +
                                m_camera.position.y * m_camera.position.y +
                                m_camera.position.z * m_camera.position.z);
  ImGui::Text("Distance: %.2f Rs", dist);
  ImGui::Text("Pos: (%.1f, %.1f, %.1f)", m_camera.position.x, m_camera.position.y,
              m_camera.position.z);
  ImGui::Text("FPS: %.1f (%.2f ms)", ImGui::GetIO().Framerate,
              1000.0f / (ImGui::GetIO().Framerate > 0 ? ImGui::GetIO().Framerate : 1.0f));
  if (m_flying) {
    if (m_flyTargetName.empty()) {
      ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Closing in...");
    } else {
      ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Flying to %s...",
                         m_flyTargetName.c_str());
    }
  }

  ImGui::Separator();
  ImGui::Text("Time:");
  bool paused = m_sceneManager->isPaused();
  float paceNow = m_sceneManager->getTimeScale();
  if (paused) {
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Paused (P to resume)");
  } else if (paceNow < 9.5f) {
    ImGui::Text("Pace: %.1fx (-/= to change)", static_cast<double>(paceNow));
  } else {
    ImGui::Text("Pace: %.0fx (-/= to change)", static_cast<double>(paceNow));
  }
  if (ImGui::Button(paused ? "Resume (P)" : "Pause (P)", ImVec2(110.0f, 0.0f))) {
    m_sceneManager->togglePause();
  }
  {
    // Adjustable time pace: Pause | 1x | 10x | 60x | 300x | 1000x
    // presets plus a log slider (1..1000). Numeric presets and the
    // slider resume a paused clock so the effect is visible.
    const char* paceLabels[kTimePacePresetCount] = {"1x", "10x", "60x", "300x", "1000x"};
    float avail = ImGui::GetContentRegionAvail().x;
    float gap = ImGui::GetStyle().ItemSpacing.x;
    float btnW = (avail - gap * 5.0f) / 6.0f;
    if (btnW < 28.0f) btnW = 28.0f;
    if (paused) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.38f, 0.12f, 1.0f));
    }
    if (ImGui::Button("II", ImVec2(btnW, 0.0f))) {
      if (!m_sceneManager->isPaused()) {
        m_sceneManager->togglePause();
      }
    }
    if (paused) {
      ImGui::PopStyleColor();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Pause time (P)");
    }
    for (int i = 0; i < kTimePacePresetCount; ++i) {
      ImGui::SameLine();
      bool active = !paused && std::abs(paceNow - kTimePacePresets[i]) < 0.001f;
      if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.45f, 0.65f, 1.0f));
      }
      if (ImGui::Button(paceLabels[i], ImVec2(btnW, 0.0f))) {
        if (m_sceneManager->isPaused()) {
          m_sceneManager->togglePause();
        }
        m_sceneManager->setTimeScale(kTimePacePresets[i]);
      }
      if (active) {
        ImGui::PopStyleColor();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Time pace %s", paceLabels[i]);
      }
    }
    float sliderV = paceNow < 1.0f ? 1.0f : paceNow;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderFloat("Speed", &sliderV, 1.0f, 1000.0f, "%.0fx",
                           ImGuiSliderFlags_Logarithmic)) {
      if (m_sceneManager->isPaused()) {
        m_sceneManager->togglePause();
      }
      m_sceneManager->setTimeScale(sliderV);
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Reset Camera", ImVec2(-1.0f, 0.0f))) {
    if (curSt) {
      m_camera.position = Vec3(0.0, 0.0, curSt->spawnDistanceRs);
      m_camera.yaw = 0.0;
      m_camera.pitch = 0.0;
      m_flying = false;
    }
  }

  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Journey Route:");

  std::string routeText;
  if (m_route.empty()) {
    routeText = "(empty)";
  } else if (m_route.size() <= 8) {
    for (size_t i = 0; i < m_route.size(); ++i) {
      if (i > 0) routeText += " > ";
      routeText += m_route[i];
    }
  } else {
    for (size_t i = 0; i < 4; ++i) {
      if (i > 0) routeText += " > ";
      routeText += m_route[i];
    }
    routeText += " > ... > ";
    for (size_t i = m_route.size() - 3; i < m_route.size(); ++i) {
      if (i > m_route.size() - 3) routeText += " > ";
      routeText += m_route[i];
    }
  }
  ImGui::TextWrapped("%s", routeText.c_str());

  float exportBtnWidth = 60.0f;
  float routeInputWidth = ImGui::GetContentRegionAvail().x - exportBtnWidth - ImGui::GetStyle().ItemSpacing.x;
  ImGui::SetNextItemWidth(routeInputWidth);
  ImGui::InputTextWithHint("##routefile", "File name (without .json)", m_routeFileBuf, sizeof(m_routeFileBuf));
  ImGui::SameLine();
  if (ImGui::Button("Export", ImVec2(exportBtnWidth, 0.0f))) {
    exportJourney();
  }
  if (ImGui::Button("Import", ImVec2(-1.0f, 0.0f))) {
    importJourney();
  }

  if (!m_journeyStatus.empty()) {
    if (m_journeyStatus.rfind("OK", 0) == 0) {
      ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", m_journeyStatus.c_str());
    } else {
      ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_journeyStatus.c_str());
    }
  }

  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Editor:");
  if (ImGui::Button("Add rock here")) {
    Vec3 f = m_camera.forward();
    Vec3 u = m_camera.up();
    Vec3 p(m_camera.position.x + f.x * 15.0,
            m_camera.position.y + f.y * 15.0,
            m_camera.position.z + f.z * 15.0);
    Vec3 v(f.x * 2.0 + u.x * 0.5,
            f.y * 2.0 + u.y * 0.5,
            f.z * 2.0 + u.z * 0.5);
    m_sceneManager->spawnRockAt(p, v);
  }
  ImGui::SameLine();
  bool hasSel = !m_sceneManager->getSelected().empty();
  if (!hasSel) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("Delete selected")) {
    m_sceneManager->deleteSelected();
  }
  if (!hasSel) {
    ImGui::EndDisabled();
  }
  bool collOn = m_sceneManager->collisionsOn();
  if (ImGui::Checkbox("Collisions", &collOn)) {
    m_sceneManager->setCollisions(collOn);
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear")) {
    m_sceneManager->clearCustom();
  }
  {
    size_t nCustom = m_sceneManager->getCustoms().size();
    std::string selName = "(none)";
    const std::string& selId = m_sceneManager->getSelected();
    if (!selId.empty()) {
      selName = selId;
      for (const auto& c : m_sceneManager->getCustoms()) {
        if (c.id == selId) {
          selName = c.name;
          break;
        }
      }
    }
    ImGui::Text("Rocks: %zu, selected: %s", nCustom, selName.c_str());
  }

  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Achievements: %d/7", static_cast<int>(m_unlockedIds.size()));
  for (const auto& achId : allAchievementIds()) {
    bool unlocked = (m_unlockedIds.find(achId) != m_unlockedIds.end());
    const char* title = achievementTitle(achId);
    const char* desc = achievementDescription(achId);
    if (unlocked) {
      ImGui::TextColored(ImVec4(120.0f / 255.0f, 230.0f / 255.0f, 140.0f / 255.0f, 1.0f), "%s", title);
      ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", desc);
    } else {
      ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.0f), "%s", title);
      ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.0f), "%s", desc);
    }
  }

  ImGui::Separator();
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

void AppGL::teleportToStation(const std::string& id) {
  m_sceneManager->teleportTo(id, m_camera);
  m_flying = false;
  if (m_route.empty() || m_route.back() != id) {
    m_route.push_back(id);
  }
  if (std::find(m_visited.begin(), m_visited.end(), id) == m_visited.end()) {
    m_visited.push_back(id);
  }
  checkAchievements();
}

void AppGL::exportJourney() {
  std::filesystem::create_directories("routes");
  std::string sanitized = journeySanitizeFileName(m_routeFileBuf);
  std::string path = "routes/" + sanitized + ".json";
  std::ofstream out(path);
  if (!out.is_open()) {
    m_journeyStatus = "Error: cannot write " + path;
    return;
  }
  out << journeySerialize(m_route);
  if (!out.good()) {
    m_journeyStatus = "Error: cannot write " + path;
    return;
  }
  m_journeyExported = true;
  m_journeyStatus = "OK: exported " + std::to_string(m_route.size()) + " stations to " + path;
  checkAchievements();
}

void AppGL::importJourney() {
  std::string sanitized = journeySanitizeFileName(m_routeFileBuf);
  std::string path = "routes/" + sanitized + ".json";
  std::ifstream in(path);
  if (!in.is_open()) {
    m_journeyStatus = "Error: file not found: " + path;
    return;
  }
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::vector<std::string> rawStations;
  std::string outError;
  if (!journeyDeserialize(text, rawStations, outError)) {
    m_journeyStatus = "Error: " + outError;
    return;
  }
  std::vector<std::string> validStations;
  size_t skipped = 0;
  for (const auto& sid : rawStations) {
    if (findStation(sid) != nullptr) {
      validStations.push_back(sid);
    } else {
      ++skipped;
    }
  }
  if (validStations.empty()) {
    m_journeyStatus = "Error: no known stations in file";
    return;
  }
  m_route = validStations;
  teleportToStation(validStations.back());
  m_journeyImported = true;
  m_journeyStatus = "OK: imported " + std::to_string(validStations.size()) + " stations";
  if (skipped > 0) {
    m_journeyStatus += " (skipped " + std::to_string(skipped) + " unknown)";
  }
  checkAchievements();
}

void AppGL::checkAchievements() {
  AchievementState st;
  st.visited = m_visited;
  st.routeSize = m_route.size();
  st.minBhDistance = m_minBhDistance;
  st.exported = m_journeyExported;
  st.imported = m_journeyImported;

  std::vector<std::string> unlocked = achievementsCheck(st);
  float now = static_cast<float>(glfwGetTime());
  for (const auto& id : unlocked) {
    if (m_unlockedIds.find(id) == m_unlockedIds.end()) {
      m_unlockedIds.insert(id);
      Toast t;
      t.title = achievementTitle(id);
      t.desc = achievementDescription(id);
      t.time = now;
      m_toasts.push_back(t);
    }
  }
}

void AppGL::drawHelpContent() {
  ImGui::BulletText("WASD move, Space / Shift up / down");
  ImGui::BulletText("Drag mouse to look around");
  ImGui::BulletText("/ focus station search");
  ImGui::BulletText("F5 export journey, F6 import journey");
  ImGui::BulletText("P pause / resume time");
  ImGui::BulletText("- / = time pace down / up");
  ImGui::BulletText("Double-click an object to fly to it");
  ImGui::BulletText("Del delete selected rock");
  ImGui::BulletText("H hide / show UI");
  ImGui::BulletText("F1 toggle this help");
  ImGui::BulletText("ESC back to menu");
}

void AppGL::drawHelpOverlay() {
  ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
  ImGui::Begin("Help (F1 to close)");
  drawHelpContent();
  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Press F1 or Close to hide");
  if (ImGui::Button("Close", ImVec2(-1.0f, 0.0f))) {
    m_helpOverlay = false;
  }
  ImGui::End();
}

void AppGL::drawMenu() {
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 menuCenter(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
  ImGui::SetNextWindowPos(menuCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_Always);
  ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
  ImGui::Begin("MainMenu", nullptr, menuFlags);

  if (m_menuPage == 0) {
    ImGui::SetWindowFontScale(2.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "ASTRO-VOYAGER");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "A journey through curved spacetime");
    ImGui::SetWindowFontScale(1.0f);

    if (ImGui::Button("Play", ImVec2(-1.0f, 40.0f))) {
      m_inMenu = false;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Resume your journey");
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Destinations:");
    for (const auto& station : builtinCatalog()) {
      std::string destLabel = station.name + " (" + station.id + ")";
      if (ImGui::Button(destLabel.c_str(), ImVec2(-1.0f, 0.0f))) {
        teleportToStation(station.id);
        m_inMenu = false;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", station.description.c_str());
      }
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Recent:");
    const Station* menuCur = m_sceneManager->getCurrentStation();
    std::string menuCurId = menuCur ? menuCur->id : "";
    std::vector<std::string> recentIds;
    std::set<std::string> recentSeen;
    for (size_t i = m_route.size(); i-- > 0;) {
      const std::string& rid = m_route[i];
      if (rid == menuCurId) continue;
      if (!findStation(rid)) continue;
      if (recentSeen.find(rid) != recentSeen.end()) continue;
      recentSeen.insert(rid);
      recentIds.push_back(rid);
      if (recentIds.size() >= 5) break;
    }
    if (recentIds.empty()) {
      ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(empty)");
    } else {
      for (const auto& rid : recentIds) {
        const Station* rst = findStation(rid);
        std::string recentLabel = rst ? (rst->name + " (" + rst->id + ")") : rid;
        if (ImGui::Button(recentLabel.c_str(), ImVec2(-1.0f, 0.0f))) {
          teleportToStation(rid);
          m_inMenu = false;
        }
        if (rst && ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", rst->description.c_str());
        }
      }
    }

    ImGui::Separator();
    float menuAvail = ImGui::GetContentRegionAvail().x;
    float menuBtnW = (menuAvail - 2.0f * ImGui::GetStyle().ItemSpacing.x) / 3.0f;
    if (ImGui::Button("Settings", ImVec2(menuBtnW, 0.0f))) {
      m_menuPage = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Help", ImVec2(menuBtnW, 0.0f))) {
      m_menuPage = 2;
    }
    ImGui::SameLine();
    if (ImGui::Button("Exit", ImVec2(menuBtnW, 0.0f))) {
      glfwSetWindowShouldClose(m_window->getNativeWindow(), GLFW_TRUE);
    }

    ImGui::Separator();
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "WASD fly | drag look | / search | dbl-click go");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "P time | F1 help | H hide UI | ESC menu");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Version %s", kVersion);
    ImGui::SetWindowFontScale(1.0f);
  } else if (m_menuPage == 1) {
    ImGui::Text("Settings");
    const char* presetNames[] = {"Low", "Med", "Ultra", "Custom"};
    int menuPreset = static_cast<int>(m_sceneManager->getPreset());
    if (ImGui::Combo("Preset", &menuPreset, presetNames, 4)) {
      m_userOverrodePreset = true;
      m_benchmarking = false;
      if (menuPreset >= 0 && menuPreset <= 2) {
        m_sceneManager->applyPreset(static_cast<PerfPreset>(menuPreset));
      } else {
        m_sceneManager->setPreset(PerfPreset::Custom);
      }
    }
    RenderSettings& menuSettings = m_sceneManager->getSettings();
    bool menuChanged = false;
    menuChanged |= ImGui::SliderFloat("FOV", &menuSettings.fovDegrees, 30.0f, 110.0f, "%.1f deg");
    float menuExposure = m_sceneManager->getExposure();
    if (ImGui::SliderFloat("Exposure", &menuExposure, 0.3f, 2.0f, "%.2f")) {
      m_sceneManager->setExposure(menuExposure);
      menuChanged = true;
    }
    menuChanged |= ImGui::Checkbox("Disk", &menuSettings.diskOn);
    menuChanged |= ImGui::Checkbox("Labels", &m_showLabels);
    bool menuOrbits = m_sceneManager->showOrbits();
    if (ImGui::Checkbox("Orbits", &menuOrbits)) {
      m_sceneManager->setShowOrbits(menuOrbits);
      menuChanged = true;
    }
    bool menuBelt = m_sceneManager->showBelt();
    if (ImGui::Checkbox("Belt", &menuBelt)) {
      m_sceneManager->setShowBelt(menuBelt);
      menuChanged = true;
    }
    if (menuChanged) {
      m_userOverrodePreset = true;
      m_benchmarking = false;
      m_sceneManager->setPreset(PerfPreset::Custom);
    }
    if (ImGui::Button("Back", ImVec2(-1.0f, 0.0f))) {
      m_menuPage = 0;
    }
  } else {
    ImGui::Text("Help - Controls");
    drawHelpContent();
    if (ImGui::Button("Back", ImVec2(-1.0f, 0.0f))) {
      m_menuPage = 0;
    }
  }

  ImGui::End();
}

void AppGL::drawWorldLabels() {
  if (!m_showLabels) return;
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
  } else if (curSt->type == StationType::SolarSystem) {
    const std::vector<BodyState>& bodies = m_sceneManager->getBodies();
    if (!bodies.empty()) {
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

      for (const auto& body : bodies) {
        if (!body.def) continue;
      glm::vec3 pos(static_cast<float>(body.pos.x),
                        static_cast<float>(body.pos.y),
                        static_cast<float>(body.pos.z));
        glm::vec4 clip = mvp * glm::vec4(pos, 1.0f);
        if (clip.w <= 0.001f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;

        double dx = cp.x - static_cast<double>(pos.x);
        double dy = cp.y - static_cast<double>(pos.y);
        double dz = cp.z - static_cast<double>(pos.z);
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        float sx = (ndc.x * 0.5f + 0.5f) * displaySize.x;
        float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;

        float circleRadius = 4.0f;
        ImU32 circleColor = IM_COL32(255, 245, 225, 220);
        drawList->AddCircle(ImVec2(sx, sy), circleRadius, circleColor, 16, 1.5f);

        float nameFontSize = 13.0f;
        ImVec2 textPos(sx + circleRadius + 5.0f, sy - nameFontSize * 0.55f);
        ImU32 nameColor = IM_COL32(255, 255, 255, 255);
        drawList->AddText(font, nameFontSize, textPos, nameColor, body.def->name.c_str());

        float currentY = textPos.y + nameFontSize + 2.0f;
        if (dist < 30.0) {
          ImU32 detailColor = IM_COL32(170, 200, 230, 230);
          for (const auto& fact : body.def->facts) {
            drawList->AddText(font, 12.0f, ImVec2(textPos.x, currentY), detailColor, fact.c_str());
            currentY += 14.0f;
          }
        }
        if (dist < 12.0) {
          char distBuf[64];
          std::snprintf(distBuf, sizeof(distBuf), "distance: %.1f units", dist);
          ImU32 distColor = IM_COL32(100, 230, 120, 255);
          drawList->AddText(font, 12.0f, ImVec2(textPos.x, currentY), distColor, distBuf);
        }
      }
      const std::string& selId = m_sceneManager->getSelected();
      if (!selId.empty()) {
        for (const auto& custom : m_sceneManager->getCustoms()) {
          if (custom.id != selId) continue;
          glm::vec3 cpos(static_cast<float>(custom.pos.x),
                         static_cast<float>(custom.pos.y),
                         static_cast<float>(custom.pos.z));
          glm::vec4 clip = mvp * glm::vec4(cpos, 1.0f);
          if (clip.w <= 0.001f) break;
          glm::vec3 ndc = glm::vec3(clip) / clip.w;
          if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) break;
          float sx = (ndc.x * 0.5f + 0.5f) * displaySize.x;
          float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;
          ImU32 amber = IM_COL32(255, 190, 70, 255);
          ImU32 amberDim = IM_COL32(255, 190, 70, 140);
          drawList->AddCircle(ImVec2(sx, sy), 10.0f, amber, 24, 2.0f);
          drawList->AddCircle(ImVec2(sx, sy), 13.0f, amberDim, 24, 1.0f);
          ImVec2 textPos(sx + 13.0f + 5.0f, sy - 13.0f * 0.55f);
          ImU32 nameColor = IM_COL32(255, 255, 255, 255);
          drawList->AddText(font, 13.0f, textPos, nameColor, custom.name.c_str());
          break;
        }
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

    // M31 cluster labels: fixed features, base offsets measured at zoom == 1
    // (fractions of screen center scaling). px = cx + bx*zoom*W*0.25.
    struct ClusterLabel {
      const char* name;
      const char* fact;
      float bx;
      float by;
    };
    const ClusterLabel kClusters[] = {
        {"G1", "Mayall II, brightest globular of M31", -0.18f, -0.12f},
        {"NGC 206", "Giant star cloud in spiral arm", 0.14f, 0.10f},
        {"M32", "Compact elliptical companion", 0.20f, -0.14f},
        {"M110", "Elliptical satellite galaxy", -0.24f, 0.16f},
    };
    double zoom = spawn / std::max(dist, 1e-3);
    float cx = displaySize.x * 0.5f;
    float cy = displaySize.y * 0.5f;
    for (const auto& cl : kClusters) {
      float clx = cx + cl.bx * static_cast<float>(zoom) * displaySize.x * 0.25f;
      float cly = cy + cl.by * static_cast<float>(zoom) * displaySize.y * 0.25f;
      if (clx < 24.0f || cly < 24.0f) continue;
      if (clx > displaySize.x - 24.0f || cly > displaySize.y - 24.0f) continue;
      if (ratio > 0.65) continue;
      float ringR = 5.0f;
      ImU32 ringCol = IM_COL32(255, 245, 225, 220);
      drawList->AddCircle(ImVec2(clx, cly), ringR, ringCol, 16, 1.5f);
      ImVec2 namePos(clx + ringR + 5.0f, cly - 13.0f * 0.55f);
      ImU32 nameCol = IM_COL32(255, 255, 255, 255);
      drawList->AddText(font, 13.0f, namePos, nameCol, cl.name);
      if (ratio <= 0.35) {
        ImU32 factCol = IM_COL32(170, 200, 230, 230);
        drawList->AddText(font, 12.0f, ImVec2(namePos.x, namePos.y + 13.0f + 2.0f),
                          factCol, cl.fact);
      }
    }

    // M31 close range: 4 bright named stars as real 3D points (D < 10).
    // NGC 206 / G1 clouds already labeled above (kClusters), not duplicated.
    if (dist < 10.0) {
      float m31Aspect = displaySize.x / displaySize.y;
      float m31Fov = m_sceneManager->getSettings().fovDegrees;
      glm::mat4 m31Proj = glm::perspective(glm::radians(m31Fov), m31Aspect, 0.1f, 1000.0f);
      glm::vec3 m31Eye(static_cast<float>(cp.x), static_cast<float>(cp.y),
                       static_cast<float>(cp.z));
      Vec3 m31Cf = m_camera.forward();
      Vec3 m31Cu = m_camera.up();
      glm::vec3 m31Fwd(static_cast<float>(m31Cf.x), static_cast<float>(m31Cf.y),
                       static_cast<float>(m31Cf.z));
      glm::vec3 m31Up(static_cast<float>(m31Cu.x), static_cast<float>(m31Cu.y),
                      static_cast<float>(m31Cu.z));
      glm::mat4 m31View = glm::lookAt(m31Eye, m31Eye + m31Fwd, m31Up);
      glm::mat4 m31Mvp = m31Proj * m31View;
      for (const auto& bs : m_sceneManager->m31BrightStars()) {
        glm::vec4 clip = m31Mvp * glm::vec4(bs.pos, 1.0f);
        if (clip.w <= 0.001f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.x < -1.02f || ndc.x > 1.02f || ndc.y < -1.02f || ndc.y > 1.02f) continue;
        float sx = (ndc.x * 0.5f + 0.5f) * displaySize.x;
        float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;
        float ringR = 5.0f;
        ImU32 ringCol = IM_COL32(255, 245, 225, 230);
        drawList->AddCircle(ImVec2(sx, sy), ringR, ringCol, 16, 1.5f);
        ImVec2 namePos(sx + ringR + 5.0f, sy - 13.0f * 0.55f);
        ImU32 nameCol = IM_COL32(255, 255, 255, 255);
        drawList->AddText(font, 13.0f, namePos, nameCol, bs.name.c_str());
        ImU32 factCol = IM_COL32(170, 200, 230, 230);
        drawList->AddText(font, 12.0f, ImVec2(namePos.x, namePos.y + 13.0f + 2.0f),
                          factCol, bs.fact.c_str());
      }
    }
  }

  // Draw toasts in bottom-right corner, max 3 at a time, growing bottom-to-top with 8px gap
  float now = static_cast<float>(glfwGetTime());
  m_toasts.erase(
      std::remove_if(m_toasts.begin(), m_toasts.end(),
                     [now](const Toast& t) { return (now - t.time) >= 4.0f; }),
      m_toasts.end());

  size_t toastCount = std::min<size_t>(m_toasts.size(), 3);
  float curToastBottomY = displaySize.y - 16.0f;
  for (size_t i = 0; i < toastCount; ++i) {
    const auto& t = m_toasts[i];
    float age = now - t.time;
    if (age < 0.0f) age = 0.0f;
    float alphaRatio = (age > 3.0f) ? (4.0f - age) : 1.0f;
    alphaRatio = std::clamp(alphaRatio, 0.0f, 1.0f);

    std::string line1 = "Achievement: " + t.title;
    std::string line2 = t.desc;

    ImVec2 sz1 = font->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, line1.c_str());
    ImVec2 sz2 = font->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, line2.c_str());

    float padX = 14.0f;
    float padY = 8.0f;
    float lineSpacing = 3.0f;

    float toastW = std::max(sz1.x, sz2.x) + padX * 2.0f;
    float toastH = sz1.y + lineSpacing + sz2.y + padY * 2.0f;

    float toastX = displaySize.x - toastW - 16.0f;
    float toastY = curToastBottomY - toastH;

    ImU32 bgCol = IM_COL32(10, 15, 25, static_cast<int>(220.0f * alphaRatio));
    ImU32 borderCol = IM_COL32(255, 190, 70, static_cast<int>(160.0f * alphaRatio));
    ImU32 titleCol = IM_COL32(255, 255, 255, static_cast<int>(255.0f * alphaRatio));
    ImU32 descCol = IM_COL32(180, 180, 180, static_cast<int>(200.0f * alphaRatio));

    drawList->AddRectFilled(ImVec2(toastX, toastY), ImVec2(toastX + toastW, toastY + toastH), bgCol, 6.0f);
    drawList->AddRect(ImVec2(toastX, toastY), ImVec2(toastX + toastW, toastY + toastH), borderCol, 6.0f, 0, 1.0f);

    drawList->AddText(font, 14.0f, ImVec2(toastX + padX, toastY + padY), titleCol, line1.c_str());
    drawList->AddText(font, 12.0f, ImVec2(toastX + padX, toastY + padY + sz1.y + lineSpacing), descCol, line2.c_str());

    curToastBottomY = toastY - 8.0f;
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
      "sgra", "qso-3c273", "ori", "uma", "m31", "m1", "sol", "m87"
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
  // 1. M31 Andromeda Galaxy close-up inside the starfield (D = 8:
  //    volumetric M31Stars at full alpha + named bright star labels).
  app.m_sceneManager->teleportTo("m31", app.m_camera);
  app.m_camera.position.z = 8.0;
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

  // 3. SgrA* close-up (D = 5 Rs: shadow fills half the screen, exercises
  //    the close-camera shadow mask + disk smoothing + pole damping).
  app.m_sceneManager->teleportTo("sgra", app.m_camera);
  app.m_camera.position = Vec3(0.0, 0.0, 5.0);
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
  std::string sgraCloseFile = outDir + "/sgra_close.png";
  if (app.saveScreenshot(sgraCloseFile)) {
    std::cout << "Captured screenshot: " << sgraCloseFile << "\n";
  }

  // 4. Quasar close-up (D = 5.0: u_QZoom = 5, shadow well resolved, checks
  //    the dark shadow + ring-free core up close. A bigger zoom would burn
  //    the core white (1/(1+r^2*4000) fills the frame), hiding ring/disk/jets.
  app.m_sceneManager->teleportTo("qso-3c273", app.m_camera);
  app.m_camera.position = Vec3(0.0, 0.0, 5.0);
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
  std::string qsoCloseFile = outDir + "/qso_close.png";
  if (app.saveScreenshot(qsoCloseFile)) {
    std::cout << "Captured screenshot: " << qsoCloseFile << "\n";
  }

  return 0;
}

}  // namespace astro
