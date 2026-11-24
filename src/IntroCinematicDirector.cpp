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
      {10.5f, glm::vec3(14.0f, 8.5f, -8.0f), glm::vec3(0.0f, 8.0f, -45.0f), glm::vec3(-0.06f, 0.99f, 0.0f), 65.0f, 3.0f},
      // Index 5: Phase 3 Megastructure Reveal & Approach
      {14.5f, glm::vec3(3.2f, 3.8f, -2.2f), glm::vec3(0.0f, 12.0f, -45.0f), glm::vec3(0.0f, 1.0f, 0.0f), 62.0f, 0.0f},
      // Index 6: Phase 4 Canopy Zoom into 3D Bridge
      {18.0f, glm::vec3(0.0f, 1.45f, 1.95f), glm::vec3(0.0f, 1.15f, -1.8f), glm::vec3(0.0f, 1.0f, 0.0f), 72.0f, 0.0f},
      // Index 7: Phase 5 Hero Title Glory & Settle
      {22.0f, glm::vec3(0.0f, 1.45f, 1.90f), glm::vec3(0.0f, 1.15f, -2.0f), glm::vec3(0.0f, 1.0f, 0.0f), 70.0f, 0.0f},
      // Index 8: Post-anchor
      {24.0f, glm::vec3(0.0f, 1.45f, 1.85f), glm::vec3(0.0f, 1.15f, -2.2f), glm::vec3(0.0f, 1.0f, 0.0f), 70.0f, 0.0f}
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
  m = glm::scale(m, glm::vec3(6.0f));
  return m;
}

