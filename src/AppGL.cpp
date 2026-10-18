#include "AppGL.h"

#include "GLCompat.h"
#include <GLFW/glfw3.h>

#include <cmath>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
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
#include "Mesh.h"
#include "Shader.h"
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

// === Cinematic === begin (агент C): шрифт с кириллицей для русских строк.
// Пробуем системные шрифты (без новых deps/файлов). Дефолтный шрифт ImGui
// не заменяем: киношрифт используется точечно в drawCinematic(), поэтому
// эталонные скриншоты попиксельно не меняются. Промах = nullptr (fallback).
namespace {
ImFont* loadCineFont() {
  ImGuiIO& io = ImGui::GetIO();
  static const char* kPaths[] = {
      "C:\\Windows\\Fonts\\segoeui.ttf",
      "C:\\Windows\\Fonts\\arial.ttf",
      "/System/Library/Fonts/Supplemental/Arial.ttf",
      "/System/Library/Fonts/Supplemental/Verdana.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/TTF/DejaVuSans.ttf",
  };
  for (const char* p : kPaths) {
    ImFont* f = io.Fonts->AddFontFromFileTTF(
        p, 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    if (f) return f;
  }
  return nullptr;
}
}  // namespace
// === Cinematic === end

namespace {
// v3.6.0: человекочитаемое имя клавиши для страницы Controls. Печатные
// символы берём из GLFW (главный алфавит), спецклавиши — из таблицы.
std::string keyNameForCode(int key) {
  const char* name = glfwGetKeyName(key, 0);
  if (name && name[0] != '\0') {
    std::string s(name);
    for (char& ch : s) {
      ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return s;
  }
  if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25) {
    return "F" + std::to_string(key - GLFW_KEY_F1 + 1);
  }
  switch (key) {
    case GLFW_KEY_SPACE: return "Space";
    case GLFW_KEY_ENTER: return "Enter";
    case GLFW_KEY_TAB: return "Tab";
    case GLFW_KEY_LEFT_SHIFT: return "LShift";
    case GLFW_KEY_RIGHT_SHIFT: return "RShift";
    case GLFW_KEY_LEFT_CONTROL: return "LCtrl";
    case GLFW_KEY_RIGHT_CONTROL: return "RCtrl";
    case GLFW_KEY_LEFT_ALT: return "LAlt";
    case GLFW_KEY_RIGHT_ALT: return "RAlt";
    case GLFW_KEY_LEFT: return "Left";
    case GLFW_KEY_RIGHT: return "Right";
    case GLFW_KEY_UP: return "Up";
    case GLFW_KEY_DOWN: return "Down";
    case GLFW_KEY_DELETE: return "Del";
    case GLFW_KEY_ESCAPE: return "Esc";
    default: break;
  }
  return "Key " + std::to_string(key);
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
  // === Cinematic === begin (агент C): шрифт с кириллицей для русских строк.
  // Дефолтный шрифт не заменяем (иначе поплывут эталонные скриншоты):
  // киношрифт используется только внутри drawCinematic().
  m_cineFont = loadCineFont();
  // === Cinematic === end

  ImGui_ImplGlfw_InitForOpenGL(m_window->getNativeWindow(), true);
  ImGui_ImplOpenGL3_Init("#version 330");
}

AppGL::~AppGL() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

int AppGL::run() {
  // === SaveGame === (v3.4.0): интерактивный старт — грузим настройки и
  // профиль песочницы (тихо). Харнесс скриншотов сюда не заходит:
  // m_saveInteractive остаётся false, файлы профиля не читаются/не пишутся.
  m_saveInteractive = true;
  loadProfileNow(true);
  loadExpeditionFileNow();  // === Expedition === (v3.5.0): свой сейв арки.
  float lastTime = static_cast<float>(glfwGetTime());
  static float menuAngle = 0.0f;

  while (!m_window->shouldClose()) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // === Audio === begin (агент AU: ленивый старт при первом вводе,
    // update музыки/SFX + polling-триггеры; варп/тур/поиск не трогаем.
    // Харнесс captureAllScreenshots run() не вызывает: init там нет, PNG чистые).
    audioEnsureOnFirstInput();
    audioProcessKey();
    audioUpdate(deltaTime);
    // === Audio === end

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

      // === MenuSky === (v3.5.0): живой космос за меню — собственный
      // фуллскрин-шейдер (туманность + MW-полоса + параллакс-звёзды),
      // лениво инициализируется при первом кадре меню. Показ не зависит
      // от того, какая станция осталась в профиле (uma/mw/m31 = пустой
      // фон), свежий запуск и загруженный профиль видят одинаковый hero-вид.
      if (!m_menuSkyTried) {
        m_menuSkyTried = true;
        m_menuSkyShader =
            std::make_unique<Shader>("shaders/quad.vert", "shaders/menu.frag");
        m_menuSkyQuad = std::make_unique<ScreenQuad>();
      }
      if (m_menuSkyShader && m_menuSkyQuad) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        m_menuSkyShader->bind();
        m_menuSkyShader->setUniform1f("u_Time", currentTime);
        m_menuSkyShader->setUniform2f("u_Res", static_cast<float>(menuFbW),
                                      static_cast<float>(menuFbH));
        m_menuSkyQuad->draw();
        m_menuSkyShader->unbind();
      } else {
        // Fallback: прежняя орбита вокруг текущей станции.
        m_sceneManager->render(m_camera, menuFbW, menuFbH, currentTime);
      }

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

    // === Physics === begin: столкновения и захват горизонтом.
    Vec3 prevCamPos = m_camera.position;
    if (!m_captureAnim) {
      processInput(deltaTime);
      processMouse();
    }
    const Station* physSt = m_sceneManager->getCurrentStation();
    if (!m_captureAnim && physSt &&
        physSt->type == StationType::BlackHole) {
      const double rsNow =
          static_cast<double>(m_sceneManager->getSettings().rs);
      double capR = physSt->collisionRadius;
      if (capR <= 0.0) capR = rsNow * 1.05;
      const double d0 = std::sqrt(m_camera.position.x * m_camera.position.x +
                                  m_camera.position.y * m_camera.position.y +
                                  m_camera.position.z * m_camera.position.z);
      if (d0 <= capR) startHorizonCapture();
    }
    if (!m_captureAnim && physSt &&
        physSt->type == StationType::Quasar) {
      // v3.6.0: ядро квазара — не «стена» (slide), а катсцена захвата и
      // выброса: подлёт к ядру запускает ту же последовательность, что и
      // горизонт чёрной дыры, с собственными текстами.
      double qR = physSt->collisionRadius;
      if (qR <= 0.0) qR = 2.0;
      const double qd = std::sqrt(m_camera.position.x * m_camera.position.x +
                                  m_camera.position.y * m_camera.position.y +
                                  m_camera.position.z * m_camera.position.z);
      if (qd <= qR + 0.5) startHorizonCapture();
    }
    // === Physics === end

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
      // === Physics ===: скольжение вместо выталкивания (без залипания).
      for (const auto& body : m_sceneManager->getBodies()) {
        if (!body.def) continue;
        double factor = 1.05;
        if (body.def->type == 0) factor = 1.02;
        else if (body.def->type == 1 || body.def->type == 3) factor = 1.1;
        double minDist = static_cast<double>(body.def->radius) * factor;
        if (minDist < 0.8) minDist = 0.8;
        m_camera.position = interaction::slideOnSphere(
            prevCamPos, m_camera.position, body.pos.x, body.pos.y,
            body.pos.z, minDist);
      }
    }
    updateHorizonCapture(deltaTime);  // === Physics ===
    checkAchievements();

    // === Ship survival === (v3.3.0): топливо/щиты/гравитация; до render(),
    // чтобы пост-эффекты низкого щита попали в этот же кадр.
    updateShip(deltaTime);

    // === Camera views + speed FX === (v3.3.0): скорость камеры/варпа =>
    // доплер+стрики+микро-FOV; камера кадра со сносом вида; корабль в кадре.
    updateSpeedFx(deltaTime);
    applyViewCamera(m_renderCam);
    if (!m_jumpCine) {
      if (m_viewMode != view::ViewMode::Cockpit && !m_inMenu) {
        warpdrive::setShip(true, static_cast<float>(m_camera.position.x),
                           static_cast<float>(m_camera.position.y),
                           static_cast<float>(m_camera.position.z),
                           static_cast<float>(m_camera.yaw));
      } else {
        warpdrive::setShip(false, 0.0f, 0.0f, 0.0f, 0.0f);
      }
    }

    int fbWidth = 0, fbHeight = 0;
    m_window->getFramebufferSize(fbWidth, fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Render active scene
    m_sceneManager->render(m_renderCam, fbWidth, fbHeight, currentTime);

    // === WarpDrive === (снять тряску камеры после render()).
    finishWarpFrame();

    // Render ImGui UI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (!m_hideUI) {
      drawImGui();
    }
    if (m_photoMode) {
      drawPhotoPopup();
    }
    if (m_helpOverlay) {
      drawHelpOverlay();
    }
    drawWorldLabels();
    // === Cinematic === begin (агент C): тур/карточки/HUD только в игре.
    // Харнесс (captureAllScreenshots) сюда не заходит: PNG чистые.
    drawCinematic();
    // === Cinematic === end
    drawHorizonCapture(ImGui::GetForegroundDrawList());  // === Physics ===

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    m_window->swapBuffers();
  }

  // === SaveGame === (v3.4.0): автосейв при выходе (тихо, без тостов).
  saveProfileNow(true);
  saveExpeditionFileNow();  // === Expedition === (v3.5.0).

  return 0;
}

void AppGL::processInput(float deltaTime) {
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();
  // v3.6.0: флаг ввода движения сбрасывается каждый кадр; блок движения ниже
  // поднимет его, если игрок действительно жмёт WASD/Space/Shift.
  m_moveInputActive = false;
  // === Audio === begin (агент AU: первый ввод + M mute; варп/тур ниже не тронуты).
  audioEnsureOnFirstInput();
  audioProcessKey();
  // === Audio === end
  // === WarpDrive === begin (агент W): ключи W/[ ], таймеры прыжка,
  // FOV +10deg*K, дрейф, тряска. Возврат true = катсцена прыжка.
  if (updateWarp(deltaTime)) return;
  // === WarpDrive === end
  // === Cinematic === begin (агент C): клавиша T + автопилот тура.
  // Возврат true = ввод consumed, дальше обычный ввод не идёт.
  if (processCinematicInput(deltaTime)) return;
  // === Cinematic === end

  const bool escDown = (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS);
  if (escDown && !m_prevEscPressed && !io.WantCaptureKeyboard && !m_slidesOpen) {
    // === Expedition === (v3.5.0): Esc сначала закрывает карту глав; на
    // слайдах Esc обрабатывает сам оверлей (там же — «досмотреть»).
    if (m_expeditionMapOpen) {
      m_expeditionMapOpen = false;
    } else {
      m_inMenu = true;
    }
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

  // === SaveGame === (v3.4.0): F9 — быстрое сохранение, F10 — загрузка.
  const bool f9Down = (glfwGetKey(win, GLFW_KEY_F9) == GLFW_PRESS);
  if (f9Down && !m_prevF9Pressed && !io.WantCaptureKeyboard) {
    saveProfileNow(false);
  }
  m_prevF9Pressed = f9Down;

  const bool f10Down = (glfwGetKey(win, GLFW_KEY_F10) == GLFW_PRESS);
  if (f10Down && !m_prevF10Pressed && !io.WantCaptureKeyboard) {
    loadProfileNow(false);
  }
  m_prevF10Pressed = f10Down;

  // === StoryJournal === (v3.5.0): J — открыть/закрыть бортовой журнал.
  // Открытие помечает записи прочитанными (счётчик "новых" в статус-строке
  // обнуляется при первом же показе панели).
  const bool jDown = (glfwGetKey(win, m_keyJournal) == GLFW_PRESS);
  if (jDown && !m_prevJPressed && !io.WantCaptureKeyboard) {
    m_storyOpen = !m_storyOpen;
    if (m_storyOpen) m_story.markAllRead();
  }
  m_prevJPressed = jDown;

  // === Expedition === (v3.5.0): N — карта глав экспедиции (M занят
  // audio-mute; в прологе и меню не мешает).
  const bool nDown = (glfwGetKey(win, m_keyMap) == GLFW_PRESS);
  if (nDown && !m_prevNPressed && !io.WantCaptureKeyboard && !m_inMenu &&
      m_slidesCurrent.empty()) {
    m_expeditionMapOpen = !m_expeditionMapOpen;
  }
  m_prevNPressed = nDown;

  const bool pDown = (glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS);
  if (pDown && !m_prevPPressed && !io.WantCaptureKeyboard) {
    m_sceneManager->togglePause();
  }
  m_prevPPressed = pDown;

  // === Camera views === (v3.3.0): V цикл кабина -> за кормой -> обзор.
  // Во время катсцены прыжка вид не трогаем (камерой владеет updateJumpCine).
  const bool vDown = (glfwGetKey(win, m_keyView) == GLFW_PRESS);
  if (vDown && !m_prevVPressed && !io.WantCaptureKeyboard && !m_jumpCine) {
    m_viewMode = view::nextView(m_viewMode);
    // Снос камеры меняет камеру кадра: без сброса рел. скорости был бы
    // ложный скачок beta (доплер-вспышка) на кадр переключения.
    m_sceneManager->resetRelativity();
    static const char* kViewNameRu[] = {"Кабина", "За кормой", "Дальний обзор"};
    static const char* kViewNameEn[] = {"Cockpit", "Behind", "Overview"};
    const int vi = static_cast<int>(m_viewMode);
    Toast vt;
    vt.title = uiRu("Вид камеры", "Camera view");
    vt.desc = uiRu(kViewNameRu[vi], kViewNameEn[vi]);
    vt.time = static_cast<float>(glfwGetTime());
    m_toasts.push_back(vt);
  }
  m_prevVPressed = vDown;

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

  // === Beauty === F8 photo mode, O auto-orbit (v2.7).
  const bool f8Down = (glfwGetKey(win, GLFW_KEY_F8) == GLFW_PRESS);
  if (f8Down && !m_prevF8Pressed && !io.WantCaptureKeyboard) {
    m_photoMode = !m_photoMode;
    if (m_photoMode) m_hideUI = true;  // скрыть всё, попап рисуем сами
  }
  m_prevF8Pressed = f8Down;

  const bool oDown = (glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS);
  if (oDown && !m_prevOPressed && !io.WantCaptureKeyboard) {
    m_orbitOn = !m_orbitOn;
    if (m_orbitOn) {
      // Стартуем с текущего азимута, чтобы не было рывка.
      const double px = m_camera.position.x;
      const double pz = m_camera.position.z;
      m_orbitAngle = static_cast<float>(std::atan2(pz, px));
    }
  }
  m_prevOPressed = oDown;
  // === Beauty === end

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

  // === Beauty === автоорбита: крутим камеру вокруг фокуса станции (O).
  // Идёт до гварда клавиатуры, чтобы орбита жила и при открытом ImGui.
  // Любое WASD-движение выключает орбиту — управление возвращается игроку.
  if (m_orbitOn) {
    const bool wasdDown =
        (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) ||
        (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) ||
        (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) ||
        (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) ||
        (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) ||
        (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    if (wasdDown) {
      m_orbitOn = false;
    } else {
      updateOrbit(deltaTime);
    }
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

  // v3.6.0: инерционное движение — корабль плавно разгоняется и тормозит,
  // после отпускания клавиш сохраняет ход (космический «ил»). Скорость
  // копится в m_shipVel; гравитационный дрейф остаётся отдельным слоем,
  // а орбитальный ассист (см. updateShip) мягко стабилизирует курс.
  const double axisF =
      (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS ? 1.0 : 0.0) -
      (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS ? 1.0 : 0.0);
  const double axisR =
      (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS ? 1.0 : 0.0) -
      (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS ? 1.0 : 0.0);
  const double axisU =
      (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS ? 1.0 : 0.0) -
      (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 1.0 : 0.0);
  m_moveInputActive = (axisF != 0.0 || axisR != 0.0 || axisU != 0.0);
  const Vec3 fwd = m_camera.forward();
  const Vec3 rgt = m_camera.right();
  const Vec3 want((fwd.x * axisF + rgt.x * axisR) * m_camera.moveSpeed,
                  (fwd.y * axisF + axisU) * m_camera.moveSpeed,
                  (fwd.z * axisF + rgt.z * axisR) * m_camera.moveSpeed);
  // Разгон быстрее торможения: тяга ~3/с, свободный дрейф ~0.9/с.
  const double resp = m_moveInputActive ? 3.0 : 0.9;
  const double kAcc = 1.0 - std::exp(-static_cast<double>(deltaTime) * resp);
  m_shipVel = Vec3(m_shipVel.x + (want.x - m_shipVel.x) * kAcc,
                   m_shipVel.y + (want.y - m_shipVel.y) * kAcc,
                   m_shipVel.z + (want.z - m_shipVel.z) * kAcc);
  if (std::fabs(m_shipVel.x) + std::fabs(m_shipVel.y) + std::fabs(m_shipVel.z) <
      0.01) {
    m_shipVel = Vec3();
  }
  m_camera.position = Vec3(m_camera.position.x + m_shipVel.x * deltaTime,
                           m_camera.position.y + m_shipVel.y * deltaTime,
                           m_camera.position.z + m_shipVel.z * deltaTime);
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

// === Beauty === (v2.7): автоорбита + фото-режим.
void AppGL::updateOrbit(float deltaTime) {
  // Камера кружит вокруг фокуса текущей станции (мир всегда центрирован
  // на origin — teleportTo ставит камеру на сферу вокруг (0,0,0)).
  const double px = m_camera.position.x;
  const double py = m_camera.position.y;
  const double pz = m_camera.position.z;
  const double radius = std::sqrt(px * px + pz * pz);
  if (radius < 1e-3) return;
  // ~12 deg/s: медленный облёт для созерцания.
  m_orbitAngle += static_cast<float>(deltaTime * 0.21);
  m_camera.position.x = std::cos(static_cast<double>(m_orbitAngle)) * radius;
  m_camera.position.z = std::sin(static_cast<double>(m_orbitAngle)) * radius;
  m_camera.position.y = py;  // высоту держим
  // Смотрим в фокус (origin).
  const double dx = -m_camera.position.x;
  const double dy = -m_camera.position.y;
  const double dz = -m_camera.position.z;
  m_camera.yaw = std::atan2(dx, dz);
  const double horiz = std::sqrt(dx * dx + dz * dz);
  m_camera.pitch = std::atan2(dy, horiz);
}

void AppGL::drawPhotoPopup() {
  // Маленький попап в правом нижнем: exposure + Save PNG + подсказка.
  // Рисуется даже при m_hideUI (фото-режим сам его выставил).
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2(disp.x - 12.0f, disp.y - 12.0f),
                          ImGuiCond_Always, ImVec2(1.0f, 1.0f));
  ImGui::SetNextWindowBgAlpha(0.90f);
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_AlwaysAutoResize;
  if (ImGui::Begin("##photo_mode", nullptr, flags)) {
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f),
                       "%s", uiRu("Фото-режим (F8)", "Photo mode (F8)"));
    float expNow = m_sceneManager->getExposure();
    if (ImGui::SliderFloat("Exposure", &expNow, 0.2f, 4.0f, "%.2f")) {
      m_sceneManager->setExposure(expNow);
    }
    if (ImGui::Button(uiRu("Сохранить PNG", "Save PNG"))) {
      // Имя по станции + unix-время — не перетирает эталоны docs/screens.
      const Station* st = m_sceneManager->getCurrentStation();
      char buf[128];
      std::snprintf(buf, sizeof(buf), "docs/photos/%s_%lld.png",
                    st ? st->id.c_str() : "shot",
                    static_cast<long long>(std::time(nullptr)));
      if (saveScreenshot(buf)) {
        m_lastScreenshotStatus = std::string("Saved: ") + buf;
      } else {
        m_lastScreenshotStatus = "Failed to save screenshot";
      }
    }
    if (!m_lastScreenshotStatus.empty()) {
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s",
                         m_lastScreenshotStatus.c_str());
    }
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s",
                       uiRu("O - орбита, H - UI, Esc - меню",
                            "O - orbit, H - UI, Esc - menu"));
  }
  ImGui::End();
}

void AppGL::processMouse() {
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();
  // === Cinematic === begin (агент C): в туре автопилот владеет камерой.
  if (m_tour.active()) {
    const bool anyDown =
        (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    m_prevLeftDown = anyDown;
    m_firstMouse = true;
    m_mouseDragging = false;
    return;
  }
  // === Cinematic === end
  // === WarpJump === begin (агент J): катсцена держит камеру — тот же
  // паттерн, что m_captureAnim у капчи горизонта и тур выше: drag-состояние
  // сбрасываем, осмотр не идёт. В Cooldown управление уже у игрока.
  if (m_jumpCine && m_jump.phase() != JumpPhase::Cooldown) {
    const bool jumpDown =
        (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    m_prevLeftDown = jumpDown;
    m_firstMouse = true;
    m_mouseDragging = false;
    return;
  }
  // === WarpJump === end

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

  // v3.6.0: чувствительность и инверсия вертикали из настроек (Controls).
  const float sensitivity = 0.003f * m_mouseSensitivity;
  m_camera.yaw += xoffset * sensitivity;
  m_camera.pitch += yoffset * sensitivity * (m_invertY ? -1.0f : 1.0f);

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
  glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1e7f);
  // Виды (v3.3.0): пикинг объектов — по камере кадра (снос вида 2/3).
  Vec3 cp = m_renderCam.position;
  Vec3 cf = m_renderCam.forward();
  Vec3 cu = m_renderCam.up();
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

namespace {
struct EditorPreset {
  int type;
  int detail;
  float radius;
  float colA[3];
  float colB[3];
};

// v3.2.0 object editor: palette index -> planet-shader look + sensible
// default size and palette for that class of object.
EditorPreset editorPresetFor(int idx) {
  EditorPreset p{};
  switch (idx) {
    case 0:
      p = {2, 0, 0.30f, {0.62f, 0.58f, 0.52f}, {0.37f, 0.35f, 0.31f}};
      break;
    case 1:
      p = {2, 2, 0.60f, {0.45f, 0.42f, 0.38f}, {0.28f, 0.26f, 0.24f}};
      break;
    case 2:
      p = {2, 5, 0.45f, {0.62f, 0.62f, 0.63f}, {0.30f, 0.30f, 0.32f}};
      break;
    case 3:
      p = {1, 7, 1.20f, {0.78f, 0.66f, 0.48f}, {0.55f, 0.40f, 0.28f}};
      break;
    default:
      p = {0, 1, 1.00f, {1.00f, 0.85f, 0.55f}, {0.90f, 0.30f, 0.10f}};
      break;
  }
  return p;
}

// Reverse mapping for live editing: which palette entry does this body use?
int editorIndexFor(const CustomBody& b) {
  if (b.type == 0 || b.detail == 1) return 4;
  if (b.detail >= 7) return 3;
  if (b.detail == 5) return 2;
  if (b.detail == 2) return 1;
  return 0;
}

const char* kEditorTypes[] = {"Rock", "Rocky planet", "Moon", "Gas giant", "Star"};
}  // namespace

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
  glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1e7f);
  // Виды (v3.3.0): пикинг — по камере кадра (учёт сноса вида 2/3).
  Vec3 cp = m_renderCam.position;
  Vec3 cf = m_renderCam.forward();
  Vec3 cu = m_renderCam.up();
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

  // v3.1.0: double-click a bright named star flies to it (every galaxy).
  if (curSt->type == StationType::Galaxy) {
    bool haveHit = false;
    double bestScreen = 1e30;
    Vec3 bestPos;
    std::string bestName;
    for (const auto& bs : m_sceneManager->galaxyBrightStars()) {
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

// === ModernUI === begin (агент UI: тёмная "стеклянная" тема, статус-бар,
// транспортная панель, карточки станций).
// Только оформление + кнопки-индикаторы поверх существующего состояния:
// вызываются только готовые teleportToStation/togglePause/setTimeScale/
// warpRetarget/startWarpJump/startCineTour/stopCineTour/skip и геттеры
// SceneManager. Никакой новой логики варпа/тура/времени/поиска/звука.
// Харнесс captureAllScreenshots вызывает drawImGui(): 8 сценовых PNG
// рендерятся вообще без ImGui и остаются чистыми, gui.png осознанно
// обновляется (новый вид).
void AppGL::applyModernThemeOnce() {
  static bool done = false;
  if (done) return;
  done = true;
  ImGuiStyle& st = ImGui::GetStyle();
  // Скругления 8px по всем элементам.
  st.WindowRounding = 8.0f;
  st.ChildRounding = 8.0f;
  st.FrameRounding = 8.0f;
  st.GrabRounding = 8.0f;
  st.PopupRounding = 8.0f;
  st.ScrollbarRounding = 8.0f;
  st.TabRounding = 8.0f;
  st.WindowBorderSize = 1.0f;
  st.FrameBorderSize = 1.0f;
  // "Стекло": alpha панелей ~0.92 поверх StyleColorsDark.
  st.Colors[ImGuiCol_WindowBg].w = 0.92f;
  st.Colors[ImGuiCol_ChildBg].w = 0.92f;
  st.Colors[ImGuiCol_PopupBg].w = 0.94f;
  // Акценты проекта: циан (0.40, 0.85, 1.00) + янтарь (1.00, 0.75, 0.28).
  const ImVec4 cyan(0.40f, 0.85f, 1.00f, 1.00f);
  const ImVec4 cyanDim(0.16f, 0.38f, 0.55f, 1.00f);
  const ImVec4 cyanDeep(0.10f, 0.28f, 0.42f, 1.00f);
  const ImVec4 amber(1.00f, 0.75f, 0.28f, 1.00f);
  const ImVec4 amberDim(0.55f, 0.38f, 0.12f, 1.00f);
  st.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.32f, 0.48f, 1.00f);
  st.Colors[ImGuiCol_ButtonHovered] = cyanDim;
  st.Colors[ImGuiCol_ButtonActive] = cyanDeep;
  st.Colors[ImGuiCol_Header] = amberDim;
  st.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.65f, 0.46f, 0.16f, 1.00f);
  st.Colors[ImGuiCol_HeaderActive] = ImVec4(0.70f, 0.52f, 0.20f, 1.00f);
  st.Colors[ImGuiCol_CheckMark] = cyan;
  st.Colors[ImGuiCol_SliderGrab] = cyan;
  st.Colors[ImGuiCol_SliderGrabActive] = amber;
  st.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.30f, 0.42f, 1.00f);
  st.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.36f, 0.50f, 1.00f);
  st.Colors[ImGuiCol_TitleBg] = ImVec4(0.32f, 0.22f, 0.07f, 0.92f);
  st.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.45f, 0.31f, 0.10f, 0.92f);
  st.Colors[ImGuiCol_Separator] = ImVec4(1.00f, 0.75f, 0.28f, 0.35f);
  st.Colors[ImGuiCol_SeparatorHovered] = amber;
  st.Colors[ImGuiCol_SeparatorActive] = amber;
  st.Colors[ImGuiCol_TabHovered] = cyanDim;
  st.Colors[ImGuiCol_TabActive] = cyanDeep;
}

