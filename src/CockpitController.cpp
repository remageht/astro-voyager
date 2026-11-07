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
  m_hullTemp = 0.0f;
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
                               float sunVisibility,
                               const glm::vec3& worldVel,
                               const glm::vec3& nearestBodyPos,
                               float nearestBodyRadius,
                               bool hasAtmosphere) {
  if (deltaTime <= 0.0f || deltaTime > 0.25f) {
    deltaTime = 0.016f;
  }

  if (!m_initialized) {
    m_prevPos = camPos;
    m_prevFwd = camFwd;
    m_initialized = true;
  }

  // Linear velocity and acceleration
  glm::vec3 effectiveVel = (glm::length(worldVel) > 0.001f)
                               ? worldVel
                               : ((camPos - m_prevPos) / deltaTime);
  float spd = glm::length(effectiveVel);
  if (spd < 1e-4f) spd = currentSpeed;

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

  // v4.6.0 Realism: Prograde and Retrograde velocity vector projection
  if (spd > 0.4f) {
    float fwdDot = glm::dot(effectiveVel, camFwd);
    float rightDot = glm::dot(effectiveVel, camRight);
    float upDot = glm::dot(effectiveVel, camUp);

    // Prograde projection onto collimated HUD plane
    if (fwdDot > 0.05f * spd) {
      float projX = (rightDot / fwdDot) * 1.15f;
      float projY = (upDot / fwdDot) * 1.15f;
      float dScreen = std::sqrt(projX * projX + projY * projY);
      if (dScreen < 1.05f) {
        m_hud.progradeVisible = true;
        m_hud.progradeScreenPos = glm::vec2(projX, projY);
      } else {
        m_hud.progradeVisible = false;
      }
    } else {
      m_hud.progradeVisible = false;
    }

    // Retrograde projection (opposite direction)
    if (-fwdDot > 0.05f * spd) {
      float projX = (-rightDot / -fwdDot) * 1.15f;
      float projY = (-upDot / -fwdDot) * 1.15f;
      float dScreen = std::sqrt(projX * projX + projY * projY);
      if (dScreen < 1.05f) {
        m_hud.retrogradeVisible = true;
        m_hud.retrogradeScreenPos = glm::vec2(projX, projY);
      } else {
        m_hud.retrogradeVisible = false;
      }
    } else {
      m_hud.retrogradeVisible = false;
    }
  } else {
    m_hud.progradeVisible = false;
    m_hud.retrogradeVisible = false;
  }

  // v4.6.0 Realism: G-force physics and physiological blackout/redout
  float turnMag = std::abs(turnYaw) + std::abs(turnPitch);
  float linAcc = glm::length(offsetForce) * 0.18f;
  float feltG = 1.0f + linAcc + turnMag * 0.45f + (throttleInput > 1.0f ? 1.6f : 0.0f);
  m_hud.gForce += (feltG - m_hud.gForce) * (1.0f - std::exp(-deltaTime * 5.0f));

  // High positive G (> 3.5 G) produces tunnel-vision blackout
  float targetBlackout = 0.0f;
  if (m_hud.gForce > 3.5f) {
    targetBlackout = std::clamp((m_hud.gForce - 3.5f) / 4.5f, 0.0f, 0.85f);
  }
  m_hud.blackoutK += (targetBlackout - m_hud.blackoutK) * (1.0f - std::exp(-deltaTime * 4.0f));

  // Negative pitch rate / negative vertical G causes redout
  float targetRedout = 0.0f;
  if (turnPitch < -0.55f) {
    targetRedout = std::clamp((-turnPitch - 0.55f) * 0.6f, 0.0f, 0.75f);
  }
  m_hud.redoutK += (targetRedout - m_hud.redoutK) * (1.0f - std::exp(-deltaTime * 4.0f));

  // v4.6.0 Realism: Radar Altimeter AGL & Terrain Clearance Alert
  if (nearestBodyRadius > 0.01f) {
    glm::vec3 relBody = camPos - nearestBodyPos;
    float distToCenter = glm::length(relBody);
    float altitude = distToCenter - nearestBodyRadius;

    // Activate within 1.5 radii above surface
    if (altitude >= 0.0f && altitude < nearestBodyRadius * 1.5f) {
      m_hud.radarAltitudeKm = altitude * 100.0f; // scaled to km

      // Vertical speed (rate of climb / descent along planetary radius)
      if (distToCenter > 1e-4f) {
        glm::vec3 upRadius = relBody / distToCenter;
        float vRad = glm::dot(effectiveVel, upRadius); // negative when descending
        m_hud.verticalSpeedMs = vRad * 50.0f;

        // Terrain proximity alert envelope: low altitude and fast descent
        if (altitude < nearestBodyRadius * 0.18f && vRad < -0.15f) {
          m_hud.terrainAlert = true;
        } else {
          m_hud.terrainAlert = false;
        }
      } else {
        m_hud.verticalSpeedMs = 0.0f;
        m_hud.terrainAlert = false;
      }
    } else {
      m_hud.radarAltitudeKm = -1.0f;
      m_hud.verticalSpeedMs = 0.0f;
      m_hud.terrainAlert = false;
    }
  } else {
    m_hud.radarAltitudeKm = -1.0f;
    m_hud.verticalSpeedMs = 0.0f;
    m_hud.terrainAlert = false;
  }

  // v4.6.0 / v4.9.0 Realism: Atmospheric hypersonic re-entry plasma ionization & thermal modeling
  float currentAtmoDensity = 0.0f;
  if (hasAtmosphere && nearestBodyRadius > 0.01f && m_hud.radarAltitudeKm >= 0.0f) {
    float atmThickness = nearestBodyRadius * 0.22f;
    glm::vec3 relBody = camPos - nearestBodyPos;
    float alt = glm::length(relBody) - nearestBodyRadius;
    if (alt >= 0.0f && alt < atmThickness) {
      float altNorm = alt / atmThickness;
      currentAtmoDensity = std::exp(-altNorm * 3.8f);
      float speedFactor = std::clamp((spd - 8.0f) / 20.0f, 0.0f, 1.0f);
      float targetPlasma = currentAtmoDensity * speedFactor;
      m_hud.entryPlasma += (targetPlasma - m_hud.entryPlasma) * (1.0f - std::exp(-deltaTime * 6.0f));
    } else {
      m_hud.entryPlasma *= std::exp(-deltaTime * 4.0f);
    }
  } else {
    m_hud.entryPlasma *= std::exp(-deltaTime * 4.0f);
  }
  m_hud.atmoDensity = currentAtmoDensity;

  // Thermal dissipation and hull heating
  // Aerodynamic compression heating + engine boost heat, radiated via Stefan-Boltzmann cooling
  float targetHullHeat = std::clamp(m_hud.entryPlasma * 1.15f + (throttleInput > 1.0f ? 0.35f : 0.0f), 0.0f, 1.0f);
  if (targetHullHeat > m_hullTemp) {
    m_hullTemp += (targetHullHeat - m_hullTemp) * (1.0f - std::exp(-deltaTime * 3.5f));
  } else {
    m_hullTemp -= (m_hullTemp - targetHullHeat) * (1.0f - std::exp(-deltaTime * 0.8f));
  }
  m_hud.hullTempNorm = std::clamp(m_hullTemp, 0.0f, 1.0f);

  // Atmospheric buffeting / violent cockpit shaking during hypersonic re-entry
  if (m_hud.entryPlasma > 0.02f) {
    float atmoShake = m_hud.entryPlasma * 0.85f;
    m_rumble = std::max(m_rumble, atmoShake);
  }

  // Target relative direction, distance, and 2D radar disk projection
  m_hud.targetName = targetName;
  if (!targetName.empty()) {
    glm::vec3 toTarget = targetPos - camPos;
    float dist = glm::length(toTarget);
    m_hud.targetDistLy = dist;
    if (dist > 1e-4f) {
      glm::vec3 dir = toTarget / dist;
      float dotFwd = glm::dot(dir, camFwd);
      float dotRight = glm::dot(dir, camRight);
      m_hud.targetAngleDeg = std::acos(std::clamp(dotFwd, -1.0f, 1.0f)) * (180.0f / 3.14159265f);
      m_hud.targetLocked = (m_hud.targetAngleDeg < 15.0f);

      // Project into normalized radar disk coordinates
      // Radar X = right, Radar Y = forward component (projected on ship horizontal plane)
      float radarDistNorm = std::clamp(dist / 40.0f, 0.10f, 0.88f);
      float angleHoriz = std::atan2(dotRight, dotFwd);
      m_hud.targetRadarPos = glm::vec2(std::sin(angleHoriz) * radarDistNorm, std::cos(angleHoriz) * radarDistNorm);
    } else {
      m_hud.targetAngleDeg = 0.0f;
      m_hud.targetLocked = true;
      m_hud.targetRadarPos = glm::vec2(0.0f);
    }
  } else {
    m_hud.targetDistLy = 0.0f;
    m_hud.targetAngleDeg = 0.0f;
    m_hud.targetLocked = false;
    m_hud.targetRadarPos = glm::vec2(0.0f);
  }
}

void CockpitController::setLandingTelemetry(int gearState, float gearProgress, bool isLanded, float surfaceDustK, float suspension) {
  m_hud.gearState = gearState;
  m_hud.gearProgress = gearProgress;
  m_hud.isLanded = isLanded;
  m_hud.surfaceDustK = surfaceDustK;
  m_hud.suspensionCompression = suspension;
}

void CockpitController::setOrbitalCruiseTelemetry(bool active, bool locked, float radius, float altKm, float speed, float periodSec, float anomalyDeg) {
  m_hud.orbitCruiseActive = active;
  m_hud.orbitCruiseLocked = locked;
  m_hud.orbitRadius = radius;
  m_hud.orbitAltitudeKm = altKm;
  m_hud.orbitSpeed = speed;
  m_hud.orbitPeriodSec = periodSec;
  m_hud.orbitTrueAnomalyDeg = anomalyDeg;
}

void CockpitController::setSurfaceDrillTelemetry(bool active, float depthM, float bitTempC, int state, bool analyzed) {
  m_hud.drillActive = active;
  m_hud.drillDepthM = depthM;
  m_hud.drillBitTempC = bitTempC;
  m_hud.drillState = state;
  m_hud.coreAnalyzed = analyzed;
}

}  // namespace astro
