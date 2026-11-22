#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace astro {

enum class IntroPhase : int {
  VoidDrift = 0,           // 0.0s - 3.5s: Deep cosmic silence & distant Kerr accretion disk
  WarpBurst = 1,           // 3.5s - 7.0s: Gravitational lensing distortion & warp emergence
  ShipPass = 2,            // 7.0s - 10.5s: Hero flyby tracking the Alcubierre cruiser
  MegastructureReveal = 3, // 10.5s - 14.5s: Colossal Orbital Megastructure pan & rotation
  BridgeEnter = 4,         // 14.5s - 18.0s: Seamless camera dive through canopy into 3D bridge
  TitleGlory = 5,          // 18.0s - 22.0s: Holographic title display «ASTRO-VOYAGER»
  Finished = 6
};

struct SplineKeyframe {
  float time;
  glm::vec3 pos;
  glm::vec3 target;
  glm::vec3 up;
  float fov;
  float roll;
};

class IntroCinematicDirector {
 public:
  IntroCinematicDirector();

  void start();
  void skip();
  void reset();

  void update(float dt);
  void drawHUD(float screenW, float screenH);

  bool isActive() const { return m_active && m_phase != IntroPhase::Finished; }
  bool isFinished() const { return m_phase == IntroPhase::Finished; }
  IntroPhase phase() const { return m_phase; }
  float phaseProgress() const { return m_phaseProgress; }
  float totalProgress() const { return m_totalProgress; }
  float currentTime() const { return m_timer; }

  // 3D Camera outputs
  glm::vec3 cameraPosition() const { return m_camPos; }
  glm::vec3 cameraTarget() const { return m_camTarget; }
  glm::vec3 cameraUp() const { return m_camUp; }
  float cameraFov() const { return m_camFov; }
  float cameraRoll() const { return m_camRoll; }

  // Visual post-process effects
  float warpDistortion() const { return m_warpDistortion; }
  float flashIntensity() const { return m_flashIntensity; }
  float anamorphicStreak() const { return m_anamorphicStreak; }
  float fadeAlpha() const { return m_fadeAlpha; }

  // Scene actor transformations
  glm::mat4 shipModelMatrix() const;
  glm::mat4 megastructureModelMatrix() const;
  glm::mat4 bridgeModelMatrix() const;

  // Sound triggers
  bool consumeWarpBoomTrigger();
  bool consumeEngineWhooshTrigger();
  bool consumeBridgeOnlineTrigger();
  bool consumeTitleChimeTrigger();

 private:
  bool m_active = false;
  bool m_skipping = false;
  float m_skipFadeTimer = 0.0f;
  float m_timer = 0.0f;
  float m_totalDuration = 22.0f;

  IntroPhase m_phase = IntroPhase::VoidDrift;
  float m_phaseProgress = 0.0f;
  float m_totalProgress = 0.0f;

  // Camera state
  glm::vec3 m_camPos{0.0f, 0.0f, 50.0f};
  glm::vec3 m_camTarget{0.0f, 0.0f, 0.0f};
  glm::vec3 m_camUp{0.0f, 1.0f, 0.0f};
  float m_camFov = 65.0f;
  float m_camRoll = 0.0f;

  // Effect intensities
  float m_warpDistortion = 0.0f;
  float m_flashIntensity = 0.0f;
  float m_anamorphicStreak = 0.0f;
  float m_fadeAlpha = 0.0f;

  // Sound trigger flags
  bool m_triggerWarpBoom = false;
  bool m_triggerEngineWhoosh = false;
  bool m_triggerBridgeOnline = false;
  bool m_triggerTitleChime = false;

  bool m_firedWarpBoom = false;
  bool m_firedEngineWhoosh = false;
  bool m_firedBridgeOnline = false;
  bool m_firedTitleChime = false;

  std::vector<SplineKeyframe> m_keyframes;

  void setupKeyframes();
  void evaluateSpline(float t);
  static glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                              const glm::vec3& p2, const glm::vec3& p3, float t);
};

}  // namespace astro
