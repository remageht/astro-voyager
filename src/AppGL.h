#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Camera.h"
#include "SceneManager.h"
#include "Window.h"

namespace astro {

struct Toast {
  std::string title;
  std::string desc;
  float time = 0.0f;
};

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
  void flyToObjectAt(double px, double py);
  void startObjectFlight(const Vec3& objPos, double viewDist,
                         const std::string& name, const Vec3& lookTarget);
  void startDollyFlight();
  void computeLookYawPitch(const Vec3& fromPos, const Vec3& target,
                           double& outYaw, double& outPitch);
  void updateFlight(float deltaTime);
  void exportJourney();
  void importJourney();
  void checkAchievements();

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

  // Achievements
  std::vector<std::string> m_visited;
  std::set<std::string> m_unlockedIds;
  double m_minBhDistance = 1e9;
  bool m_journeyExported = false;
  bool m_journeyImported = false;
  std::vector<Toast> m_toasts;

  // Double-click detection (left-button press edges)
  double m_lastPressTime = -10.0;
  double m_lastPressX = 0.0;
  double m_lastPressY = 0.0;
  bool m_prevLeftDown = false;

  // Smooth fly-to flight state
  bool m_flying = false;
  float m_flyT = 0.0f;
  float m_flyDur = 1.2f;
  Vec3 m_flyFrom;
  Vec3 m_flyTo;
  double m_yawFrom = 0.0;
  double m_yawTo = 0.0;
  double m_pitchFrom = 0.0;
  double m_pitchTo = 0.0;
  std::string m_flyTargetName;
};

}  // namespace astro