const char* AppGL::stationTypeRuShort(StationType t) {
  switch (t) {
    case StationType::BlackHole: return "ЧД";
    case StationType::Quasar: return "Квазар";
    case StationType::Constellation: return "Созв.";
    case StationType::Galaxy: return "Галакт.";
    case StationType::Nebula: return "Туманн.";
    case StationType::SolarSystem: return "Система";
    case StationType::Pulsar: return "Пульс.";
    case StationType::StarCluster: return "Скопл.";
  }
  return "?";
}

unsigned int AppGL::stationTypeChip(StationType t) {
  // Цветной чип типа станции (фон; текст поверх — тёмный).
  switch (t) {
    case StationType::BlackHole: return IM_COL32(150, 120, 255, 255);
    case StationType::Quasar: return IM_COL32(255, 190, 70, 255);
    case StationType::Constellation: return IM_COL32(120, 220, 255, 255);
    case StationType::Galaxy: return IM_COL32(150, 200, 255, 255);
    case StationType::Nebula: return IM_COL32(255, 130, 200, 255);
    case StationType::SolarSystem: return IM_COL32(130, 230, 140, 255);
    case StationType::Pulsar: return IM_COL32(120, 240, 255, 255);
    case StationType::StarCluster: return IM_COL32(255, 215, 140, 255);
  }
  return IM_COL32(140, 140, 140, 255);
}

const char* AppGL::uiRu(const char* ru, const char* en) const {
  return m_cineFont ? ru : en;
}

bool AppGL::modernPushFont() const {
  if (m_cineFont) {
    ImGui::PushFont(m_cineFont);
    return true;
  }
  return false;
}

void AppGL::drawModernStatusBar() {
  // Slim-бар сверху справа: станция | FPS | темп | варп | звук.
  // Только чтение геттеров, состояние не меняем.
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  if (disp.x <= 0.0f || disp.y <= 0.0f) return;
  ImGui::SetNextWindowPos(ImVec2(disp.x - 12.0f, 12.0f), ImGuiCond_Always,
                          ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowBgAlpha(0.92f);
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_AlwaysAutoResize;
  const bool pushed = modernPushFont();
  if (ImGui::Begin("##modern_status", nullptr, flags)) {
    const Station* st = m_sceneManager->getCurrentStation();
    const bool paused = m_sceneManager->isPaused();
    const float pace = m_sceneManager->getTimeScale();
    char paceBuf[32];
    if (paused) {
      std::snprintf(paceBuf, sizeof(paceBuf), "%s",
                    uiRu("пауза", "paused"));
    } else if (pace < 9.5f) {
      std::snprintf(paceBuf, sizeof(paceBuf), "%.1fx",
                    static_cast<double>(pace));
    } else {
      std::snprintf(paceBuf, sizeof(paceBuf), "%.0fx",
                    static_cast<double>(pace));
    }
    char warpBuf[48];
    if (!m_warpOn) {
      std::snprintf(warpBuf, sizeof(warpBuf), "%s",
                    uiRu("варп выкл", "warp off"));
    } else {
      std::snprintf(warpBuf, sizeof(warpBuf), "%s F%d",
                    warpStatusName(m_warpStatus), m_warpFactor);
    }
    // Звук: аудиоподсистемы в дереве нет — нейтральный плейсхолдер.
    // Точка вставки AU-агента: заменить на чтение его состояния.
    // Строка строго Basic Latin + Cyrillic (дефис вместо тире).
    const char* soundTxt = uiRu("звук: нет", "sound: n/a");
    ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.00f, 1.00f), "%s",
                       st ? st->name.c_str() : "None");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    ImGui::Text("FPS %.0f", static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    ImGui::Text("%s: %s", uiRu("темп", "pace"), paceBuf);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    ImGui::Text("%s", warpBuf);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "%s", soundTxt);
    // === StoryJournal === (v3.5.0): счётчик новых записей журнала.
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    const int storyNew = m_story.unreadCount();
    if (storyNew > 0) {
      char storyBuf[16];
      std::snprintf(storyBuf, sizeof(storyBuf), "%d", storyNew);
      ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.30f, 1.00f), "%s: %s",
                         uiRu("Журнал", "journal"), storyBuf);
    } else {
      ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "%s: %s",
                         uiRu("Журнал", "journal"), uiRu("нет новых", "no news"));
    }
    // === Beauty === часы гравитационного замедления у ЧД:
    // dτ/dt = sqrt(1 − Rs/r), r = |camPos| в единицах Rs станции.
    if (st && st->type == StationType::BlackHole) {
      double rs = static_cast<double>(m_sceneManager->getSettings().rs);
      if (rs < 0.1) rs = 1.0;
      const double rx = m_camera.position.x;
      const double ry = m_camera.position.y;
      const double rz = m_camera.position.z;
      const double r = std::sqrt(rx * rx + ry * ry + rz * rz);
      const double ratioRs = r / rs;
      double clockRate = 0.0;
      if (ratioRs > 1.001) {
        clockRate = std::sqrt(1.0 - 1.0 / ratioRs);
      }
      char clockBuf[48];
      std::snprintf(clockBuf, sizeof(clockBuf), "%.3f x", clockRate);
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.30f, 1.00f),
                         "%s %s @ %.1f Rs",
                         uiRu("часы", "clock"), clockBuf,
                         static_cast<double>(ratioRs));
    }
  }
  ImGui::End();
  if (pushed) ImGui::PopFont();
}

void AppGL::drawStoryPanel() {
  // === StoryJournal === (v3.5.0): панель бортового журнала (клавиша J).
  // Правый край, фиксированная высота; открытые записи — заголовок янтарным,
  // текст светло-серым; закрытые не показываются вовсе.
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2(disp.x - 20.0f, 64.0f), ImGuiCond_FirstUseEver,
                          ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(430.0f, disp.y * 0.60f), ImGuiCond_FirstUseEver);
  const bool pushed = modernPushFont();
  if (ImGui::Begin(uiRu("Бортовой журнал (J)", "Story Journal (J)"), &m_storyOpen)) {
    int shown = 0;
    for (const auto& e : m_story.entries()) {
      if (!e.unlocked) continue;
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.30f, 1.00f));
      ImGui::TextUnformatted(uiRu(e.titleRu.c_str(), e.titleEn.c_str()));
      ImGui::PopStyleColor();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.80f, 0.85f, 1.00f));
      ImGui::TextWrapped("%s", uiRu(e.textRu.c_str(), e.textEn.c_str()));
      ImGui::PopStyleColor();
      ImGui::Spacing();
      ++shown;
    }
    if (shown == 0) {
      ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "%s",
                         uiRu("Записи открываются по мере путешествия.",
                              "Entries unlock as you travel."));
    }
  }
  ImGui::End();
  if (pushed) ImGui::PopFont();
}

void AppGL::startExpeditionIntro() {
  // === Expedition === (v3.5.0): вводные слайды (идут после Play при
  // первой игре); по завершении — высадка в Солнечной системе.
  m_slidesCurrent = storyIntroSlides();
  m_slidesIntro = true;
  m_slidesIdx = 0;
  m_slidesOpen = true;
  m_inMenu = false;
}

void AppGL::openChapterSlide(int slideIdx) {
  // === Expedition === (v3.5.0): одиночный слайд — итог главы поверх игры.
  const auto& slides = storyChapterSlides();
  if (slideIdx < 0 || slideIdx >= static_cast<int>(slides.size())) return;
  m_slidesCurrent.assign(1, slides[slideIdx]);
  m_slidesHead.clear();
  for (const auto& c : expeditionChapters()) {
    if (c.slideIdx == slideIdx) {
      m_slidesHead = uiRu(c.titleRu.c_str(), c.titleEn.c_str());
      break;
    }
  }
  m_slidesIntro = false;
  m_slidesIdx = 0;
  m_slidesOpen = true;
}

void AppGL::saveExpeditionFileNow() {
  // === Expedition === (v3.5.0): state в отдельном expedition.json рядом
  // с профилями игры (схема SaveGame не тронута — kSaveSchema=1).
  const std::string dir = saveProfileDir();
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  std::ofstream f(expeditionFilePathFor(dir), std::ios::binary | std::ios::trunc);
  if (!f.is_open()) return;
  f.write(serializeExpedition(m_expedition).c_str(),
          static_cast<std::streamsize>(serializeExpedition(m_expedition).size()));
}

void AppGL::loadExpeditionFileNow() {
  const std::string dir = saveProfileDir();
  std::ifstream f(expeditionFilePathFor(dir), std::ios::binary);
  if (!f.is_open()) return;
  std::ostringstream buf;
  buf << f.rdbuf();
  ExpeditionState st;
  if (deserializeExpedition(buf.str(), st)) {
    m_expedition = st;
  }
}

void AppGL::drawSlidesOverlay() {
  // === Expedition === (v3.5.0): фуллскрин-экран слайдов: тёмное стекло,
  // векторная миниатюра сверху, текст в центре, подсказка внизу.
  // Space/Enter/клик — далее; Esc — досмотреть/закрыть.
  ImGuiIO& io = ImGui::GetIO();
  if (m_slidesIdx >= static_cast<int>(m_slidesCurrent.size())) {
    m_slidesOpen = false;
    m_slidesCurrent.clear();
    return;
  }
  const StorySlide& sl = m_slidesCurrent[m_slidesIdx];
  const ImVec2 disp = io.DisplaySize;

  const bool advance = ImGui::IsKeyPressed(ImGuiKey_Space, false) ||
                       ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                       ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||
                       ImGui::IsMouseClicked(0);
  const bool skip = ImGui::IsKeyPressed(ImGuiKey_Escape, false);

  if (m_slidesIntro && (advance || skip)) {
    // Пролог: Space/клик листают, Esc досматривает до конца; конец ==
    // старт миссии (teleport в sol сам завершит главу «Дом»).
    if (skip || m_slidesIdx + 1 >= static_cast<int>(m_slidesCurrent.size())) {
      m_expedition.introSeen = true;
      m_slidesOpen = false;
      m_slidesCurrent.clear();
      saveExpeditionFileNow();
      teleportToStation("sol");
      return;
    }
    ++m_slidesIdx;
    return;
  }
  if (skip || advance) {
    // Одиночный done-слайд: любое действие закрывает.
    m_slidesOpen = false;
    m_slidesCurrent.clear();
    return;
  }
  if (m_slidesCurrent.empty() || m_slidesOpen == false) return;
  if (m_slidesIdx >= static_cast<int>(m_slidesCurrent.size())) return;

  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(disp);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.010f, 0.014f, 0.024f, 0.90f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("##slides", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);

  ImDrawList* dl = ImGui::GetWindowDrawList();
  const float artSize = disp.y < 620.0f ? disp.y * 0.36f : 286.0f;
  const ImVec2 artPos((disp.x - artSize) * 0.5f, disp.y * 0.13f);
  drawStoryArt(dl, sl.art, artPos, artSize);

  const char* headTxt =
      m_slidesIntro ? uiRu("Экспедиция · пролог", "Expedition · Prologue")
                    : m_slidesHead.c_str();
  if (headTxt && headTxt[0] != '\0') {
    ImGui::SetCursorScreenPos(ImVec2((disp.x - ImGui::CalcTextSize(headTxt).x) * 0.5f,
                                     disp.y * 0.10f));
    ImGui::SetWindowFontScale(1.25f);
    ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.28f, 1.0f), "%s", headTxt);
    ImGui::SetWindowFontScale(1.0f);
  }

  const float textW = disp.x < 900.0f ? disp.x * 0.66f : 620.0f;
  ImGui::SetCursorScreenPos(
      ImVec2((disp.x - textW) * 0.5f, artPos.y + artSize + 26.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.89f, 0.94f, 1.00f));
  ImGui::TextWrapped("%s", uiRu(sl.textRu.c_str(), sl.textEn.c_str()));
  ImGui::PopStyleColor();

  const char* hint =
      m_slidesIntro ? uiRu("Пробел / Enter - далее, Esc - пропуск целиком",
                           "Space / Enter - next, Esc - skip all")
                    : uiRu("Пробел / клик - продолжить", "Space / click - continue");
  ImGui::SetCursorScreenPos(ImVec2((disp.x - ImGui::CalcTextSize(hint).x) * 0.5f,
                                   disp.y - 46.0f));
  ImGui::SetWindowFontScale(0.9f);
  ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.60f, 1.0f), "%s", hint);
  ImGui::SetWindowFontScale(1.0f);

  ImGui::End();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor();
}

void AppGL::drawExpeditionHud() {
  // === Expedition ===: компактная карточка главы вверху по центру.
  const ExpeditionChapter* ch = m_expedition.current();
  ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, 56.0f), ImGuiCond_Always,
                          ImVec2(0.5f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(470.0f, 0.0f), ImGuiCond_Always);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.020f, 0.030f, 0.050f, 0.58f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 0.75f, 0.28f, 0.18f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 8.0f));
  ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
  if (!ch || ImGui::Begin("##expeditionHud", nullptr, fl)) {
    if (ch) {
      ImGui::TextUnformatted(uiRu(m_expedition.statusRu().c_str(),
                                  m_expedition.statusEn().c_str()));
      ImGui::SameLine();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.30f, 1.00f));
      ImGui::TextUnformatted(uiRu(ch->titleRu.c_str(), ch->titleEn.c_str()));
      ImGui::PopStyleColor();
      ImGui::SetWindowFontScale(0.88f);
      ImGui::TextColored(ImVec4(0.62f, 0.65f, 0.72f, 1.0f), "%s",
                         uiRu(ch->goalRu.c_str(), ch->goalEn.c_str()));
      ImGui::SetWindowFontScale(1.0f);
    }
  }
  if (ch) ImGui::End();
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(2);
}

void AppGL::drawExpeditionMap() {
  // === Expedition ===: карта глав — сделано/текущая/закрыто.
  ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                          ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_Always);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.020f, 0.030f, 0.050f, 0.80f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 0.75f, 0.28f, 0.25f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
  ImGuiWindowFlags fl = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
  if (ImGui::Begin("##expeditionMap", nullptr, fl)) {
    const auto& chapters = expeditionChapters();
    ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.28f, 1.0f), "%s",
                       uiRu("Карта экспедиции", "Expedition Map"));
    ImGui::Spacing();
    for (int i = 0; i < static_cast<int>(chapters.size()); ++i) {
      const ExpeditionChapter& c = chapters[i];
      const bool done = i < m_expedition.chapterIndex;
      const bool current = i == m_expedition.chapterIndex;
      const char* mark = done ? "OK " : (current ? ">> " : "?? ");
      if (i > 0) ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_Text, done ? ImVec4(0.55f, 0.80f, 0.55f, 1.0f)
                               : (current ? ImVec4(0.95f, 0.75f, 0.30f, 1.0f)
                                          : ImVec4(0.45f, 0.47f, 0.52f, 1.0f)));
      ImGui::TextUnformatted(mark);
      ImGui::PopStyleColor();
      ImGui::SameLine();
      ImGui::TextUnformatted(uiRu(c.titleRu.c_str(), c.titleEn.c_str()));
      if (current && !m_expedition.finished()) {
        ImGui::SetWindowFontScale(0.88f);
        ImGui::Indent();
        ImGui::TextColored(ImVec4(0.62f, 0.65f, 0.72f, 1.0f), "%s",
                           uiRu(c.goalRu.c_str(), c.goalEn.c_str()));
        ImGui::Unindent();
        ImGui::SetWindowFontScale(1.0f);
      }
    }
    if (m_expedition.finished()) {
      ImGui::Spacing();
      ImGui::TextColored(
          ImVec4(0.95f, 0.75f, 0.30f, 1.0f), "%s",
          uiRu("Колония основана. Сюжет завершён.", "Colony founded. Story complete."));
    }
    ImGui::Spacing();
    if (ImGui::Button(uiRu("Закрыть (N / Esc)", "Close (N / Esc)"))) {
      m_expeditionMapOpen = false;
    }
  }
  ImGui::End();
  ImGui::PopStyleVar(1);
  ImGui::PopStyleColor(2);
}

void AppGL::drawModernTransportBar() {
  // Нижняя транспортная панель по центру (как медиаплеер): пауза, пресеты
  // времени, фактор варпа, тур, прыжок. Только кнопки-индикаторы: каждый
  // вызов повторяет соответствующий обработчик из Navigator/хоткеев
  // (togglePause / setTimeScale / warpRetarget / startCineTour /
  // stopCineTour / startWarpJump с их же гвардами). Новой логики нет.
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  if (disp.x <= 0.0f || disp.y <= 0.0f) return;
  ImGui::SetNextWindowPos(ImVec2(disp.x * 0.5f, disp.y - 12.0f),
                          ImGuiCond_Always, ImVec2(0.5f, 1.0f));
  ImGui::SetNextWindowBgAlpha(0.92f);
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_AlwaysAutoResize;
  const bool pushed = modernPushFont();
  if (ImGui::Begin("##modern_transport", nullptr, flags)) {
    const bool paused = m_sceneManager->isPaused();
    const float paceNow = m_sceneManager->getTimeScale();
    // Пауза (тот же вызов, что кнопка "Pause (P)" и хоткей P).
    // Метки строго ASCII ("II", "|>"): в атласе киношрифта только
    // Basic Latin + Cyrillic, прочие глифы дали бы "?".
    if (ImGui::Button(paused ? uiRu("|> (P)", "Resume (P)")
                             : uiRu("II (P)", "Pause (P)"))) {
      m_sceneManager->togglePause();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", uiRu("Пауза/продолжить (P)", "Pause/resume (P)"));
    }
    // Пресеты времени (тот же вызов, что кнопки 1x..1000x: снять паузу,
    // поставить шкалу).
    ImGui::SameLine();
    const char* paceLabels[kTimePacePresetCount] = {"1x", "10x", "60x",
                                                   "300x", "1000x"};
    for (int i = 0; i < kTimePacePresetCount; ++i) {
      if (i > 0) ImGui::SameLine();
      const bool active =
          !paused && std::abs(paceNow - kTimePacePresets[i]) < 0.001f;
      if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImVec4(0.16f, 0.45f, 0.65f, 1.0f));
      }
      if (ImGui::Button(paceLabels[i])) {
        if (m_sceneManager->isPaused()) m_sceneManager->togglePause();
        m_sceneManager->setTimeScale(kTimePacePresets[i]);
      }
      if (active) ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s %s",
                          uiRu("Темп времени", "Time pace"), paceLabels[i]);
      }
    }
    // Фактор варпа (те же гварды, что слайдер варп-панели и [ ]: только
    // при включённом варпе, не в туннеле; иначе просто индикатор).
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    const bool warpAdj = (m_warpOn && m_warpStatus != 3);
    if (!warpAdj) ImGui::BeginDisabled();
    if (ImGui::Button("F-")) {
      if (m_warpFactor > 1) {
        --m_warpFactor;
        warpRetarget(warpTargetK(m_warpFactor));
      }
    }
    if (!warpAdj) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", uiRu("Фактор варпа меньше ([)", "Warp factor down ([)"));
    }
    ImGui::SameLine();
    if (m_warpOn) {
      ImGui::TextColored(ImVec4(0.40f, 0.85f, 1.00f, 1.00f), "F%d %s",
                         m_warpFactor, warpStatusName(m_warpStatus));
    } else {
      ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "%s (X)",
                         uiRu("Варп выкл", "Warp off"));
    }
    ImGui::SameLine();
    if (!warpAdj) ImGui::BeginDisabled();
    if (ImGui::Button("F+")) {
      if (m_warpFactor < 9) {
        ++m_warpFactor;
        warpRetarget(warpTargetK(m_warpFactor));
      }
    }
    if (!warpAdj) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", uiRu("Фактор варпа больше (])", "Warp factor up (])"));
    }
    // Тур (те же вызовы, что хоткей T).
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.00f), "|");
    ImGui::SameLine();
    if (ImGui::Button(m_tour.active() ? uiRu("Стоп-тур", "Stop tour")
                                      : uiRu("Тур (T)", "Tour (T)"))) {
      if (m_tour.active())
        stopCineTour();
      else
        startCineTour();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", uiRu("Кинотур (T)", "Cinematic tour (T)"));
    }
    // Прыжок (тот же вызов и гвард, что "Engage Jump": только из Cruise).
    ImGui::SameLine();
    const std::vector<Station>& cat = builtinCatalog();
    int sel = m_warpSel;
    if (sel < 0 || sel >= static_cast<int>(cat.size())) sel = 0;
    const Station* curSt = m_sceneManager->getCurrentStation();
    const std::string tgtId = cat[static_cast<size_t>(sel)].id;
    const bool canJump = (m_warpOn && m_warpStatus == 1 && curSt &&
                          curSt->id != tgtId);
    if (!canJump) ImGui::BeginDisabled();
    if (ImGui::Button(uiRu("Прыжок", "Jump"))) {
      startWarpJump(tgtId);
    }
    if (!canJump) ImGui::EndDisabled();
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s: %s", uiRu("Прыжок к", "Jump to"),
                        cat[static_cast<size_t>(sel)].name.c_str());
    }
    // Hint-полоса в новом виде: статическая строка хоткеев (логику
    // 10-секундной hint-полосы агента C не трогаем и не дублируем —
    // там авто-гашение, здесь постоянная slim-подсказка).
    ImGui::Separator();
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(
        ImVec4(0.55f, 0.58f, 0.62f, 1.00f), "%s",
         uiRu("WASD - полет | мышь - осмотр | T - кинотур | X - варп | "
              "P - пауза | -/= - темп | / - поиск | F1 - помощь | H - скрыть | "
              "F8 - фото | O - орбита | ESC - меню",
              "WASD fly | drag look | T tour | X warp | P pause | -/= pace | "
              "/ search | F1 help | H hide | F8 photo | O orbit | ESC menu"));
    ImGui::SetWindowFontScale(1.0f);
  }
  ImGui::End();
  if (pushed) ImGui::PopFont();
}
// === ModernUI === end

