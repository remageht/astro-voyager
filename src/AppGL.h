#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "Camera.h"
#include "Expedition.h"
#include "SaveGame.h"
#include "SceneManager.h"
#include "StoryJournal.h"
#include "StorySlides.h"
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

// === MenuSky ===: forward-decls (Shader/ScreenQuad отвечаются в .cpp).
class Shader;
class ScreenQuad;

struct Toast {
  std::string title;
  std::string desc;
  float time = 0.0f;
  float duration = 4.0f;
  bool isMini = false;
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
  static int captureStationCheck(const std::string& outPath);

 private:
  void processInput(float deltaTime);
  void processMouse();
  void drawImGui();
  void drawMenu();
  void drawHelpContent();
  void drawHelpOverlay();
  void drawWorldLabels();
  void drawStoryPanel();
  // v3.8.0: stopJumpCine=false — только для телепорта Бурста в jump-катсцене
  // (внутренний вызов не должен убивать собственный прыжок).
  void teleportToStation(const std::string& id, bool stopJumpCine = true);
  // === Expedition === (v3.5.0): сюжетная арка + вводные слайды.
  void drawSlidesOverlay();
  void drawExpeditionHud();
  void drawExpeditionMap();
  void startExpeditionIntro();
  void openChapterSlide(int slideIdx);
  // v4.0: "Новая игра" — пролог Егорки + создание слота Adventure.
  void startNewAdventure();
  void saveAdventureSlotNow(bool quiet);
  unsigned int loadUiTexture(const std::string& rel, int* w, int* h);
  void saveExpeditionFileNow();
  void loadExpeditionFileNow();
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

  // === StoryJournal === (v3.5.0): бортовой журнал (J), анлоки выводятся
  // из того же прогресса, что и ачивки/сейв — без изменений схемы сейва.
  StoryJournal m_story;
  bool m_storyOpen = false;
  bool m_prevJPressed = false;

  // === Expedition === (v3.5.0): сюжетная арка + вводные слайды (рестайл).
  ExpeditionState m_expedition;
  bool m_expeditionMapOpen = false;
  bool m_prevNPressed = false;  // N — карта глав (M занят аудио-mute)
  std::vector<StorySlide> m_slidesCurrent;
  std::string m_slidesHead;  // заголовок слайда главы (для done-карточек)
  bool m_slidesOpen = false;
  bool m_slidesIntro = false;
  // v4.0: пролог кнопки "Новая игра" (Егорка) — тот же оверлей слайдов,
  // но в конце создаётся слот save-adventure.json.
  bool m_slidesAdv = false;
  int m_slidesIdx = 0;
  // v4.0: ленивые 2D-текстуры для фотографических слайдов (res/story/*.jpg).
  struct UiTex { unsigned int id = 0; int w = 0; int h = 0; };
  std::unordered_map<std::string, UiTex> m_uiTex;
  // v4.0: автоплей видео-слайда (одна активная GL-текстура кадра).
  double m_slideVStart = -1.0;
  int m_videoFrames = 0;
  int m_videoIdx = -1;
  unsigned int m_videoTex = 0;
  int m_videoW = 0;
  int m_videoH = 0;
  // P1: player-facing prologue animation state (typewriter + Ken-Burns).
  // The screenshot harness never touches these; it keeps the old static path.
  int m_slideAnimIdx = -1;
  double m_slideTextStart = 0.0;
  double m_slideShownAt = 0.0;
  static unsigned int uploadRgba(const unsigned char* px, int w, int h);
  void invalidateVideoSlide();

  // === MenuSky === (v3.5.0): "живой космос" за главным меню — фуллскрин-
  // шейдерный бэкдроп (туманность + MW-полоса + параллакс-звёзды), ленивая
  // инициализация при первом кадре меню; не зависит от текущей станции.
  std::unique_ptr<Shader> m_menuSkyShader;
  std::unique_ptr<ScreenQuad> m_menuSkyQuad;
  bool m_menuSkyTried = false;

