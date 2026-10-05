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

namespace astro {

struct RenderSettings {
  float rs = 1.0f;
  float shellRadius = 30.0f;
  float stepSize = 0.05f;
  int maxSteps = 600;
  float fovDegrees = 60.0f;
};

class SceneManager {
 public:
  SceneManager();
  ~SceneManager() = default;

  SceneManager(const SceneManager&) = delete;
  SceneManager& operator=(const SceneManager&) = delete;

  void teleportTo(const std::string& stationId, Camera& camera);
  void render(const Camera& camera, int fbWidth, int fbHeight, float time);

  const Station* getCurrentStation() const { return m_currentStation; }
  RenderSettings& getSettings() { return m_settings; }
  const RenderSettings& getSettings() const { return m_settings; }

 private:
  void initConstellationOrion();
  void initConstellationUma();

  const Station* m_currentStation = nullptr;
  RenderSettings m_settings;

  std::unique_ptr<ScreenQuad> m_screenQuad;
  std::unique_ptr<Shader> m_bhShader;
  std::unique_ptr<Shader> m_quasarShader;
  std::unique_ptr<Shader> m_linesShader;
  std::unique_ptr<Shader> m_galaxyShader;
  std::unique_ptr<Shader> m_nebulaShader;
  std::unique_ptr<Cubemap> m_skybox;

  LineMesh m_orionMesh;
  LineMesh m_umaMesh;
};

}  // namespace astro