void AppGL::drawImGui() {
// Детерминизм при съёмке: в горнеле FPS-темп всегда 60.
if (s_harnessPinFps) ImGui::GetIO().Framerate = 60.0f;
  // v3.6.0: захват клавиши для переназначения (страница Controls): любая
  // клавиша записывается в выбранную строку, Esc — отмена без изменений.
  if (m_captureKeyFor >= 0) {
    int* keyTargets[5] = {&m_keyColony, &m_keyView, &m_keyMap, &m_keyJournal,
                          &m_keyTour};
    const int idx = (m_captureKeyFor < 5) ? m_captureKeyFor : 4;
    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(GLFW_KEY_ESCAPE), false)) {
      m_captureKeyFor = -1;
    } else {
      for (int k = 32; k <= 348; ++k) {
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) {
          *keyTargets[idx] = k;
          m_captureKeyFor = -1;
          break;
        }
      }
    }
  }
  // === ModernUI ===: тема применяется один раз (флаг внутри).
  applyModernThemeOnce();
  // === Expedition === (v3.5.0): слайды катсцены — поверх всего и только
  // они (игровой HUD не засоряет кадр истории).
  if (m_slidesOpen) {
    drawSlidesOverlay();
    return;
  }
  // === Ship survival === (v3.3.0): HUD снизу слева, вне меню.
  if (!m_inMenu) drawShipHud();
  // === StoryJournal === (v3.5.0): панель журнала поверх сцены (в меню скрыта).
  if (m_storyOpen && !m_inMenu) drawStoryPanel();
  // === Expedition === (v3.5.0): HUD-карточка главы над сценой.
  if (!m_inMenu) {
    drawExpeditionHud();
    if (m_expeditionMapOpen) drawExpeditionMap();
  }
  ImGui::SetNextWindowPos(ImVec2(15.0f, 15.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(340.0f, 500.0f), ImGuiCond_FirstUseEver);

  ImGui::Begin("astro-voyager Navigator");
  // === ModernUI ===: тело 15px (дефолт ~13px), заголовки ниже — 20px.
  ImGui::SetWindowFontScale(1.15f);

  const Station* curSt = m_sceneManager->getCurrentStation();
  std::string curId = curSt ? curSt->id : "";

  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("Active Station:");
  ImGui::SetWindowFontScale(1.15f);
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", curSt ? curSt->name.c_str() : "None");
  // === ModernUI ===: перенос описания (при шрифте 15px иначе обрезается).
  ImGui::TextWrapped("%s",
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

  // === ModernUI ===: карточки станций (сетка 2 колонки: имя + цветной
  // чип типа + дистанция + факт + кнопка). Фильтр/Enter/очистка — 1:1.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Teleport Stations:");
  ImGui::SetWindowFontScale(1.15f);
  if (!query.empty()) {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(%zu station(s) matched)", matchedStations.size());
  }

  if (matchedStations.empty()) {
    ImGui::TextColored(ImVec4(0.7f, 0.5f, 0.5f, 1.0f), "No matches");
  } else {
    const bool cardFont = modernPushFont();
    if (ImGui::BeginTable("##stcards", 2)) {
      for (const auto* stPtr : matchedStations) {
        const auto& station = *stPtr;
        ImGui::TableNextColumn();
        const bool isCurrent = (station.id == curId);
        // Карточка: рамка + скругление из темы; у текущей — янтарная рамка.
        if (isCurrent) {
          ImGui::PushStyleColor(ImGuiCol_ChildBg,
                                ImVec4(0.20f, 0.30f, 0.20f, 0.92f));
          ImGui::PushStyleColor(ImGuiCol_Border,
                                ImVec4(1.0f, 0.75f, 0.28f, 1.0f));
        }
        std::string cardId = std::string("##card_") + station.id;
        if (ImGui::BeginChild(cardId.c_str(), ImVec2(0.0f, 0.0f), true)) {
          ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s",
                             station.name.c_str());
          // Чип типа (цветной) + дистанция в одну строку.
          // RU — при киношрифте с кириллицей, иначе EN typeToString.
          const std::string tagOwn =
              m_cineFont ? std::string(stationTypeRuShort(station.type))
                         : typeToString(station.type);
          const ImU32 chipCol =
              static_cast<ImU32>(stationTypeChip(station.type));
          const ImVec4 chipBg = ImGui::ColorConvertU32ToFloat4(chipCol);
          ImGui::PushStyleColor(ImGuiCol_Button, chipBg);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, chipBg);
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, chipBg);
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.08f, 0.10f, 0.14f, 1.0f));
          ImGui::BeginDisabled();
          ImGui::SmallButton(tagOwn.c_str());
          ImGui::EndDisabled();
          ImGui::PopStyleColor(4);
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.65f, 0.68f, 0.72f, 1.0f), "%.0f",
                             station.spawnDistanceRs);
          // Факт из Catalog (не выдумываем), затем кнопка телепорта.
          ImGui::TextWrapped("%s", station.description.c_str());
          if (isCurrent) {
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
          }
          if (ImGui::Button(isCurrent ? "Here" : "Fly here",
                            ImVec2(-1.0f, 0.0f))) {
            teleportToStation(station.id);
          }
          if (isCurrent) {
            ImGui::PopStyleColor();
          }
        }
        ImGui::EndChild();
        if (isCurrent) {
          ImGui::PopStyleColor(2);
        }
      }
      ImGui::EndTable();
    }
    if (cardFont) ImGui::PopFont();
  }
  ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                     "(Double-click an object to fly to it)");

  ImGui::Separator();
  // === ModernUI ===: заголовки секций 20px, тело остаётся 15px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Quality Preset:");
  ImGui::SetWindowFontScale(1.15f);
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

  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Parameters:");
  ImGui::SetWindowFontScale(1.15f);

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
  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Camera & Performance:");
  ImGui::SetWindowFontScale(1.15f);
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
  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::Text("Time:");
  ImGui::SetWindowFontScale(1.15f);
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
  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Journey Route:");
  ImGui::SetWindowFontScale(1.15f);

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
  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Editor:");
  ImGui::SetWindowFontScale(1.15f);
  // v3.2.0 flexible object editor: pick a surface style, tune size, palette
  // and ring, then Add spawns the object 15 units ahead of the camera.
  // Objects can be re-selected by clicking and edited live below
  // (Solar System station only).
  int prevType = m_editorType;
  ImGui::SetNextItemWidth(-1.0f);
  ImGui::Combo("##editortype", &m_editorType, kEditorTypes, IM_ARRAYSIZE(kEditorTypes));
  if (m_editorType != prevType) {
    EditorPreset pr = editorPresetFor(m_editorType);
    m_editorRadius = pr.radius;
    m_editorColA[0] = pr.colA[0];
    m_editorColA[1] = pr.colA[1];
    m_editorColA[2] = pr.colA[2];
    m_editorColB[0] = pr.colB[0];
    m_editorColB[1] = pr.colB[1];
    m_editorColB[2] = pr.colB[2];
    m_editorRing = false;
  }
  ImGui::SliderFloat("Size", &m_editorRadius, 0.05f, 3.0f, "%.2f");
  ImGui::ColorEdit3("Colour A", m_editorColA);
  ImGui::ColorEdit3("Colour B", m_editorColB);
  ImGui::Checkbox("Ring", &m_editorRing);
  ImGui::SameLine();
  if (ImGui::Button("Random seed")) {
    m_editorSeed = 0.5f + static_cast<float>(rand() % 900) / 100.0f;
  }
  if (ImGui::Button("Add")) {
    EditorPreset pr = editorPresetFor(m_editorType);
    CustomBody b;
    b.type = pr.type;
    b.detail = pr.detail;
    b.radius = m_editorRadius;
    b.color = Vec3(m_editorColA[0], m_editorColA[1], m_editorColA[2]);
    b.colorB = Vec3(m_editorColB[0], m_editorColB[1], m_editorColB[2]);
    b.ringOuter = m_editorRing ? m_editorRadius * 2.0f : 0.0f;
    b.seed = m_editorSeed;
    Vec3 f = m_camera.forward();
    Vec3 u = m_camera.up();
    Vec3 p(m_camera.position.x + f.x * 15.0,
            m_camera.position.y + f.y * 15.0,
            m_camera.position.z + f.z * 15.0);
    Vec3 v(f.x * 2.0 + u.x * 0.5,
            f.y * 2.0 + u.y * 0.5,
            f.z * 2.0 + u.z * 0.5);
    m_sceneManager->spawnCustom(b, p, v);
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
    ImGui::Text("Objects: %zu, selected: %s", nCustom, selName.c_str());
  }
  if (hasSel) {
    CustomBody* sel = m_sceneManager->findCustom(m_sceneManager->getSelected());
    if (sel) {
      ImGui::Separator();
      ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Selected: %s", sel->name.c_str());
      // Live edit: palette/size/palette-colours/ring/seed applied on the
      // spot (click-selected object, no confirm needed).
      int selIdx = editorIndexFor(*sel);
      ImGui::SetNextItemWidth(-1.0f);
      if (ImGui::Combo("##seltype", &selIdx, kEditorTypes, IM_ARRAYSIZE(kEditorTypes))) {
        EditorPreset pr = editorPresetFor(selIdx);
        sel->type = pr.type;
        sel->detail = pr.detail;
        sel->radius = pr.radius;
        sel->color = Vec3(pr.colA[0], pr.colA[1], pr.colA[2]);
        sel->colorB = Vec3(pr.colB[0], pr.colB[1], pr.colB[2]);
        sel->ringOuter = 0.0f;
      }
      ImGui::SliderFloat("Size##sel", &sel->radius, 0.05f, 3.0f, "%.2f");
      float ca[3] = {static_cast<float>(sel->color.x),
                     static_cast<float>(sel->color.y),
                     static_cast<float>(sel->color.z)};
      if (ImGui::ColorEdit3("Colour A##sel", ca)) {
        sel->color = Vec3(ca[0], ca[1], ca[2]);
      }
      float cb[3] = {static_cast<float>(sel->colorB.x),
                     static_cast<float>(sel->colorB.y),
                     static_cast<float>(sel->colorB.z)};
      if (ImGui::ColorEdit3("Colour B##sel", cb)) {
        sel->colorB = Vec3(cb[0], cb[1], cb[2]);
      }
      bool ringSel = sel->ringOuter > 0.0f;
      if (ImGui::Checkbox("Ring##sel", &ringSel)) {
        sel->ringOuter = ringSel ? sel->radius * 2.0f : 0.0f;
      }
      ImGui::SameLine();
      if (ImGui::Button("Random seed##sel")) {
        sel->seed = 0.5f + static_cast<float>(rand() % 900) / 100.0f;
      }
      ImGui::Text("Pos: %.1f %.1f %.1f", sel->pos.x, sel->pos.y, sel->pos.z);
    }
  }

  ImGui::Separator();
  // v3.7.0: сводка прогресса до ачивок.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Progress");
  ImGui::SetWindowFontScale(1.15f);
  ImGui::BulletText("Colonies founded: %d/2", static_cast<int>(m_colonies.size()));
  ImGui::BulletText("Debris dodged: %d", m_asteroidDodges);
  ImGui::BulletText("Event horizon crossed: %s",
                    m_capturedHorizon ? "yes" : "no");
  ImGui::BulletText("Stations visited: %d", static_cast<int>(m_visited.size()));
  ImGui::Spacing();
  // === ModernUI ===: заголовок секции 20px.
  ImGui::SetWindowFontScale(1.5f);
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Achievements: %d/10", static_cast<int>(m_unlockedIds.size()));
  ImGui::SetWindowFontScale(1.15f);
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

  // === Audio === begin (агент AU: слайдер 0-100 + M/mute + нота В ПАНЕЛИ.
  // До первого ввода движок не инициализирован — рисуем пустоту, поэтому
  // харнесс (init нет) даёт gui.png байт-в-байт).
  audioDrawPanel();
  // === Audio === end

  ImGui::End();

  // === ModernUI ===: статус-бар и транспортная панель (только индикаторы
  // поверх существующего состояния; при m_hideUI drawImGui не вызывается).
  drawModernStatusBar();
  drawModernTransportBar();

  // === WarpDrive === (отдельное окно; при выключенном варпе пусто,
  // Navigator выше не менялся — gui.png чистый).
  drawWarpPanel();
}

void AppGL::teleportToStation(const std::string& id, bool stopJumpCine) {
  // === Cinematic === begin (агент C): ручной телепорт останавливает тур.
  if (m_tour.active()) stopCineTour();
  // === Cinematic === end
  // v3.8.0: внешние телепорты прерывают варп-катсцену — иначе загрузка
  // профиля или импорт маршрута посреди прыжка оставляют «призрачный» полёт
  // (контроллер WarpJump живёт дальше смазанного мира, поля не сброшены).
  // Внутренний телепорт Бурста (stopJumpCine=false) не убивает свой прыжок.
  if (m_jumpCine && stopJumpCine) {
    abortJumpCine();
  }
  m_sceneManager->teleportTo(id, m_camera);
  m_flying = false;
  // v3.6.0: телепорт обнуляет накопленный ход и напоминание об орбите.
  m_shipVel = Vec3();
  m_orbitNoticeShown = false;
  if (m_route.empty() || m_route.back() != id) {
    m_route.push_back(id);
  }
  if (std::find(m_visited.begin(), m_visited.end(), id) == m_visited.end()) {
    m_visited.push_back(id);
  }
  // === Cinematic === begin (агент C): интро-карточка 3с (только ручной путь;
  // тур рисует свой капшн сам и сюда не заходит).
  if (const Station* cst = m_sceneManager->getCurrentStation()) {
    if (cst->id == id) showIntroCard(cst);
  }
  // === Cinematic === end
  checkAchievements();

  // === Expedition === (v3.5.0): посещение цели текущей главы прокручивает
  // сюжет; в прологе свои слайды — главы озвучиваем только после него.
  const int expBefore = m_expedition.chapterIndex;
  m_expedition.markVisited(id);
  if (m_expedition.chapterIndex > expBefore && m_expedition.introSeen) {
    openChapterSlide(expeditionChapters()[expBefore].slideIdx);
  }
  saveExpeditionFileNow();
}

// === Ship survival === (v3.3.0) begin.
// Один тик выживания: гравитация к ближайшей массе, радиация пульсаров,
// топливо у звезды, смерть и пост-эффекты низкого щита. Зовётся из run()
// до render(); пауза (P) и нулевой dt его останавливают.
void AppGL::updateShip(float deltaTime) {
  if (deltaTime <= 0.0f || m_sceneManager->isPaused()) return;
  const Station* st = m_sceneManager->getCurrentStation();
  if (!st) return;

  bool nearStar = false;
  Vec3 gravAcc(0.0, 0.0, 0.0);
  bool haveGrav = false;

  if (st->type == StationType::SolarSystem) {
    // Ближайшее массивное тело тянет камеру; у Солнца топливо восполняется.
    // Тела приходят из последнего render() (m_lastBodies) — лаг в кадр ок.
    double bestD2 = 1e30;
    for (const auto& body : m_sceneManager->getBodies()) {
      if (!body.def) continue;
      const double dx = body.pos.x - m_camera.position.x;
      const double dy = body.pos.y - m_camera.position.y;
      const double dz = body.pos.z - m_camera.position.z;
      const double d2 = dx * dx + dy * dy + dz * dz;
      if (body.def->mass > 0.0f && d2 < bestD2) {
        bestD2 = d2;
        gravAcc = ship::gravAccelToward(body.def->mass, m_camera.position,
                                        body.pos);
        haveGrav = true;
      }
      if (body.def->id == "sun" &&
          d2 < ship::kStarFuelRange * ship::kStarFuelRange) {
        nearStar = true;
      }
    }
  } else if (st->mass > 0.0) {
    // Одиночный массивный центр станции (ЧД sgra/m87, квазар, пульсары):
    // начало координат — центр станции.
    gravAcc = ship::gravAccelToward(st->mass, m_camera.position,
                                    Vec3(0.0, 0.0, 0.0));
    haveGrav = true;
  }

  // Радиация пульсара: опасно ближе пяти его радиусов от центра.
  bool inRadZone = false;
  if (st->type == StationType::Pulsar) {
    const double d =
        std::sqrt(m_camera.position.x * m_camera.position.x +
                  m_camera.position.y * m_camera.position.y +
                  m_camera.position.z * m_camera.position.z);
    inRadZone = d < ship::kPulsarRadius * ship::kPulsarDamageRadii;
  }

  // v3.6.0: сложность масштабирует баланс (Easy: быстрая заправка и мягкая
  // радиация; Hard: наоборот). K=1 в difficultyTuning(Normal) - дефолт.
  const DifficultyTuning dtune = difficultyTuning(m_difficulty);
  ship::updateFuel(m_ship, deltaTime, nearStar, dtune.starFuelK);
  const float shieldBefore = m_ship.shield;
  ship::applyRadiation(m_ship, deltaTime, inRadZone, dtune.radiationK);
  // Вспышка щита (v3.3.0): импульс от потери щита + экспоненциальное затухание.
  if (m_ship.shield < shieldBefore) {
    m_shieldFlashK =
        ship::shieldFlashHit(m_shieldFlashK, shieldBefore - m_ship.shield);
  }
  m_shieldFlashK = ship::shieldFlashDecay(m_shieldFlashK, deltaTime);
  if (haveGrav && !m_jumpCine) {
    m_camera.applyGravityStep(m_ship.gravVel, gravAcc, deltaTime);
    if (st->type == StationType::BlackHole) {
      // Дрейф не должен утаскивать под горизонт — тот же клэмп, что в run().
      m_camera.clampNearHorizon(m_sceneManager->getSettings().rs);
    }
    // v3.6.0: орбитальный ассист — без активного ввода вблизи массивного
    // центра корабль сам выходит на круговое курсирование (притяжение +
    // мягкая стабилизация тангенциальной скорости).
    if (!m_moveInputActive && !m_captureAnim &&
        ship::orbitAssist(m_camera.position, gravAcc, m_shipVel, m_ship.gravVel,
                          static_cast<double>(deltaTime))) {
      if (!m_orbitNoticeShown) {
        m_orbitNoticeShown = true;
        m_shipNotice = uiRu("Орбита: автокурсирование стабилизировано",
                            "Orbit: cruise stabilized (auto)");
        m_shipNoticeT = 3.5f;
      }
    }
  }

  // Пустой бак: одноразовый тост + постоянная строка в HUD (см. drawShipHud).
  if (m_ship.fuel <= 0.0f && !m_ship.fuelNoticeShown) {
    m_ship.fuelNoticeShown = true;
    Toast t;
    t.title = uiRu("Топливо исчерпано", "Out of fuel");
    t.desc = uiRu("Дрейф к ближайшей звезде для дозаправки.",
                  "Drift to the nearest star to refuel.");
    t.time = static_cast<float>(glfwGetTime());
    m_toasts.push_back(t);
  } else if (m_ship.fuel > 0.0f) {
    m_ship.fuelNoticeShown = false;
  }

  // Смерть: щиты пробиты — аварийный прыжок к Солнцу и баланс респавна.
  if (ship::isDead(m_ship) && !m_jumpCine && !m_captureAnim) {
    teleportToStation("sol");
    ship::respawn(m_ship);
    m_shipVel = Vec3();
    m_orbitNoticeShown = false;
    m_shipNotice = uiRu("Щиты уничтожены. Аварийный прыжок к Солнцу.",
                        "Shields destroyed. Emergency jump to the Sun.");
    m_shipNoticeT = 5.0f;
  }

  if (m_shipNoticeT > 0.0f) m_shipNoticeT -= deltaTime;

  // Низкий щит: хроматика (<30) и помехи (<10). Ноль при полном щите —
  // композит остаётся байт-в-байт на скриншот-харнессе.
  m_sceneManager->setShipFx(ship::chromaK(m_ship.shield),
                            ship::staticK(m_ship.shield), m_shieldFlashK);

  // === v3.6.0: колонии === подсказка у планеты-сайта и hotkey C.
  // Сайты приходят из прошлого кадра (m_sceneManager->colonySites()
  // заполняется в render()); дрейф позиции за кадр незаметен.
  m_colonyHint.clear();
  m_colonyHintId.clear();
  if (!m_inMenu && !m_slidesOpen && !m_jumpCine && !m_captureAnim) {
    const Station* stNow = m_sceneManager->getCurrentStation();
    if (stNow) {
      for (const SceneManager::ColonySite& site : m_sceneManager->colonySites()) {
        if (site.stationId != stNow->id) continue;
        if (m_sceneManager->hasColony(site.id)) continue;
        const double dx = m_camera.position.x - site.pos.x;
        const double dy = m_camera.position.y - site.pos.y;
        const double dz = m_camera.position.z - site.pos.z;
        const double lim = static_cast<double>(site.radius) * 8.0 + 2.5;
        if (dx * dx + dy * dy + dz * dz < lim * lim) {
          m_colonyHint = site.name;
          m_colonyHintId = site.id;
          break;
        }
      }
    }
  }
  const bool cDown =
      (glfwGetKey(m_window->getNativeWindow(), m_keyColony) == GLFW_PRESS);
  ImGuiIO& colonyIo = ImGui::GetIO();
  if (cDown && !m_prevCPressed && !colonyIo.WantCaptureKeyboard &&
      !m_colonyHintId.empty()) {
    m_colonies.push_back(m_colonyHintId);
    m_sceneManager->setColonies(m_colonies);
    Toast ct;
    ct.title = uiRu("Колония основана", "Colony established");
    ct.desc = m_colonyHint + uiRu(" — под вашим флагом.", " — under your flag.");
    ct.time = static_cast<float>(glfwGetTime());
    m_toasts.push_back(ct);
    m_shipNotice = uiRu("Колония основана: ", "Colony established: ") + m_colonyHint;
    m_shipNoticeT = 4.0f;
    checkAchievements();  // v3.7.0: colonist / architect
    saveProfileNow(false);
    m_colonyHint.clear();
    m_colonyHintId.clear();
  }
  m_prevCPressed = cDown;
}