  // === Physics === (horizon capture / slide state)
  bool m_captureAnim = false;
  float m_captureT = 0.0f;
  bool m_capturedHorizon = false;
  float m_captureBaseFov = 60.0f;
  bool m_horizonWarped = false;  // v3.6.0: перенос к точке входа уже сделан
  // v3.6.0: инерция движения — накопленная скорость свободного полёта и
  // признак активного ввода WASD (для орбитального ассиста).
  Vec3 m_shipVel;
  bool m_moveInputActive = false;
  bool m_orbitNoticeShown = false;
  // v3.6.0: управление — чувствительность/инверсия мыши и переназначение
  // клавиш (числа = дефолты GLFW; меняются на странице «Controls»).
  float m_mouseSensitivity = 1.0f;
  bool m_invertY = false;
  int m_keyColony = 67;   // GLFW_KEY_C
  int m_keyView = 86;     // GLFW_KEY_V
  int m_keyMap = 78;      // GLFW_KEY_N
  int m_keyJournal = 74;  // GLFW_KEY_J
  int m_keyTour = 84;     // GLFW_KEY_T
  int m_captureKeyFor = -1;  // идёт захват клавиши для строки (см. UI)
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
  // === HUD Label Engine (v3.12.0 / L1) ===
  std::unordered_map<std::string, float> m_labelAlpha;
  std::unordered_map<std::string, float> m_labelCloseAlpha;
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
  // === v3.6.0: колонии === (Mars в sol, Hercules b в m13): подлёт к телу
  // сайта включает подсказку [C], колония сохраняется в профиль и горит
  // кольцом-маркером в сцене.
  std::vector<std::string> m_colonies;
  bool m_prevCPressed = false;
  std::string m_colonyHint;    // имя сайта в радиусе посадки ("" = нет)
  std::string m_colonyHintId;  // id сайта для hotkey
  // v3.7.0: жизненный счётчик успешных уклонений от обломков (ачивка + GUI).
  int m_asteroidDodges = 0;
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
  int m_shipStyle = 0;  // корпус корабля: 0 Dart, 1 Ring Ship v2, 2 Shuttle, 3 Model (OBJ)
  // === Orbital Shipyard & Station Docking (v3.11.0) ===
  bool m_shipyardOpen = false;
  bool m_prevKPressed = false;
  std::string m_dockHint;
  std::string m_dockHintId;
  ShipCustomization m_shipCustomization;
  void toggleShipyard();
  void dockAtStation(const std::string& dockId);
  void undockFromStation();
  void drawShipyard();
  // === Batch A1: Action Feedback Animations ===
  enum class DockAnimState {
    None,
    DockPhase1_Clamps,   // ~0.9s: speed damping and soft pull
    DockPhase2_Service,  // ~0.9s: refueling & shield recharge
    Undock_Release       // ~1.0s: undocking drift
  };
  DockAnimState m_dockAnimState = DockAnimState::None;
  float m_dockAnimTimer = 0.0f;
  std::string m_dockTargetId;
  Vec3 m_dockTargetPos;
  float m_dockInitialFuel = 0.0f;
  float m_dockInitialShield = 0.0f;
  void startDockSequence(const std::string& dockId);
  void startUndockSequence();

  // Ship switch animation (~1.0s transition in shipyard)
  float m_shipSwitchTimer = 0.0f;
  int m_pendingShipStyle = 0;
  void setShipStyleAnimated(int style);

  // Jump-to-station warp-in halo (~1.2s: white flash 0.15s + reveal 1.05s) & input cooldown
  float m_teleportHaloTimer = 0.0f;
  float m_inputCooldown = 0.0f;
  void triggerWarpInHalo(float duration = 1.2f);
  // === Settings v3.6.0 ===: пользовательские настройки (сохраняются в профиль).
  int m_difficulty = 1;  // 0 Easy, 1 Normal, 2 Hard (см. difficultyTuning)
  int m_aaMode = 1;      // 0 Off, 1 FXAA (PostFX)
  bool m_prevWPressed = false;
  bool m_prevXPressed = false;
  bool m_prevLBracketPressed = false;
  bool m_prevRBracketPressed = false;
  float m_warpShakePhase = 0.0f;
  float m_warpShakeYaw = 0.0f;
  float m_warpShakePitch = 0.0f;
  void warpRetarget(float to);
  void disengageWarp();
  // v3.6.0: смена корпуса с учётом варп-ролей: если у нового корпуса нет
  // варп-двигателя (Shuttle), варп-режим аккуратно гасится.
  void setShipStyleNow(int style);
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
  // v3.6.0 (пасс D): сглаженная полоса обломков -1..1 (сдвиг камней против
  // ввода уклонения; дефолт 0, харнесс чистый). См. warpdrive::setAsteroids.
  float m_astLane = 0.0f;
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

