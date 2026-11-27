#include "IntroCinematicDirector.h"
#include <cmath>
#include <algorithm>
#ifndef ASTRO_NO_IMGUI
#include <imgui.h>
#endif
#include <glm/gtc/matrix_transform.hpp>

namespace astro {

IntroCinematicDirector::IntroCinematicDirector() {
  setupKeyframes();
  reset();
}

void IntroCinematicDirector::reset() {
  m_active = false;
  m_skipping = false;
  m_skipFadeTimer = 0.0f;
  m_timer = 0.0f;
  m_phase = IntroPhase::VoidDrift;
  m_phaseProgress = 0.0f;
  m_totalProgress = 0.0f;

  m_warpDistortion = 0.0f;
  m_flashIntensity = 0.0f;
  m_anamorphicStreak = 0.0f;
  m_fadeAlpha = 0.0f;

  m_firedWarpBoom = false;
  m_firedEngineWhoosh = false;
  m_firedBridgeOnline = false;
  m_firedTitleChime = false;
  m_firedBassDrop = false;

  m_triggerWarpBoom = false;
  m_triggerEngineWhoosh = false;
  m_triggerBridgeOnline = false;
  m_triggerTitleChime = false;
  m_triggerBassDrop = false;

  evaluateSpline(0.0f);
}

void IntroCinematicDirector::start() {
  reset();
  m_active = true;
}

void IntroCinematicDirector::skip() {
  if (!m_active || m_phase == IntroPhase::Finished) return;
  m_skipping = true;
}

void IntroCinematicDirector::setupKeyframes() {
  m_keyframes.clear();
  // Time-aware Hermite keyframes spanning 0.0s to 22.0s around Kerr BH at (0,0,0),
  // Megastructure at (14, 5.5, 20), and Bridge at (0, 1, 50) facing -Z.
  m_keyframes = {
      // Index 0: Pre-anchor
      {-2.0f, glm::vec3(-10.0f, 18.0f, 86.0f), glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 58.0f, 2.5f},
      // Index 1: Phase 0 Void Drift start — dramatic angled approach to Kerr accretion disk
      {0.0f, glm::vec3(-7.0f, 15.0f, 76.0f), glm::vec3(0.0f, 1.8f, 0.0f), glm::vec3(0.04f, 0.999f, 0.0f), 60.0f, 1.5f},
      // Index 2: Phase 0 -> Phase 1 Warp Burst trigger
      {3.5f, glm::vec3(-5.0f, 11.0f, 61.0f), glm::vec3(-1.5f, 2.5f, 24.0f), glm::vec3(0.0f, 1.0f, 0.0f), 62.0f, 0.0f},
      // Index 3: Phase 1 Warp Exit & Ship Pass close flank
      {7.0f, glm::vec3(-11.0f, 8.0f, 47.0f), glm::vec3(2.0f, 4.2f, 28.0f), glm::vec3(0.05f, 0.998f, 0.0f), 66.0f, -3.0f},
      // Index 4: Phase 2 -> Phase 3 Megastructure orbital reveal
      {10.5f, glm::vec3(-12.0f, 6.2f, 35.0f), glm::vec3(10.0f, 5.0f, 16.0f), glm::vec3(-0.04f, 0.999f, 0.0f), 62.0f, 2.0f},
      // Index 5: Phase 3 -> Phase 4 Orbit around station into Bridge approach
      {14.5f, glm::vec3(8.5f, 4.6f, 43.0f), glm::vec3(6.0f, 2.8f, 14.0f), glm::vec3(0.0f, 1.0f, 0.0f), 62.0f, 0.0f},
      // Index 6: Phase 4 -> Phase 5 Inside Bridge behind holo-table looking out canopy at Kerr + Megastructure
      {18.0f, glm::vec3(1.9f, 1.82f, 51.2f), glm::vec3(1.0f, 1.35f, 16.0f), glm::vec3(0.0f, 1.0f, 0.0f), 68.0f, 0.0f},
      // Index 7: Phase 5 Title Glory gentle bridge pan
      {22.0f, glm::vec3(-1.8f, 1.78f, 50.8f), glm::vec3(0.2f, 1.35f, 14.0f), glm::vec3(0.0f, 1.0f, 0.0f), 66.0f, 0.0f},
      // Index 8: Post-anchor
      {24.0f, glm::vec3(-3.6f, 1.75f, 50.6f), glm::vec3(-0.3f, 1.35f, 12.0f), glm::vec3(0.0f, 1.0f, 0.0f), 65.0f, 0.0f}
  };
}

glm::vec3 IntroCinematicDirector::hermite(const glm::vec3& p0, const glm::vec3& p1,
                                          const glm::vec3& m0, const glm::vec3& m1, float t) {
  const float t2 = t * t;
  const float t3 = t2 * t;
  return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 +
         (t3 - 2.0f * t2 + t) * m0 +
         (-2.0f * t3 + 3.0f * t2) * p1 +
         (t3 - t2) * m1;
}

float IntroCinematicDirector::hermite(float p0, float p1, float m0, float m1, float t) {
  const float t2 = t * t;
  const float t3 = t2 * t;
  return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 +
         (t3 - 2.0f * t2 + t) * m0 +
         (-2.0f * t3 + 3.0f * t2) * p1 +
         (t3 - t2) * m1;
}

void IntroCinematicDirector::evaluateSpline(float t) {
  if (m_keyframes.size() < 4) return;

  size_t idx = 1;
  while (idx + 2 < m_keyframes.size() && m_keyframes[idx + 1].time < t) {
    ++idx;
  }

  const auto& k0 = m_keyframes[idx - 1];
  const auto& k1 = m_keyframes[idx];
  const auto& k2 = m_keyframes[idx + 1];
  const auto& k3 = m_keyframes[idx + 2];

  const float segDuration = std::max(k2.time - k1.time, 0.001f);
  const float u = std::clamp((t - k1.time) / segDuration, 0.0f, 1.0f);
  // Time-aware central tangents keep velocity continuous at every keyframe.
  const float span02 = std::max(k2.time - k0.time, 0.001f);
  const float span13 = std::max(k3.time - k1.time, 0.001f);
  constexpr float tension = 0.85f;
  const glm::vec3 posM1 = (k2.pos - k0.pos) * (segDuration / span02) * tension;
  const glm::vec3 posM2 = (k3.pos - k1.pos) * (segDuration / span13) * tension;
  const glm::vec3 targetM1 = (k2.target - k0.target) * (segDuration / span02) * tension;
  const glm::vec3 targetM2 = (k3.target - k1.target) * (segDuration / span13) * tension;
  const glm::vec3 upM1 = (k2.up - k0.up) * (segDuration / span02) * tension;
  const glm::vec3 upM2 = (k3.up - k1.up) * (segDuration / span13) * tension;

  m_camPos = hermite(k1.pos, k2.pos, posM1, posM2, u);
  m_camTarget = hermite(k1.target, k2.target, targetM1, targetM2, u);
  m_camUp = glm::normalize(hermite(k1.up, k2.up, upM1, upM2, u));
  m_camFov = hermite(k1.fov, k2.fov,
                     (k2.fov - k0.fov) * (segDuration / span02) * tension,
                     (k3.fov - k1.fov) * (segDuration / span13) * tension, u);
  m_camRoll = hermite(k1.roll, k2.roll,
                      (k2.roll - k0.roll) * (segDuration / span02) * tension,
                      (k3.roll - k1.roll) * (segDuration / span13) * tension, u);
}

void IntroCinematicDirector::update(float dt) {
  if (!m_active || m_phase == IntroPhase::Finished) return;

  if (m_skipping) {
    m_skipFadeTimer += dt;
    m_fadeAlpha = std::clamp(m_skipFadeTimer / 0.35f, 0.0f, 1.0f);
    if (m_skipFadeTimer >= 0.35f) {
      m_phase = IntroPhase::Finished;
      m_active = false;
      return;
    }
  }

  m_timer += dt;
  m_totalProgress = std::clamp(m_timer / m_totalDuration, 0.0f, 1.0f);

  // Phase state machine
  if (m_timer < 3.5f) {
    m_phase = IntroPhase::VoidDrift;
    m_phaseProgress = m_timer / 3.5f;
    m_warpDistortion = 0.0f;
    m_flashIntensity = 0.0f;
    m_anamorphicStreak = 0.0f;
  } else if (m_timer < 7.0f) {
    m_phase = IntroPhase::WarpBurst;
    m_phaseProgress = (m_timer - 3.5f) / 3.5f;
    // Sound trigger at 3.55s
    if (!m_firedWarpBoom && m_timer >= 3.55f) {
      m_firedWarpBoom = true;
      m_triggerWarpBoom = true;
    }
    // Soft, low-intensity flash: no full-frame white spike or strobe.
    const float flashTime = m_timer - 3.5f;
    m_flashIntensity = flashTime < 1.2f
        ? 0.22f * std::sin(std::clamp(flashTime / 1.2f, 0.0f, 1.0f) * 3.14159f)
        : 0.0f;
    m_warpDistortion = std::max(0.0f, std::sin(m_phaseProgress * 3.14159f) * 0.45f);
    m_anamorphicStreak = m_flashIntensity * 0.35f;
  } else if (m_timer < 10.5f) {
    m_phase = IntroPhase::ShipPass;
    m_phaseProgress = (m_timer - 7.0f) / 3.5f;
    if (!m_firedEngineWhoosh && m_timer >= 7.1f) {
      m_firedEngineWhoosh = true;
      m_triggerEngineWhoosh = true;
    }
    m_warpDistortion = 0.0f;
    m_flashIntensity = 0.0f;
    m_anamorphicStreak = std::max(0.0f, 0.08f * (1.0f - m_phaseProgress));
  } else if (m_timer < 14.5f) {
    m_phase = IntroPhase::MegastructureReveal;
    m_phaseProgress = (m_timer - 10.5f) / 4.0f;
    m_warpDistortion = 0.0f;
    m_flashIntensity = 0.0f;
    m_anamorphicStreak = 0.03f;
  } else if (m_timer < 18.0f) {
    m_phase = IntroPhase::BridgeEnter;
    m_phaseProgress = (m_timer - 14.5f) / 3.5f;
    if (!m_firedBridgeOnline && m_timer >= 15.0f) {
      m_firedBridgeOnline = true;
      m_triggerBridgeOnline = true;
    }
    // Subtle cockpit canopy glass flash when passing through
    float canopyHit = (m_timer - 14.5f);
    if (canopyHit > 0.4f && canopyHit < 0.9f) {
      m_flashIntensity = std::sin((canopyHit - 0.4f) / 0.5f * 3.14159f) * 0.08f;
    } else {
      m_flashIntensity = 0.0f;
    }
  } else if (m_timer < 22.0f) {
    m_phase = IntroPhase::TitleGlory;
    m_phaseProgress = (m_timer - 18.0f) / 4.0f;
    if (!m_firedTitleChime && m_timer >= 18.1f) {
      m_firedTitleChime = true;
      m_triggerTitleChime = true;
    }
    if (!m_firedBassDrop && m_timer >= 18.05f) {
      m_firedBassDrop = true;
      m_triggerBassDrop = true;
    }
    m_anamorphicStreak = 0.05f;
    // Fade out during last 0.8s
    if (m_timer >= 21.2f) {
      m_fadeAlpha = (m_timer - 21.2f) / 0.8f;
    }
  } else {
    m_phase = IntroPhase::Finished;
    m_active = false;
  }

  evaluateSpline(m_timer);
}

glm::mat4 IntroCinematicDirector::shipModelMatrix() const {
  glm::mat4 m(1.0f);
  if (m_timer < 3.5f) {
    m = glm::translate(m, glm::vec3(0.0f, 0.0f, 5000.0f));
    return m;
  }

  const float tShip = m_timer - 3.5f;
  const float zPos = 54.0f - tShip * 4.7f;
  const float xPos = -6.0f + tShip * 2.1f;
  const float yPos = 5.5f - std::sin(tShip * 0.4f) * 0.8f;

  m = glm::translate(m, glm::vec3(xPos, yPos, zPos));
  const float bank = -std::sin(tShip * 0.45f) * 0.25f;
  m = glm::rotate(m, 3.14159265f - 0.42f, glm::vec3(0.0f, 1.0f, 0.0f));
  m = glm::rotate(m, bank, glm::vec3(0.0f, 0.0f, 1.0f));
  m = glm::scale(m, glm::vec3(5.5f));
  return m;
}

glm::mat4 IntroCinematicDirector::megastructureModelMatrix() const {
  glm::mat4 m(1.0f);
  // Positioned right of Kerr BH so it sits in the right third during orbit & bridge reveal.
  m = glm::translate(m, glm::vec3(14.0f, 5.5f, 20.0f));
  const float rot = m_timer * 0.045f;
  m = glm::rotate(m, rot, glm::vec3(0.0f, 0.0f, 1.0f));
  m = glm::rotate(m, 0.45f, glm::vec3(1.0f, 0.0f, 0.0f));
  m = glm::scale(m, glm::vec3(0.85f));
  return m;
}

glm::mat4 IntroCinematicDirector::bridgeModelMatrix() const {
  glm::mat4 m(1.0f);
  // Rotate 180 deg around Y so the panoramic canopy opens towards -Z (Kerr BH & station),
  // and offset to Z=50 outside the event horizon.
  m = glm::translate(m, glm::vec3(0.0f, 1.0f, 50.0f));
  m = glm::rotate(m, 3.14159265f, glm::vec3(0.0f, 1.0f, 0.0f));
  m = glm::scale(m, glm::vec3(1.6f));
  return m;
}

bool IntroCinematicDirector::consumeWarpBoomTrigger() {
  bool val = m_triggerWarpBoom;
  m_triggerWarpBoom = false;
  return val;
}

bool IntroCinematicDirector::consumeEngineWhooshTrigger() {
  bool val = m_triggerEngineWhoosh;
  m_triggerEngineWhoosh = false;
  return val;
}

bool IntroCinematicDirector::consumeBridgeOnlineTrigger() {
  bool val = m_triggerBridgeOnline;
  m_triggerBridgeOnline = false;
  return val;
}

bool IntroCinematicDirector::consumeTitleChimeTrigger() {
  bool val = m_triggerTitleChime;
  m_triggerTitleChime = false;
  return val;
}

bool IntroCinematicDirector::consumeBassDropTrigger() {
  bool val = m_triggerBassDrop;
  m_triggerBassDrop = false;
  return val;
}

void IntroCinematicDirector::drawHUD(float screenW, float screenH,
                                     ImFont* titleFont, ImFont* bodyFont) {
#ifndef ASTRO_NO_IMGUI
  if (!m_active || m_phase == IntroPhase::Finished) return;

  ImDrawList* dl = ImGui::GetForegroundDrawList();
  ImFont* fTitle = titleFont ? titleFont : ImGui::GetFont();
  ImFont* fBody = bodyFont ? bodyFont : ImGui::GetFont();
  const float bodySz = bodyFont ? std::clamp(screenH * 0.022f, 15.0f, 20.0f) : ImGui::GetFontSize();

  // 1. Cinematic 2.39:1 letterbox bars
  float targetAspect = 2.39f;
  float currentAspect = screenW / std::max(screenH, 1.0f);
  float barH = screenH * 0.08f;
  if (currentAspect > 0.0f && currentAspect < targetAspect) {
    float idealH = screenW / targetAspect;
    barH = std::max((screenH - idealH) * 0.5f, screenH * 0.08f);
  }
  dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, barH), IM_COL32(0, 0, 0, 255));
  dl->AddRectFilled(ImVec2(0.0f, screenH - barH), ImVec2(screenW, screenH), IM_COL32(0, 0, 0, 255));

  // 2. Soft anamorphic lens streak overlay
  if (m_anamorphicStreak > 0.01f) {
    float streakAlpha = std::clamp(m_anamorphicStreak, 0.0f, 1.0f);
    float cy = screenH * 0.48f;
    ImU32 cyanGlow = IM_COL32(0, 220, 255, static_cast<int>(streakAlpha * 50));
    ImU32 coreGlow = IM_COL32(220, 245, 255, static_cast<int>(streakAlpha * 80));
    dl->AddLine(ImVec2(0.0f, cy), ImVec2(screenW, cy), cyanGlow, 2.4f);
    dl->AddLine(ImVec2(screenW * 0.2f, cy), ImVec2(screenW * 0.8f, cy), coreGlow, 1.2f);
  }

  // 3. Screen flash overlay
  if (m_flashIntensity > 0.01f) {
    int fAlpha = static_cast<int>(std::clamp(m_flashIntensity, 0.0f, 1.0f) * 70);
    dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, screenH), IM_COL32(235, 250, 255, fAlpha));
  }

  // 4. Fade to black (on finish / skip)
  if (m_fadeAlpha > 0.01f) {
    int alpha = static_cast<int>(std::clamp(m_fadeAlpha, 0.0f, 1.0f) * 255);
    dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, screenH), IM_COL32(0, 0, 0, alpha));
  }

  // 5. Editorial Title Lockup & Phase Subtitles
  if (m_phase == IntroPhase::TitleGlory) {
    const float tRel = m_timer - 18.0f;
    const float cx = screenW * 0.5f;
    const float cy = screenH * 0.36f;

    const float fadeIn = std::clamp(tRel / 0.55f, 0.0f, 1.0f);
    const float fadeOut = std::clamp((4.0f - tRel) / 0.8f, 0.0f, 1.0f);
    const float masterAlpha = fadeIn * fadeOut;

    dl->AddRectFilledMultiColor(
        ImVec2(0.0f, cy - 85.0f), ImVec2(screenW, cy + 165.0f),
        IM_COL32(3, 6, 12, static_cast<int>(10 * masterAlpha)),
        IM_COL32(3, 6, 12, static_cast<int>(10 * masterAlpha)),
        IM_COL32(3, 6, 12, static_cast<int>(95 * masterAlpha)),
        IM_COL32(3, 6, 12, static_cast<int>(95 * masterAlpha)));

    const float kickerSz = std::clamp(screenH * 0.020f, 14.0f, 18.0f);
    const char* kicker = "SGR A*  //  ORBITAL BRIDGE";
    const ImVec2 kSz = fBody->CalcTextSizeA(kickerSz, 10000.0f, 0.0f, kicker);
    const float kX = cx - kSz.x * 0.5f;
    dl->AddRectFilled(ImVec2(kX - 28.0f, cy - 30.0f + kickerSz * 0.45f),
                      ImVec2(kX - 10.0f, cy - 30.0f + kickerSz * 0.45f + 1.5f),
                      IM_COL32(111, 211, 232, static_cast<int>(220 * masterAlpha)));
    dl->AddText(fBody, kickerSz, ImVec2(kX, cy - 30.0f),
                IM_COL32(111, 211, 232, static_cast<int>(230 * masterAlpha)), kicker);

    const float lineProgress = std::clamp(tRel / 0.60f, 0.0f, 1.0f);
    const float lineEase = 1.0f - std::pow(1.0f - lineProgress, 3.0f);
    const float titleFontSz = titleFont ? std::clamp(screenH * 0.068f, 36.0f, 54.0f) : ImGui::GetFontSize() * 2.2f;
    const float halfSpan = (screenW * 0.20f) * lineEase;
    const float ruleY = cy + titleFontSz + 10.0f;

    if (halfSpan > 1.0f) {
      dl->AddLine(ImVec2(cx - halfSpan, ruleY), ImVec2(cx + halfSpan, ruleY),
                  IM_COL32(111, 211, 232, static_cast<int>(150 * masterAlpha)), 1.2f);
      dl->AddLine(ImVec2(cx - halfSpan * 0.45f, ruleY), ImVec2(cx + halfSpan * 0.45f, ruleY),
                  IM_COL32(235, 248, 255, static_cast<int>(220 * masterAlpha)), 1.2f);
    }

    const char* fullTitle = "A S T R O  -  V O Y A G E R";
    const ImVec2 tSz = fTitle->CalcTextSizeA(titleFontSz, 10000.0f, 0.0f, fullTitle);
    const float textStartX = cx - tSz.x * 0.5f;
    dl->AddText(fTitle, titleFontSz, ImVec2(textStartX, cy + 2.0f),
                IM_COL32(0, 0, 0, static_cast<int>(170 * masterAlpha)), fullTitle);
    dl->AddText(fTitle, titleFontSz, ImVec2(textStartX, cy),
                IM_COL32(244, 249, 253, static_cast<int>(255 * masterAlpha)), fullTitle);

    if (tRel >= 0.45f) {
      const float subAlpha = std::clamp((tRel - 0.45f) / 0.6f, 0.0f, 1.0f) * masterAlpha;
      const char* sub = "ALCUBIERRE WARP TRANSIT  //  EXPEDITION CHRONICLE";
      const ImVec2 sSz = fBody->CalcTextSizeA(bodySz, 10000.0f, 0.0f, sub);
      dl->AddText(fBody, bodySz, ImVec2(cx - sSz.x * 0.5f, ruleY + 14.0f),
                  IM_COL32(184, 206, 224, static_cast<int>(220 * subAlpha)), sub);
    }

    if (tRel >= 1.1f) {
      const float prAlpha = std::clamp((tRel - 1.1f) / 0.5f, 0.0f, 1.0f) * masterAlpha;
      const char* prompt = "[ ПРОБЕЛ / ENTER — ВХОД В МЕНЮ ]";
      const float prSz = std::max(13.0f, bodySz * 0.85f);
      const ImVec2 pSz = fBody->CalcTextSizeA(prSz, 10000.0f, 0.0f, prompt);
      const float pulse = 0.72f + 0.28f * std::sin(m_timer * 3.5f);
      dl->AddText(fBody, prSz, ImVec2(cx - pSz.x * 0.5f, ruleY + 52.0f),
                  IM_COL32(199, 165, 106, static_cast<int>(230 * prAlpha * pulse)), prompt);
    }
  } else {
    const char* caption = nullptr;
    ImU32 capCol = IM_COL32(180, 215, 235, 200);
    if (m_phase == IntroPhase::VoidDrift) {
      caption = "СЕКТОР 01  //  ГРАВИТАЦИОННАЯ СИНГУЛЯРНОСТЬ КЕРРА";
      capCol = IM_COL32(184, 210, 232, 210);
    } else if (m_phase == IntroPhase::WarpBurst) {
      caption = "ВСПЫШКА МЕТРИКИ АЛЬКУБЬЕРРЕ  //  WARP COLLAPSE";
      capCol = IM_COL32(111, 211, 232, 230);
    } else if (m_phase == IntroPhase::ShipPass) {
      caption = "КРЕЙСЕР ДАЛЬНЕЙ РАЗВЕДКИ  //  РЕЛЯТИВИСТСКИЙ ПРОЛЁТ";
      capCol = IM_COL32(210, 232, 246, 220);
    } else if (m_phase == IntroPhase::MegastructureReveal) {
      caption = "ОРБИТАЛЬНАЯ МЕГАСТРУКТУРА ЦЕНТАВРА  //  ДОКОВЫЙ КОНТУР";
      capCol = IM_COL32(224, 186, 124, 225);
    } else if (m_phase == IntroPhase::BridgeEnter) {
      caption = "КОМАНДНЫЙ МОСТИК  //  ВСЕ СИСТЕМЫ В НОРМЕ";
      capCol = IM_COL32(140, 228, 196, 225);
    }
    if (caption) {
      const float phaseFade = std::min(std::clamp(m_phaseProgress / 0.15f, 0.0f, 1.0f),
                                       std::clamp((1.0f - m_phaseProgress) / 0.15f, 0.0f, 1.0f));
      const ImU32 fadeCol = (capCol & 0x00FFFFFFu) |
                            (static_cast<ImU32>(static_cast<int>(((capCol >> 24) & 0xFF) * phaseFade)) << 24);
      const ImVec2 cSz = fBody->CalcTextSizeA(bodySz, 10000.0f, 0.0f, caption);
      const float capY = screenH - barH * 0.5f - cSz.y * 0.5f;
      dl->AddText(fBody, bodySz, ImVec2((screenW - cSz.x) * 0.5f, capY), fadeCol, caption);
    }
  }

  // 6. Discreet Skip Hint in top right letterbox
  const char* skipHint = "[ ПРОБЕЛ / ESC : ПРОПУСК ]";
  const float skipSz = std::max(13.0f, bodySz * 0.85f);
  const ImVec2 skSz = fBody->CalcTextSizeA(skipSz, 10000.0f, 0.0f, skipHint);
  dl->AddText(fBody, skipSz, ImVec2(screenW - skSz.x - 24.0f, (barH - skSz.y) * 0.5f),
              IM_COL32(160, 188, 212, 165), skipHint);

  // Subtle progress hairline along bottom edge of top letterbox
  dl->AddLine(ImVec2(0.0f, barH), ImVec2(screenW * m_totalProgress, barH),
              IM_COL32(111, 211, 232, 175), 1.5f);
#else
  (void)screenW;
  (void)screenH;
  (void)titleFont;
  (void)bodyFont;
#endif
}

}  // namespace astro