// Нижний левый HUD: полосы топлива (оранжевая) и щитов (синяя) + нотисы.
void AppGL::drawShipHud() {
  ImGuiIO& io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(12.0f, io.DisplaySize.y - 94.0f),
                          ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.55f);
  if (ImGui::Begin("##shipHud", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoFocusOnAppearing |
                       ImGuiWindowFlags_NoNav |
                       ImGuiWindowFlags_NoSavedSettings)) {
    char buf[48];
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                          ImVec4(1.0f, 0.55f, 0.15f, 1.0f));
    std::snprintf(buf, sizeof(buf), "%s %.0f", uiRu("Топливо", "Fuel"),
                  m_ship.fuel);
    ImGui::ProgressBar(m_ship.fuel / 100.0f, ImVec2(180.0f, 12.0f), buf);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                          ImVec4(0.25f, 0.6f, 1.0f, 1.0f));
    std::snprintf(buf, sizeof(buf), "%s %.0f", uiRu("Щиты", "Shields"),
                  m_ship.shield);
    ImGui::ProgressBar(m_ship.shield / 100.0f, ImVec2(180.0f, 12.0f), buf);
    ImGui::PopStyleColor();
    // Вид камеры (v3.3.0): строка-подсказка для клавиши V.
    // v3.6.0: сложность Hard отключает подсказки (difficultyTuning().hints).
    if (difficultyTuning(m_difficulty).hints) {
      static const char* kViewRu[3] = {"Кабина", "За кормой", "Обзор"};
      static const char* kViewEn[3] = {"Cockpit", "Behind", "Overview"};
      const int vi = static_cast<int>(m_viewMode);
      ImGui::TextColored(ImVec4(0.6f, 0.75f, 0.9f, 1.0f), "%s: %s",
                         uiRu("Вид", "View"), uiRu(kViewRu[vi], kViewEn[vi]));
    }
    if (m_shipNoticeT > 0.0f && !m_shipNotice.empty()) {
      ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s",
                         m_shipNotice.c_str());
    } else if (m_ship.fuel <= 0.0f) {
      ImGui::TextColored(
          ImVec4(1.0f, 0.55f, 0.15f, 1.0f), "%s",
          uiRu("Топливо исчерпано. Дрейф к ближайшей звезде...",
               "Out of fuel. Drifting to the nearest star..."));
    }
    // v3.6.0: подсказка колонизации у планеты-сайта.
    if (!m_colonyHint.empty()) {
      ImGui::TextColored(ImVec4(0.45f, 0.85f, 1.0f, 1.0f), "%s %s",
                         uiRu("[C] Основать колонию:", "[C] Colonize:"),
                         m_colonyHint.c_str());
    }
  }
  ImGui::End();
}
// === Ship survival === end.

// === Camera views + speed FX === (v3.3.0) begin.
// Камера кадра: в меню и катсцене — как есть; в видах 2/3 — снос назад
// вдоль курса и чуть выше. Логическая m_camera не меняется (движение,
// коллизии, ачивки и sun-link считаются по ней).
void AppGL::applyViewCamera(Camera& out) {
  out = m_camera;
  m_renderCamValid = true;
  if (m_jumpCine || m_inMenu) return;
  if (m_viewMode == view::ViewMode::Cockpit) return;
  out.position =
      view::offsetCameraPos(m_camera.position, m_camera.forward(), m_viewMode);
  const Station* st = m_sceneManager->getCurrentStation();
  if (st && st->type == StationType::BlackHole) {
    // Снос не должен затаскивать камеру кадра под горизонт.
    out.clampNearHorizon(m_sceneManager->getSettings().rs);
  }
}

// Скорость: реальная (u/s, сглаженная) и варп-фактор дают boost beta для
// доплера/абберации (setSpeedFx), стрики в композите (u_SpeedK) и микро-FOV.
// Варп: туннель не двигает камеру — без этого «скорость» не чувствуется.
void AppGL::updateSpeedFx(float deltaTime) {
  if (deltaTime > 0.0f && deltaTime < 0.5f) {
    const double dx = m_camera.position.x - m_prevSpeedPos.x;
    const double dy = m_camera.position.y - m_prevSpeedPos.y;
    const double dz = m_camera.position.z - m_prevSpeedPos.z;
    const double inst = std::sqrt(dx * dx + dy * dy + dz * dz) / deltaTime;
    m_prevSpeedPos = m_camera.position;
    if (inst < 1500.0) {  // телепорт не считаем скоростью
      const float k = 1.0f - std::exp(-deltaTime * 5.0f);
      m_camSpeed += (static_cast<float>(inst) - m_camSpeed) * k;
      if (inst > 0.5) {
        m_speedDir = Vec3(dx / inst, dy / inst, dz / inst);
      }
    } else {
      m_camSpeed = 0.0f;
    }
  }
  // Полный мировой эффект от 12 u/s; варп — по фактору (m_warpK 0..1).
  // Во время катсцены прыжка камерой владеет cine: мир-скорость = 0.
  const float worldK = m_jumpCine ? 0.0f : view::speedFx(m_camSpeed, 12.0f);
  const float warpK =
      m_warpOn ? (m_warpK < 0.0f ? 0.0f : (m_warpK > 1.0f ? 1.0f : m_warpK))
               : 0.0f;
  const float speedK = worldK > warpK ? worldK : warpK;
  const float betaWorld = worldK * 0.40f;
  const float betaWarp = warpK * 0.65f;
  const float betaBoost = betaWorld > betaWarp ? betaWorld : betaWarp;
  Vec3 dir = m_speedDir;
  if (warpK > worldK) {
    dir = m_camera.forward();  // в туннеле «летим» по курсу
  }
  m_sceneManager->setSpeedFx(betaBoost, speedK, static_cast<float>(dir.x),
                             static_cast<float>(dir.y),
                             static_cast<float>(dir.z), warpK);
  m_speedK = speedK;
  // v3.3.0: тяга масштабирует шлейф/движок корабля и гул в audioUpdate.
  warpdrive::setThruster(speedK);
  // Микро-FOV: +3° на полном эффекте поверх варп-кривой (+10° * m_warpK).
  auto& st = m_sceneManager->getSettings();
  if (m_warpBaseFov <= 1.0f) m_warpBaseFov = st.fovDegrees;  // ленивая база
  const float want = m_warpBaseFov + 10.0f * m_warpK + 3.0f * speedK;
  if (std::fabs(st.fovDegrees - want) > 0.05f) {
    st.fovDegrees = want;
    m_warpLastFov = want;  // не спутать с ручной правкой слайдера FOV
  }
}
// === Camera views + speed FX === end.

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

// === SaveGame === (v3.4.0) begin.
// JSON-профиль: настройки (settings.json) + песочница (save-sandbox.json).
// Конверсия редакторских объектов CustomBody <-> SaveCustom — поле в поле.
namespace {
SaveCustom toSaveCustom(const CustomBody& c) {
  SaveCustom s;
  s.id = c.id;
  s.name = c.name;
  s.pos = c.pos;
  s.vel = c.vel;
  s.radius = c.radius;
  s.color = c.color;
  s.colorB = c.colorB;
  s.type = c.type;
  s.detail = c.detail;
  s.ringOuter = c.ringOuter;
  s.seed = c.seed;
  return s;
}

CustomBody toCustomBody(const SaveCustom& s) {
  CustomBody c;
  c.id = s.id;
  c.name = s.name;
  c.pos = s.pos;
  c.vel = s.vel;
  c.radius = s.radius;
  c.color = s.color;
  c.colorB = s.colorB;
  c.type = s.type;
  c.detail = s.detail;
  c.ringOuter = s.ringOuter;
  c.seed = s.seed;
  return c;
}
}  // namespace

void AppGL::applySaveSettings(const SaveSettings& s) {
  const int preset = std::min(std::max(s.perfPreset, 0), 3);
  m_sceneManager->applyPreset(static_cast<PerfPreset>(preset));
  m_userOverrodePreset = true;  // сохранённый пресет не перебивается бенчмарком
  m_audioVolume = std::min(std::max(s.audioVolume, 0), 100);
  m_audio.setVolume(static_cast<float>(m_audioVolume) / 100.0f);
  if (s.muted != m_audio.isMuted()) m_audio.toggleMute();
  m_viewMode = static_cast<view::ViewMode>(std::min(std::max(s.viewMode, 0), 2));
  // === Settings v3.6.0 ===: сложность, сглаживание, палитра и кастомный
  // рейтмарчинг. Параметры с sentinel 0 восстанавливаются только для
  // Custom-пресета (иначе действуют значения пресета).
  m_difficulty = std::min(std::max(s.difficulty, 0), 2);
  m_aaMode = std::min(std::max(s.aaMode, 0), 1);
  m_sceneManager->setFxaa(m_aaMode == 1);
  RenderSettings& rs = m_sceneManager->getSettings();
  rs.shipReflections = s.shipReflections;
  rs.enginePalette = std::min(std::max(s.enginePalette, 0), 3);
  if (preset == 3) {
    if (s.renderScale > 0.0f) rs.rs = s.renderScale;
    if (s.shellRadius > 0.0f) rs.shellRadius = s.shellRadius;
    if (s.stepSize > 0.0f) rs.stepSize = s.stepSize;
    if (s.maxSteps > 0) rs.maxSteps = s.maxSteps;
  }
  // v3.6.0: управление — мышь и переназначенные клавиши (с валидацией).
  m_mouseSensitivity = std::min(std::max(s.mouseSensitivity, 0.3f), 3.0f);
  m_invertY = (s.invertY != 0);
  auto keyOk = [](int k) { return k >= 32 && k <= 348; };
  m_keyColony = keyOk(s.keyColony) ? s.keyColony : 67;
  m_keyView = keyOk(s.keyView) ? s.keyView : 86;
  m_keyMap = keyOk(s.keyMap) ? s.keyMap : 78;
  m_keyJournal = keyOk(s.keyJournal) ? s.keyJournal : 74;
  m_keyTour = keyOk(s.keyTour) ? s.keyTour : 84;
  m_renderCamValid = false;
}

void AppGL::applySaveGame(const SaveGame& g) {
  // v3.6.0: загрузка посреди катсцены горизонта недопустима — сброс.
  m_captureAnim = false;
  m_horizonWarped = false;
  m_sceneManager->setHorizonFx(0.0f, 0.0f);
  // Объекты редактора: заменяем текущие загруженными (id/имена сохраняются).
  m_sceneManager->clearCustom();
  for (const SaveCustom& c : g.customs) {
    m_sceneManager->addLoadedCustom(toCustomBody(c));
  }
  // Прогресс и ачивки.
  m_visited = g.progress.visited;
  m_unlockedIds.clear();
  for (const std::string& id : g.progress.unlocked) m_unlockedIds.insert(id);
  m_trophyKnown = m_unlockedIds;  // загруженные ачивки не вспыхивают заново
  m_minBhDistance = g.progress.minBhDistance;
  m_journeyExported = g.progress.exported;
  m_journeyImported = g.progress.imported;
  m_capturedHorizon = g.progress.capturedHorizon;
  // v3.6.0: колонии (профиль → менеджер сцены для маркеров).
  m_colonies = g.progress.colonies;
  m_sceneManager->setColonies(m_colonies);
  // v3.7.0: жизненный счётчик уклонений.
  m_asteroidDodges = g.progress.asteroidDodges;
  m_colonyHint.clear();
  m_colonyHintId.clear();
  // Станция и камера. teleportToStation сам добавит станцию в маршрут и
  // visited — перезаписываем их профильными значениями.
  if (findStation(g.session.station)) {
    teleportToStation(g.session.station);
    m_route = g.progress.route;
    m_visited = g.progress.visited;
  }
  m_camera.position = g.session.camPos;
  m_camera.yaw = g.session.yaw;
  m_camera.pitch = g.session.pitch;
  m_sceneManager->setTimeScale(g.session.timeScale);
  // Корабль и стиль.
  m_ship.fuel = g.session.fuel;
  m_ship.shield = g.session.shield;
  m_ship.gravVel = Vec3{};
  m_shipVel = Vec3();
  m_orbitNoticeShown = false;
  m_shipNoticeT = 0.0f;
  setShipStyleNow(g.session.shipStyle);  // v3.6.0: с учётом варп-ролей
  // Скоростные эффекты: после прыжка камеры не меряем «скачок скорости»
  // (иначе ложный доплер/стрики на первом кадре).
  m_prevSpeedPos = m_camera.position;
  m_camSpeed = 0.0f;
  m_renderCamValid = false;
  m_sceneManager->resetRelativity();
}

void AppGL::saveProfileNow(bool quiet) {
  if (!m_saveInteractive) return;
  std::string err;

  SaveSettings st;
  st.perfPreset = static_cast<int>(m_sceneManager->getPreset());
  st.audioVolume = m_audioVolume;
  st.muted = m_audio.isMuted();
  st.viewMode = static_cast<int>(m_viewMode);
  // === Settings v3.6.0 ===
  st.difficulty = m_difficulty;
  st.aaMode = m_aaMode;
  const RenderSettings& rsNow = m_sceneManager->getSettings();
  st.shipReflections = rsNow.shipReflections;
  st.enginePalette = rsNow.enginePalette;
  if (st.perfPreset == static_cast<int>(PerfPreset::Custom)) {
    // Кастомный рейтмарчинг; sentinel 0 означает «параметр не менялся».
    st.renderScale = rsNow.rs;
    st.shellRadius = rsNow.shellRadius;
    st.stepSize = rsNow.stepSize;
    st.maxSteps = rsNow.maxSteps;
  }
  // v3.6.0: управление — мышь и переназначенные клавиши.
  st.mouseSensitivity = m_mouseSensitivity;
  st.invertY = m_invertY ? 1 : 0;
  st.keyColony = m_keyColony;
  st.keyView = m_keyView;
  st.keyMap = m_keyMap;
  st.keyJournal = m_keyJournal;
  st.keyTour = m_keyTour;
  const bool settingsOk =
      saveWriteFile(saveSettingsFilePath(), saveSerializeSettings(st), err);

  SaveGame g;
  g.mode = SaveMode::Sandbox;
  g.savedUtc = saveUtcNowISO8601();
  g.progress.visited = m_visited;
  g.progress.unlocked.assign(m_unlockedIds.begin(), m_unlockedIds.end());
  g.progress.route = m_route;
  g.progress.colonies = m_colonies;
  g.progress.asteroidDodges = m_asteroidDodges;  // v3.7.0
  g.progress.minBhDistance = m_minBhDistance;
  g.progress.exported = m_journeyExported;
  g.progress.imported = m_journeyImported;
  g.progress.capturedHorizon = m_capturedHorizon;
  for (const CustomBody& c : m_sceneManager->getCustoms()) {
    g.customs.push_back(toSaveCustom(c));
  }
  if (const Station* cur = m_sceneManager->getCurrentStation()) {
    g.session.station = cur->id;
  }
  g.session.camPos = m_camera.position;
  g.session.yaw = m_camera.yaw;
  g.session.pitch = m_camera.pitch;
  g.session.timeScale = m_sceneManager->getTimeScale();
  g.session.fuel = m_ship.fuel;
  g.session.shield = m_ship.shield;
  g.session.shipStyle = m_shipStyle;
  const bool saveOk =
      saveWriteFile(saveFilePath(SaveMode::Sandbox), saveSerialize(g), err);

  if (saveOk && settingsOk) {
    m_saveStatus = uiRu("Профиль сохранён", "Profile saved");
    if (!quiet) {
      m_audio.playClick();
      Toast t;
      t.title = uiRu("Сохранение", "Save");
      t.desc = uiRu("Песочница записана (F10 — загрузить).",
                    "Sandbox written to the profile (F10 to load).");
      t.time = static_cast<float>(glfwGetTime());
      m_toasts.push_back(t);
    }
  } else {
    m_saveStatus = std::string(uiRu("Ошибка сохранения", "Save failed")) +
                   ": " + err;
    if (!quiet) m_audio.playBlip();
  }
}

void AppGL::loadProfileNow(bool quiet) {
  if (!m_saveInteractive) return;
  std::string text;
  std::string err;

  // Настройки — не критично: нет файла, остаются текущие значения.
  SaveSettings st;
  if (saveReadFile(saveSettingsFilePath(), text, err) &&
      saveDeserializeSettings(text, st, err)) {
    applySaveSettings(st);
  }

  const std::string path = saveFilePath(SaveMode::Sandbox);
  if (!saveFileExists(path)) {
    m_saveStatus = uiRu("Профиль не найден (первый запуск)",
                        "No profile yet (first run)");
    return;
  }
  if (!saveReadFile(path, text, err)) {
    m_saveStatus = std::string(uiRu("Ошибка чтения", "Read failed")) + ": " + err;
    if (!quiet) m_audio.playBlip();
    return;
  }
  SaveGame g;
  if (!saveDeserialize(text, g, err)) {
    m_saveStatus =
        std::string(uiRu("Ошибка профиля", "Profile error")) + ": " + err;
    if (!quiet) m_audio.playBlip();
    return;
  }
  applySaveGame(g);
  m_saveStatus = std::string(uiRu("Профиль загружен", "Profile loaded")) +
                 ": " + g.session.station;
  if (!quiet) {
    m_audio.playChime();
    Toast t;
    t.title = uiRu("Загрузка", "Load");
    t.desc = uiRu("Состояние восстановлено из профиля.",
                  "State restored from the profile.");
    t.time = static_cast<float>(glfwGetTime());
    m_toasts.push_back(t);
  }
}
// === SaveGame === end

void AppGL::checkAchievements() {
  AchievementState st;
  st.visited = m_visited;
  st.routeSize = m_route.size();
  st.minBhDistance = m_minBhDistance;
  st.capturedHorizon = m_capturedHorizon;  // === Physics ===
  st.exported = m_journeyExported;
  st.imported = m_journeyImported;
  // v3.7.0: колонии + уклонения от обломков.
  st.colonyCount = m_colonies.size();
  st.asteroidDodged = (m_asteroidDodges > 0);

  // === StoryJournal === (v3.5.0): журнал обновляется из того же снапшота
  // прогресса; вызовы checkAchievements покрывают все точки прогресса
  // (старт, телепорт, прыжки, горизонт, экспорт/импорт).
  StoryJournal::Progress sp;
  sp.visited = st.visited;
  sp.minBhDistance = st.minBhDistance;
  sp.exported = st.exported;
  sp.imported = st.imported;
  sp.capturedHorizon = st.capturedHorizon;
  m_story.refresh(sp);

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
  ImGui::BulletText("V camera view: cockpit / behind / overview");  // v3.3.0
  ImGui::BulletText("Drag mouse to look around");
  ImGui::BulletText("/ focus station search");
  ImGui::BulletText("F5 export journey, F6 import journey");
  ImGui::BulletText("J story journal (bilingual)");  // v3.5.0
  ImGui::BulletText("N expedition map");             // v3.5.0
  ImGui::BulletText("P pause / resume time");
  ImGui::BulletText("X warp drive on / off");
  ImGui::BulletText("- / = time pace down / up");
  ImGui::BulletText("Double-click an object to fly to it");
  ImGui::BulletText("Del delete selected rock");
  ImGui::BulletText("H hide / show UI");
  ImGui::BulletText("F8 photo mode (exposure + Save PNG)");
  ImGui::BulletText("O auto-orbit around station (WASD exits)");
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
  ImGui::SetNextWindowSize(ImVec2(460.0f, 0.0f), ImGuiCond_Always);
  ImGuiWindowFlags menuFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

  // === MenuStyle === (v3.5.0): стеклянная панель поверх живого космоса —
  // тёмное стекло с золотой кромкой, hover-кнопки теплеют.
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.020f, 0.030f, 0.050f, 0.72f));
  ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 0.75f, 0.28f, 0.20f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.060f, 0.105f, 0.160f, 0.78f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.110f, 0.190f, 0.290f, 0.92f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.150f, 0.240f, 0.360f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.86f, 0.92f, 1.00f));
  ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.00f, 0.75f, 0.28f, 0.28f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 18.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.0f, 5.0f));
  ImGui::Begin("MainMenu", nullptr, menuFlags);

  if (m_menuPage == 0) {
    // Title: чёрная тень + золотое свечение + золотая строка (DrawList).
    {
      ImDrawList* tdl = ImGui::GetWindowDrawList();
      const char* titleText = "ASTRO-VOYAGER";
      ImGui::SetWindowFontScale(2.0f);
      const ImVec2 tpos = ImGui::GetCursorScreenPos();
      const float tsize = ImGui::GetFontSize();
      tdl->AddText(ImGui::GetFont(), tsize, ImVec2(tpos.x + 2.0f, tpos.y + 3.0f),
                   IM_COL32(0, 0, 0, 200), titleText);
      tdl->AddText(ImGui::GetFont(), tsize + 2.0f, ImVec2(tpos.x - 1.0f, tpos.y - 1.0f),
                   IM_COL32(255, 191, 71, 60), titleText);
      tdl->AddText(ImGui::GetFont(), tsize, tpos, IM_COL32(255, 196, 77, 255), titleText);
      ImGui::Dummy(ImVec2(0.0f, tsize + 6.0f));
      ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(ImVec4(0.6f, 0.62f, 0.68f, 1.0f),
                       "A journey through curved spacetime");
    ImGui::SetWindowFontScale(1.0f);

    // Gold gradient divider (левый край ярче).
    auto menuGlow = [&]() {
      ImDrawList* gdl = ImGui::GetWindowDrawList();
      const ImVec2 gp = ImGui::GetCursorScreenPos();
      const float gw = ImGui::GetContentRegionAvail().x;
      gdl->AddRectFilledMultiColor(gp, ImVec2(gp.x + gw, gp.y + 1.0f),
                                   IM_COL32(255, 191, 71, 150), IM_COL32(255, 191, 71, 0),
                                   IM_COL32(255, 191, 71, 0), IM_COL32(255, 191, 71, 150));
      ImGui::Dummy(ImVec2(gw, 2.0f));
    };

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.19f, 0.05f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.42f, 0.29f, 0.08f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.50f, 0.34f, 0.09f, 1.00f));
    if (ImGui::Button(uiRu("Начать экспедицию", "Begin Expedition"),
                      ImVec2(-1.0f, 42.0f))) {
      // === Expedition === (v3.5.0): сюжетный режим. Первый запуск — вводные
      // слайды (человек на Земле → решение улететь), затем высадка в
      // Солнечной системе; повторные — сразу полёт.
      if (!m_expedition.introSeen && m_expedition.chapterIndex == 0) {
        startExpeditionIntro();
      } else {
        m_inMenu = false;
      }
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(m_expedition.introSeen ? "Resume your journey"
                                               : "Begin the expedition");
    }
    ImGui::PopStyleColor(3);

    // === Expedition === (v3.5.0): свободный полёт — без пролога и сюжета.
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.12f, 0.17f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.19f, 0.26f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.24f, 0.32f, 1.00f));
    if (ImGui::Button(uiRu("Свободный визит", "Free Visit"),
                      ImVec2(-1.0f, 32.0f))) {
      m_inMenu = false;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Free flight, no story scenes");
    }
    ImGui::PopStyleColor(3);

    menuGlow();
    ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.28f, 1.0f), "Destinations");
    ImGui::SetWindowFontScale(0.95f);
    {
      const int gridCols = 2;
      int gridCol = 0;
      const float cellW =
          (ImGui::GetContentRegionAvail().x -
           static_cast<float>(gridCols - 1) * ImGui::GetStyle().ItemSpacing.x) /
          static_cast<float>(gridCols);
      for (const auto& station : builtinCatalog()) {
        if (gridCol > 0) ImGui::SameLine();
        // Имя станции дублируется в "Recent" — уникальный ID через ##id
        // (иначе Dear ImGui: "2 visible items with conflicting ID").
        const std::string destLabel = station.name + "##dest-" + station.id;
        if (ImGui::Button(destLabel.c_str(), ImVec2(cellW, 0.0f))) {
          teleportToStation(station.id);
          m_inMenu = false;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s\n%s", station.id.c_str(),
                            station.description.c_str());
        }
        gridCol = (gridCol + 1) % gridCols;
      }
    }
    ImGui::SetWindowFontScale(1.0f);

    menuGlow();
    ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.28f, 1.0f), "Recent");
    ImGui::SetWindowFontScale(0.95f);
    {
      const int gridCols = 2;
      int gridCol = 0;
      const float cellW =
          (ImGui::GetContentRegionAvail().x -
           static_cast<float>(gridCols - 1) * ImGui::GetStyle().ItemSpacing.x) /
          static_cast<float>(gridCols);
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
          std::string recentLabel = rst ? rst->name : rid;
          // Уникальный ID: "Milky Way" есть и в Destinations выше (##recent-).
          recentLabel += "##recent-" + rid;
          if (gridCol > 0) ImGui::SameLine();
          if (ImGui::Button(recentLabel.c_str(), ImVec2(cellW, 0.0f))) {
            teleportToStation(rid);
            m_inMenu = false;
          }
          if (rst && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", rst->description.c_str());
          }
          gridCol = (gridCol + 1) % gridCols;
        }
      }
    }
    ImGui::SetWindowFontScale(1.0f);

    menuGlow();
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

    // === SaveGame === (v3.4.0): профили сохраняются между запусками.
    menuGlow();
    const float saveBtnW = (menuAvail - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    if (ImGui::Button("Save game", ImVec2(saveBtnW, 0.0f))) {
      saveProfileNow(false);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load game", ImVec2(saveBtnW, 0.0f))) {
      loadProfileNow(false);
    }
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "F9 save | F10 load | autosave on exit");
    if (!m_saveStatus.empty()) {
      ImGui::TextColored(ImVec4(0.55f, 0.75f, 0.55f, 1.0f), "%s",
                         m_saveStatus.c_str());
    }
    ImGui::SetWindowFontScale(1.0f);

    menuGlow();
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "WASD fly | drag look | / search | dbl-click go");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "P time | F1 help | H hide UI | ESC menu");
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Version %s", kVersion);
    ImGui::SetWindowFontScale(1.0f);
  } else if (m_menuPage == 1) {
    ImGui::Text("Settings");
    // === Settings v3.6.0 ===: страница выросла - скролл-область фиксированной
    // высоты, Back остаётся внизу панели. Харнесс снимает только страницу 0
    // (menu.png), эталон не затронут.
    ImGui::BeginChild("##settingsScroll", ImVec2(0.0f, 424.0f));
    RenderSettings& menuSettings = m_sceneManager->getSettings();
    bool menuChanged = false;

    ImGui::SeparatorText("Ray tracing (black holes)");
    const char* rayNames[] = {"Low", "Med", "Ultra", "Custom"};
    int rayPreset = static_cast<int>(m_sceneManager->getPreset());
    if (ImGui::Combo("Ray quality", &rayPreset, rayNames, 4)) {
      m_userOverrodePreset = true;
      m_benchmarking = false;
      if (rayPreset >= 0 && rayPreset <= 2) {
        m_sceneManager->applyPreset(static_cast<PerfPreset>(rayPreset));
      } else {
        m_sceneManager->setPreset(PerfPreset::Custom);
      }
    }
    if (ImGui::SliderFloat("Shell radius", &menuSettings.shellRadius, 10.0f,
                           60.0f, "%.0f")) {
      menuChanged = true;
    }
    if (ImGui::SliderFloat("Step size", &menuSettings.stepSize, 0.01f, 0.12f,
                           "%.3f")) {
      menuChanged = true;
    }
    if (ImGui::SliderInt("Max steps", &menuSettings.maxSteps, 100, 2000)) {
      menuChanged = true;
    }
    menuChanged |=
        ImGui::Checkbox("Ship reflections", &menuSettings.shipReflections);

    ImGui::SeparatorText("Anti-aliasing");
    const char* aaNames[] = {"Off", "FXAA"};
    if (ImGui::Combo("AA mode", &m_aaMode, aaNames, 2)) {
      m_sceneManager->setFxaa(m_aaMode == 1);
    }

    ImGui::SeparatorText("Ship");
    const char* hullNames[] = {"Dart", "Ring Ship", "Shuttle", "Model (OBJ)"};
    int hullSel = m_shipStyle;
    if (ImGui::Combo("Hull", &hullSel, hullNames, 4)) {
      setShipStyleNow(hullSel);
    }
    const char* engineNames[] = {"Red", "Ion", "Emerald", "Plasma"};
    ImGui::Combo("Engine", &menuSettings.enginePalette, engineNames, 4);

    ImGui::SeparatorText("Game");
    const char* diffNames[] = {"Easy", "Normal", "Hard"};
    ImGui::Combo("Difficulty", &m_difficulty, diffNames, 3);
    const DifficultyTuning menuTune = difficultyTuning(m_difficulty);
    ImGui::SetWindowFontScale(0.85f);
    ImGui::TextColored(
        ImVec4(0.6f, 0.62f, 0.68f, 1.0f),
        "Jump cost x%.1f, star refuel x%.1f, radiation x%.1f, hints %s",
        menuTune.jumpCostK, menuTune.starFuelK, menuTune.radiationK,
        menuTune.hints ? "on" : "off");
    ImGui::SetWindowFontScale(1.0f);

    // v3.6.0: страница Controls — мышь и переназначение клавиш. Захват
    // новой клавиши стартует кнопкой, ловится в начале drawImGui.
    ImGui::SeparatorText("Controls");
    ImGui::SliderFloat("Mouse sensitivity", &m_mouseSensitivity, 0.3f, 3.0f,
                       "%.2f");
    ImGui::Checkbox("Invert look Y", &m_invertY);
    {
      struct KeyRow {
        const char* label;
        int* key;
      };
      const KeyRow keyRows[5] = {{"Colony", &m_keyColony},
                                 {"Camera view", &m_keyView},
                                 {"Navigation", &m_keyMap},
                                 {"Journal", &m_keyJournal},
                                 {"Cine tour", &m_keyTour}};
      for (int i = 0; i < 5; ++i) {
        ImGui::TextUnformatted(keyRows[i].label);
        ImGui::SameLine(150.0f);
        const bool capturing = (m_captureKeyFor == i);
        const std::string keyLabel =
            keyNameForCode(*keyRows[i].key) +
            (capturing ? " <press a key>" : "");
        const std::string btnId =
            keyLabel + "##keybind" + std::to_string(i);
        if (ImGui::SmallButton(btnId.c_str())) {
          m_captureKeyFor = capturing ? -1 : i;
        }
      }
    }

    ImGui::SeparatorText("View and audio");
    menuChanged |= ImGui::SliderFloat("FOV", &menuSettings.fovDegrees, 30.0f,
                                      110.0f, "%.1f deg");
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
    if (ImGui::SliderInt("Volume", &m_audioVolume, 0, 100)) {
      m_audio.setVolume(static_cast<float>(m_audioVolume) / 100.0f);
    }
    bool menuMuted = m_audio.isMuted();
    if (ImGui::Checkbox("Mute", &menuMuted)) {
      m_audio.toggleMute();
    }
    if (menuChanged) {
      m_userOverrodePreset = true;
      m_benchmarking = false;
      m_sceneManager->setPreset(PerfPreset::Custom);
    }
    ImGui::EndChild();
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
  ImGui::PopStyleVar(4);
  ImGui::PopStyleColor(7);
}