  // === Camera views + speed FX === (v3.3.0) begin.
  // V: кабина -> за кормой -> дальний обзор. В видах 2/3 рисуется корабль,
  // камера кадра сносится назад (логика/коллизии остаются на m_camera).
  // Скорость (реальная + варп-фактор) масштабирует доплер/абберацию света,
  // стрики в композите и микро-FOV (+3°). Дефолты 0 => харнесс байт-в-байт.
  view::ViewMode m_viewMode = view::ViewMode::Cockpit;
  bool m_prevVPressed = false;
  // v4.0: зум колёсиком в видах Chase/Overview (кокпит не трогаем).
  // 1 = дефолт; 0.35..2.5 — множитель сноса камеры назад.
  float m_viewZoom = 1.0f;
  float m_viewZoomT = 1.0f;  // цель (сглаживание кадром)
  Camera m_renderCam;  // камера кадра (со сносом вида); метки/пикинг — по ней
  bool m_renderCamValid = false;  // false в меню/харнессе => метки по m_camera
  Vec3 m_prevSpeedPos;
  Vec3 m_speedDir{0.0, 0.0, 1.0};
  float m_camSpeed = 0.0f;  // сглаженная |v| камеры, u/s
  float m_speedK = 0.0f;        // итоговый эффект скорости 0..1 (аудио/тяга)
  float m_shieldFlashK = 0.0f;  // вспышка щита при уроне 0..1 (v3.3.0)
  void updateSpeedFx(float deltaTime);
  void applyViewCamera(Camera& out);
  // === Camera views + speed FX === end

  // === SaveGame === (v3.4.0) begin.
  // JSON-профиль: settings.json (глобально: пресет/звук/вид) +
  // save-sandbox.json (прогресс, объекты редактора, сессия; слот adventure
  // зарезервирован под v4.0). F9/F10 — быстрое сохранение/загрузка, автосейв
  // на выходе, кнопки в меню. Файловый ввод-вывод — только в интерактивном
  // run(): харнесс (captureAllScreenshots) run() не зовёт и профиль не трогает
  // (m_saveInteractive=false), PNG остаются байт-в-байт.
  bool m_saveInteractive = false;
  bool m_prevF9Pressed = false;
  bool m_prevF10Pressed = false;
  std::string m_saveStatus;  // последняя операция (строка в меню)
  void saveProfileNow(bool quiet);
  void loadProfileNow(bool quiet = false);
  void applySaveSettings(const SaveSettings& s);
  void applySaveGame(const SaveGame& g);
  // === SaveGame === end
};

// Связь AppGL -> SceneManager без правок SceneManager.h:
// состояние оверлея (u_WarpK/u_Flash), дефолт 0. Определено в
// SceneManager.cpp в блоке // === WarpDrive ===.
// === WarpJump === (агент J): +u_Burst и корабль-силуэт, дефолты 0/скрыт.
namespace warpdrive {
void setState(float k, float flash);
void setBurst(float b);
void setShip(bool v, float x, float y, float z, float yaw);
bool shipVisible();
float shipX();
float shipY();
float shipZ();
float shipYaw();
void setShipStyle(int s);  // 0 Dart, 1 Ring Ship (IXS), 2 Shuttle, 3 Model
// v3.11.0: кастомизация корабля (ливрея, свечение, спин колец, апгрейды)
void setShipCustomization(const ShipCustomization& c);
const ShipCustomization& shipCustomization();
void setDockSitesVisible(bool v);
bool dockSitesVisible();
void setThruster(float k);  // v3.3.0: тяга 0..1 (шлейф/движок от скорости)
// v3.6.0 (пасс D): астероидное поле в Travel. k 0..1 — плотность/видимость
// (дефолт 0, харнесс чистый), lane -1..1 — сглаженный сдвиг полосы обломков
// навстречу уклонению (D сдвигает поле влево; читается как манёвр корабля).
void setAsteroids(float k, float lane);
}  // namespace warpdrive

}  // namespace astro
