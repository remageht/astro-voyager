#include "SceneManager.h"
#include "Constellation.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace astro {

SceneManager::SceneManager() {
  m_screenQuad = std::make_unique<ScreenQuad>();
  m_bhShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/bh.frag");
  m_quasarShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/quasar.frag");
  m_linesShader = std::make_unique<Shader>("shaders/lines.vert", "shaders/lines.frag");
  m_galaxyShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/galaxy.frag");
  m_nebulaShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/nebula.frag");
  m_skybox = std::make_unique<Cubemap>();

  initConstellationOrion();
  initConstellationUma();

  m_currentStation = findStation("sgra");
  if (!m_currentStation && !builtinCatalog().empty()) {
    m_currentStation = &builtinCatalog()[0];
  }

  applyPreset(PerfPreset::Med);
}

void SceneManager::applyPreset(PerfPreset preset) {
  if (preset == PerfPreset::Custom) {
    m_preset = PerfPreset::Custom;
    return;
  }
  m_preset = preset;
  RenderSettings p = getPresetSettings(preset);
  m_settings.rs = p.rs;
  m_settings.shellRadius = p.shellRadius;
  m_settings.stepSize = p.stepSize;
  m_settings.maxSteps = p.maxSteps;
  m_settings.fovDegrees = p.fovDegrees;
}

void SceneManager::initConstellationOrion() {
  const ConstellationInfo* info = findConstellation("ori");
  if (!info) return;

  std::vector<glm::vec3> vertices;
  vertices.reserve(info->stars.size());
  for (const auto& s : info->stars) {
    vertices.emplace_back(s.x, s.y, s.z);
  }

  std::vector<unsigned int> indices;
  indices.reserve(info->segments.size() * 2);
  for (const auto& seg : info->segments) {
    indices.push_back(seg.first);
    indices.push_back(seg.second);
  }

  m_orionMesh.setGeometry(vertices, indices);
}

void SceneManager::initConstellationUma() {
  const ConstellationInfo* info = findConstellation("uma");
  if (!info) return;

  std::vector<glm::vec3> vertices;
  vertices.reserve(info->stars.size());
  for (const auto& s : info->stars) {
    vertices.emplace_back(s.x, s.y, s.z);
  }

  std::vector<unsigned int> indices;
  indices.reserve(info->segments.size() * 2);
  for (const auto& seg : info->segments) {
    indices.push_back(seg.first);
    indices.push_back(seg.second);
  }

  m_umaMesh.setGeometry(vertices, indices);
}

void SceneManager::teleportTo(const std::string& stationId, Camera& camera) {
  const Station* st = findStation(stationId);
  if (!st) {
    std::cerr << "SceneManager::teleportTo: station not found: " << stationId << std::endl;
    return;
  }

  m_currentStation = st;
  camera.position = Vec3(0.0, 0.0, st->spawnDistanceRs);
  camera.yaw = 0.0;
  camera.pitch = 0.0;

  if (st->type == StationType::BlackHole) {
    m_settings.rs = static_cast<float>(st->params.rs);
    if (m_preset != PerfPreset::Custom) {
      applyPreset(m_preset);
    }
    camera.clampNearHorizon(m_settings.rs);
  }
}