void AppGL::drawWorldLabels() {
  if (!m_showLabels) return;
  if (m_slidesOpen) return;  // === Expedition ===: катсцена чистая, без меток
  const Station* curSt = m_sceneManager->getCurrentStation();
  if (!curSt) return;

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  if (!drawList) return;
  // Виды (v3.3.0): метки считаем по камере кадра (учёт сноса в видах 2/3).
  // В меню и в harнесс-снимках (камера кадра не обновлялась) — по m_camera.
  const Camera& cam =
      (!m_inMenu && m_renderCamValid) ? m_renderCam : m_camera;

  ImVec2 displaySize = ImGui::GetIO().DisplaySize;
  if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) return;

  ImFont* font = ImGui::GetFont();
  if (!font) return;

  // === Physics === begin: маркер центра ЧД/квазара рисуем ВСЕГДА
  // (без cull по дистанции/ndc — тело не должно "исчезать" вдали;
  // дальний импостор держит минимальный размер в шейдере).
  if (curSt->type == StationType::BlackHole ||
      curSt->type == StationType::Quasar) {
    const float aspectM = displaySize.x / displaySize.y;
    const float fovM = m_sceneManager->getSettings().fovDegrees;
    const glm::mat4 projM =
        glm::perspective(glm::radians(fovM), aspectM, 0.1f, 1e7f);
    const Vec3 cpM = cam.position;
    const Vec3 cfM = cam.forward();
    const Vec3 cuM = cam.up();
    const glm::vec3 eyeM(static_cast<float>(cpM.x), static_cast<float>(cpM.y),
                         static_cast<float>(cpM.z));
    const glm::vec3 fwdM(static_cast<float>(cfM.x), static_cast<float>(cfM.y),
                         static_cast<float>(cfM.z));
    const glm::vec3 upM(static_cast<float>(cuM.x), static_cast<float>(cuM.y),
                        static_cast<float>(cuM.z));
    const glm::mat4 mvpM = projM * glm::lookAt(eyeM, eyeM + fwdM, upM);
    const glm::vec4 clipM = mvpM * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (clipM.w > 0.001f) {
      glm::vec3 ndcM = glm::vec3(clipM) / clipM.w;
      ndcM.x = std::clamp(ndcM.x, -0.95f, 0.95f);
      ndcM.y = std::clamp(ndcM.y, -0.95f, 0.95f);
      const float sxM = (ndcM.x * 0.5f + 0.5f) * displaySize.x;
      const float syM = (1.0f - (ndcM.y * 0.5f + 0.5f)) * displaySize.y;
      drawList->AddCircle(ImVec2(sxM, syM), 5.0f,
                          IM_COL32(255, 220, 130, 230), 16, 2.0f);
      drawList->AddText(font, 14.0f, ImVec2(sxM + 9.0f, syM - 9.0f),
                        IM_COL32(255, 230, 170, 230), curSt->name.c_str());
    }
  }
  // === Physics === end

  if (curSt->type == StationType::Constellation) {
    const ConstellationInfo* cInfo = findConstellation(curSt->id);
    if (!cInfo) return;

    float aspect = displaySize.x / displaySize.y;
    float fov = m_sceneManager->getSettings().fovDegrees;
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1e7f);

    Vec3 cp = cam.position;
    Vec3 cf = cam.forward();
    Vec3 cu = cam.up();

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
      glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 1e7f);

      Vec3 cp = cam.position;
      Vec3 cf = cam.forward();
      Vec3 cu = cam.up();

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
    Vec3 cp = cam.position;
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

    // M31 cluster labels (m31 only; other galaxies have own heroes below):
    // fixed features, base offsets measured at zoom == 1
    // (fractions of screen center scaling). px = cx + bx*zoom*W*0.25.
    if (curSt->id == "m31") {
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
    }  // end m31 companions guard

    // Close range: hero stars as real 3D points (D < 10), every galaxy.
    // NGC 206 / G1 clouds already labeled above (kClusters), not duplicated.
    if (dist < 10.0) {
      float m31Aspect = displaySize.x / displaySize.y;
      float m31Fov = m_sceneManager->getSettings().fovDegrees;
      glm::mat4 m31Proj = glm::perspective(glm::radians(m31Fov), m31Aspect, 0.1f, 1e7f);
      glm::vec3 m31Eye(static_cast<float>(cp.x), static_cast<float>(cp.y),
                       static_cast<float>(cp.z));
      Vec3 m31Cf = cam.forward();
      Vec3 m31Cu = cam.up();
      glm::vec3 m31Fwd(static_cast<float>(m31Cf.x), static_cast<float>(m31Cf.y),
                       static_cast<float>(m31Cf.z));
      glm::vec3 m31Up(static_cast<float>(m31Cu.x), static_cast<float>(m31Cu.y),
                      static_cast<float>(m31Cu.z));
      glm::mat4 m31View = glm::lookAt(m31Eye, m31Eye + m31Fwd, m31Up);
      glm::mat4 m31Mvp = m31Proj * m31View;
      for (const auto& bs : m_sceneManager->galaxyBrightStars()) {
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
  s_harnessPinFps = true;
  std::filesystem::create_directories(outDir);

  // Create AppGL (with window 1280x720)
  AppGL app(1280, 720, false);

  const std::vector<std::string> targetStations = {
      "sgra", "qso-3c273", "ori", "uma", "m31", "m1", "sol", "m87", "m51", "m104",
      "psr-vela", "psr-crab", "sgr-1806", "psr-b1913", "mw", "m13"
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

  // === Expedition === (v3.5.0): gui.png — игровой кадр (не меню): видны
  // HUD-карточка главы, корабельный HUD и бары; состояние свежее => PNG
  // детерминирован, ловит регрессы игрового HUD байт-в-байт.
  app.m_inMenu = false;

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

  // === Expedition === (v3.5.0): intro.png — кадр катсцены (первый слайд
  // пролога поверх сцены Sgr A*). Детерминирован: без времени и ввода.
  app.startExpeditionIntro();
  for (int frame = 0; frame < 3; ++frame) {
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
  std::string introFile = outDir + "/intro.png";
  if (app.saveScreenshot(introFile)) {
    std::cout << "Captured screenshot: " << introFile << "\n";
  }
  // Пролог держим открытым только для кадра intro.png: следующие захваты
  // (close-up, editor) идут с игровым HUD, без оверлея слайдов.
  app.m_slidesOpen = false;
  app.m_slidesCurrent.clear();
  app.m_slidesIdx = 0;

  // === MenuSky === (v3.5.0): menu.png — hero-вид главного меню (живой
  // космос + стеклянная панель). u_Time фиксирован => кадр детерминирован,
  // PNG ловит регресс шейдера меню и стиля панели байт-в-байт.
  app.m_inMenu = true;
  if (!app.m_menuSkyTried) {
    app.m_menuSkyTried = true;
    app.m_menuSkyShader =
        std::make_unique<Shader>("shaders/quad.vert", "shaders/menu.frag");
    app.m_menuSkyQuad = std::make_unique<ScreenQuad>();
  }
  for (int frame = 0; frame < 3; ++frame) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    app.m_menuSkyShader->bind();
    app.m_menuSkyShader->setUniform1f("u_Time", 12.0f);
    app.m_menuSkyShader->setUniform2f("u_Res", static_cast<float>(fbWidth),
                                      static_cast<float>(fbHeight));
    app.m_menuSkyQuad->draw();
    app.m_menuSkyShader->unbind();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    app.drawMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  std::string menuFile = outDir + "/menu.png";
  if (app.saveScreenshot(menuFile)) {
    std::cout << "Captured screenshot: " << menuFile << "\n";
  }
  app.m_inMenu = false;

  // === Ship lighting pass: ship.png — герой-кадр корабля (PBR-lite + IBL
  // скайбокса + ключевой свет станции). Камера на дефолтной позиции sgra,
  // u_Time фиксирован => кадр детерминирован. HUD не рисуем.
  warpdrive::setShip(true, 1.28f, -0.98f, 5.95f, 2.35f);
  warpdrive::setThruster(0.45f);
  for (int frame = 0; frame < 4; ++frame) {
    app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  std::string shipFile = outDir + "/ship.png";
  if (app.saveScreenshot(shipFile)) {
    std::cout << "Captured screenshot: " << shipFile << "\n";
  }
  warpdrive::setShip(false, 0.0f, 0.0f, 0.0f, 0.0f);
  warpdrive::setThruster(0.7f);

  // === Ship model shot (агент M, v3.8.1): аддитивный герой-кадр стилей
  // Model (OBJ). Эталон ship.png не трогаем: отдельный файл ship_model.png,
  // стиль через warpdrive::setShipStyle, после кадра — откат.
  {
    const int styleBackup = app.m_shipStyle;
    warpdrive::setShipStyle(3);
    warpdrive::setShip(true, 1.28f, -0.98f, 5.95f, 2.35f);
    warpdrive::setThruster(0.45f);
    for (int frame = 0; frame < 6; ++frame) {
      app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      app.m_window->swapBuffers();
      app.m_window->pollEvents();
    }
    std::string shipModelFile = outDir + "/ship_model.png";
    if (app.saveScreenshot(shipModelFile)) {
      std::cout << "Captured screenshot: " << shipModelFile << "\n";
    }
    // Close-up того же корпуса: камера смотрит в -Z, ставим корабль в 3.5 ед.
    // перед ней — читаются карты PBR-набора (панели, нормали, металл).
    {
      const Vec3 cam = app.m_camera.position;
      warpdrive::setShip(true, static_cast<float>(cam.x) + 0.52f,
                         static_cast<float>(cam.y) - 0.40f,
                         static_cast<float>(cam.z) - 3.6f, 2.35f);
      for (int frame = 0; frame < 6; ++frame) {
        app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        app.m_window->swapBuffers();
        app.m_window->pollEvents();
      }
      std::string closeFile = outDir + "/ship_model_close.png";
      if (app.saveScreenshot(closeFile)) {
        std::cout << "Captured screenshot: " << closeFile << "\n";
      }
    }
    // Ring Ship skin: тот же близкий ракурс, но процедурный корпус с
    // панельной «обшивкой» (u_SkinOn) — отдельный кадр ship_ring.png.
    {
      warpdrive::setShipStyle(1);
      const Vec3 cam = app.m_camera.position;
      warpdrive::setShip(true, static_cast<float>(cam.x) + 0.52f,
                         static_cast<float>(cam.y) - 0.40f,
                         static_cast<float>(cam.z) - 3.6f, 2.35f);
      for (int frame = 0; frame < 6; ++frame) {
        app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        app.m_window->swapBuffers();
        app.m_window->pollEvents();
      }
      std::string ringFile = outDir + "/ship_ring.png";
      if (app.saveScreenshot(ringFile)) {
        std::cout << "Captured screenshot: " << ringFile << "\n";
      }
    }
    warpdrive::setShip(false, 0.0f, 0.0f, 0.0f, 0.0f);
    warpdrive::setShipStyle(app.m_shipStyle);
    warpdrive::setThruster(0.7f);
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

  // 5./6. Solar System close-ups (v3.2.0): Saturn (ring shadow on the
  // planet, planet eclipse on the rings, Titan) and Earth (ocean glint).
  // Positions are read back from the running system: the harness never
  // advances sim time, so bodies sit exactly at their angle0 literals and
  // the framings stay deterministic.
  app.m_sceneManager->teleportTo("sol", app.m_camera);
  for (int frame = 0; frame < 2; ++frame) {
    app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
    app.m_window->swapBuffers();
    app.m_window->pollEvents();
  }
  Vec3 satP, earP;
  for (const auto& body : app.m_sceneManager->getBodies()) {
    if (!body.def) continue;
    if (body.def->id == "saturn") satP = body.pos;
    if (body.def->id == "earth") earP = body.pos;
  }
  // Aim helper: yaw/pitch = inverse of Camera::forward() (see the
  // screenToWorld-style inverse above: forward = (-sin y cos p, sin p,
  // -cos y cos p)).
  auto aimAt = [](Camera& cam, const Vec3& target) {
    Vec3 d(target.x - cam.position.x, target.y - cam.position.y,
           target.z - cam.position.z);
    const double dl = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (dl < 1e-9) return;
    d.x /= dl;
    d.y /= dl;
    d.z /= dl;
    cam.yaw = std::atan2(-d.x, -d.z);
    cam.pitch = std::asin(d.y);
  };
  if (satP.x != 0.0 || satP.z != 0.0) {
    // Hover 5 units from Saturn on a 3/4-phase view: the sunward direction
    // rotated 1.0 rad about Y. A head-on sunward view (v3.2.0 draft) hides
    // the ring shadow on the planet and the planet eclipse on the rings
    // behind the limb; at ~60 deg phase both read clearly.
    const double a = std::atan2(satP.z, satP.x) + 3.141592653589793 + 1.0;
    app.m_camera.position = Vec3(satP.x + 5.0 * std::cos(a),
                                 satP.y + 2.4,
                                 satP.z + 5.0 * std::sin(a));
    aimAt(app.m_camera, satP);
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
    std::string solCloseFile = outDir + "/sol_close.png";
    if (app.saveScreenshot(solCloseFile)) {
      std::cout << "Captured screenshot: " << solCloseFile << "\n";
    }
  }
  if (earP.x != 0.0 || earP.z != 0.0) {
    // Hover 2.6 units sunward of Earth, just above the equator (the sun
    // sits at the origin, so the day side + ocean glint face the camera).
    const double k = 1.0 - 2.6 / 14.0;
    app.m_camera.position = Vec3(earP.x * k, earP.y * k + 0.6, earP.z * k);
    aimAt(app.m_camera, earP);
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
    std::string earthCloseFile = outDir + "/earth_close.png";
    if (app.saveScreenshot(earthCloseFile)) {
      std::cout << "Captured screenshot: " << earthCloseFile << "\n";
    }
  }
  // 7. Public README shots (v3.5.0): the gate references above are captured
  //    with the disk off (the doppler gate subtracts them from the disk-on
  //    run), so the accretion disk never appears on docs/screens/*.png. The
  //    pub set renders the same stations with the disk ON and the camera
  //    lifted above the disk plane (the classic lensed-disk framing).
  //    Committed to docs/screens/pub/ and used by the README gallery.
  {
    app.m_sceneManager->getSettings().diskOn = true;
    struct PubShot {
      const char* id;
      double spawn;  // camera distance (Rs units for BH stations)
      double elev;   // elevation angle above the disk plane (rad)
    };
    const std::vector<PubShot> pubShots = {
        {"sgra", 10.0, 0.22},
        {"m87", 14.0, 0.20},
    };
    for (const auto& shot : pubShots) {
      app.m_sceneManager->teleportTo(shot.id, app.m_camera);
      app.m_camera.position = Vec3(0.0, shot.spawn * std::sin(shot.elev),
                                   shot.spawn * std::cos(shot.elev));
      aimAt(app.m_camera, Vec3(0.0, 0.0, 0.0));
      for (int frame = 0; frame < 5; ++frame) {
        app.m_sceneManager->render(app.m_camera, fbWidth, fbHeight, 1.0f);
        app.m_window->swapBuffers();
        app.m_window->pollEvents();
      }
      std::string outFile =
          outDir + "/pub/" + std::string(shot.id) + ".png";
      if (app.saveScreenshot(outFile)) {
        std::cout << "Captured pub screenshot: " << outFile << "\n";
      }
    }
  }
  // 7. Editor demo (v3.2.0 helper frame, transient like *_close.png): spawns
  // a ringed gas giant, a star and a moon through the same spawnCustom path
  // the editor UI uses, captures the trio, then clears them. Placed at
  // x ~ 20, z ~ 0 so the sun (world origin) lights the visible sides from
  // behind the camera. Deterministic: zero velocities, fixed seeds.
  {
    CustomBody gas;
    gas.type = 1;
    gas.detail = 7;
    gas.radius = 1.2f;
    gas.color = Vec3(0.78, 0.66, 0.48);
    gas.colorB = Vec3(0.55, 0.40, 0.28);
    gas.ringOuter = 2.4f;
    gas.seed = 3.0f;
    app.m_sceneManager->spawnCustom(gas, Vec3(24.0, 0.4, 0.8), Vec3(0.0, 0.0, 0.0));
    CustomBody star;
    star.type = 0;
    star.detail = 1;
    star.radius = 1.0f;
    star.color = Vec3(1.0, 0.85, 0.55);
    star.colorB = Vec3(0.9, 0.3, 0.1);
    star.seed = 1.0f;
    app.m_sceneManager->spawnCustom(star, Vec3(21.5, 1.6, -2.2), Vec3(0.0, 0.0, 0.0));
    CustomBody moon;
    moon.type = 2;
    moon.detail = 5;
    moon.radius = 0.45f;
    moon.color = Vec3(0.62, 0.62, 0.63);
    moon.colorB = Vec3(0.30, 0.30, 0.32);
    moon.seed = 7.0f;
    app.m_sceneManager->spawnCustom(moon, Vec3(20.5, -0.2, 1.8), Vec3(0.0, 0.0, 0.0));
    // Raised vantage (~25 deg above the ring plane): a horizontal look from
    // the sun side would be edge-on to the 0.45-rad tilted ring.
    app.m_camera.position = Vec3(13.0, 7.0, -2.0);
    aimAt(app.m_camera, Vec3(23.0, 0.2, 0.5));
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
    std::string editorFile = outDir + "/editor_demo.png";
    if (app.saveScreenshot(editorFile)) {
      std::cout << "Captured screenshot: " << editorFile << "\n";
    }
    app.m_sceneManager->clearCustom();
  }

  return 0;
}

// === WarpDrive === begin (агент W: режим путешествий на варп-двигателе).
// Отдельный режим: W вкл/выкл, фактор 1-9 (слайдер + [ ]), скорость
// логарифмическая (показушные св.лет/с), статусы Cruise/Charging/Jump/
// Cooldown. Смена фактора — ease in-out 1.5с. Стрики/виньетка/кайма и
// вспышка — только своим warp.frag-оверлеем (u_WarpK/u_Flash, дефолт 0):
// sky.frag чужой, m_exposure не трогаем. Прыжок = туннель 4с поверх +
// штатный телепорт под вспышкой (честной смены координат вселенной нет).
// Скриншот-блок выше не тронут: варп там выключен, PNG байт-в-байт.
double AppGL::warpSpeedLY(int factor) {
  if (factor < 1) factor = 1;
  if (factor > 9) factor = 9;
  return std::pow(10.0, (static_cast<double>(factor) - 1.0) * 0.75);
}

float AppGL::warpTargetK(int factor) {
  if (factor < 1) factor = 1;
  if (factor > 9) factor = 9;
  return 0.15f + 0.85f * (static_cast<float>(factor - 1) / 8.0f);
}

const char* AppGL::warpStatusName(int status) {
  switch (status) {
    case 1: return "Cruise";
    case 2: return "Charging";
    case 3: return "Jump";
    case 4: return "Cooldown";
    default: return "Off";
  }
}

void AppGL::warpRetarget(float to) {
  m_warpKFrom = m_warpK;
  m_warpKTo = to;
  m_warpKAnim = 0.0f;
}

void AppGL::disengageWarp() {
  m_warpOn = false;
  m_warpStatus = 0;
  m_warpJumpTarget.clear();
  warpRetarget(0.0f);
  m_warpFlash = 0.35f;  // v3.6.0: мягкая вспышка выхода (комфорт для глаз)
}

void AppGL::setShipStyleNow(int style) {
  m_shipStyle = std::min(std::max(style, 0), 3);
  warpdrive::setShipStyle(m_shipStyle);
  // Shuttle без варп-двигателя: если варп был активен (Cruise/Cooldown),
  // гасим его; посреди катсцены не трогаем (досматриваем).
  if (!ship::styleCanWarp(m_shipStyle) && m_warpOn && !m_jumpCine &&
      (m_warpStatus == 1 || m_warpStatus == 4)) {
    disengageWarp();
  }
}

void AppGL::startWarpJump(const std::string& id) {
  const Station* cur = m_sceneManager->getCurrentStation();
  if (!m_warpOn || m_warpStatus != 1) return;  // только из Cruise
  if (id.empty() || (cur && cur->id == id)) return;
  if (!findStation(id)) return;
  // v3.6.0: варп-роли корпусов — без варп-двигателя прыжок не летит.
  if (!ship::styleCanWarp(m_shipStyle)) {
    m_shipNotice = uiRu("У этого корпуса нет варп-двигателя (Shuttle)",
                        "This hull has no warp drive (Shuttle)");
    m_shipNoticeT = 3.0f;
    return;
  }
  // === Ship survival === (v3.3.0): без пяти единиц топлива прыжок не летит.
  // v3.6.0: цена зависит от сложности (Hard x1.6, Easy x0.6).
  const DifficultyTuning jtune = difficultyTuning(m_difficulty);
  if (!ship::canJump(m_ship, jtune.jumpCostK)) {
    char need[64];
    std::snprintf(need, sizeof(need),
                  uiRu("Недостаточно топлива (нужно %.0f)",
                       "Not enough fuel (%.0f needed)"),
                  static_cast<double>(ship::kJumpFuelCost * jtune.jumpCostK));
    m_shipNotice = need;
    m_shipNoticeT = 3.0f;
    return;
  }
  // === WarpJump === (агент J): прыжок идёт через кино-контроллер поверх
  // статусов (Charge->Pullback->Burst->Travel->Arrival->Pushin->Cooldown).
  // Старый прямой путь Charging(3с)/Jump(4с) ниже остаётся fallback'ом
  // (статический чек 'warp jump machine 3-4s' на него опирается).
  m_jumpTarget = id;
  m_jumpDidTeleport = false;
  m_jumpBurst = 0.0f;
  m_jumpShipPos = m_camera.position;
  m_jumpShipDir = m_camera.forward();
  m_jumpFpPos = m_camera.position;
  m_jumpFpYaw = m_camera.yaw;
  m_jumpFpPitch = m_camera.pitch;
  m_jumpCamPos = m_camera.position;
  m_jumpCamVel = Vec3(0.0, 0.0, 0.0);
  // v3.6.0 (пасс D): маршруливание астероидного поля — детерминированный
  // ролл от локального генератора; логика полосы живёт в WarpJump.
  static std::mt19937 s_astRng(std::random_device{}());
  static std::uniform_real_distribution<float> s_astRoll01(0.0f, 1.0f);
  m_jump.start(id, s_astRoll01(s_astRng));
  m_jumpPrevPhase = JumpPhase::Idle;
  m_jumpCine = true;
}

bool AppGL::updateWarp(float deltaTime) {
  // === WarpJump === (агент J): пока катсцена активна, управление у нового
  // контроллера; статусы 2/3/4 он маппит сам (UI/аудио не ломаются).
  if (m_jumpCine) return updateJumpCine(deltaTime);
  // === WarpJump === end
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();

  // Ключи (фронты, как остальные хоткеи; при фокусе ввода ImGui молчим).
  // W: вкл/выкл. ВНИМАНИЕ: W также держит движение вперёд (чужой блок
  // ниже): короткое нажатие включает варп, повторное — выключает.
  // [ / ]: фактор 1-9 (в туннеле прыжка фактор растёт сам, ключи глушим).
  if (!io.WantCaptureKeyboard) {
    const bool xDown = (glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
    if (xDown && !m_prevXPressed) {
      if (!m_warpOn) {
        m_warpOn = true;
        m_warpStatus = 1;  // Cruise
        m_warpBaseFov = m_sceneManager->getSettings().fovDegrees;
        m_warpLastFov = m_warpBaseFov;
        m_warpFovDirty = true;
        warpRetarget(warpTargetK(m_warpFactor));
        m_warpFlash = 0.4f;  // v3.6.0: мягкая вспышка входа (комфорт для глаз)
      } else if (m_warpStatus == 1 || m_warpStatus == 4) {
        disengageWarp();  // в Charging/Jump игнор — катсцену досматриваем
      }
    }
    m_prevXPressed = xDown;

    const bool lbDown = (glfwGetKey(win, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
    const bool rbDown = (glfwGetKey(win, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
    if (m_warpOn && m_warpStatus != 3) {
      if (lbDown && !m_prevLBracketPressed && m_warpFactor > 1) {
        --m_warpFactor;
        warpRetarget(warpTargetK(m_warpFactor));
      }
      if (rbDown && !m_prevRBracketPressed && m_warpFactor < 9) {
        ++m_warpFactor;
        warpRetarget(warpTargetK(m_warpFactor));
      }
    }
    m_prevLBracketPressed = lbDown;
    m_prevRBracketPressed = rbDown;
  }

  // Плавная кривая 1.5с (ease in-out) к целевой интенсивности.
  if (m_warpKAnim < 1.0f) {
    m_warpKAnim += deltaTime / 1.5f;
    if (m_warpKAnim > 1.0f) m_warpKAnim = 1.0f;
    const float e = m_warpKAnim * m_warpKAnim * (3.0f - 2.0f * m_warpKAnim);
    m_warpK = m_warpKFrom + (m_warpKTo - m_warpKFrom) * e;
  } else {
    m_warpK = m_warpKTo;
  }

  // Машина состояний прыжка.
  if (m_warpOn) {
    if (m_warpStatus == 2) {  // Charging: обратный отсчёт 3-2-1
      m_warpStateT += deltaTime;
      if (m_warpStateT >= 3.0f) {
        m_warpStatus = 3;  // Jump
        m_warpStateT = 0.0f;
        m_warpJumpFromFactor = m_warpFactor;
        warpRetarget(1.0f);  // туннель на максимуме
      }
    } else if (m_warpStatus == 3) {  // Jump: туннель 4с, фактор растёт сам
      m_warpStateT += deltaTime;
      float t = m_warpStateT / 4.0f;
      if (t < 0.0f) t = 0.0f;
      if (t > 1.0f) t = 1.0f;
      m_warpFactor = m_warpJumpFromFactor +
                     static_cast<int>(t * (9 - m_warpJumpFromFactor) + 0.5f);
      if (m_warpStateT >= 4.0f) {
        const std::string tgt = m_warpJumpTarget;
        m_warpJumpTarget.clear();
        teleportToStation(tgt);  // штатный телепорт под вспышкой
        ship::spendJumpFuel(m_ship, difficultyTuning(m_difficulty).jumpCostK);
        m_warpStatus = 4;        // Cooldown
        m_warpStateT = 0.0f;
        m_warpFlash = 0.5f;  // v3.6.0: мягкая вспышка прибытия (комфорт для глаз)
        warpRetarget(warpTargetK(m_warpFactor));
      }
    } else if (m_warpStatus == 4) {  // Cooldown 2с
      m_warpStateT += deltaTime;
      if (m_warpStateT >= 2.0f) {
        m_warpStatus = 1;  // Cruise
        m_warpStateT = 0.0f;
      }
    }
  }

  // Затухание вспышки.
  if (m_warpFlash > 0.0f) {
    m_warpFlash -= deltaTime * 0.9f;
    if (m_warpFlash < 0.0f) m_warpFlash = 0.0f;
  }

  // FOV +10deg*K. Чужой слайдер FOV не глушим: внешнюю правку считаем
  // новой базой (иначе слайдер казался бы залипшим в варпе).
  RenderSettings& st = m_sceneManager->getSettings();
  if (m_warpOn || m_warpK > 0.001f) {
    if (st.fovDegrees != m_warpLastFov) {
      m_warpBaseFov = st.fovDegrees - 10.0f * m_warpK;
    }
    float eff = m_warpBaseFov + 10.0f * m_warpK;
    if (eff < 30.0f) eff = 30.0f;
    if (eff > 110.0f) eff = 110.0f;
    st.fovDegrees = eff;
    m_warpLastFov = eff;
    m_warpFovDirty = true;
  } else if (m_warpFovDirty) {
    st.fovDegrees = m_warpBaseFov;
    m_warpLastFov = m_warpBaseFov;
    m_warpFovDirty = false;
  }

  // Крейсерский дрейф вперёд (в катсцене стоим: прыжок — это оверлей,
  // не разгон сквозь станцию). Сейфти-клампы ниже в run() применяются.
  if (m_warpOn && m_warpStatus == 1) {
    const Vec3 f = m_camera.forward();
    const double sp = 0.35 * static_cast<double>(m_warpFactor);
    m_camera.position.x += f.x * sp * static_cast<double>(deltaTime);
    m_camera.position.y += f.y * sp * static_cast<double>(deltaTime);
    m_camera.position.z += f.z * sp * static_cast<double>(deltaTime);
  }

  // Лёгкая тряска камеры ~K (снимается в finishWarpFrame после render).
  m_warpShakePhase += deltaTime * (18.0f + 30.0f * m_warpK);
  const float amp = 0.0016f * m_warpK;
  m_warpShakeYaw = amp * std::sin(m_warpShakePhase);
  m_warpShakePitch = amp * 0.7f * std::sin(m_warpShakePhase * 1.37f + 1.3f);
  m_camera.yaw += m_warpShakeYaw;
  m_camera.pitch += m_warpShakePitch;

  // Состояние оверлея для SceneManager (дефолт 0 — харнесс чистый).
  warpdrive::setState(m_warpK, m_warpFlash);

  return (m_warpOn && (m_warpStatus == 2 || m_warpStatus == 3));
}

void AppGL::finishWarpFrame() {
  m_camera.yaw -= m_warpShakeYaw;
  m_camera.pitch -= m_warpShakePitch;
  m_warpShakeYaw = 0.0f;
  m_warpShakePitch = 0.0f;
}

// === WarpJump === begin (агент J: кинематографичный варп-прыжок).
// Sequence: Charge(1с) -> Pullback FP→TP(1.2с) -> Burst(0.5с, flash-пик +
// телепорт) -> Travel(3.5с) -> Arrival(0.4с) -> Pushin TP→FP(1.2с) ->
// Cooldown(1.5с). Статусы 2/3/4 маппятся на старые значения, поэтому
// drawWarpPanel и аудио-polling (playJump-фронт на входе в 3, setWarpLevel
// через m_warpK) работают без правок: playJump звучит ровно в пик Burst
// (статус 3 выставляется вместе с switchStation), rumble гаснет в Arrival
// (m_warpK идёт к крейсерскому через обычный ease). Прямых вызовов
// m_audio.* здесь нет, чтобы не двоить голоса (Jump длится 1.6с).
void AppGL::abortJumpCine() {
  m_jump.stop();
  m_jumpCine = false;
  m_jumpPrevPhase = JumpPhase::Idle;
  m_jumpBurst = 0.0f;
  m_warpFlash = 0.0f;
  m_camera.position = m_jumpFpPos;
  m_camera.yaw = m_jumpFpYaw;
  m_camera.pitch = m_jumpFpPitch;
  m_warpStatus = 1;  // назад в Cruise
  warpRetarget(warpTargetK(m_warpFactor));
  warpdrive::setBurst(0.0f);
  warpdrive::setShip(false, 0.0f, 0.0f, 0.0f, 0.0f);
  warpdrive::setAsteroids(0.0f, 0.0f);  // v3.6.0: прерванный прыжок — поле off
}

bool AppGL::updateJumpCine(float deltaTime) {
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();

  // Фронты W/[ ] за время катсцены поглощаем (не залипать после неё);
  // ESC — прерывание всегда в Idle (как Tour::stop). Фронт съедаем,
  // меню после abort не открываем.
  if (!io.WantCaptureKeyboard) {
    m_prevWPressed = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
    m_prevXPressed = (glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
    m_prevLBracketPressed = (glfwGetKey(win, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
    m_prevRBracketPressed = (glfwGetKey(win, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
    const bool escDown = (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS);
    if (escDown && !m_prevEscPressed) {
      m_prevEscPressed = true;
      abortJumpCine();
      return false;
    }
    m_prevEscPressed = escDown;
  }

  // v3.6.0 (пасс D): ось уклонения A/D во время Travel. Копится на уровне
  // контроллера; lane сглаживается для сдвига полосы обломков в сцене.
  if (!io.WantCaptureKeyboard && m_jump.phase() == JumpPhase::Travel) {
    const float dodgeAxis =
        (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS ? 1.0f : 0.0f) -
        (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS ? 1.0f : 0.0f);
    m_jump.setDodgeAxis(dodgeAxis);
    const float laneK = 1.0f - std::exp(-static_cast<double>(deltaTime) * 6.0);
    m_astLane += (dodgeAxis - m_astLane) * laneK;
  } else if (m_astLane != 0.0f) {
    m_astLane *= 1.0f - std::exp(-static_cast<double>(deltaTime) * 6.0);
    if (std::fabs(m_astLane) < 0.01f) m_astLane = 0.0f;
  }

  JumpEvent jumpEv = m_jump.update(deltaTime);
  const JumpPhase ph = m_jump.phase();
  const float t01 = m_jump.phaseT01();
  const bool entered = (ph != m_jumpPrevPhase);
  m_jumpPrevPhase = ph;

  // v3.6.0 (пасс D): тосты и урон щита от событий астероидного поля.
  if (jumpEv.asteroidEnter) {
    m_shipNotice =
        uiRu("Обломки по курсу: удерживайте D (или A) для манёвра",
             "Debris field ahead: hold D (or A) to dodge");
    m_shipNoticeT = 4.5f;
  }
  if (jumpEv.asteroidDodged) {
    m_shipNotice =
        uiRu("Манёвр выполнен: полоса обломков пройдена",
             "Dodge complete: debris field cleared");
    m_shipNoticeT = 3.0f;
    // v3.7.0: жизненный счётчик + ачивка debris_dancer.
    m_asteroidDodges++;
    checkAchievements();
    saveProfileNow(false);
  }
  if (jumpEv.asteroidHit) {
    // v3.8.0: урон обломков масштабируется сложностью (Easy мягче, Hard злее).
    const float damage =
        WarpJump::kAstDamage * difficultyTuning(m_difficulty).hazardK;
    const float shieldBefore = m_ship.shield;
    m_ship.shield = std::max(0.0f, m_ship.shield - damage);
    m_shieldFlashK = ship::shieldFlashHit(m_shieldFlashK,
                                          shieldBefore - m_ship.shield);
    char msg[96];
    std::snprintf(msg, sizeof(msg),
                  uiRu("Столкновение с обломками: щит %d%%",
                       "Debris impact: shield %d%%"),
                  static_cast<int>(m_ship.shield));
    m_shipNotice = msg;
    m_shipNoticeT = 3.5f;
  }

  const float cruiseK = warpTargetK(m_warpFactor);
  bool shipVisible = false;

  switch (ph) {
    case JumpPhase::Idle:
      break;  // ниже: финиш (finished уже выдан контроллером)
    case JumpPhase::Charge: {
      m_warpStatus = 2;
      break;  // FP стоит (ввод заморожен consumed), K крейсерский
    }
    case JumpPhase::Pullback: {
      m_warpStatus = 2;
      if (entered) {
        // TP-риг: корабль = FP на старте отъезда.
        m_jumpShipPos = m_camera.position;
        m_jumpShipDir = m_camera.forward();
        m_jumpFpPos = m_camera.position;
        m_jumpFpYaw = m_camera.yaw;
        m_jumpFpPitch = m_camera.pitch;
        m_jumpCamPos = m_camera.position;
        m_jumpCamVel = Vec3(0.0, 0.0, 0.0);
        warpRetarget(1.0f);  // туннель нарастает к максимуму
      }
      shipVisible = true;
      break;
    }
    case JumpPhase::Burst: {
      if (entered) warpRetarget(1.0f);  // держать максимум
      // Статус 3 — только со второй половины (в пик, вместе с телепортом):
      // polling-фронт audioUpdate сыграет playJump ровно в пик.
      if (t01 >= 0.5f) m_warpStatus = 3;
      m_jumpBurst = t01;
      m_warpFlash = WarpJump::burstEnv(t01);  // прямая запись, пик 1
      shipVisible = true;
      break;
    }
    case JumpPhase::Travel: {
      m_warpStatus = 3;
      if (entered) warpRetarget(1.0f);
      m_warpK = 1.0f;
      m_warpKTo = 1.0f;
      m_jumpBurst = 1.0f * WarpJump::travelK(t01);  // ровная полка 1
      shipVisible = true;
      break;
    }
    case JumpPhase::Arrival: {
      m_warpStatus = 3;
      if (entered) warpRetarget(cruiseK);  // rumble гаснет через ease+polling
      m_jumpBurst = 1.0f - t01;
      shipVisible = true;
      break;
    }
    case JumpPhase::Pushin: {
      m_warpStatus = 4;
      if (entered) {
        m_jumpPushinPos = m_jumpCamPos;
        m_jumpPushinYaw = m_camera.yaw;
        m_jumpPushinPitch = m_camera.pitch;
      }
      shipVisible = true;
      break;
    }
    case JumpPhase::Cooldown: {
      m_warpStatus = 4;
      if (entered) {
        // Точное FP: интерполяция сошлась, фиксируем без дрейфа.
        m_camera.position = m_jumpFpPos;
        m_camera.yaw = m_jumpFpYaw;
        m_camera.pitch = m_jumpFpPitch;
        m_jumpCamPos = m_jumpFpPos;
        m_jumpCamVel = Vec3(0.0, 0.0, 0.0);
        m_jumpBurst = 0.0f;
      }
      break;
    }
  }

  // Пик Burst: штатный телепорт под белым flash=1 + rebase рига в новую
  // систему (скачок прячет вспышка). Статус 3 здесь же — playJump-фронт.
  if (jumpEv.switchStation && !m_jumpDidTeleport) {
    m_jumpDidTeleport = true;
    m_warpStatus = 3;
    teleportToStation(m_jumpTarget, false);  // v3.8.0: свой телепорт внутри катсцены
    ship::spendJumpFuel(m_ship, difficultyTuning(m_difficulty).jumpCostK);
    m_jumpShipPos = m_camera.position;
    m_jumpShipDir = m_camera.forward();
    m_jumpFpPos = m_camera.position;
    m_jumpFpYaw = m_camera.yaw;
    m_jumpFpPitch = m_camera.pitch;
    // Камера сразу за кораблём новой станции (D=6, H=1.2), пружина с нуля.
    {
      const float D = 6.0f, H = 1.2f;
      m_jumpCamPos = Vec3(m_jumpShipPos.x - m_jumpShipDir.x * D,
                          m_jumpShipPos.y - m_jumpShipDir.y * D + H,
                          m_jumpShipPos.z - m_jumpShipDir.z * D);
      m_jumpCamVel = Vec3(0.0, 0.0, 0.0);
      m_camera.position = m_jumpCamPos;
      computeLookYawPitch(m_jumpCamPos, m_jumpShipPos, m_camera.yaw,
                          m_camera.pitch);
    }
  }

  // TP-камера без рефакторинга Camera: пружина тянет позицию к desired,
  // взгляд — computeLookYawPitch на корабль. Pullback: D 2.5->6 по pullK;
  // Burst/Travel/Arrival: D=6 висеть. Pushin: обратная интерполяция к FP.
  if (ph == JumpPhase::Pullback || ph == JumpPhase::Burst ||
      ph == JumpPhase::Travel || ph == JumpPhase::Arrival) {
    float pullD = 6.0f;
    if (ph == JumpPhase::Pullback) {
      pullD = 2.5f + 3.5f * WarpJump::pullK(t01);
      // Коллизия: на ЧД-станциях камере нельзя ближе rs*6 к центру.
      const Station* jst = m_sceneManager->getCurrentStation();
      if (jst && jst->type == StationType::BlackHole) {
        double rs = static_cast<double>(m_sceneManager->getSettings().rs);
        if (rs < 0.1) rs = 1.0;
        const double minR = rs * 6.0;
        while (pullD > 1.0f) {
          const double dx = m_jumpShipPos.x - m_jumpShipDir.x * pullD;
          const double dy = m_jumpShipPos.y - m_jumpShipDir.y * pullD + 1.2;
          const double dz = m_jumpShipPos.z - m_jumpShipDir.z * pullD;
          if (std::sqrt(dx * dx + dy * dy + dz * dz) >= minR) break;
          pullD -= 0.25f;
        }
      }
    }
    const float H = 1.2f;
    const Vec3 des(m_jumpShipPos.x - m_jumpShipDir.x * pullD,
                   m_jumpShipPos.y - m_jumpShipDir.y * pullD + H,
                   m_jumpShipPos.z - m_jumpShipDir.z * pullD);
    // Critically-damped пружина k=8 (w=8, c=2w); dt кламп 1/30.
    float sdt = deltaTime;
    if (sdt < 0.0f) sdt = 0.0f;
    if (sdt > 1.0f / 30.0f) sdt = 1.0f / 30.0f;
    const float w = 8.0f;
    const Vec3 acc(w * w * (des.x - m_jumpCamPos.x) - 2.0f * w * m_jumpCamVel.x,
                   w * w * (des.y - m_jumpCamPos.y) - 2.0f * w * m_jumpCamVel.y,
                   w * w * (des.z - m_jumpCamPos.z) - 2.0f * w * m_jumpCamVel.z);
    m_jumpCamVel = Vec3(m_jumpCamVel.x + acc.x * sdt,
                        m_jumpCamVel.y + acc.y * sdt,
                        m_jumpCamVel.z + acc.z * sdt);
    m_jumpCamPos = Vec3(m_jumpCamPos.x + m_jumpCamVel.x * sdt,
                        m_jumpCamPos.y + m_jumpCamVel.y * sdt,
                        m_jumpCamPos.z + m_jumpCamVel.z * sdt);
    m_camera.position = m_jumpCamPos;
    computeLookYawPitch(m_jumpCamPos, m_jumpShipPos, m_camera.yaw,
                        m_camera.pitch);
  } else if (ph == JumpPhase::Pushin) {
    const float e = WarpJump::pushK(t01);
    m_camera.position = Vec3(m_jumpPushinPos.x + (m_jumpFpPos.x - m_jumpPushinPos.x) * e,
                             m_jumpPushinPos.y + (m_jumpFpPos.y - m_jumpPushinPos.y) * e,
                             m_jumpPushinPos.z + (m_jumpFpPos.z - m_jumpPushinPos.z) * e);
    const double kPi = 3.14159265358979323846;
    double yawDelta = m_jumpFpYaw - m_jumpPushinYaw;
    while (yawDelta > kPi) yawDelta -= 2.0 * kPi;
    while (yawDelta < -kPi) yawDelta += 2.0 * kPi;
    m_camera.yaw = m_jumpPushinYaw + yawDelta * e;
    m_camera.pitch = m_jumpPushinPitch + (m_jumpFpPitch - m_jumpPushinPitch) * e;
    m_jumpCamPos = m_camera.position;
  }

  // Плавная кривая m_warpK (ease in-out, как в обычном updateWarp).
  if (m_warpKAnim < 1.0f) {
    m_warpKAnim += deltaTime / 1.5f;
    if (m_warpKAnim > 1.0f) m_warpKAnim = 1.0f;
    const float e = m_warpKAnim * m_warpKAnim * (3.0f - 2.0f * m_warpKAnim);
    m_warpK = m_warpKFrom + (m_warpKTo - m_warpKFrom) * e;
  } else {
    m_warpK = m_warpKTo;
  }
  if (ph == JumpPhase::Travel) m_warpK = 1.0f;  // полка без просадки ease

  // Затухание вспышки (в Burst — прямая запись конверта выше).
  if (ph != JumpPhase::Burst && m_warpFlash > 0.0f) {
    m_warpFlash -= deltaTime * 0.9f;
    if (m_warpFlash < 0.0f) m_warpFlash = 0.0f;
  }

  // FOV +10deg*K (та же формула, что в обычном updateWarp).
  RenderSettings& jst = m_sceneManager->getSettings();
  if (m_warpOn || m_warpK > 0.001f) {
    if (jst.fovDegrees != m_warpLastFov) {
      m_warpBaseFov = jst.fovDegrees - 10.0f * m_warpK;
    }
    float eff = m_warpBaseFov + 10.0f * m_warpK;
    if (eff < 30.0f) eff = 30.0f;
    if (eff > 110.0f) eff = 110.0f;
    jst.fovDegrees = eff;
    m_warpLastFov = eff;
    m_warpFovDirty = true;
  } else if (m_warpFovDirty) {
    jst.fovDegrees = m_warpBaseFov;
    m_warpLastFov = m_warpBaseFov;
    m_warpFovDirty = false;
  }

  // Тряска ~K (снимается в finishWarpFrame после render, как обычно).
  m_warpShakePhase += deltaTime * (18.0f + 30.0f * m_warpK);
  const float amp = 0.0016f * m_warpK;
  m_warpShakeYaw = amp * std::sin(m_warpShakePhase);
  m_warpShakePitch = amp * 0.7f * std::sin(m_warpShakePhase * 1.37f + 1.3f);
  m_camera.yaw += m_warpShakeYaw;
  m_camera.pitch += m_warpShakePitch;

  // Мосты в SceneManager: оверлей + u_Burst + корабль (виден только в
  // Pullback/Travel/Arrival/Pushin; дефолт скрыт — харнесс чистый).
  warpdrive::setState(m_warpK, m_warpFlash);
  warpdrive::setBurst(m_jumpBurst);
  // v3.6.0 (пасс D): огибающая видимости обломков в Travel (вход по
  // kAstEnterT..kAstDangerStart, выход за 0.9). Вне Travel поле ровно 0.
  {
    float astK = 0.0f;
    if (ph == JumpPhase::Travel && m_jump.asteroidField()) {
      const float fin = (t01 - WarpJump::kAstEnterT) /
                        (WarpJump::kAstDangerStart - WarpJump::kAstEnterT);
      const float fout = (1.0f - t01) / 0.10f;
      float k = fin;
      if (fout < k) k = fout;
      if (k < 0.0f) k = 0.0f;
      if (k > 1.0f) k = 1.0f;
      astK = k;
    }
    warpdrive::setAsteroids(astK, m_astLane);
  }
  if (shipVisible) {
    const double shipYaw = std::atan2(-m_jumpShipDir.x, -m_jumpShipDir.z);
    warpdrive::setShip(true, static_cast<float>(m_jumpShipPos.x),
                       static_cast<float>(m_jumpShipPos.y),
                       static_cast<float>(m_jumpShipPos.z),
                       static_cast<float>(shipYaw));
  } else {
    warpdrive::setShip(false, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // Финиш sequence: контроллер уже в Idle — отдать Cruise, флаг снять.
  // Управление игроку возвращено ещё в Cooldown (consumed=false ниже).
  if (jumpEv.finished || ph == JumpPhase::Idle) {
    m_jumpCine = false;
    m_jumpPrevPhase = JumpPhase::Idle;
    m_warpStatus = 1;  // Cruise
    m_jumpBurst = 0.0f;
    warpRetarget(cruiseK);
    warpdrive::setBurst(0.0f);
    warpdrive::setShip(false, 0.0f, 0.0f, 0.0f, 0.0f);
    warpdrive::setAsteroids(0.0f, 0.0f);
    return false;
  }

  // Катсцена держит ввод до конца Pushin; в Cooldown управление уже у игрока
  // (как m_captureAnim-паттерн: WASD/мышь заморожены, дальше обычный ввод).
  return (ph != JumpPhase::Cooldown);
}
// === WarpJump === end

void AppGL::drawWarpPanel() {
  if (!m_warpOn) return;  // выключен — окна нет, gui.png байт-в-байт
  ImGui::SetNextWindowPos(ImVec2(370.0f, 15.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f), ImGuiCond_FirstUseEver);
  ImGui::Begin("Warp Drive (X)");

  ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Status: %s",
                     warpStatusName(m_warpStatus));
  int f = m_warpFactor;
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::SliderInt("Factor [ ]", &f, 1, 9)) {
    if (m_warpStatus != 3) {  // в туннеле фактор растёт сам
      m_warpFactor = f;
      warpRetarget(warpTargetK(f));
    }
  }
  ImGui::Text("Speed: %g ly/s (showcase)", warpSpeedLY(m_warpFactor));

  if (m_warpStatus == 2) {
    const int n = 3 - static_cast<int>(m_warpStateT);
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Jump in %d...",
                       (n < 1 ? 1 : n));
  } else if (m_warpStatus == 3) {
    ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "WARP TUNNEL %.1fs",
                       4.0 - static_cast<double>(m_warpStateT));
  } else if (m_warpStatus == 4) {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Cooldown %.1fs",
                       2.0 - static_cast<double>(m_warpStateT));
  }

  // Выбор пункта прыжка (каталог не трогаем, только читаем).
  const std::vector<Station>& cat = builtinCatalog();
  if (m_warpSel < 0 || m_warpSel >= static_cast<int>(cat.size())) m_warpSel = 0;
  if (ImGui::BeginCombo("Destination", cat[static_cast<size_t>(m_warpSel)].name.c_str())) {
    for (size_t i = 0; i < cat.size(); ++i) {
      const bool sel = (static_cast<int>(i) == m_warpSel);
      if (ImGui::Selectable(cat[i].name.c_str(), sel)) {
        m_warpSel = static_cast<int>(i);
      }
      if (sel) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  const Station* curSt = m_sceneManager->getCurrentStation();
  const std::string tgtId = cat[static_cast<size_t>(m_warpSel)].id;
  // v3.6.0: дистанция до цели (св. годы) и упрощённая оценка времени
  // перелёта по классической шкале v = f^3 c — «пустота космоса» в цифрах.
  {
    const Station& tgt = cat[static_cast<size_t>(m_warpSel)];
    ImGui::Text("Distance: %s", formatDistanceLy(tgt.distanceLy).c_str());
    if (tgt.distanceLy > 0.0) {
      ImGui::Text("ETA at Warp %d: ~%s", m_warpFactor,
                  formatEtaYears(warpEtaYears(tgt.distanceLy, m_warpFactor))
                      .c_str());
    }
  }
  const bool warpCap = ship::styleCanWarp(m_shipStyle);
  if (!warpCap) {
    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.35f, 1.0f),
                       "No warp drive on this hull (Shuttle)");
  }
  const bool canJump =
      (m_warpStatus == 1 && curSt && curSt->id != tgtId && warpCap);
  // Выбор корпуса корабля (смена вида).
  {
  const char* hullNames[] = {"Dart", "Ring Ship", "Shuttle", "Model (OBJ)"};
  int hullSel = m_shipStyle;
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::BeginCombo("Hull", hullNames[hullSel])) {
    for (int n = 0; n < 4; ++n) {
      const bool sel = (n == hullSel);
      if (ImGui::Selectable(hullNames[n], sel)) {
        setShipStyleNow(n);
      }
      if (sel) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  }
  if (!canJump) ImGui::BeginDisabled();
  if (ImGui::Button("Engage Jump", ImVec2(-1.0f, 0.0f))) {
    startWarpJump(tgtId);
  }
  if (!canJump) ImGui::EndDisabled();
  if (ImGui::Button("Disengage (X)", ImVec2(-1.0f, 0.0f))) {
    // === WarpJump === (агент J): посреди катсцены не гасим (досматриваем).
    if ((m_warpStatus == 1 || m_warpStatus == 4) && !m_jumpCine) disengageWarp();
  }
  ImGui::End();

  // Крупный центр-экран: обратный отсчёт и туннель (только в катсцене).
  // === WarpJump === (агент J): в кино-режиме тексты/прогресс — по фазам
  // контроллера (старые формулы 3-2-1/4с к новым длительностям не подходят).
  if (m_jumpCine) {
    ImFont* font = ImGui::GetFont();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (font && dl && ds.x > 0.0f && ds.y > 0.0f) {
      const JumpPhase jp = m_jump.phase();
      if (jp == JumpPhase::Charge || jp == JumpPhase::Pullback) {
        float toPeak = WarpJump::kBurstDur * 0.5f;
        if (jp == JumpPhase::Charge) {
          toPeak += (WarpJump::kChargeDur - m_jump.phaseTime()) +
                    WarpJump::kPullbackDur;
        } else {
          toPeak += (WarpJump::kPullbackDur - m_jump.phaseTime());
        }
        int n = static_cast<int>(std::ceil(toPeak));
        if (n < 1) n = 1;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "T-%d", n);
        const ImVec2 sz = font->CalcTextSizeA(64.0f, FLT_MAX, 0.0f, buf);
        dl->AddText(font, 64.0f,
                    ImVec2((ds.x - sz.x) * 0.5f, ds.y * 0.32f),
                    IM_COL32(120, 220, 255, 255), buf);
      } else if (jp == JumpPhase::Burst) {
        const char* msg = "BURST";
        const ImVec2 sz = font->CalcTextSizeA(64.0f, FLT_MAX, 0.0f, msg);
        dl->AddText(font, 64.0f,
                    ImVec2((ds.x - sz.x) * 0.5f, ds.y * 0.32f),
                    IM_COL32(255, 255, 255, 255), msg);
      } else if (jp == JumpPhase::Travel) {
        const char* msg = "WARP TUNNEL";
        const ImVec2 sz = font->CalcTextSizeA(40.0f, FLT_MAX, 0.0f, msg);
        dl->AddText(font, 40.0f,
                    ImVec2((ds.x - sz.x) * 0.5f, ds.y * 0.32f),
                    IM_COL32(150, 220, 255, 255), msg);
        const float t = m_jump.phaseT01();
        const float bw = ds.x * 0.4f;
        const ImVec2 p0((ds.x - bw) * 0.5f, ds.y * 0.32f + 60.0f);
        const ImVec2 p1(p0.x + bw, p0.y + 10.0f);
        dl->AddRectFilled(p0, p1, IM_COL32(40, 60, 90, 220));
        dl->AddRectFilled(p0, ImVec2(p0.x + bw * (t > 1.0f ? 1.0f : t), p1.y),
                          IM_COL32(120, 220, 255, 255));
      }
    }
  } else if (m_warpStatus == 2 || m_warpStatus == 3) {
    ImFont* font = ImGui::GetFont();
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (font && dl && ds.x > 0.0f && ds.y > 0.0f) {
  // === WarpJump === (агент J): в кино-режиме статусы — по контроллеру.
  if (m_jumpCine) {
    const float jt = m_jump.phaseTime();
    switch (m_jump.phase()) {
      case JumpPhase::Charge:
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Charging %.1fs",
                           WarpJump::kChargeDur - static_cast<double>(jt));
        break;
      case JumpPhase::Pullback:
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Pullback %.1fs",
                           WarpJump::kPullbackDur - static_cast<double>(jt));
        break;
      case JumpPhase::Burst:
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "BURST");
        break;
      case JumpPhase::Travel:
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "WARP TUNNEL %.1fs",
                           WarpJump::kTravelDur - static_cast<double>(jt));
        break;
      case JumpPhase::Arrival:
        ImGui::TextColored(ImVec4(0.5f, 0.9f, 1.0f, 1.0f), "ARRIVAL");
        break;
      case JumpPhase::Pushin:
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Approach %.1fs",
                           WarpJump::kPushinDur - static_cast<double>(jt));
        break;
      case JumpPhase::Cooldown:
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Cooldown %.1fs",
                           WarpJump::kCooldownDur - static_cast<double>(jt));
        break;
      case JumpPhase::Idle:
        break;
    }
  } else if (m_warpStatus == 2) {
        const int n = 3 - static_cast<int>(m_warpStateT);
        char buf[16];
        std::snprintf(buf, sizeof(buf), "T-%d", (n < 1 ? 1 : n));
        const ImVec2 sz = font->CalcTextSizeA(64.0f, FLT_MAX, 0.0f, buf);
        dl->AddText(font, 64.0f,
                    ImVec2((ds.x - sz.x) * 0.5f, ds.y * 0.32f),
                    IM_COL32(120, 220, 255, 255), buf);
      } else {
        const char* msg = "WARP TUNNEL";
        const ImVec2 sz = font->CalcTextSizeA(40.0f, FLT_MAX, 0.0f, msg);
        dl->AddText(font, 40.0f,
                    ImVec2((ds.x - sz.x) * 0.5f, ds.y * 0.32f),
                    IM_COL32(150, 220, 255, 255), msg);
        const float t = m_warpStateT / 4.0f;
        const float bw = ds.x * 0.4f;
        const ImVec2 p0((ds.x - bw) * 0.5f, ds.y * 0.32f + 60.0f);
        const ImVec2 p1(p0.x + bw, p0.y + 10.0f);
        dl->AddRectFilled(p0, p1, IM_COL32(40, 60, 90, 220));
        dl->AddRectFilled(p0, ImVec2(p0.x + bw * (t > 1.0f ? 1.0f : t), p1.y),
                          IM_COL32(120, 220, 255, 255));
      }
    }
  }
}
// === WarpDrive === end

// === Cinematic === begin (агент C: кинотур T, интро-карточки, HUD-полиш).
// Весь код ниже — новый. Скриншот-блок выше, варп (fly-to + WarpDrive),
// поиск, время и меню не тронуты. Русские строки рисуются киношрифтом
// с кириллицей (дефолтный шрифт не меняем). В атласе только Basic Latin +
// Cyrillic, поэтому разделители строго ASCII ("|", "-"): иначе были бы "?".

const char* AppGL::cineTypeRu(StationType t) {
  switch (t) {
    case StationType::BlackHole: return "Чёрная дыра";
    case StationType::Quasar: return "Квазар";
    case StationType::Constellation: return "Созвездие";
    case StationType::Galaxy: return "Галактика";
    case StationType::Nebula: return "Туманность";
    case StationType::SolarSystem: return "Планетная система";
    case StationType::Pulsar: return "Пульсар";
    case StationType::StarCluster: return "Скопление";
  }
  return "?";
}

void AppGL::startCineTour() {
  std::vector<std::string> ids;
  for (const auto& st : builtinCatalog()) ids.push_back(st.id);
  if (ids.empty()) return;
  m_flying = false;
  m_cardActive = false;
  // Тур — чистый автопилот: варп гасим его же API (код WarpDrive не меняем).
  if (m_warpOn) disengageWarp();
  m_tour.start(ids);
}

void AppGL::stopCineTour() { m_tour.stop(); }

void AppGL::tourTeleport(const std::string& id) {
  m_sceneManager->teleportTo(id, m_camera);
  m_flying = false;
  if (m_route.empty() || m_route.back() != id) m_route.push_back(id);
  if (std::find(m_visited.begin(), m_visited.end(), id) == m_visited.end()) {
    m_visited.push_back(id);
  }
  // База облёта снимается ПОСЛЕ телепорта (у sol своя точка/питч).
  m_tourBasePos = m_camera.position;
  m_tourBaseDist =
      std::sqrt(m_camera.position.x * m_camera.position.x +
                m_camera.position.y * m_camera.position.y +
                m_camera.position.z * m_camera.position.z);
  if (m_tourBaseDist < 1e-3) m_tourBaseDist = 1e-3;
  checkAchievements();
}

void AppGL::showIntroCard(const Station* st) {
  if (!st) return;
  m_cardName = st->name;
  m_cardTypeRu = cineTypeRu(st->type);
  m_cardFact = st->description;  // факт из Catalog, не выдумываем
  m_cardDist = st->spawnDistanceRs;
  m_cardT = 0.0f;
  m_cardActive = true;
}

bool AppGL::processCinematicInput(float deltaTime) {
  GLFWwindow* win = m_window->getNativeWindow();
  ImGuiIO& io = ImGui::GetIO();

  // T: старт/стоп тура (только в игре, не в строке поиска).
  const bool tDown = (glfwGetKey(win, m_keyTour) == GLFW_PRESS);
  if (tDown && !m_prevTPressed && !io.WantCaptureKeyboard && !m_inMenu) {
    if (m_tour.active())
      stopCineTour();
    else
      startCineTour();
  }
  m_prevTPressed = tDown;

  // Ребро пробела считаем всегда (иначе залипший флаг съест первый пропуск).
  const bool spaceDown = (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);
  const bool spaceEdge =
      (spaceDown && !m_prevSpacePressed && !io.WantCaptureKeyboard);
  m_prevSpacePressed = spaceDown;

  // Сессия/карточка/трофей тикают в игре всегда, даже без тура.
  updateCinematic(deltaTime);
  if (!m_tour.active()) return false;

  // ESC всегда выходит из тура; ребро съедаем, меню по нему не откроется
  // (второй ESC уже откроет меню обычным путём).
  const bool escDown = (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS);
  if (escDown && !m_prevEscPressed && !io.WantCaptureKeyboard) stopCineTour();
  m_prevEscPressed = escDown;
  if (!m_tour.active()) return true;

  // Пробел = пропуск станции; на последней пропуск = финиш.
  if (spaceEdge) {
    m_tour.skip();
    if (!m_tour.active()) m_finishT = static_cast<float>(glfwGetTime());
  }
  if (!m_tour.active()) return true;

  TourEvent ev = m_tour.update(deltaTime);
  if (ev.stationChanged) tourTeleport(ev.stationId);
  if (ev.finished) m_finishT = static_cast<float>(glfwGetTime());

  // Медленный облёт: база + lissajous, взгляд в центр.
  if (m_tour.active() && m_tour.phase() == TourPhase::Dwell) {
    double dx = 0.0, dy = 0.0, dYaw = 0.0, dPitch = 0.0;
    Tour::dwellOffsets(m_tour.phaseTime(), dx, dy, dYaw, dPitch);
    double k = m_tourBaseDist / 20.0;
    if (k < 0.4) k = 0.4;
    if (k > 3.0) k = 3.0;
    m_camera.position = Vec3(m_tourBasePos.x + dx * k,
                             m_tourBasePos.y + dy * k, m_tourBasePos.z);
    Vec3 origin(0.0, 0.0, 0.0);
    computeLookYawPitch(m_camera.position, origin, m_camera.yaw,
                        m_camera.pitch);
    m_camera.yaw += dYaw;
    m_camera.pitch += dPitch;
    const double kMaxP = 1.55;
    if (m_camera.pitch > kMaxP) m_camera.pitch = kMaxP;
    if (m_camera.pitch < -kMaxP) m_camera.pitch = -kMaxP;
  }
  return true;
}

void AppGL::updateCinematic(float deltaTime) {
  if (!m_inMenu) m_sessionT += deltaTime;
  if (m_cardActive) {
    m_cardT += deltaTime;
    if (m_cardT >= 3.0f) m_cardActive = false;
  }
  // Трофей: замечаем новые достижения локально, checkAchievements не трогаем.
  const float now = static_cast<float>(glfwGetTime());
  for (const auto& id : m_unlockedIds) {
    if (m_trophyKnown.insert(id).second) {
      m_trophyTitle = achievementTitle(id);
      m_trophyT = now;
    }
  }
}

void AppGL::drawCineTopBar(ImDrawList* dl, ImFont* font, float distNow) {
  const Station* st = m_sceneManager->getCurrentStation();
  if (!st) return;
  char buf[192];
  std::snprintf(buf, sizeof(buf), "Цель: %s  |  %.1f", st->name.c_str(),
                static_cast<double>(distNow));
  ImVec2 tsz = font->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, buf);
  const float padX = 16.0f, padY = 6.0f;
  const float w = tsz.x + padX * 2.0f, h = tsz.y + padY * 2.0f;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  const float x = (disp.x - w) * 0.5f, y = 10.0f;
  dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                    IM_COL32(8, 12, 20, 190), 7.0f);
  dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
              IM_COL32(255, 190, 70, 110), 7.0f, 0, 1.0f);
  dl->AddText(font, 15.0f, ImVec2(x + padX, y + padY),
              IM_COL32(255, 255, 255, 255), buf);
}

void AppGL::drawCineCard(ImDrawList* dl, ImFont* font) {
  // Плавное гашение: последние 0.8с из 3с.
  float a = (m_cardT < 2.2f) ? 1.0f : 1.0f - (m_cardT - 2.2f) / 0.8f;
  if (a < 0.0f) a = 0.0f;
  if (a > 1.0f) a = 1.0f;
  char line2[192];
  std::snprintf(line2, sizeof(line2), "%s  |  дистанция %.1f",
                m_cardTypeRu.c_str(), m_cardDist);
  ImVec2 sName = font->CalcTextSizeA(24.0f, FLT_MAX, 0.0f, m_cardName.c_str());
  ImVec2 sType = font->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, line2);
  ImVec2 sFact = font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, m_cardFact.c_str());
  float maxW = sName.x;
  if (sType.x > maxW) maxW = sType.x;
  if (sFact.x > maxW) maxW = sFact.x;
  const float padX = 26.0f, padY = 14.0f, gap = 5.0f;
  const float w = maxW + padX * 2.0f;
  const float h = sName.y + gap + sType.y + gap + sFact.y + padY * 2.0f;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  const float x = (disp.x - w) * 0.5f;
  const float y = disp.y * 0.30f - h * 0.5f;
  dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                    IM_COL32(8, 12, 20, static_cast<int>(225.0f * a)), 8.0f);
  dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h),
              IM_COL32(255, 190, 70, static_cast<int>(200.0f * a)), 8.0f, 0,
              1.0f);
  float cy = y + padY;
  dl->AddText(font, 24.0f, ImVec2(x + (w - sName.x) * 0.5f, cy),
              IM_COL32(255, 255, 255, static_cast<int>(255.0f * a)),
              m_cardName.c_str());
  cy += sName.y + gap;
  dl->AddText(font, 14.0f, ImVec2(x + (w - sType.x) * 0.5f, cy),
              IM_COL32(170, 200, 230, static_cast<int>(230.0f * a)), line2);
  cy += sType.y + gap;
  dl->AddText(font, 13.0f, ImVec2(x + (w - sFact.x) * 0.5f, cy),
              IM_COL32(210, 210, 210, static_cast<int>(220.0f * a)),
              m_cardFact.c_str());
}

