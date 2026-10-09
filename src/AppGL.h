#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Camera.h"
#include "SceneManager.h"
#include "Window.h"

namespace astro {

class AppGL {
 public:
  AppGL(int width = 1280, int height = 720, bool visible = true);
  ~AppGL();

  AppGL(const AppGL&) = delete;
  AppGL& operator=(const AppGL&) = delete;

  int run();
  bool saveScreenshot(const std::string& filepath);
  static int captureAllScreenshots(const std::string& outDir = "docs/screens", bool diskOn = false);

 private:
  void processInput(float deltaTime);
  void processMouse();
  void drawImGui();
  void drawWorldLabels();
  void teleportToStation(const std::string& id);
  void exportJourney();
  void importJourney();

  std::unique_ptr<Window> m_window;
  std::unique_ptr<SceneManager> m_sceneManager;
  Camera m_camera;

  float m_lastMouseX = 640.0f;
  float m_lastMouseY = 360.0f;
  bool m_firstMouse = true;
  bool m_mouseDragging = false;
  std::string m_lastScreenshotStatus;

  // Auto-performance benchmarking
  bool m_benchmarking = true;
  bool m_userOverrodePreset = false;
  float m_benchmarkTimer = 0.0f;
  int m_benchmarkFrames = 0;

  // Station fuzzy search
  char m_searchBuf[64] = {};
  bool m_searchFocusRequest = false;
  bool m_prevSlashPressed = false;

  // Journey route history
  std::vector<std::string> m_route;
  char m_routeFileBuf[64] = "journey";
  std::string m_journeyStatus;
  bool m_prevF5Pressed = false;
  bool m_prevF6Pressed = false;
};

}  // namespace astro
