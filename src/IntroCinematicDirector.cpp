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

  m_triggerWarpBoom = false;
  m_triggerEngineWhoosh = false;
  m_triggerBridgeOnline = false;
  m_triggerTitleChime = false;

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
  // Catmull-Rom requires at least 4 keyframes; we define keyframes spanning 0.0s to 22.0s
  m_keyframes = {
      // Index 0: Pre-anchor (for smooth Catmull-Rom curvature)
      {-2.0f, glm::vec3(0.0f, 5.0f, 90.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 58.0f, 3.0f},
      // Index 1: Phase 0 Void Drift start
      {0.0f, glm::vec3(0.0f, 4.0f, 75.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.05f, 0.998f, 0.0f), 60.0f, 2.0f},
      // Index 2: Phase 0 -> Phase 1 Warp Burst trigger
      {3.5f, glm::vec3(0.0f, 2.5f, 42.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 62.0f, 0.0f},
      // Index 3: Phase 1 Warp Exit & Ship Burst
      {7.0f, glm::vec3(-12.0f, 6.0f, 22.0f), glm::vec3(0.0f, 1.2f, 4.0f), glm::vec3(0.08f, 0.99f, 0.08f), 68.0f, -4.0f},
      // Index 4: Phase 2 Ship Pass & Flank track
      {10.5f, glm::vec3(14.0f, 8.5f, -8.0f), glm::vec3(0.0f, 2.0f, -28.0f), glm::vec3(-0.06f, 0.99f, 0.0f), 65.0f, 3.0f},
      // Index 5: Phase 3 Megastructure Reveal & Approach
      {14.5f, glm::vec3(3.2f, 3.8f, -2.2f), glm::vec3(0.0f, 1.6f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), 62.0f, 0.0f},
      // Index 6: Phase 4 Canopy Zoom into 3D Bridge
      {18.0f, glm::vec3(0.0f, 1.45f, 1.95f), glm::vec3(0.0f, 1.15f, -1.8f), glm::vec3(0.0f, 1.0f, 0.0f), 72.0f, 0.0f},
      // Index 7: Phase 5 Hero Title Glory & Settle
      {22.0f, glm::vec3(0.0f, 1.45f, 1.90f), glm::vec3(0.0f, 1.15f, -2.0f), glm::vec3(0.0f, 1.0f, 0.0f), 70.0f, 0.0f},
      // Index 8: Post-anchor
      {24.0f, glm::vec3(0.0f, 1.45f, 1.85f), glm::vec3(0.0f, 1.15f, -2.2f), glm::vec3(0.0f, 1.0f, 0.0f), 70.0f, 0.0f}
  };
}

glm::vec3 IntroCinematicDirector::catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                                            const glm::vec3& p2, const glm::vec3& p3, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  return 0.5f * ((2.0f * p1) +
                 (-p0 + p2) * t +
                 (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                 (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

void IntroCinematicDirector::evaluateSpline(float t) {
  if (m_keyframes.size() < 4) return;

  // Find segment [idx, idx+1]
  size_t idx = 1;
  while (idx + 2 < m_keyframes.size() && m_keyframes[idx + 1].time < t) {
    ++idx;
  }

  const auto& k0 = m_keyframes[idx - 1];
  const auto& k1 = m_keyframes[idx];
  const auto& k2 = m_keyframes[idx + 1];
  const auto& k3 = m_keyframes[idx + 2];

  float segDuration = std::max(k2.time - k1.time, 0.001f);
  float u = std::clamp((t - k1.time) / segDuration, 0.0f, 1.0f);

  m_camPos = catmullRom(k0.pos, k1.pos, k2.pos, k3.pos, u);
  m_camTarget = catmullRom(k0.target, k1.target, k2.target, k3.target, u);
  m_camUp = glm::normalize(catmullRom(k0.up, k1.up, k2.up, k3.up, u));
  m_camFov = glm::mix(k1.fov, k2.fov, u);
  m_camRoll = glm::mix(k1.roll, k2.roll, u);
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
    // Flash spike then decay
    float flashTime = m_timer - 3.5f;
    if (flashTime < 0.3f) {
      m_flashIntensity = flashTime / 0.3f;
    } else {
      m_flashIntensity = std::max(0.0f, 1.0f - (flashTime - 0.3f) / 1.8f);
    }
    m_warpDistortion = std::max(0.0f, std::sin(m_phaseProgress * 3.14159f) * 1.2f);
    m_anamorphicStreak = m_flashIntensity * 0.85f;
  } else if (m_timer < 10.5f) {
    m_phase = IntroPhase::ShipPass;
    m_phaseProgress = (m_timer - 7.0f) / 3.5f;
    if (!m_firedEngineWhoosh && m_timer >= 7.1f) {
      m_firedEngineWhoosh = true;
      m_triggerEngineWhoosh = true;
    }
    m_warpDistortion = 0.0f;
    m_flashIntensity = 0.0f;
    m_anamorphicStreak = std::max(0.0f, 0.4f * (1.0f - m_phaseProgress));
  } else if (m_timer < 14.5f) {
    m_phase = IntroPhase::MegastructureReveal;
    m_phaseProgress = (m_timer - 10.5f) / 4.0f;
    m_warpDistortion = 0.0f;
    m_flashIntensity = 0.0f;
    m_anamorphicStreak = 0.15f;
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
      m_flashIntensity = std::sin((canopyHit - 0.4f) / 0.5f * 3.14159f) * 0.45f;
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
    m_anamorphicStreak = 0.75f;
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
  // Ship emerges from warp at t=3.5s and speeds past origin
  if (m_timer < 3.5f) {
    // Hidden before warp emergence
    m = glm::translate(m, glm::vec3(0.0f, 0.0f, 5000.0f));
    return m;
  }

  float tShip = m_timer - 3.5f;
  // High speed deceleration from warp
  float zPos = 80.0f - (tShip * 16.0f) + (tShip * tShip * 0.35f);
  float xPos = -std::sin(tShip * 0.4f) * 6.0f;
  float yPos = 1.0f + std::cos(tShip * 0.35f) * 1.5f;

  m = glm::translate(m, glm::vec3(xPos, yPos, zPos));
  // Ship bank angle
  float bank = -std::sin(tShip * 0.45f) * 0.35f;
  m = glm::rotate(m, bank, glm::vec3(0.0f, 0.0f, 1.0f));
  m = glm::rotate(m, 3.14159f, glm::vec3(0.0f, 1.0f, 0.0f)); // Pointing forward (-Z)
  return m;
}

glm::mat4 IntroCinematicDirector::megastructureModelMatrix() const {
  glm::mat4 m(1.0f);
  // Placed majestically at planetary orbit distance
  m = glm::translate(m, glm::vec3(80.0f, 40.0f, -220.0f));
  // Slow continuous majestic axial rotation
  float rot = m_timer * 0.045f;
  m = glm::rotate(m, rot, glm::vec3(0.0f, 0.0f, 1.0f));
  m = glm::rotate(m, 0.45f, glm::vec3(1.0f, 0.0f, 0.0f));
  m = glm::scale(m, glm::vec3(8.5f));
  return m;
}

glm::mat4 IntroCinematicDirector::bridgeModelMatrix() const {
  glm::mat4 m(1.0f);
  // When inside the bridge (Phase 4 & 5), the bridge is aligned with the camera world
  m = glm::translate(m, glm::vec3(0.0f, 0.0f, 0.0f));
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

void IntroCinematicDirector::drawHUD(float screenW, float screenH) {
#ifndef ASTRO_NO_IMGUI
  if (!m_active || m_phase == IntroPhase::Finished) return;

  ImDrawList* dl = ImGui::GetForegroundDrawList();

  // 1. Cinematic 2.39:1 letterbox bars
  float targetAspect = 2.39f;
  float currentAspect = screenW / std::max(screenH, 1.0f);
  if (currentAspect > 0.0f) {
    float barH = 0.0f;
    if (currentAspect < targetAspect) {
      // Top & bottom black bars
      float idealH = screenW / targetAspect;
      barH = std::max((screenH - idealH) * 0.5f, screenH * 0.08f);
    } else {
      barH = screenH * 0.08f;
    }
    // Top bar
    dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, barH), IM_COL32(0, 0, 0, 255));
    // Bottom bar
    dl->AddRectFilled(ImVec2(0.0f, screenH - barH), ImVec2(screenW, screenH), IM_COL32(0, 0, 0, 255));
  }

  // 2. Anamorphic lens streak overlay
  if (m_anamorphicStreak > 0.01f) {
    float streakAlpha = std::clamp(m_anamorphicStreak, 0.0f, 1.0f);
    float cy = screenH * 0.48f;
    ImU32 cyanGlow = IM_COL32(0, 220, 255, static_cast<int>(streakAlpha * 140));
    ImU32 coreGlow = IM_COL32(220, 245, 255, static_cast<int>(streakAlpha * 200));

    dl->AddLine(ImVec2(0.0f, cy), ImVec2(screenW, cy), cyanGlow, 2.8f);
    dl->AddLine(ImVec2(screenW * 0.2f, cy), ImVec2(screenW * 0.8f, cy), coreGlow, 1.5f);
  }

  // 3. Screen flash overlay
  if (m_flashIntensity > 0.01f) {
    int fAlpha = static_cast<int>(std::clamp(m_flashIntensity, 0.0f, 1.0f) * 230);
    dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, screenH), IM_COL32(235, 250, 255, fAlpha));
  }

  // 4. Fade to black (on finish / skip)
  if (m_fadeAlpha > 0.01f) {
    int alpha = static_cast<int>(std::clamp(m_fadeAlpha, 0.0f, 1.0f) * 255);
    dl->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(screenW, screenH), IM_COL32(0, 0, 0, alpha));
  }

  // 5. Telemetry & Title Displays
  if (m_phase == IntroPhase::TitleGlory) {
    // Grand cinematic title
    const char* title = "ASTRO - VOYAGER";
    const char* sub = "// ALCUBIERRE WARP TRANSIT & ORBITAL EXPEDITIONS //";
    const char* prompt = "[ НАЖМИТЕ SPACE / ENTER / ESC ДЛЯ ВХОДА В МЕНЮ ]";

    float titleW = ImGui::CalcTextSize(title).x;
    float subW = ImGui::CalcTextSize(sub).x;
    float promptW = ImGui::CalcTextSize(prompt).x;

    float cy = screenH * 0.42f;

    // Glowing title backdrop shadow
    dl->AddText(ImVec2((screenW - titleW) * 0.5f + 2.0f, cy + 2.0f),
                IM_COL32(0, 180, 255, 180), title);
    dl->AddText(ImVec2((screenW - titleW) * 0.5f, cy),
                IM_COL32(245, 252, 255, 255), title);

    // Subtitle
    dl->AddText(ImVec2((screenW - subW) * 0.5f, cy + 32.0f),
                IM_COL32(140, 220, 245, 220), sub);

    // Blinking prompt
    float blink = std::sin(m_timer * 5.0f) * 0.5f + 0.5f;
    int pAlpha = static_cast<int>(120 + blink * 135);
    dl->AddText(ImVec2((screenW - promptW) * 0.5f, cy + 85.0f),
                IM_COL32(255, 215, 120, pAlpha), prompt);
  } else if (m_phase == IntroPhase::VoidDrift) {
    const char* prologue = "СЕКТОР 01: ГРАВИТАЦИОННАЯ СИНГУЛЯРНОСТЬ КЕРРА";
    float w = ImGui::CalcTextSize(prologue).x;
    dl->AddText(ImVec2((screenW - w) * 0.5f, screenH * 0.88f),
                IM_COL32(180, 215, 235, 180), prologue);
  } else if (m_phase == IntroPhase::WarpBurst) {
    const char* warpTxt = "ВСПЫШКА МЕТРИКИ АЛЬКУБЬЕРРЕ // WARP COLLAPSE EVENT";
    float w = ImGui::CalcTextSize(warpTxt).x;
    dl->AddText(ImVec2((screenW - w) * 0.5f, screenH * 0.88f),
                IM_COL32(0, 240, 255, 230), warpTxt);
  } else if (m_phase == IntroPhase::MegastructureReveal) {
    const char* stTxt = "ОРБИТАЛЬНАЯ МЕГАСТРУКТУРА ЦЕНТАВРА // DOCKING PROTOCOL READY";
    float w = ImGui::CalcTextSize(stTxt).x;
    dl->AddText(ImVec2((screenW - w) * 0.5f, screenH * 0.88f),
                IM_COL32(255, 220, 140, 220), stTxt);
  } else if (m_phase == IntroPhase::BridgeEnter) {
    const char* brTxt = "КОМАНДНЫЙ МОСТИК // ВСЕ СИСТЕМЫ В НОРМЕ";
    float w = ImGui::CalcTextSize(brTxt).x;
    dl->AddText(ImVec2((screenW - w) * 0.5f, screenH * 0.88f),
                IM_COL32(80, 245, 180, 230), brTxt);
  }

  // 6. Discreet Skip Hint in top right
  const char* skipHint = "[ ПРОБЕЛ / ESC : ПРОПУСК ]";
  float skipW = ImGui::CalcTextSize(skipHint).x;
  dl->AddText(ImVec2(screenW - skipW - 24.0f, 22.0f),
              IM_COL32(160, 190, 215, 160), skipHint);

  // Subtle progress line at the bottom edge of top bar
  float barH = screenH * 0.08f;
  dl->AddLine(ImVec2(0.0f, barH), ImVec2(screenW * m_totalProgress, barH),
              IM_COL32(0, 210, 255, 180), 1.8f);
#else
  (void)screenW;
  (void)screenH;
#endif
}

}  // namespace astro