void AppGL::drawCineCaption(ImDrawList* dl, ImFont* font) {
  if (m_tour.phase() != TourPhase::Dwell &&
      m_tour.phase() != TourPhase::FadeIn) {
    return;
  }
  const Station* st = findStation(m_tour.currentId());
  if (!st) return;
  const size_t n = m_tour.total();
  const size_t cur = m_tour.index();
  if (n == 0 || cur >= n) return;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  if (m_cineFont) ImGui::PushFont(m_cineFont);
  ImGui::SetNextWindowPos(ImVec2(disp.x * 0.5f, disp.y - 16.0f),
                          ImGuiCond_Always, ImVec2(0.5f, 1.0f));
  ImGui::SetNextWindowSize(ImVec2(540.0f, 0.0f), ImGuiCond_Always);
  ImGui::SetNextWindowBgAlpha(0.88f);
  const ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_AlwaysAutoResize;
  bool open = ImGui::Begin("##cinetour", nullptr, flags);
  if (open) {
    char head[64];
    std::snprintf(head, sizeof(head), "КИНОТУР  |  %zu/%zu", cur + 1, n);
    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "%s", head);
    ImGui::SetWindowFontScale(1.3f);
    ImGui::TextWrapped("%s", st->name.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.82f, 1.0f), "%s",
                       st->description.c_str());
    // Прогресс: точки-кружки + счёт (кружки примитивами: глифов нет в атласе).
    char prog[32];
    std::snprintf(prog, sizeof(prog), "%zu/%zu", cur + 1, n);
    ImVec2 progSz = font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, prog);
    const float r = 4.0f, gap = 15.0f;
    const float rowW =
        static_cast<float>(n - 1) * gap + static_cast<float>(n) * r * 2.0f +
        10.0f + progSz.x;
    ImDrawList* wdl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    const float avail = ImGui::GetContentRegionAvail().x;
    const float x0 = p0.x + (avail - rowW) * 0.5f;
    const float yc = p0.y + r + 2.0f;
    for (size_t i = 0; i < n; ++i) {
      ImVec2 c(x0 + r + static_cast<float>(i) * (r * 2.0f + gap), yc);
      if (i <= cur)
        wdl->AddCircleFilled(c, r, IM_COL32(255, 190, 70, 255));
      else
        wdl->AddCircle(c, r, IM_COL32(140, 140, 140, 255), 12, 1.0f);
    }
    wdl->AddText(font, 13.0f,
                 ImVec2(x0 + rowW - progSz.x, yc - progSz.y * 0.5f),
                 IM_COL32(200, 200, 200, 255), prog);
    ImGui::Dummy(ImVec2(rowW, r * 2.0f + 4.0f));
    // Кнопки без нав-фокуса: пробел не должен жать их повторно.
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
    const float bw = (ImGui::GetContentRegionAvail().x -
                      ImGui::GetStyle().ItemSpacing.x) *
                     0.5f;
    if (ImGui::Button("Пропустить (Space)", ImVec2(bw, 0.0f))) {
      m_tour.skip();
      if (!m_tour.active()) m_finishT = static_cast<float>(glfwGetTime());
    }
    ImGui::SameLine();
    if (ImGui::Button("Выйти (ESC)", ImVec2(bw, 0.0f))) stopCineTour();
    ImGui::PopItemFlag();
  }
  ImGui::End();
  if (m_cineFont) ImGui::PopFont();
  (void)dl;
}

