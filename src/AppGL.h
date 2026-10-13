#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Camera.h"
#include "SceneManager.h"
#include "Tour.h"
#include "Window.h"

// === Cinematic ===: forward-decls для ImGui-типов (imgui.h тянуть в хедер не
// нужно; дефолтный шрифт и чужие виджеты не тронуты).
struct ImDrawList;
struct ImFont;

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
  void drawMenu();
  void drawHelpContent();
  void drawHelpOverlay();
  void drawWorldLabels();
  void teleportToStation(const std::string& id);
  void flyToObjectAt(double px, double py);
  std::string pickCustomAt(double px, double py);
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
  bool m_prevPPressed = false;
  bool m_prevMinusPressed = false;
  bool m_prevEqualPressed = false;
  void stepTimePace(int dir);

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
  bool m_prevDeletePressed = false;

  // Main menu / overlay state
  bool m_inMenu = true;
  int m_menuPage = 0;  // 0 main, 1 settings, 2 help
  bool m_hideUI = false;
  bool m_helpOverlay = false;
  bool m_showLabels = true;
  bool m_prevEscPressed = false;
  bool m_prevHPressed = false;
  bool m_prevF1Pressed = false;

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

  // === Cinematic === (агент C: кинотур, интро-карточки, HUD-полиш).
  // Весь новый штат только здесь. Чужие блоки выше не тронуты:
  // поиск, варп (fly-to), время, меню, скриншоты — без изменений.
  // Всё рисуется из drawCinematic(), который харнесс скриншотов
  // (captureAllScreenshots) не вызывает: PNG остаются чистыми.
  Tour m_tour;
  Vec3 m_tourBasePos{0.0, 0.0, 10.0};
  double m_tourBaseDist = 10.0;
  bool m_prevTPressed = false;
  bool m_prevSpacePressed = false;
  // Интро-карточка ручного телепорта (3с, гаснет сама).
  bool m_cardActive = false;
  float m_cardT = 0.0f;
  std::string m_cardName;
  std::string m_cardTypeRu;
  std::string m_cardFact;
  double m_cardDist = 0.0;
  // Полоса-подсказка: первые 10с игровой сессии, гаснет сама.
  float m_sessionT = 0.0f;
  // Вспышка-трофей (вместо звука): ведём увиденные достижения локально,
  // checkAchievements() не трогаем.
  float m_trophyT = -10.0f;
  std::string m_trophyTitle;
  std::set<std::string> m_trophyKnown;
  // Баннер финиша тура (4с).
  float m_finishT = -10.0f;
  // Шрифт с кириллицей для русских строк (дефолтный шрифт не трогаем,
  // иначе поплывут эталонные скриншоты). nullptr = fallback на дефолт.
  struct ImFont* m_cineFont = nullptr;
  void startCineTour();
  void stopCineTour();
  void tourTeleport(const std::string& id);
  void showIntroCard(const Station* st);
  // Возвращает true, если ввод consumed (тур владеет камерой/клавишами).
  bool processCinematicInput(float deltaTime);
  void updateCinematic(float deltaTime);
  void drawCinematic();
  void drawCineTopBar(ImDrawList* dl, ImFont* font, float distNow);
  void drawCineCard(ImDrawList* dl, ImFont* font);
  void drawCineCaption(ImDrawList* dl, ImFont* font);
  void drawCineHint(ImDrawList* dl, ImFont* font);
  void drawCineTrophy(ImDrawList* dl, ImFont* font, float now);
  void drawCineFade(ImDrawList* dl);
  static const char* cineTypeRu(StationType t);

  // === WarpDrive === (агент W: отдельный режим путешествий на варпе).
  // Весь новый штат только здесь. Чужие блоки выше и блок Cinematic
  // не тронуты. Панель рисуется отдельным окном drawWarpPanel() и только
  // при активном варпе, поэтому харнесс скриншотов (варп выключен,
  // u_WarpK = 0) остаётся байт-в-байт. Состояния: 0 Off, 1 Cruise,
  // 2 Charging (обратный отсчёт 3с), 3 Jump (туннель 4с), 4 Cooldown (2с).
  int m_warpStatus = 0;
  bool m_warpOn = false;
  int m_warpFactor = 1;  // 1..9, скорость логарифмическая
  float m_warpK = 0.0f;  // визуальная интенсивность 0..1 (ease 1.5с)
  float m_warpKFrom = 0.0f;
  float m_warpKTo = 0.0f;
  float m_warpKAnim = 1.0f;
  float m_warpFlash = 0.0f;  // вспышка входа/выхода/прибытия (свой множитель)
  float m_warpBaseFov = 60.0f;
  float m_warpLastFov = 60.0f;
  bool m_warpFovDirty = false;
  float m_warpStateT = 0.0f;  // таймер Charging/Jump/Cooldown
  std::string m_warpJumpTarget;
  int m_warpJumpFromFactor = 1;
  int m_warpSel = 0;  // выбор пункта в варп-панели
  bool m_prevWPressed = false;
  bool m_prevLBracketPressed = false;
  bool m_prevRBracketPressed = false;
  float m_warpShakePhase = 0.0f;
  float m_warpShakeYaw = 0.0f;
  float m_warpShakePitch = 0.0f;
  void warpRetarget(float to);
  void disengageWarp();
  void startWarpJump(const std::string& id);
  // Возвращает true, если кадр consumed (катсцена прыжка: WASD глушим,
  // осмотр мышью живёт). Вызывать в начале processInput().
  bool updateWarp(float deltaTime);
  // Снимает тряску камеры после render(). Вызывать сразу после него.
  void finishWarpFrame();
  void drawWarpPanel();  // отдельное окно, только при m_warpOn
  static double warpSpeedLY(int factor);  // показушные св.лет/с от фактора
  static float warpTargetK(int factor);   // целевой u_WarpK от фактора
  static const char* warpStatusName(int status);
};

// Связь AppGL -> SceneManager без правок SceneManager.h:
// состояние оверлея (u_WarpK/u_Flash), дефолт 0. Определено в
// SceneManager.cpp в блоке // === WarpDrive ===.
namespace warpdrive {
void setState(float k, float flash);
}  // namespace warpdrive

}  // namespace astro