void SceneManager::render(const Camera& camera, int fbWidth, int fbHeight, float time) {
  if (!m_currentStation) return;

  const float aspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight > 0 ? fbHeight : 1);
  const Vec3 cp = camera.position;
  const Vec3 cf = camera.forward();
  const Vec3 cr = camera.right();
  const Vec3 cu = camera.up();

  if (m_currentStation->type == StationType::BlackHole) {
    // Sgr A* Schwarzschild Raytracing
    m_bhShader->bind();
    m_bhShader->setUniform3f("u_CameraPosition", static_cast<float>(cp.x),
                             static_cast<float>(cp.y), static_cast<float>(cp.z));
    m_bhShader->setUniform3f("u_CameraForward", static_cast<float>(cf.x),
                             static_cast<float>(cf.y), static_cast<float>(cf.z));
    m_bhShader->setUniform3f("u_CameraRight", static_cast<float>(cr.x),
                             static_cast<float>(cr.y), static_cast<float>(cr.z));
    m_bhShader->setUniform3f("u_CameraUp", static_cast<float>(cu.x),
                             static_cast<float>(cu.y), static_cast<float>(cu.z));
    m_bhShader->setUniform1f("u_Aspect", aspect);
    m_bhShader->setUniform1f("u_FovY", glm::radians(m_settings.fovDegrees));
    m_bhShader->setUniform1f("u_Rs", m_settings.rs);
    m_bhShader->setUniform1f("u_ShellRadius", m_settings.shellRadius);
    m_bhShader->setUniform1f("u_StepSize", m_settings.stepSize);
    m_bhShader->setUniform1i("u_MaxSteps", m_settings.maxSteps);
    m_bhShader->setUniform1i("u_DiskOn", m_settings.diskOn ? 1 : 0);
    m_bhShader->setUniform1i("u_Skybox", 0);

    m_skybox->bind(0);
    m_screenQuad->draw();
    m_bhShader->unbind();
  } else if (m_currentStation->type == StationType::Quasar) {
    // 3C 273 Quasar disk + jets + glow
    m_quasarShader->bind();
    m_quasarShader->setUniform3f("u_CoreColor", 1.0f, 0.85f, 0.65f);
    m_quasarShader->setUniform1f("u_Time", time);

    m_screenQuad->draw();
    m_quasarShader->unbind();
  } else if (m_currentStation->type == StationType::Constellation) {
    // Orion or Ursa Major Constellation lines
    glClearColor(0.01f, 0.015f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 proj = glm::perspective(glm::radians(m_settings.fovDegrees), aspect, 0.1f, 1000.0f);
    glm::vec3 eye(cp.x, cp.y, cp.z);
    glm::vec3 fwd(cf.x, cf.y, cf.z);
    glm::vec3 up(cu.x, cu.y, cu.z);
    glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
    glm::mat4 mvp = proj * view;

    m_linesShader->bind();
    m_linesShader->setUniformMat4f("u_MVP", mvp);

    if (m_currentStation->id == "ori") {
      // Orion: bright cyan lines + stars
      m_linesShader->setUniform3f("u_LineColor", 0.35f, 0.75f, 1.0f);
      m_orionMesh.drawLines();

      m_linesShader->setUniform3f("u_LineColor", 1.0f, 0.95f, 0.85f);
      m_orionMesh.drawPoints();
    } else {
      // Big Dipper: cyan-amber lines + stars
      m_linesShader->setUniform3f("u_LineColor", 0.4f, 0.85f, 0.95f);
      m_umaMesh.drawLines();

      m_linesShader->setUniform3f("u_LineColor", 1.0f, 1.0f, 0.88f);
      m_umaMesh.drawPoints();
    }

    m_linesShader->unbind();
  } else if (m_currentStation->type == StationType::Galaxy) {
    // M31 Andromeda Galaxy impostor sprite
    m_galaxyShader->bind();
    m_galaxyShader->setUniform1f("u_Time", time);
    m_galaxyShader->setUniform1f("u_Aspect", aspect);

    double dist = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
    double spawn = m_currentStation->spawnDistanceRs;
    float zoom = static_cast<float>(std::clamp(spawn / std::max(dist, 1e-3), 0.2, 4.0));
    m_galaxyShader->setUniform1f("u_Zoom", zoom);

    m_screenQuad->draw();
    m_galaxyShader->unbind();
  } else if (m_currentStation->type == StationType::Nebula) {
    // M1 Crab Nebula impostor sprite
    m_nebulaShader->bind();
    m_nebulaShader->setUniform1f("u_Time", time);
    m_nebulaShader->setUniform1f("u_Aspect", aspect);
    m_screenQuad->draw();
    m_nebulaShader->unbind();
  }
}

}  // namespace astro