void AppGL::drawCineHint(ImDrawList* dl, ImFont* font) {
  if (m_sessionT >= 10.0f) return;
  float a = (m_sessionT < 8.0f) ? 0.9f : 0.9f * (10.0f - m_sessionT) / 2.0f;
  if (a < 0.0f) a = 0.0f;
  const char* txt =
      "WASD - полет | мышь - осмотр | T - кинотур | / - поиск | F1 - помощь | "
      "ESC - меню";
  ImVec2 tsz = font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, txt);
  const float padX = 14.0f, padY = 5.0f;
  const float w = tsz.x + padX * 2.0f, h = tsz.y + padY * 2.0f;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  const float x = 12.0f, y = disp.y - h - 12.0f;
  dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h),
                    IM_COL32(8, 12, 20, static_cast<int>(190.0f * a)), 6.0f);
  dl->AddText(font, 13.0f, ImVec2(x + padX, y + padY),
              IM_COL32(200, 200, 200, static_cast<int>(255.0f * a)), txt);
}

void AppGL::drawCineTrophy(ImDrawList* dl, ImFont* font, float now) {
  const float age = now - m_trophyT;
  if (age < 0.0f || age > 1.6f) return;
  const float a = 1.0f - age / 1.6f;
  const char* line1 = "ДОСТИЖЕНИЕ";
  ImVec2 s1 = font->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, line1);
  ImVec2 s2 =
      font->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, m_trophyTitle.c_str());
  float maxW = (s1.x > s2.x) ? s1.x : s2.x;
  const float iconW = 30.0f, gap = 10.0f;
  const float w = iconW + gap + maxW;
  const float h = s1.y + 3.0f + s2.y;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  const float x = (disp.x - w) * 0.5f;
  const float y = 50.0f;
  // Вспышка-кольцо вместо звука.
  dl->AddCircle(ImVec2(x + iconW * 0.5f, y + h * 0.5f), 12.0f + age * 52.0f,
                IM_COL32(255, 200, 90, static_cast<int>(200.0f * a)), 32, 2.0f);
  // Кубок примитивами (глифа трофея в атласе нет).
  const ImU32 amber = IM_COL32(255, 190, 70, static_cast<int>(255.0f * a));
  const float ix = x + iconW * 0.5f, iy = y + h * 0.5f - 10.0f;
  dl->AddQuad(ImVec2(ix - 10.0f, iy), ImVec2(ix + 10.0f, iy),
              ImVec2(ix + 6.0f, iy + 12.0f), ImVec2(ix - 6.0f, iy + 12.0f),
              amber, 2.0f);
  dl->AddLine(ImVec2(ix, iy + 12.0f), ImVec2(ix, iy + 20.0f), amber, 2.0f);
  dl->AddLine(ImVec2(ix - 7.0f, iy + 20.0f), ImVec2(ix + 7.0f, iy + 20.0f),
              amber, 2.0f);
  dl->AddText(font, 15.0f, ImVec2(x + iconW + gap, y),
              IM_COL32(255, 190, 70, static_cast<int>(255.0f * a)), line1);
  dl->AddText(font, 14.0f, ImVec2(x + iconW + gap, y + s1.y + 3.0f),
              IM_COL32(255, 255, 255, static_cast<int>(255.0f * a)),
              m_trophyTitle.c_str());
}

