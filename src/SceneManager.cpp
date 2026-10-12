#include "SceneManager.h"
#include "Constellation.h"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <string>

namespace astro {

SceneManager::SceneManager() {
  m_screenQuad = std::make_unique<ScreenQuad>();
  m_bhShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/bh.frag");
  m_quasarShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/quasar.frag");
  m_linesShader = std::make_unique<Shader>("shaders/lines.vert", "shaders/lines.frag");
  m_starShader = std::make_unique<Shader>("shaders/star.vert", "shaders/star.frag");
  m_galaxyShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/galaxy.frag");
  m_nebulaShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/nebula.frag");
  m_planetShader = std::make_unique<Shader>("shaders/planet.vert", "shaders/planet.frag");
  m_cometShader = std::make_unique<Shader>("shaders/planet.vert", "shaders/comet.frag");
  m_ringShader = std::make_unique<Shader>("shaders/ring.vert", "shaders/ring.frag");
  m_skyShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/sky.frag");
  m_skyboxDeep = std::make_unique<Cubemap>();
  m_skyboxLocal = std::make_unique<Cubemap>();
  m_skyboxLocal->regenerate(0.985f, 0.05f, 1234u);

  initConstellationOrion();
  initConstellationUma();

  {
    const int kBgCount = 1500;
    const float kBgRadius = 800.0f;
    const float kPi = 3.14159265358979323846f;
    uint32_t state = 0x0B65EA11u;
    auto next01 = [&state]() -> float {
      state = state * 1664525u + 1013904223u;
      return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
    };
    std::vector<glm::vec3> bgPos;
    std::vector<float> bgMags;
    bgPos.reserve(static_cast<size_t>(kBgCount));
    bgMags.reserve(static_cast<size_t>(kBgCount));
    for (int i = 0; i < kBgCount; ++i) {
      float u = next01();
      float v = next01();
      float w = next01();
      float y = 1.0f - 2.0f * u;
      float phi = 2.0f * kPi * v;
      float rr = std::sqrt(std::max(0.0f, 1.0f - y * y));
      glm::vec3 dir(rr * std::cos(phi), y, rr * std::sin(phi));
      bgPos.push_back(dir * kBgRadius);
      bgMags.push_back(3.5f + 2.0f * w);
    }
    m_bgStars.setStars(bgPos, bgMags);
  }

  m_sphere.setSphere(48, 96);
  m_saturnRing.setRing(1.7f, 2.7f, 128);
  m_belt.setBelt(2200, 19.5f, 22.5f);

  {
    std::vector<glm::vec3> orbitVerts;
    std::vector<unsigned int> orbitIdx;
    const std::vector<PlanetDef>& defs = m_sol.defs();
    for (const auto& d : defs) {
      if (d.orbitRadius <= 0.0f) continue;
      if (!d.parentId.empty()) continue;
      std::vector<Vec3> pts = m_sol.orbitCircle(d.orbitRadius, 128);
      if (pts.empty()) continue;
      unsigned int base = static_cast<unsigned int>(orbitVerts.size());
      for (const auto& p : pts) {
        orbitVerts.emplace_back(static_cast<float>(p.x),
                                static_cast<float>(p.y),
                                static_cast<float>(p.z));
      }
      unsigned int n = static_cast<unsigned int>(pts.size());
      for (unsigned int i = 0; i < n; ++i) {
        orbitIdx.push_back(base + i);
        orbitIdx.push_back(base + ((i + 1) % n));
      }
    }
    m_orbits.setGeometry(orbitVerts, orbitIdx);
  }

  m_currentStation = findStation("sgra");
  if (!m_currentStation && !builtinCatalog().empty()) {
    m_currentStation = &builtinCatalog()[0];
  }

  applyPreset(PerfPreset::Med);
}

void SceneManager::spawnRockAt(const Vec3& p, const Vec3& v) {
  ++m_customSeq;
  CustomBody b;
  b.id = "rock-" + std::to_string(m_customSeq);
  b.name = "Rock " + std::to_string(m_customSeq);
  b.pos = p;
  b.vel = v;
  b.radius = 0.3f;
  b.color = Vec3(0.62, 0.58, 0.52);
  b.seed = static_cast<float>(m_customSeq);
  m_custom.push_back(b);
}

void SceneManager::deleteSelected() {
  if (m_selectedId.empty()) return;
  for (size_t i = 0; i < m_custom.size(); ++i) {
    if (m_custom[i].id == m_selectedId) {
      m_custom.erase(m_custom.begin() + static_cast<std::ptrdiff_t>(i));
      break;
    }
  }
  m_selectedId.clear();
}

void SceneManager::clearCustom() {
  m_custom.clear();
  m_selectedId.clear();
}

void SceneManager::updateCustom(float dt) {
  if (dt != 0.0f) {
    for (auto& b : m_custom) {
      b.pos.x += b.vel.x * dt;
      b.pos.y += b.vel.y * dt;
      b.pos.z += b.vel.z * dt;
    }
  }
  if (!m_collisions) return;
  // Pairwise merge in index order (deterministic): bigger absorbs smaller,
  // volume adds up (radius = cbrt(ra^3 + rb^3)), survivor keeps the bigger
  // body's color/name/id/seed/vel at the earlier index.
  for (size_t i = 0; i < m_custom.size(); ++i) {
    for (size_t j = i + 1; j < m_custom.size();) {
      double dx = m_custom[i].pos.x - m_custom[j].pos.x;
      double dy = m_custom[i].pos.y - m_custom[j].pos.y;
      double dz = m_custom[i].pos.z - m_custom[j].pos.z;
      double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
      double touch = static_cast<double>(m_custom[i].radius) +
                     static_cast<double>(m_custom[j].radius);
      if (dist >= touch) {
        ++j;
        continue;
      }
      size_t big = (m_custom[i].radius >= m_custom[j].radius) ? i : j;
      double ra = static_cast<double>(m_custom[i].radius);
      double rb = static_cast<double>(m_custom[j].radius);
      float merged = static_cast<float>(std::cbrt(ra * ra * ra + rb * rb * rb));
      std::string survivorId = m_custom[big].id;
      std::string erasedId = m_custom[j].id;
      CustomBody survivor = m_custom[big];
      survivor.radius = merged;
      m_custom[i] = survivor;
      m_custom.erase(m_custom.begin() + static_cast<std::ptrdiff_t>(j));
      if (m_selectedId == erasedId) m_selectedId = survivorId;
      // Do not advance j: a new body shifted into slot j, retest it.
    }
  }
  // Absorption by sun/planets: custom inside bodyRadius + customR is erased.
  for (size_t i = m_custom.size(); i-- > 0;) {
    bool absorbed = false;
    for (const auto& body : m_lastBodies) {
      if (!body.def) continue;
      double dx = m_custom[i].pos.x - body.pos.x;
      double dy = m_custom[i].pos.y - body.pos.y;
      double dz = m_custom[i].pos.z - body.pos.z;
      double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
      double touch = static_cast<double>(body.def->radius) +
                     static_cast<double>(m_custom[i].radius);
      if (dist < touch) {
        absorbed = true;
        break;
      }
    }
    if (absorbed) {
      if (m_custom[i].id == m_selectedId) m_selectedId.clear();
      m_custom.erase(m_custom.begin() + static_cast<std::ptrdiff_t>(i));
    }
  }
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
  std::vector<float> mags;
  mags.reserve(info->stars.size());
  for (const auto& s : info->stars) {
    vertices.emplace_back(s.x, s.y, s.z);
    mags.push_back(s.mag);
  }

  std::vector<unsigned int> indices;
  indices.reserve(info->segments.size() * 2);
  for (const auto& seg : info->segments) {
    indices.push_back(seg.first);
    indices.push_back(seg.second);
  }

  m_orionMesh.setGeometry(vertices, indices);
  m_orionStars.setStars(vertices, mags);
}

void SceneManager::initConstellationUma() {
  const ConstellationInfo* info = findConstellation("uma");
  if (!info) return;

  std::vector<glm::vec3> vertices;
  vertices.reserve(info->stars.size());
  std::vector<float> mags;
  mags.reserve(info->stars.size());
  for (const auto& s : info->stars) {
    vertices.emplace_back(s.x, s.y, s.z);
    mags.push_back(s.mag);
  }

  std::vector<unsigned int> indices;
  indices.reserve(info->segments.size() * 2);
  for (const auto& seg : info->segments) {
    indices.push_back(seg.first);
    indices.push_back(seg.second);
  }

  m_umaMesh.setGeometry(vertices, indices);
  m_umaStars.setStars(vertices, mags);
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

  if (st->type == StationType::SolarSystem) {
    camera.position = Vec3(0.0, 20.0, 68.0);
    camera.yaw = 0.0;
    camera.pitch = -0.28;
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
    m_bhShader->setUniform1f("u_Time", time);
    m_bhShader->setUniform1f("u_Exposure", m_exposure);
    m_bhShader->setUniform1i("u_Skybox", 0);

    m_skyboxDeep->bind(0);
    m_screenQuad->draw();
    m_bhShader->unbind();
  } else if (m_currentStation->type == StationType::Quasar) {
    // 3C 273 Quasar disk + jets + glow
    m_quasarShader->bind();
    m_quasarShader->setUniform3f("u_CoreColor", 1.0f, 0.85f, 0.65f);
    m_quasarShader->setUniform1f("u_Time", time);
    m_quasarShader->setUniform1f("u_Exposure", m_exposure);

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

    const float pixelScale = static_cast<float>(fbHeight) /
        (2.0f * std::tan(glm::radians(m_settings.fovDegrees) * 0.5f));
    m_starShader->bind();
    m_starShader->setUniformMat4f("u_MVP", mvp);
    m_starShader->setUniformMat4f("u_MV", view);
    m_starShader->setUniform1f("u_PixelScale", pixelScale);
    m_starShader->setUniform1f("u_SizeK", 0.22f);
    m_bgStars.draw();
    m_starShader->unbind();

    m_linesShader->bind();
    m_linesShader->setUniformMat4f("u_MVP", mvp);
    m_linesShader->setUniformMat4f("u_Model", glm::mat4(1.0f));

    if (m_currentStation->id == "ori") {
      // Orion: bright cyan lines
      m_linesShader->setUniform3f("u_LineColor", 0.35f, 0.75f, 1.0f);
      m_orionMesh.drawLines();
    } else {
      // Big Dipper: cyan-amber lines
      m_linesShader->setUniform3f("u_LineColor", 0.4f, 0.85f, 0.95f);
      m_umaMesh.drawLines();
    }

    m_linesShader->unbind();

    m_starShader->bind();
    m_starShader->setUniformMat4f("u_MVP", mvp);
    m_starShader->setUniformMat4f("u_MV", view);
    m_starShader->setUniform1f("u_PixelScale", pixelScale);
    m_starShader->setUniform1f("u_SizeK", 0.22f);
    if (m_currentStation->id == "ori") {
      m_orionStars.draw();
    } else {
      m_umaStars.draw();
    }
    m_starShader->unbind();
  } else if (m_currentStation->type == StationType::Galaxy) {
    // M31 Andromeda Galaxy impostor sprite
    m_galaxyShader->bind();
    m_galaxyShader->setUniform1f("u_Time", time);
    m_galaxyShader->setUniform1f("u_Aspect", aspect);

    double dist = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
    double spawn = m_currentStation->spawnDistanceRs;
    float zoom = static_cast<float>(std::clamp(spawn / std::max(dist, 1e-3), 0.2, 4.0));
    m_galaxyShader->setUniform1f("u_Zoom", zoom);
    m_galaxyShader->setUniform1f("u_Exposure", m_exposure);

    m_screenQuad->draw();
    m_galaxyShader->unbind();
  } else if (m_currentStation->type == StationType::Nebula) {
    // M1 Crab Nebula impostor sprite
    m_nebulaShader->bind();
    m_nebulaShader->setUniform1f("u_Time", time);
    m_nebulaShader->setUniform1f("u_Aspect", aspect);
    m_nebulaShader->setUniform1f("u_Exposure", m_exposure);
    m_screenQuad->draw();
    m_nebulaShader->unbind();
  } else if (m_currentStation->type == StationType::SolarSystem) {
    float dt = (m_lastTime < 0.0f) ? 0.0f : (time - m_lastTime);
    m_lastTime = time;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.5f) dt = 0.5f;
    if (!m_paused) m_simTime += dt * m_timeScale;
    m_sol.update(m_simTime);
    m_lastBodies = m_sol.bodies();
    if (!m_paused && dt > 0.0f) updateCustom(dt * m_timeScale);

    glm::mat4 proj = glm::perspective(glm::radians(m_settings.fovDegrees), aspect, 0.1f, 1000.0f);
    glm::vec3 eye(cp.x, cp.y, cp.z);
    glm::vec3 fwd(cf.x, cf.y, cf.z);
    glm::vec3 up(cu.x, cu.y, cu.z);
    glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
    glm::mat4 mvp = proj * view;

    glClearColor(0.004f, 0.005f, 0.009f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_skyShader->bind();
    m_skyShader->setUniform3f("u_CamFwd", static_cast<float>(cf.x),
                              static_cast<float>(cf.y), static_cast<float>(cf.z));
    m_skyShader->setUniform3f("u_CamRight", static_cast<float>(cr.x),
                              static_cast<float>(cr.y), static_cast<float>(cr.z));
    m_skyShader->setUniform3f("u_CamUp", static_cast<float>(cu.x),
                              static_cast<float>(cu.y), static_cast<float>(cu.z));
    m_skyShader->setUniform1f("u_Aspect", aspect);
    m_skyShader->setUniform1f("u_FovY", glm::radians(m_settings.fovDegrees));
    m_skyShader->setUniform1i("u_Sky", 0);
    m_skyShader->setUniform1f("u_Exposure", m_exposure);
    m_skyboxLocal->bind(0);
    glDepthMask(GL_FALSE);
    m_screenQuad->draw();
    glDepthMask(GL_TRUE);
    m_skyShader->unbind();

    m_linesShader->bind();
    m_linesShader->setUniformMat4f("u_MVP", mvp);
    m_linesShader->setUniformMat4f("u_Model", glm::mat4(1.0f));
    m_linesShader->setUniform3f("u_LineColor", 0.45f, 0.55f, 0.75f);
    if (m_showOrbits) {
      m_orbits.drawLines();
    }

    if (m_showBelt) {
      m_linesShader->setUniformMat4f("u_Model", glm::rotate(glm::mat4(1.0f), time * 0.02f,
                                                            glm::vec3(0.0f, 1.0f, 0.0f)));
      m_linesShader->setUniform3f("u_LineColor", 0.45f, 0.38f, 0.30f);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      m_belt.draw();
      glDisable(GL_BLEND);
    }
    m_linesShader->unbind();

    m_planetShader->bind();
    m_planetShader->setUniformMat4f("u_MVP", mvp);
    m_planetShader->setUniform1f("u_Time", time);
    m_planetShader->setUniform1f("u_Exposure", m_exposure);
    m_planetShader->setUniform3f("u_CamPos", static_cast<float>(cp.x),
                                 static_cast<float>(cp.y), static_cast<float>(cp.z));
    for (const auto& body : m_lastBodies) {
      const PlanetDef* def = body.def;
      if (!def) continue;
      glm::vec3 bpos(static_cast<float>(body.pos.x),
                     static_cast<float>(body.pos.y),
                     static_cast<float>(body.pos.z));
      glm::mat4 model = glm::translate(glm::mat4(1.0f), bpos) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(def->radius));
      m_planetShader->setUniformMat4f("u_Model", model);
      m_planetShader->setUniform1i("u_Type", def->type);
      m_planetShader->setUniform1f("u_Seed", def->seed);
      m_planetShader->setUniform3f("u_ColA", static_cast<float>(def->colA.x),
                                   static_cast<float>(def->colA.y),
                                   static_cast<float>(def->colA.z));
      m_planetShader->setUniform3f("u_ColB", static_cast<float>(def->colB.x),
                                   static_cast<float>(def->colB.y),
                                   static_cast<float>(def->colB.z));
      m_planetShader->setUniform1i("u_Spot", (def->id == "jupiter" ? 1 : 0));
      m_planetShader->setUniform1i("u_Clouds", (def->id == "earth" ? 1 : 0));
      m_sphere.draw();
      if (def->hasRing) {
        m_ringShader->bind();
        m_ringShader->setUniformMat4f("u_MVP", mvp);
        glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), bpos) *
                              glm::rotate(glm::mat4(1.0f), 0.45f, glm::vec3(1.0f, 0.0f, 0.0f));
        m_ringShader->setUniformMat4f("u_Model", ringModel);
        m_ringShader->setUniform1f("u_Seed", def->seed);
        m_ringShader->setUniform3f("u_ColA", 0.8f, 0.72f, 0.6f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_saturnRing.draw();
        glDisable(GL_BLEND);
        m_planetShader->bind();
      }
    }
    for (const auto& custom : m_custom) {
      glm::vec3 cpos(static_cast<float>(custom.pos.x),
                     static_cast<float>(custom.pos.y),
                     static_cast<float>(custom.pos.z));
      glm::mat4 model = glm::translate(glm::mat4(1.0f), cpos) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(custom.radius));
      m_planetShader->setUniformMat4f("u_Model", model);
      m_planetShader->setUniform1i("u_Type", 2);
      m_planetShader->setUniform1f("u_Seed", custom.seed);
      m_planetShader->setUniform3f("u_ColA", static_cast<float>(custom.color.x),
                                   static_cast<float>(custom.color.y),
                                   static_cast<float>(custom.color.z));
      m_planetShader->setUniform3f("u_ColB", static_cast<float>(custom.color.x) * 0.6f,
                                   static_cast<float>(custom.color.y) * 0.6f,
                                   static_cast<float>(custom.color.z) * 0.6f);
      m_planetShader->setUniform1i("u_Spot", 0);
      m_planetShader->setUniform1i("u_Clouds", 0);
      m_sphere.draw();
    }
    m_planetShader->unbind();

    // Halley ion tail: additive ellipsoid oriented anti-sunward.
    {
      glm::vec3 cometPos(0.0f);
      const PlanetDef* cometDef = nullptr;
      for (const auto& body : m_lastBodies) {
        if (body.def && body.def->id == "halley") {
          cometPos = glm::vec3(static_cast<float>(body.pos.x),
                               static_cast<float>(body.pos.y),
                               static_cast<float>(body.pos.z));
          cometDef = body.def;
          break;
        }
      }
      if (cometDef != nullptr) {
        float distToSun = glm::length(cometPos);
        glm::vec3 tailDir(1.0f, 0.0f, 0.0f);
        if (distToSun > 1e-3f) {
          tailDir = cometPos / distToSun;
        }
        glm::vec3 tmpUp(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(tailDir, tmpUp)) > 0.99f) {
          tmpUp = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        glm::vec3 bx = tailDir;
        glm::vec3 by = glm::normalize(glm::cross(tmpUp, bx));
        glm::vec3 bz = glm::cross(bx, by);
        float len = 2.0f + 40.0f / std::max(distToSun, 1.0f);
        glm::mat4 basis(1.0f);
        basis[0] = glm::vec4(bx, 0.0f);
        basis[1] = glm::vec4(by, 0.0f);
        basis[2] = glm::vec4(bz, 0.0f);
        glm::mat4 tailModel = glm::translate(glm::mat4(1.0f), cometPos) *
                              basis *
                              glm::scale(glm::mat4(1.0f), glm::vec3(len, 0.8f, 0.8f));
        m_cometShader->bind();
        m_cometShader->setUniformMat4f("u_MVP", mvp);
        m_cometShader->setUniformMat4f("u_Model", tailModel);
        m_cometShader->setUniform1f("u_Seed", cometDef->seed);
        m_cometShader->setUniform3f("u_ColA", 0.55f, 0.75f, 1.0f);
        m_cometShader->setUniform1f("u_Exposure", m_exposure);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        m_sphere.draw();
        glDisable(GL_BLEND);
        m_cometShader->unbind();
      }
    }
    glDisable(GL_DEPTH_TEST);
  }
}

}  // namespace astro
