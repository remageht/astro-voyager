#include "CockpitController.h"

namespace astro {

CockpitController::CockpitController() {
  reset();
}

void CockpitController::reset() {
  m_prevPos = glm::vec3(0.0f);
  m_prevFwd = glm::vec3(0.0f, 0.0f, -1.0f);
  m_headOffset = glm::vec3(0.0f);
  m_headOffsetVel = glm::vec3(0.0f);
  m_headTilt = glm::vec3(0.0f);
  m_headTiltVel = glm::vec3(0.0f);
  m_rumble = 0.0f;
  m_autoTint = 0.0f;
  m_hud = CockpitHUDData();
  m_initialized = false;
}

void CockpitController::update(float deltaTime,
                               const glm::vec3& camPos,
                               const glm::vec3& camFwd,
                               const glm::vec3& camRight,
                               const glm::vec3& camUp,
                               float currentSpeed,
                               float throttleInput,
                               bool warpOn,
                               float warpCharge,
                               float hull,
                               float shield,
                               const std::string& targetName,
                               const glm::vec3& targetPos,
                               float sunVisibility) {
  if (deltaTime <= 0.0f || deltaTime > 0.25f) {
    deltaTime = 0.016f;
  }

  if (!m_initialized) {
    m_prevPos = camPos;
    m_prevFwd = camFwd;
    m_initialized = true;
  }

  // Linear velocity and acceleration
  glm::vec3 worldVel = (camPos - m_prevPos) / deltaTime;
  glm::vec3 relVel(glm::dot(worldVel, camRight),
                   glm::dot(worldVel, camUp),
                   glm::dot(worldVel, camFwd));

  // Angular rate (turn rate)
  glm::vec3 deltaFwd = camFwd - m_prevFwd;
  float turnYaw = -glm::dot(deltaFwd, camRight) / deltaTime;
  float turnPitch = glm::dot(deltaFwd, camUp) / deltaTime;

  m_prevPos = camPos;
  m_prevFwd = camFwd;

  // Spring-damper for head inertia (G-forces)
  // Target head displacement is opposed to acceleration and turns
  glm::vec3 targetOffset(0.0f);
  targetOffset.x = std::clamp(-turnYaw * 0.015f, -0.06f, 0.06f);
  targetOffset.y = std::clamp(-turnPitch * 0.012f, -0.04f, 0.04f);
  targetOffset.z = std::clamp((throttleInput - 0.5f) * 0.05f + (warpOn ? 0.04f : 0.0f), -0.06f, 0.06f);

  constexpr float kSpringK = 35.0f;
  constexpr float kDamping = 8.0f;
  glm::vec3 offsetForce = (targetOffset - m_headOffset) * kSpringK - m_headOffsetVel * kDamping;
  m_headOffsetVel += offsetForce * deltaTime;
  m_headOffset += m_headOffsetVel * deltaTime;

  // Head tilt rotation (banking into turns)
  glm::vec3 targetTilt(0.0f);
  targetTilt.x = std::clamp(turnPitch * 0.08f, -0.12f, 0.12f);   // pitch lag
  targetTilt.y = std::clamp(turnYaw * 0.10f, -0.15f, 0.15f);     // yaw look-into-turn
  targetTilt.z = std::clamp(-turnYaw * 0.12f, -0.18f, 0.18f);    // roll bank

  glm::vec3 tiltForce = (targetTilt - m_headTilt) * 28.0f - m_headTiltVel * 7.5f;
  m_headTiltVel += tiltForce * deltaTime;
  m_headTilt += m_headTiltVel * deltaTime;

  // Canopy vibration / rumble
  float targetRumble = 0.0f;
  if (warpOn) {
    targetRumble = 0.35f + 0.15f * warpCharge;
  } else if (throttleInput > 1.0f) {
    targetRumble = 0.20f * (throttleInput - 1.0f);
  }
  m_rumble += (targetRumble - m_rumble) * (1.0f - std::exp(-deltaTime * 6.0f));

  // Photochromic auto-tint (protect pilot eyes from direct stellar flux)
  float targetTint = std::clamp(sunVisibility, 0.0f, 1.0f);
  m_autoTint += (targetTint - m_autoTint) * (1.0f - std::exp(-deltaTime * 3.0f));

  // Telemetry compilation
  m_hud.pitchDeg = std::asin(std::clamp(camFwd.y, -1.0f, 1.0f)) * (180.0f / 3.14159265f);
  m_hud.yawDeg = std::atan2(camFwd.x, camFwd.z) * (180.0f / 3.14159265f);
  if (m_hud.yawDeg < 0.0f) m_hud.yawDeg += 360.0f;

  // Roll calculated from horizon vs right vector
  glm::vec3 horizRight = glm::normalize(glm::vec3(camRight.x, 0.0f, camRight.z));
  if (glm::length(horizRight) > 0.01f) {
    m_hud.rollDeg = std::asin(std::clamp(camRight.y, -1.0f, 1.0f)) * (180.0f / 3.14159265f);
  } else {
    m_hud.rollDeg = 0.0f;
  }

  m_hud.speed = currentSpeed;
  m_hud.throttle = throttleInput;
  m_hud.warpActive = warpOn;
  m_hud.warpCharge = warpCharge;
  m_hud.hullIntegrity = std::clamp(hull, 0.0f, 1.0f);
  m_hud.shieldIntegrity = std::clamp(shield, 0.0f, 1.0f);
  m_hud.autoTint = m_autoTint;

  // Simulated pilot G-force
  float gAccel = glm::length(offsetForce) * 0.15f + (throttleInput > 1.0f ? 1.8f : 0.0f);
  m_hud.gForce = 1.0f + gAccel;

  // Target relative direction and distance
  m_hud.targetName = targetName;
  if (!targetName.empty()) {
    glm::vec3 toTarget = targetPos - camPos;
    float dist = glm::length(toTarget);
    m_hud.targetDistLy = dist;
    if (dist > 1e-4f) {
      glm::vec3 dir = toTarget / dist;
      float dotFwd = glm::dot(dir, camFwd);
      m_hud.targetAngleDeg = std::acos(std::clamp(dotFwd, -1.0f, 1.0f)) * (180.0f / 3.14159265f);
      m_hud.targetLocked = (m_hud.targetAngleDeg < 15.0f);
    } else {
      m_hud.targetAngleDeg = 0.0f;
      m_hud.targetLocked = true;
    }
  } else {
    m_hud.targetDistLy = 0.0f;
    m_hud.targetAngleDeg = 0.0f;
    m_hud.targetLocked = false;
  }
}

}  // namespace astro