void AppGL::drawCineFade(ImDrawList* dl) {
  const float a = m_tour.fadeAlpha();
  if (a <= 0.001f) return;
  int ab = static_cast<int>(a * 255.0f);
  if (ab < 0) ab = 0;
  if (ab > 255) ab = 255;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  dl->AddRectFilled(ImVec2(0.0f, 0.0f), disp, IM_COL32(0, 0, 0, ab));
}

void AppGL::drawCinematic() {
  ImDrawList* dl = ImGui::GetForegroundDrawList();
  if (!dl) return;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  if (disp.x <= 0.0f || disp.y <= 0.0f) return;
  ImFont* font = m_cineFont ? m_cineFont : ImGui::GetFont();
  if (!font) return;
  const float now = static_cast<float>(glfwGetTime());
  const double distNow =
      std::sqrt(m_camera.position.x * m_camera.position.x +
                m_camera.position.y * m_camera.position.y +
                m_camera.position.z * m_camera.position.z);
  // v3.6.0: при открытых слайдах глав экспедиции HUD-оверлеи (плашка
  // станции, трофеи, подсказки) прячем — катсцена чистая, наложений нет.
  const bool showUi = !m_hideUI && !m_slidesOpen;
  if (showUi) drawCineTopBar(dl, font, static_cast<float>(distNow));
  if (showUi) drawCineTrophy(dl, font, now);
  if (showUi) {
    // Баннер финиша тура (4с, гаснет последний 1с).
    const float fage = now - m_finishT;
    if (fage >= 0.0f && fage < 4.0f) {
      float fa = (fage < 3.0f) ? 1.0f : 1.0f - (fage - 3.0f);
      if (fa < 0.0f) fa = 0.0f;
      const char* l1 = "Кинотур завершён";
      const char* l2 = "все станции показаны";
      ImVec2 fs1 = font->CalcTextSizeA(18.0f, FLT_MAX, 0.0f, l1);
      ImVec2 fs2 = font->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, l2);
      float fw = (fs1.x > fs2.x) ? fs1.x : fs2.x;
      const float fpadX = 22.0f, fpadY = 10.0f, fgap = 4.0f;
      fw += fpadX * 2.0f;
      const float fh = fs1.y + fgap + fs2.y + fpadY * 2.0f;
      const float fx = (disp.x - fw) * 0.5f;
      const float fy = disp.y * 0.40f - fh * 0.5f;
      dl->AddRectFilled(ImVec2(fx, fy), ImVec2(fx + fw, fy + fh),
                        IM_COL32(8, 12, 20, static_cast<int>(220.0f * fa)),
                        8.0f);
      dl->AddRect(ImVec2(fx, fy), ImVec2(fx + fw, fy + fh),
                  IM_COL32(255, 190, 70, static_cast<int>(200.0f * fa)), 8.0f,
                  0, 1.5f);
      dl->AddText(font, 18.0f, ImVec2(fx + (fw - fs1.x) * 0.5f, fy + fpadY),
                  IM_COL32(255, 255, 255, static_cast<int>(255.0f * fa)), l1);
      dl->AddText(font, 13.0f,
                  ImVec2(fx + (fw - fs2.x) * 0.5f, fy + fpadY + fs1.y + fgap),
                  IM_COL32(180, 180, 180, static_cast<int>(220.0f * fa)), l2);
    }
  }
  if (showUi && m_cardActive && !m_tour.active()) drawCineCard(dl, font);
  if (showUi && m_tour.active()) drawCineCaption(dl, font);
  if (showUi) drawCineHint(dl, font);
  drawCineFade(dl);  // вуаль всегда поверх, даже при скрытом UI
}

// === Cinematic === end

// === Audio === begin (агент AU: процедурный звук, только свои методы).
// Чужие методы/блоки выше не тронуты. Все вызовы движка — только отсюда.
// Прямые хуки в чужих местах (варп/teleport/ачивка/захват) НЕ вносим:
// владелец добавит их сам по однострочникам из финала; здесь те же события
// детектируются polling-ом состояний, поэтому звук работает уже сейчас,
// а харнесс (без run()) движок не стартует и PNG остаются чистыми.
void AppGL::audioEnsureOnFirstInput() {
  if (m_audio.isInitialized()) return;
  GLFWwindow* w = m_window ? m_window->getNativeWindow() : nullptr;
  if (!w) return;
  // Первый ввод: любая из ходовых клавиш/мыши. Полный перебор кодов
  // не делаем (невалидные коды дают GLFW-ошибки) — достаточно витрины.
  static const int kKeys[] = {
      GLFW_KEY_W, GLFW_KEY_X, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D,
      GLFW_KEY_SPACE, GLFW_KEY_ESCAPE, GLFW_KEY_ENTER, GLFW_KEY_M,
      GLFW_KEY_T, GLFW_KEY_P, GLFW_KEY_H, GLFW_KEY_F1, GLFW_KEY_SLASH,
      GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_F5, GLFW_KEY_F6,
      GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_DELETE,
      GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT};
  bool any = false;
  for (int k : kKeys) {
    if (glfwGetKey(w, k) == GLFW_PRESS) {
      any = true;
      break;
    }
  }
  if (!any) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
        glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
      any = true;
    }
  }
  if (!any) return;
  m_audio.ensureInitialized();
  m_audio.setVolume(static_cast<float>(m_audioVolume) / 100.0f);
  // Снапшот состояний без триггеров, чтобы старт не давал ложный whoosh.
  if (const Station* st = m_sceneManager->getCurrentStation()) {
    m_audioPrevStation = st->id;
  }
  m_audioPrevUnlocked = m_unlockedIds.size();
  m_audioPrevWarpStatus = m_warpStatus;
  m_audioPrevTourIndex =
      m_tour.active() ? static_cast<int>(m_tour.index()) : -1;
  m_audioPrevTourActive = m_tour.active();
  m_audioPrevInMenu = m_inMenu;
  m_audioWasInit = true;
}

void AppGL::audioProcessKey() {
  GLFWwindow* w = m_window ? m_window->getNativeWindow() : nullptr;
  if (!w) return;
  ImGuiIO& io = ImGui::GetIO();
  const bool mDown =
      (glfwGetKey(w, GLFW_KEY_M) == GLFW_PRESS);
  if (mDown && !m_prevMPressed && !io.WantCaptureKeyboard) {
    audioEnsureOnFirstInput();
    // ensure мог не сработать без другого ввода — M сам и есть ввод.
    if (!m_audio.isInitialized()) {
      m_audio.ensureInitialized();
      m_audio.setVolume(static_cast<float>(m_audioVolume) / 100.0f);
      m_audioWasInit = true;
    }
    m_audio.toggleMute();
    if (!m_audio.isMuted()) m_audio.playClick();
  }
  m_prevMPressed = mDown;
}

void AppGL::audioUpdate(float deltaTime) {
  if (!m_audio.isInitialized()) return;
  m_audio.setVolume(static_cast<float>(m_audioVolume) / 100.0f);
  m_audio.update(deltaTime);

  // Варп-хук polling-ом (без правок WarpDrive): уровень + дакинг внутри.
  m_audio.setWarpLevel(m_warpK);
  // Гул двигателя (v3.3.0): от скорости полёта; в меню всегда тихо.
  m_audio.setEngineLevel(m_inMenu ? 0.0f : m_speedK);

  // Телепорт-whoosh polling-ом: смена станции любым путём
  // (меню/поиск/варп-прыжок/тур) — без правок teleportToStation.
  if (const Station* st = m_sceneManager->getCurrentStation()) {
    if (m_audioWasInit && !m_audioPrevStation.empty() &&
        st->id != m_audioPrevStation) {
      m_audio.playWhoosh();
    }
    m_audioPrevStation = st->id;
  }

  // Ачивка-chime polling-ом: рост m_unlockedIds — без правок checkAchievements.
  if (m_unlockedIds.size() > m_audioPrevUnlocked) {
    m_audio.playChime();
  }
  m_audioPrevUnlocked = m_unlockedIds.size();

  // Прыжок: вход в туннель (статус 3) — без правок updateWarp.
  if (m_warpStatus == 3 && m_audioPrevWarpStatus != 3) {
    m_audio.playJump();
  }
  m_audioPrevWarpStatus = m_warpStatus;

  // Тур-blip polling-ом: старт/смена станции тура — без правок Tour/AppGL tour.
  {
    const bool active = m_tour.active();
    const int idx = active ? static_cast<int>(m_tour.index()) : -1;
    if (active && (!m_audioPrevTourActive || idx != m_audioPrevTourIndex)) {
      m_audio.playBlip();
    }
    m_audioPrevTourActive = active;
    m_audioPrevTourIndex = idx;
  }

  // Выход из меню (Play) — короткий click.
  if (m_audioPrevInMenu && !m_inMenu) {
    m_audio.playClick();
  }
  m_audioPrevInMenu = m_inMenu;

  // Захват ЧД: камера у горизонта — тревога (зовёт PH-хук playCapture).
  // Порог Rs*1.5 с кулдауном 5с (шейдерный capture r<=Rs*1.001 недостижим
  // из-за clampNearHorizon, поэтому раннее предупреждение честнее).
  if (m_audioCaptureCooldown > 0.0) m_audioCaptureCooldown -= deltaTime;
  if (const Station* st = m_sceneManager->getCurrentStation()) {
    if (st->type == StationType::BlackHole && m_audioCaptureCooldown <= 0.0) {
      const double d = std::sqrt(m_camera.position.x * m_camera.position.x +
                                 m_camera.position.y * m_camera.position.y +
                                 m_camera.position.z * m_camera.position.z);
      const double rs = static_cast<double>(m_sceneManager->getSettings().rs);
      if (rs > 0.05 && d < rs * 1.5) {
        m_audio.playCapture();
        m_audioCaptureCooldown = 5.0;
      }
    }
  }

  // v3.1.0 Sun-link: at the Milky Way station, flying within 3 units of
  // the Sun auto-jumps home to "sol" (startWarpJump guards: warp Cruise).
  // Same polling style as the BH-capture check above; harness never calls
  // audioUpdate, and warp is off there anyway.
  if (m_sunLinkCooldown > 0.0) m_sunLinkCooldown -= deltaTime;
  if (const Station* sunSt = m_sceneManager->getCurrentStation()) {
    if (sunSt->id == "mw" && m_sunLinkCooldown <= 0.0) {
      glm::vec3 sunPos(0.0f);
      if (m_sceneManager->sunWorldPos(sunPos)) {
        const double dx = m_camera.position.x - static_cast<double>(sunPos.x);
        const double dy = m_camera.position.y - static_cast<double>(sunPos.y);
        const double dz = m_camera.position.z - static_cast<double>(sunPos.z);
        if (std::sqrt(dx * dx + dy * dy + dz * dz) < 3.0) {
          startWarpJump("sol");
          m_sunLinkCooldown = 10.0;
        }
      }
    }
  }

  // Пульсар-тик polling-ом: период по станции. Vela 89 мс — настоящий;
  // Crab P*2 (66 мс) и магнетар P/8 (0.945 с) — поделены для слышимости
  // (33 мс — жужжание, 7.56 с — тишина). Визуальный маятник шейдера
  // художественный: 60 fps физически не покажут 89 мс (см. pulsar.frag).
  if (const Station* pst = m_sceneManager->getCurrentStation()) {
    double tickPeriod = 0.0;
    if (pst->type == StationType::Pulsar && !m_audio.isMuted()) {
      const std::string& pid = pst->id;
      tickPeriod = 0.089;
      if (pid == "psr-crab") tickPeriod = 0.066;
      else if (pid == "psr-b1913") tickPeriod = 0.059;
      else if (pid == "sgr-1806") tickPeriod = 0.945;
    }
    if (tickPeriod > 0.0) {
      m_audioTickAcc += deltaTime;
      while (m_audioTickAcc >= tickPeriod) {
        m_audioTickAcc -= tickPeriod;
        m_audio.playTick();
      }
    } else {
      m_audioTickAcc = 0.0;
    }
  }
}

void AppGL::audioDrawPanel() {
  // Гард скриншотов: до init панель пуста -> gui.png байт-в-байт.
  if (!m_audio.isInitialized()) return;
  ImGui::Separator();
  ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.28f, 1.0f), "Audio (M):");
  int v = m_audioVolume;
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::SliderInt("Volume ##audio", &v, 0, 100)) {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    m_audioVolume = v;
    m_audio.setVolume(static_cast<float>(v) / 100.0f);
    m_audio.playClick();
  }
  const bool muted = m_audio.isMuted();
  const bool live = m_audio.isDeviceLive();
  // Индикатор ноты в статусе панели: нота + состояние (NULL = null-backend).
  // Ноту рисуем киношрифтом (дефолтный шрифт ImGui её не имеет — был бы "?").
  const bool pushed = (m_cineFont != nullptr);
  if (pushed) ImGui::PushFont(m_cineFont);
  ImGui::Text("%s %s %d%% (M)", muted ? "[ ]" : "♪",
              muted ? "MUTED" : (live ? "ON air" : "NULL-backend"),
              m_audioVolume);
  if (pushed) ImGui::PopFont();
  ImGui::SameLine();
  if (ImGui::Button(muted ? "Unmute" : "Mute")) {
    m_audio.toggleMute();
    if (!m_audio.isMuted()) m_audio.playClick();
  }
}
// === Audio === end

// === Physics === begin: катсцена захвата горизонтом событий.
// v3.6.0: полноценная последовательность вместо мгновенного деспавна —
// фаза прилива (линза+красное смещение, затягивание), провал со вспышкой
// и перенос к точке входа, затем фаза выброса (FOV и ослепление спадают).
void AppGL::startHorizonCapture() {
  if (m_captureAnim) return;
  m_captureAnim = true;
  m_horizonWarped = false;
  m_captureT = 0.0f;
  m_captureBaseFov = m_sceneManager->getSettings().fovDegrees;
  m_audio.playCapture();
  const Station* st = m_sceneManager->getCurrentStation();
  const bool quasar = st && st->type == StationType::Quasar;
  Toast t;
  t.title = quasar ? uiRu("Ядро квазара захватывает вас",
                          "The quasar core seizes you")
                   : uiRu("Захвачен горизонтом событий",
                          "Captured by the event horizon");
  t.desc = (st ? st->name : std::string()) +
           uiRu(" — вас уносит приливом пространства.",
                " — the tide of space pulls you in.");
  t.time = static_cast<float>(glfwGetTime());
  m_toasts.push_back(t);
}

void AppGL::updateHorizonCapture(float dt) {
  if (!m_captureAnim) return;
  m_captureT += dt;
  RenderSettings& rset = m_sceneManager->getSettings();
  float hk = 0.0f;     // приливная линза + красное смещение
  float flash = 0.0f;  // мягкая вспышка перехода
  if (m_captureT < 1.4f) {
    // Фаза 1 «прилив»: FOV ширится, линза растёт, камера втягивается.
    const float k = m_captureT / 1.4f;
    rset.fovDegrees = m_captureBaseFov + 26.0f * k;
    hk = 0.7f * k;
    const double pull = 1.0 - static_cast<double>(dt) * 0.9;
    m_camera.position = Vec3(m_camera.position.x * pull,
                             m_camera.position.y * pull,
                             m_camera.position.z * pull);
  } else if (m_captureT < 1.9f) {
    // Фаза 2 «провал»: перенос к точке входа на пике вспышки.
    if (!m_horizonWarped) {
      m_horizonWarped = true;
      if (const Station* st = m_sceneManager->getCurrentStation()) {
        m_sceneManager->teleportTo(st->id, m_camera);
      }
      // Цена перехода: щит частично испарён (не ниже 5), топливо не трогаем.
      m_ship.shield = std::max(5.0f, m_ship.shield - 30.0f);
      m_ship.gravVel = Vec3{};
      m_shipVel = Vec3();
      m_orbitNoticeShown = false;
      m_prevSpeedPos = m_camera.position;
      m_camSpeed = 0.0f;
      m_sceneManager->resetRelativity();
      m_capturedHorizon = true;
      checkAchievements();
      Toast t2;
      t2.title = uiRu("Выброшены из горизонта событий",
                      "Ejected from the event horizon");
      const Station* stNow = m_sceneManager->getCurrentStation();
      t2.desc = (stNow ? stNow->name : std::string()) +
                uiRu(": щит частично испарён, курс сброшен к точке входа.",
                     ": shields partially evaporated, course reset to the entry point.");
      t2.time = static_cast<float>(glfwGetTime());
      m_toasts.push_back(t2);
    }
    const float pk = (m_captureT - 1.4f) / 0.5f;
    hk = 0.7f + 0.3f * pk;
    flash = std::sin(pk * 3.14159265f) * 0.95f;
    rset.fovDegrees = m_captureBaseFov + 26.0f;
  } else if (m_captureT < 3.2f) {
    // Фаза 3 «выброс»: линза и ослепление плавно спадают.
    const float kk = std::clamp(1.0f - (m_captureT - 1.9f) / 1.3f, 0.0f, 1.0f);
    hk = 0.6f * kk;
    flash = 0.85f * kk * kk;
    rset.fovDegrees = m_captureBaseFov + 26.0f * kk;
  } else {
    rset.fovDegrees = m_captureBaseFov;
    m_captureAnim = false;
    m_horizonWarped = false;
    m_sceneManager->setHorizonFx(0.0f, 0.0f);
    return;
  }
  m_sceneManager->setHorizonFx(hk, flash);
}

void AppGL::drawHorizonCapture(ImDrawList* dl) {
  if (!m_captureAnim || !dl) return;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  if (disp.x <= 0.0f || disp.y <= 0.0f) return;
  // Мягкое затемнение «в колодец» в фазе прилива; после вспышки гаснет.
  float a = 0.0f;
  if (m_captureT < 1.4f) {
    a = 0.35f * (m_captureT / 1.4f);
  } else if (m_captureT < 1.9f) {
    a = 0.35f;
  } else if (m_captureT < 3.2f) {
    a = 0.18f * std::max(0.0f, (3.2f - m_captureT) / 1.3f);
  }
  int ab = static_cast<int>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
  dl->AddRectFilled(ImVec2(0.0f, 0.0f), disp, IM_COL32(0, 0, 0, ab));
}
// === Physics === end

}  // namespace astro
