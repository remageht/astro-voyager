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

  // v4.6.0 Realism Overhaul: Flight vectors, avionics & atmospheric physics
  glm::vec2 progradeScreenPos{0.0f};  // 2D collimated screen coords (-1..1)
  bool  progradeVisible = false;
  glm::vec2 retrogradeScreenPos{0.0f};
  bool  retrogradeVisible = false;
  float blackoutK = 0.0f;     // 0.0 .. 1.0 tunnel vision under high positive G
  float redoutK = 0.0f;       // 0.0 .. 1.0 crimson wash under negative vertical G
  float radarAltitudeKm = -1.0f; // AGL in km (< 0 when in deep space)
  float verticalSpeedMs = 0.0f;  // vertical ascent/descent rate (m/s)
  bool  terrainAlert = false; // flashing warning when descent rate exceeds terrain safety
  float entryPlasma = 0.0f;   // 0.0 .. 1.0 hypersonic atmospheric compression ionization

  // v4.6.1 Planetary Landing & Surface Operations
  int   gearState = 0;        // 0 Retracted, 1 Deploying, 2 Deployed, 3 Retracting
  float gearProgress = 0.0f;  // 0.0 .. 1.0
  bool  isLanded = false;
  float surfaceDustK = 0.0f;  // 0.0 .. 1.0
  float suspensionCompression = 0.0f; // 0.0 .. 1.0

  // v4.9.0 Diegetic Cockpit MFDs & Hypersonic Re-entry
  float hullTempNorm = 0.0f;          // 0.0 .. 1.0 (ambient to 2800K thermal shock)
  glm::vec2 targetRadarPos{0.0f};     // 2D polar-projected radar coordinates (-1..1)
  float atmoDensity = 0.0f;           // 0.0 (vacuum) .. 1.0 (sea level standard atmosphere)

  // v5.0.0 Bridge Comms & Radio Avionics
  bool  radioActive = false;
  float radioSignal = 0.0f;           // 0.0 .. 1.0 voice modulation

  // v5.1.0 Astro-Zettelkasten Neural Resonance & Eureka Synthesis
  float neuralResonance = 0.0f;       // 0.0 .. 1.0 brain / graph synthesis resonance
  bool  eurekaPending = false;        // true if at least one Master Node synthesis is ready

  // v5.2.0 Cosmic Ambient Cassette Deck
  int   tapeIndex = 0;                // 0 Off, 1 Paris Rain, 2 Pulsar Beat, 3 Deep Space Void
  bool  tapePlaying = false;          // true when tape reel is rotating

  // v5.3.0 Orbital Cruise & Keplerian Parking Orbit
  bool  orbitCruiseActive = false;
  bool  orbitCruiseLocked = false;
  float orbitRadius = 0.0f;
  float orbitAltitudeKm = 0.0f;
  float orbitSpeed = 0.0f;
  float orbitPeriodSec = 0.0f;
  float orbitTrueAnomalyDeg = 0.0f;

  // v5.3.0 Surface Science & Core Sampler
  bool  drillActive = false;
  float drillDepthM = 0.0f;
  float drillBitTempC = 20.0f;
  int   drillState = 0;
  bool  coreAnalyzed = false;
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
              float sunVisibility,
              const glm::vec3& worldVel = glm::vec3(0.0f),
              const glm::vec3& nearestBodyPos = glm::vec3(0.0f),
              float nearestBodyRadius = 0.0f,
              bool hasAtmosphere = false);

  // Inertial offset of pilot's head relative to canopy center (meters)
  glm::vec3 getHeadOffset() const { return m_headOffset; }

  // Rotational angular tilt (radians: pitch, yaw, roll)
  glm::vec3 getHeadAngularTilt() const { return m_headTilt; }

  // Telemetry bundle for HUD rendering
  const CockpitHUDData& getHUDData() const { return m_hud; }

  // High-frequency vibration scalar for canopy rumble [0..1]
  float getCanopyRumble() const { return m_rumble; }

  // v4.6.1 Planetary Landing & Surface Operations telemetry
  void setLandingTelemetry(int gearState, float gearProgress, bool isLanded, float surfaceDustK, float suspension);

  // v5.3.0 Orbital Cruise & Keplerian Parking Orbit telemetry
  void setOrbitalCruiseTelemetry(bool active, bool locked, float radius, float altKm, float speed, float periodSec, float anomalyDeg);

  // v5.3.0 Surface Science & Core Sampler telemetry
  void setSurfaceDrillTelemetry(bool active, float depthM, float bitTempC, int state, bool analyzed);

 private:
  glm::vec3 m_prevPos{0.0f};
  glm::vec3 m_prevFwd{0.0f, 0.0f, -1.0f};
  glm::vec3 m_headOffset{0.0f};
  glm::vec3 m_headOffsetVel{0.0f};
  glm::vec3 m_headTilt{0.0f};
  glm::vec3 m_headTiltVel{0.0f};
  float m_rumble = 0.0f;
  float m_autoTint = 0.0f;
  float m_hullTemp = 0.0f;
  CockpitHUDData m_hud;
  bool m_initialized = false;
};

}  // namespace astro
