#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Camera.h"
#include "SceneManager.h"
#include "Tour.h"
#include "Types.h"
#include "Window.h"
// === WarpJump === (агент J: чистая логика катсцены, без GL; include рядом
// с Tour.h-образцом, остальной хедер не тронут).
#include "WarpJump.h"
// === Audio === begin (агент AU: процедурный звук, только GL-таргет).
#include "audio/AudioEngine.h"
// === Audio === end

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
  // Харнесс пинит FPS-текст в 60: иначе gui.png флейкает от wall-clock.
  static inline bool s_harnessPinFps = false;

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

  // Object editor (v3.2.0): palette state for spawning custom bodies and
  // live-editing the selected one.
  int m_editorType = 1;  // 0 rock, 1 rocky planet, 2 moon, 3 gas giant, 4 star
  float m_editorRadius = 0.60f;
  float m_editorColA[3] = {0.45f, 0.42f, 0.38f};
  float m_editorColB[3] = {0.28f, 0.26f, 0.24f};
  bool m_editorRing = false;
  float m_editorSeed = 0.0f;

  // Achievements
  std::vector<std::string> m_visited;
  std::set<std::string> m_unlockedIds;
  double m_minBhDistance = 1e9;
  bool m_journeyExported = false;
  bool m_journeyImported = false;
  std::vector<Toast> m_toasts;

  // === Physics === (horizon capture / slide state)
  bool m_captureAnim = false;
  float m_captureT = 0.0f;
  bool m_capturedHorizon = false;
  float m_captureBaseFov = 60.0f;
  void startHorizonCapture();
  void updateHorizonCapture(float dt);
  void drawHorizonCapture(ImDrawList* dl);

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

  // === ModernUI === begin (агент UI: современная тема + раскладка).
  // Только оформление и кнопки-индикаторы поверх существующего состояния:
  // варп/тур/время/поиск/звук не меняем, новую логику не вводим.
  // Звук: аудиоподсистемы в дереве пока нет (параллельный агент AU) —
  // статус-бар показывает нейтральный плейсхолдер, точка вставки ниже.
  void applyModernThemeOnce();
  void drawModernStatusBar();
  void drawModernTransportBar();
  // RU-подпись типа станции для чипа карточки (возврат EN при виде
  // из харнесса не нужен: выбор делает вызывающий через uiRu()).
  static const char* stationTypeRuShort(StationType t);
  static unsigned int stationTypeChip(StationType t);
  // RU-строка, если есть киношрифт с кириллицей, иначе EN-фолбэк
  // (дефолтный шрифт ImGui кириллицы не имеет — были бы "?").
  const char* uiRu(const char* ru, const char* en) const;
  bool modernPushFont() const;
  // === ModernUI === end

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
  int m_shipStyle = 0;  // корпус корабля: 0 Dart, 1 Ring Ship (IXS), 2 Shuttle
  bool m_prevWPressed = false;
  bool m_prevXPressed = false;
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

  // === WarpJump === begin (агент J: кинематографичный варп-прыжок).
  // Контроллер живёт ПОВЕРХ статусов 2/3/4, их семантику не ломает: пока
  // m_jumpCine активен, updateWarp отдаёт управление updateJumpCine, а
  // m_warpStatus маппится на старые значения (2 = Charge/Pullback и первая
  // половина Burst, 3 = пик Burst/Travel/Arrival, 4 = Pushin/Cooldown) —
  // UI-панель и аудио-polling (playJump-фронт на входе в 3, setWarpLevel
  // через m_warpK) продолжают работать без правок. Прыжок запускается из
  // startWarpJump. Дефолт: m_jumpCine = false, m_jump в Idle — харнесс
  // идёт старым путём, PNG байт-в-байт.
  WarpJump m_jump;
  bool m_jumpCine = false;
  JumpPhase m_jumpPrevPhase = JumpPhase::Idle;
  std::string m_jumpTarget;
  bool m_jumpDidTeleport = false;
  float m_jumpBurst = 0.0f;  // u_Burst для warp.frag (дефолт 0)
  // TP-риг: корабль и FP запомнены на входе в Pullback (rebased в пик Burst
  // после телепорта — скачок прячет белый flash=1).
  Vec3 m_jumpShipPos{0.0, 0.0, 0.0};
  Vec3 m_jumpShipDir{0.0, 0.0, -1.0};
  Vec3 m_jumpFpPos{0.0, 0.0, 10.0};
  double m_jumpFpYaw = 0.0;
  double m_jumpFpPitch = 0.0;
  Vec3 m_jumpCamPos{0.0, 0.0, 10.0};  // текущая TP-позиция (пружина)
  Vec3 m_jumpCamVel{0.0, 0.0, 0.0};   // скорость пружины
  Vec3 m_jumpPushinPos{0.0, 0.0, 10.0};  // TP-старт возврата
  double m_jumpPushinYaw = 0.0;
  double m_jumpPushinPitch = 0.0;
  // Возвращает true, если кадр consumed (катсцена держит камеру: WASD
  // глушим; в Cooldown возвращает false — управление уже у игрока).
  // Вызывать из updateWarp, пока m_jumpCine активен.
  bool updateJumpCine(float deltaTime);
  // ESC-прерывание: всегда в Idle + восстановление FP-камеры.
  void abortJumpCine();
  // === WarpJump === end

  // === Audio === begin (агент AU: процедурный звук без бинарников).
  // Весь новый штат только здесь. Чужие блоки выше (поиск/варп/тур/время/
  // меню/скриншоты/ModernUI) не тронуты. Движок лениво стартует при ПЕРВОМ
  // вводе пользователя; харнесс captureAllScreenshots run() не вызывает,
  // поэтому PNG чистые. Все вызовы движка — только из Audio-якоря в .cpp.
  AudioEngine m_audio;
  int m_audioVolume = 70;  // 0..100 для слайдера панели
  bool m_prevMPressed = false;
  // Polling-триггеры SFX (без правок чужих мест): предыдущие состояния.
  std::string m_audioPrevStation;
  size_t m_audioPrevUnlocked = 0;
  int m_audioPrevWarpStatus = 0;
  int m_audioPrevTourIndex = -1;
  bool m_audioPrevTourActive = false;
  bool m_audioPrevInMenu = true;
  double m_audioCaptureCooldown = 0.0;
  double m_sunLinkCooldown = 0.0;  // v3.1.0: кулдаун автопрыжка Солнце->sol
  bool m_audioWasInit = false;
  double m_audioTickAcc = 0.0;  // v3.0.0: фаза тика пульсара (сброс при уходе)
  void audioEnsureOnFirstInput();
  void audioUpdate(float deltaTime);
  void audioProcessKey();
  void audioDrawPanel();
  // === Audio === end

  // === Beauty === (v2.7: фото-режим F8, автоорбита O, часы гравитации).
  // Харнесс сюда не заходит:PNG байт-в-байт.
  bool m_photoMode = false;   // F8: скрыть UI + попап exposure/Save PNG
  bool m_prevF8Pressed = false;
  bool m_orbitOn = false;     // O: камера кружит вокруг фокуса станции
  bool m_prevOPressed = false;
  float m_orbitAngle = 0.0f;  // rad, накапливается каждый кадр
  void updateOrbit(float deltaTime);  // вызывать в processInput до WASD
  void drawPhotoPopup();              // вызывать в drawImGui при m_photoMode
  // === Beauty === end

  // === Ship survival === (v3.3.0) begin.
  // Топливо 0..100 (−5 за варп-прыжок, регенерация у звезды), щиты 0..100
  // (−10/с в зоне пульсара), гравитационный дрейф, респавн на "sol".
  // updateShip зовётся из run() до render(); харнесс run() не вызывает —
  // сцена и пост-эффекты на дефолтах остаются байт-в-байт (меняется только
  // gui.png: снизу слева появляется HUD корабля на 100/100).
  ShipState m_ship;
  std::string m_shipNotice;    // короткий нотис в HUD (нехватка топлива и т.п.)
  float m_shipNoticeT = 0.0f;  // сек; >0 — нотис виден
  void updateShip(float deltaTime);
  void drawShipHud();
  // === Ship survival === end
};

// Связь AppGL -> SceneManager без правок SceneManager.h:
// состояние оверлея (u_WarpK/u_Flash), дефолт 0. Определено в
// SceneManager.cpp в блоке // === WarpDrive ===.
// === WarpJump === (агент J): +u_Burst и корабль-силуэт, дефолты 0/скрыт.
namespace warpdrive {
void setState(float k, float flash);
void setBurst(float b);
void setShip(bool v, float x, float y, float z, float yaw);
void setShipStyle(int s);  // 0 Dart, 1 Ring Ship (IXS), 2 Shuttle
}  // namespace warpdrive

}  // namespace astro
