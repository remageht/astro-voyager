#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Camera.h"
#include "Catalog.h"
#include "Cubemap.h"
#include "Mesh.h"
#include "Shader.h"
#include "SolarSystem.h"

namespace astro {

struct RenderSettings {
  float rs = 1.0f;
  float shellRadius = 30.0f;
  float stepSize = 0.05f;
  int maxSteps = 600;
  float fovDegrees = 60.0f;
  bool diskOn = false;
};

enum class PerfPreset {
  Low = 0,
  Med = 1,
  Ultra = 2,
  Custom = 3
};

inline RenderSettings getPresetSettings(PerfPreset preset) {
  RenderSettings s;
  switch (preset) {
    case PerfPreset::Low:
      s.rs = 1.0f;
      s.shellRadius = 20.0f;
      s.stepSize = 0.08f;
      s.maxSteps = 300;
      s.fovDegrees = 60.0f;
      break;
    case PerfPreset::Med:
    default:
      s.rs = 1.0f;
      s.shellRadius = 30.0f;
      s.stepSize = 0.05f;
      s.maxSteps = 600;
      s.fovDegrees = 60.0f;
      break;
    case PerfPreset::Ultra:
      s.rs = 1.0f;
      s.shellRadius = 45.0f;
      s.stepSize = 0.025f;
      s.maxSteps = 1200;
      s.fovDegrees = 60.0f;
      break;
  }
  return s;
}

class SceneManager {
 public:
  SceneManager();
  ~SceneManager() = default;

  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;

  void teleportTo(const std::string& stationId, Camera& camera);
  void render(const Camera& camera, int fbWidth, int fbHeight, float time);

  const Station* getCurrentStation() const { return m_currentStation; }
  const std::vector<BodyState>& getBodies() const { return m_lastBodies; }
  RenderSettings& getSettings() { return m_settings; }
  const RenderSettings& getSettings() const { return m_settings; }

  PerfPreset getPreset() const { return m_preset; }
  void applyPreset(PerfPreset preset);
  void setPreset(PerfPreset preset) { m_preset = preset; }

 private:
  void initConstellationOrion();
  void initConstellationUma();

  const Station* m_currentStation = nullptr;
  RenderSettings m_settings;
  PerfPreset m_preset = PerfPreset::Med;

  std::unique_ptr<ScreenQuad> m_screenQuad;
  std::unique_ptr<Shader> m_bhShader;
  std::unique_ptr<Shader> m_quasarShader;
  std::unique_ptr<Shader> m_linesShader;
  std::unique_ptr<Shader> m_starShader;
  std::unique_ptr<Shader> m_galaxyShader;
  std::unique_ptr<Shader> m_nebulaShader;
  std::unique_ptr<Shader> m_planetShader;
  std::unique_ptr<Shader> m_ringShader;
  std::unique_ptr<Shader> m_cometShader;
  std::unique_ptr<Shader> m_skyShader;
  std::unique_ptr<Cubemap> m_skyboxDeep;
  std::unique_ptr<Cubemap> m_skyboxLocal;

  LineMesh m_orionMesh;
  LineMesh m_umaMesh;
  StarPoints m_orionStars;
  StarPoints m_umaStars;
  StarPoints m_bgStars;

  SolarSystem m_sol;
  SphereMesh m_sphere;
  RingMesh m_saturnRing;
  BeltPoints m_belt;
  LineMesh m_orbits;
  std::vector<BodyState> m_lastBodies;
};

}  // namespace astro
