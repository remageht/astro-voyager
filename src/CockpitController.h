#pragma once

#include <string>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include "Types.h"

namespace astro {

struct CockpitHUDData {
  float pitchDeg = 0.0f;
  float rollDeg = 0.0f;
  float yawDeg = 0.0f;
  float speed = 0.0f;
  float throttle = 0.0f;      // 0.0 .. 1.0 (or >1.0 on boost)
  float warpCharge = 0.0f;    // 0.0 .. 1.0
  bool  warpActive = false;
  float hullIntegrity = 1.0f; // 0.0 .. 1.0
  float shieldIntegrity = 1.0f;// 0.0 .. 1.0
  float gForce = 1.0f;        // in Gs (1.0 = 1G baseline)
  std::string targetName = "";
  float targetDistLy = 0.0f;
  float targetAngleDeg = 0.0f;
  bool  targetLocked = false;
  float autoTint = 0.0f;      // 0.0 = clear, 1.0 = full dark tint
};

class CockpitController {
 public:
  CockpitController();
  ~CockpitController() = default;

  // Reset inertial states (e.g. upon teleport or station transition)
  void reset();

  // Update physical flight response per frame
  void update(float deltaTime,
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
              float sunVisibility);

  // Inertial offset of pilot's head relative to canopy center (meters)
  glm::vec3 getHeadOffset() const { return m_headOffset; }

  // Rotational angular tilt (radians: pitch, yaw, roll)
  glm::vec3 getHeadAngularTilt() const { return m_headTilt; }

  // Telemetry bundle for HUD rendering
  const CockpitHUDData& getHUDData() const { return m_hud; }

  // High-frequency vibration scalar for canopy rumble [0..1]
  float getCanopyRumble() const { return m_rumble; }

 private:
  glm::vec3 m_prevPos{0.0f};
  glm::vec3 m_prevFwd{0.0f, 0.0f, -1.0f};
  glm::vec3 m_headOffset{0.0f};
  glm::vec3 m_headOffsetVel{0.0f};
  glm::vec3 m_headTilt{0.0f};
  glm::vec3 m_headTiltVel{0.0f};
  float m_rumble = 0.0f;
  float m_autoTint = 0.0f;
  CockpitHUDData m_hud;
  bool m_initialized = false;
};

}  // namespace astro