glm::mat4 IntroCinematicDirector::megastructureModelMatrix() const {
  glm::mat4 m(1.0f);
  // Placed on the reveal axis so the 10.5–14.5s camera pan keeps it in frame.
  m = glm::translate(m, glm::vec3(0.0f, 22.0f, -95.0f));
  // Slow continuous majestic axial rotation
  float rot = m_timer * 0.045f;
  m = glm::rotate(m, rot, glm::vec3(0.0f, 0.0f, 1.0f));
  m = glm::rotate(m, 0.45f, glm::vec3(1.0f, 0.0f, 0.0f));
  m = glm::scale(m, glm::vec3(5.0f));
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

bool IntroCinematicDirector::consumeBassDropTrigger() {
  bool val = m_triggerBassDrop;
  m_triggerBassDrop = false;
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
    ImU32 cyanGlow = IM_COL32(0, 220, 255, static_cast<int>(streakAlpha * 50));
    ImU32 coreGlow = IM_COL32(220, 245, 255, static_cast<int>(streakAlpha * 80));

    dl->AddLine(ImVec2(0.0f, cy), ImVec2(screenW, cy), cyanGlow, 2.8f);
    dl->AddLine(ImVec2(screenW * 0.2f, cy), ImVec2(screenW * 0.8f, cy), coreGlow, 1.5f);
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

  // 5. Telemetry & Title Displays
  if (m_phase == IntroPhase::TitleGlory) {
    float tRel = m_timer - 18.0f;
    float cy = screenH * 0.42f;
    float cx = screenW * 0.5f;

    // Technical Corner Framing Brackets
    float marginX = screenW * 0.06f;
    float marginY = screenH * 0.12f;
    float bracketLen = 22.0f;
    ImU32 bracketColor = IM_COL32(0, 210, 255, 120);

    // Top-left
    dl->AddLine(ImVec2(marginX, marginY), ImVec2(marginX + bracketLen, marginY), bracketColor, 1.5f);
    dl->AddLine(ImVec2(marginX, marginY), ImVec2(marginX, marginY + bracketLen), bracketColor, 1.5f);
    // Top-right
    dl->AddLine(ImVec2(screenW - marginX, marginY), ImVec2(screenW - marginX - bracketLen, marginY), bracketColor, 1.5f);
    dl->AddLine(ImVec2(screenW - marginX, marginY), ImVec2(screenW - marginX, marginY + bracketLen), bracketColor, 1.5f);
    // Bottom-left
    dl->AddLine(ImVec2(marginX, screenH - marginY), ImVec2(marginX + bracketLen, screenH - marginY), bracketColor, 1.5f);
    dl->AddLine(ImVec2(marginX, screenH - marginY), ImVec2(marginX, screenH - marginY - bracketLen), bracketColor, 1.5f);
    // Bottom-right
    dl->AddLine(ImVec2(screenW - marginX, screenH - marginY), ImVec2(screenW - marginX - bracketLen, screenH - marginY), bracketColor, 1.5f);
    dl->AddLine(ImVec2(screenW - marginX, screenH - marginY), ImVec2(screenW - marginX, screenH - marginY - bracketLen), bracketColor, 1.5f);

    // Dynamic Expanding Technical Rule Line (0.0s to 0.45s)
    float lineProgress = std::clamp(tRel / 0.45f, 0.0f, 1.0f);
    float lineEase = 1.0f - std::pow(1.0f - lineProgress, 3.0f);
    float halfSpan = (screenW * 0.28f) * lineEase;
    float ruleY = cy - 14.0f;

    if (halfSpan > 1.0f) {
      dl->AddLine(ImVec2(cx - halfSpan, ruleY), ImVec2(cx + halfSpan, ruleY),
                  IM_COL32(0, 180, 255, 140), 2.2f);
      dl->AddLine(ImVec2(cx - halfSpan * 0.6f, ruleY), ImVec2(cx + halfSpan * 0.6f, ruleY),
                  IM_COL32(230, 250, 255, 220), 1.0f);
      // End ticks
      dl->AddLine(ImVec2(cx - halfSpan, ruleY - 4.0f), ImVec2(cx - halfSpan, ruleY + 4.0f),
                  IM_COL32(0, 220, 255, 180), 1.5f);
      dl->AddLine(ImVec2(cx + halfSpan, ruleY - 4.0f), ImVec2(cx + halfSpan, ruleY + 4.0f),
                  IM_COL32(0, 220, 255, 180), 1.5f);
    }

    // Telemetry Badges (fades in 0.5s to 1.1s)
    if (tRel >= 0.5f) {
      float badgeAlpha = std::clamp((tRel - 0.5f) / 0.6f, 0.0f, 1.0f);
      int bAlpha = static_cast<int>(badgeAlpha * 210);
      const char* badges = "[ WARP DRIVE: ONLINE ]    [ ORBIT: CAPTURED ]    [ SYSTEMS: NOMINAL ]";
      float badgeW = ImGui::CalcTextSize(badges).x;
      dl->AddText(ImVec2((screenW - badgeW) * 0.5f, ruleY - 24.0f),
                  IM_COL32(0, 225, 255, bAlpha), badges);
    }

    // Typewriter Write-On Title Reveal (0.22s to 1.35s)
    const std::string fullTitle = "A S T R O - V O Y A G E R";
    size_t charCount = fullTitle.size();
    if (tRel < 1.35f) {
      float typeP = std::clamp((tRel - 0.22f) / 1.13f, 0.0f, 1.0f);
      float typeEase = typeP * typeP * (3.0f - 2.0f * typeP);
      charCount = static_cast<size_t>(typeEase * fullTitle.size());
    }
    std::string visibleTitle = fullTitle.substr(0, charCount);

    if (!visibleTitle.empty()) {
      float fullW = ImGui::CalcTextSize(fullTitle.c_str()).x;
      float textStartX = (screenW - fullW) * 0.5f;

      // Glow drop shadow
      dl->AddText(ImVec2(textStartX + 2.0f, cy + 2.0f),
                  IM_COL32(0, 180, 255, 180), visibleTitle.c_str());
      dl->AddText(ImVec2(textStartX, cy),
                  IM_COL32(245, 252, 255, 255), visibleTitle.c_str());

      // Blinking typewriter cursor during write-on
      if (tRel < 1.35f && charCount < fullTitle.size()) {
        float curW = ImGui::CalcTextSize(visibleTitle.c_str()).x;
        dl->AddRectFilled(ImVec2(textStartX + curW + 4.0f, cy + 2.0f),
                          ImVec2(textStartX + curW + 12.0f, cy + 18.0f),
                          IM_COL32(0, 240, 255, 220));
      }
    }

    // Subtitle (fades in 0.8s to 1.4s)
    if (tRel >= 0.8f) {
      float subAlpha = std::clamp((tRel - 0.8f) / 0.6f, 0.0f, 1.0f);
      int sAlpha = static_cast<int>(subAlpha * 220);
      const char* sub = "// ALCUBIERRE WARP TRANSIT & ORBITAL EXPEDITIONS //";
      float subW = ImGui::CalcTextSize(sub).x;
      dl->AddText(ImVec2((screenW - subW) * 0.5f, cy + 34.0f),
                  IM_COL32(140, 220, 245, sAlpha), sub);
    }

    // Blinking prompt (fades in 1.4s onward)
    if (tRel >= 1.4f) {
      const char* prompt = "[ НАЖМИТЕ SPACE / ENTER / ESC ДЛЯ ВХОДА В МЕНЮ ]";
      float promptW = ImGui::CalcTextSize(prompt).x;
      float blink = std::sin(m_timer * 5.0f) * 0.5f + 0.5f;
      int pAlpha = static_cast<int>(120 + blink * 135);
      dl->AddText(ImVec2((screenW - promptW) * 0.5f, cy + 85.0f),
                  IM_COL32(255, 215, 120, pAlpha), prompt);
    }
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
