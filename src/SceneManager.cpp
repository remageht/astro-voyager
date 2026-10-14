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

namespace {
// M31 spiral pattern speed: rad per sim-second. Barely visible live
// (a full turn takes ~20 min at 1x), clearly alive at high time pace.
constexpr float kM31SpinRate = 0.005f;
}  // namespace

// === WarpDrive === (агент W: состояние варп-оверлея).
// AppGL пишет сюда u_WarpK/u_Flash каждый кадр (дефолт 0 — харнесс
// чистый), render() в общем хвосте ставит uniform и рисует warp.frag.
// Отдельный namespace + function-local shader: SceneManager.h не тронут,
// ветки станций не тронуты.
// === WarpJump === (агент J: +u_Burst и корабль-силуэт). Дефолты: burst 0,
// корабль скрыт — хвост скипается целиком, ни одного GL-вызова.
namespace warpdrive {
namespace {
float g_warpK = 0.0f;
float g_warpFlash = 0.0f;
float g_burst = 0.0f;
bool g_shipVisible = false;
float g_shipPos[3] = {0.0f, 0.0f, 0.0f};
float g_shipYaw = 0.0f;
}  // namespace
void setState(float k, float flash) {
  g_warpK = k;
  g_warpFlash = flash;
}
float warpK() { return g_warpK; }
float warpFlash() { return g_warpFlash; }
void setBurst(float b) { g_burst = b; }
float burst() { return g_burst; }
void setShip(bool v, float x, float y, float z, float yaw) {
  g_shipVisible = v;
  g_shipPos[0] = x;
  g_shipPos[1] = y;
  g_shipPos[2] = z;
  g_shipYaw = yaw;
}
bool shipVisible() { return g_shipVisible; }
float shipX() { return g_shipPos[0]; }
float shipY() { return g_shipPos[1]; }
float shipZ() { return g_shipPos[2]; }
float shipYaw() { return g_shipYaw; }
}  // namespace warpdrive

SceneManager::SceneManager() {
  m_screenQuad = std::make_unique<ScreenQuad>();
  m_bhShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/bh.frag");
  m_quasarShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/quasar.frag");
  m_linesShader = std::make_unique<Shader>("shaders/lines.vert", "shaders/lines.frag");
  m_starShader = std::make_unique<Shader>("shaders/star.vert", "shaders/star.frag");
  m_m31StarShader = std::make_unique<Shader>("shaders/m31star.vert", "shaders/m31star.frag");
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
  initM31Disk();

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

void SceneManager::initSStars() {
  // Deterministic LCG swarm (stable run to run): 250 stars, r 0.8-2.5,
  // random orbit planes, Kepler-ish angular speed (inner stars move visibly
  // within seconds), white-blue mags 1.0-2.0 for 2-3 px billboards.
  const int kCount = 250;
  const float kPi = 3.14159265358979323846f;
  uint32_t state = 0x59A11u;
  auto next01 = [&state]() -> float {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
  };
  m_sStarParams.clear();
  m_sStarParams.reserve(static_cast<size_t>(kCount));
  for (int i = 0; i < kCount; ++i) {
    float uR = next01();
    float uY = next01();
    float uPhi = next01();
    float uDir = next01();
    float uPhase = next01();
    float uMag = next01();
    float r = 0.8f + 1.7f * uR;
    float y = 1.0f - 2.0f * uY;
    float phi = 2.0f * kPi * uPhi;
    float rr = std::sqrt(std::max(0.0f, 1.0f - y * y));
    glm::vec3 n(rr * std::cos(phi), y, rr * std::sin(phi));
    glm::vec3 ref(0.0f, 1.0f, 0.0f);
    if (std::abs(glm::dot(n, ref)) > 0.99f) {
      ref = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    glm::vec3 uu = glm::normalize(glm::cross(ref, n));
    glm::vec3 vv = glm::cross(n, uu);
    float dir = (uDir < 0.5f) ? -1.0f : 1.0f;
    SStarParam s;
    s.r = r;
    // PH x1.5 spin-up (was 0.8): the swarm reads alive at 1x pace, inner
    // stars lap visibly within seconds. Phase is referenced to the harness
    // epoch (t = 1.0): th(1.0) matches the legacy angles up to float
    // rounding (~1e-7 rad, far subpixel), so reference PNGs are unchanged.
    float omLegacy = dir * 0.8f / std::pow(r, 1.5f);
    s.omega = dir * 1.2f / std::pow(r, 1.5f);
    s.phase = 2.0f * kPi * uPhase + (omLegacy - s.omega) * 1.0f;
    s.u = uu;
    s.v = vv;
    s.mag = 1.0f + 1.0f * uMag;
    m_sStarParams.push_back(s);
  }
}

void SceneManager::initM31Disk() {
  // M31 volumetric disk starfield: ~4500 points in the disk plane (XZ),
  // radius 6..30 (uniform area), thickness ~0.8 in Y, ~70% pulled toward
  // the two logarithmic spiral arms (same phase as galaxy.frag).
  // Deterministic LCG so screenshots stay stable run to run.
  // Plus 4 bright named stars near the nucleus for close-range labels.
  const int kDiskCount = 4500;
  const float kPi = 3.14159265358979323846f;

  m_m31Bright = {
      {{2.5f, 0.2f, 4.0f}, "AE Andromedae", "Luminous blue variable in M31"},
      {{-2.2f, -0.3f, 3.5f}, "AF Andromedae", "Luminous blue variable in M31"},
      {{0.8f, 0.3f, 5.0f}, "S Andromedae", "SN 1885, Type Ia supernova"},
      {{-0.8f, 0.5f, 6.0f}, "M31N 2008-12a", "Recurrent nova in M31"},
  };

  uint32_t state = 0xA531A531u;
  auto next01 = [&state]() -> float {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
  };

  std::vector<glm::vec3> pos;
  std::vector<float> mags;
  pos.reserve(static_cast<size_t>(kDiskCount) + m_m31Bright.size());
  mags.reserve(static_cast<size_t>(kDiskCount) + m_m31Bright.size());

  for (int i = 0; i < kDiskCount; ++i) {
    float u = next01();
    float v = next01();
    float w1 = next01();
    float w2 = next01();
    float w3 = next01();
    float m = next01();
    // Uniform-area radius in annulus 6..30.
    float r = std::sqrt(36.0f + (900.0f - 36.0f) * u);
    float th = 2.0f * kPi * v;
    // Pull toward spiral arms (galaxy.frag phase: 2*phi - log(r)*3.5).
    float armPhase = 2.0f * th - std::log(std::max(r, 0.01f)) * 3.5f;
    float pull = (w3 < 0.30f) ? 0.0f : 1.0f;
    th -= pull * 0.30f * std::sin(armPhase);
    // Thickness ~0.8: sum of 3 uniforms ~= gaussian around the plane.
    float y = (w1 + w2 + next01() - 1.5f) * 0.53f;
    pos.emplace_back(std::cos(th) * r, y, std::sin(th) * r);
    float mag;
    if (m < 0.05f) {
      mag = 0.0f + next01() * 1.5f;  // leaders
    } else if (m < 0.20f) {
      mag = 1.5f + next01() * 1.0f;
    } else {
      mag = 2.5f + next01() * 2.5f;  // disk background
    }
    mags.push_back(mag);
  }

  for (const auto& b : m_m31Bright) {
    pos.push_back(b.pos);
    mags.push_back(0.0f);
  }

  m_m31Stars.setStars(pos, mags);
}

float SceneManager::tickSimTime(float time) {
  float dt = (m_lastTime < 0.0f) ? 0.0f : (time - m_lastTime);
  m_lastTime = time;
  if (dt < 0.0f) dt = 0.0f;
  if (dt > 0.5f) dt = 0.5f;
  if (!m_paused) m_simTime += dt * m_timeScale;
  return dt;
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

  // PH: never spawn inside the station collision shell (BH capture radius
  // rs*1.05, quasar core-touch radius). No-op for all current spawns
  // (sgra 10 > 1.05, m87 14 > 1.575, qso 25 > 2.0), harness frames kept.
  if (st->collisionRadius > 0.0) {
    double d = std::sqrt(camera.position.x * camera.position.x +
                         camera.position.y * camera.position.y +
                         camera.position.z * camera.position.z);
    if (d < st->collisionRadius && d > 1e-9) {
      double k = st->collisionRadius / d;
      camera.position = Vec3(camera.position.x * k, camera.position.y * k,
                             camera.position.z * k);
    }
  }

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

  // === PostFX === весь кадр идёт в scene FBO, композит в конце render():
  // и петля, и харнесс скриншотов зовут только этот метод.
  m_postFx.beginScene(fbWidth, fbHeight);

  const float aspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight > 0 ? fbHeight : 1);
  const Vec3 cp = camera.position;
  const Vec3 cf = camera.forward();
  const Vec3 cr = camera.right();
  const Vec3 cu = camera.up();

  // === Relativity === скорость наблюдателя: дельта позиции камеры за кадр.
  // dt <= 0 (константный time в харнессе) => скорость нулевая, beta = 0,
  // доплер/абберация выключены. Телепорт (дельта > 1500 ед./с) сбрасывает.
  {
    const float relDt = (m_relTime >= 0.0f) ? (time - m_relTime) : 0.0f;
    if (relDt > 1e-5f) {
      Vec3 vel((cp.x - m_relPos.x) / static_cast<double>(relDt),
               (cp.y - m_relPos.y) / static_cast<double>(relDt),
               (cp.z - m_relPos.z) / static_cast<double>(relDt));
      const double sp =
          std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
      if (sp > 1500.0) {
        m_relVel = Vec3(0.0, 0.0, 0.0);
      } else {
        const double k = 1.0 - std::exp(-static_cast<double>(relDt) * 5.0);
        m_relVel.x += (vel.x - m_relVel.x) * k;
        m_relVel.y += (vel.y - m_relVel.y) * k;
        m_relVel.z += (vel.z - m_relVel.z) * k;
      }
    } else {
      m_relVel = Vec3(0.0, 0.0, 0.0);
    }
    m_relTime = time;
    m_relPos = cp;
    const double rsp =
        std::sqrt(m_relVel.x * m_relVel.x + m_relVel.y * m_relVel.y +
                  m_relVel.z * m_relVel.z);
    double beta = rsp / 60.0;  // референс-скорость эффекта
    if (beta < 0.0) beta = 0.0;
    if (beta > 0.95) beta = 0.95;
    m_relBeta = static_cast<float>(beta);
    if (rsp > 1e-6) {
      m_relDir.x = m_relVel.x / rsp;
      m_relDir.y = m_relVel.y / rsp;
      m_relDir.z = m_relVel.z / rsp;
    }
  }

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
    m_bhShader->setUniform1i("u_JetOn", (m_currentStation->id == "m87" ? 1 : 0));
    const bool isSgra = (m_currentStation->id == "sgra");
    const bool isM87 = (m_currentStation->id == "m87");
    m_bhShader->setUniform1f("u_BHRingK", isSgra ? 1.8f : (isM87 ? 1.25f : 1.0f));
    m_bhShader->setUniform1f("u_ThinDisk", isSgra ? 1.0f : 0.0f);
    m_bhShader->setUniform1f("u_BandK", isSgra ? 1.7f : 1.0f);
    // Close-camera distance in Rs: drives disk-detail smoothing and pole
    // damping in bh.frag (u_CamDist <= 0 means far/legacy on shader side).
    {
      double camLen =
          std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
      float camDistRs = static_cast<float>(
          camLen / std::max(static_cast<double>(m_settings.rs), 1e-3));
      m_bhShader->setUniform1f("u_CamDist", camDistRs);
    }
    m_bhShader->setUniform1f("u_Time", time);
    m_bhShader->setUniform1f("u_Exposure", m_exposure);
    m_bhShader->setUniform1i("u_Skybox", 0);
    // === Relativity === абберация направления на звёзды при быстром полёте.
    m_bhShader->setUniform3f("u_VelDir", static_cast<float>(m_relDir.x),
                             static_cast<float>(m_relDir.y),
                             static_cast<float>(m_relDir.z));
    m_bhShader->setUniform1f("u_Beta", m_relBeta);

    m_skyboxDeep->bind(0);
    m_screenQuad->draw();
    m_bhShader->unbind();

    if (isSgra) {
      // S-star swarm: ~250 fast white-blue points orbiting the center
      // (r 0.8-2.5, angle from u_Time). Separate additive star-billboard
      // pass on top of the raytraced frame, depth write off.
      if (!m_sStarsInit) {
        initSStars();
        m_sStarsInit = true;
      }
      std::vector<glm::vec3> sPos;
      std::vector<float> sMags;
      sPos.reserve(m_sStarParams.size());
      sMags.reserve(m_sStarParams.size());
      for (const auto& s : m_sStarParams) {
        float th = s.phase + time * s.omega;
        float c = std::cos(th);
        float si = std::sin(th);
        sPos.emplace_back(s.r * (c * s.u.x + si * s.v.x),
                          s.r * (c * s.u.y + si * s.v.y),
                          s.r * (c * s.u.z + si * s.v.z));
        sMags.push_back(s.mag);
      }
      m_sStars.setStars(sPos, sMags);
      // PH: far plane 1e7 (was 1000): the swarm must not clip when the
      // camera pulls far back; near kept, spawn framing pixel-identical.
      glm::mat4 sProj =
          glm::perspective(glm::radians(m_settings.fovDegrees), aspect, 0.1f, 1e7f);
      glm::vec3 sEye(cp.x, cp.y, cp.z);
      glm::vec3 sFwd(cf.x, cf.y, cf.z);
      glm::vec3 sUp(cu.x, cu.y, cu.z);
      glm::mat4 sView = glm::lookAt(sEye, sEye + sFwd, sUp);
      glm::mat4 sMvp = sProj * sView;
      const float sPixelScale = static_cast<float>(fbHeight) /
          (2.0f * std::tan(glm::radians(m_settings.fovDegrees) * 0.5f));
      m_starShader->bind();
      m_starShader->setUniformMat4f("u_MVP", sMvp);
      m_starShader->setUniformMat4f("u_MV", sView);
      m_starShader->setUniform1f("u_PixelScale", sPixelScale);
      m_starShader->setUniform1f("u_SizeK", 0.045f);
      m_starShader->setUniform1f("u_Time", time);
      glDepthMask(GL_FALSE);
      glDisable(GL_DEPTH_TEST);
      m_sStars.draw();
      glDepthMask(GL_TRUE);
      m_starShader->unbind();
    }
  } else if (m_currentStation->type == StationType::Quasar) {
    // 3C 273 Quasar disk + jets + glow
    m_quasarShader->bind();
    m_quasarShader->setUniform3f("u_CoreColor", 1.0f, 0.85f, 0.65f);
    m_quasarShader->setUniform1f("u_Time", time);
    m_quasarShader->setUniform1f("u_Exposure", m_exposure);
    // Bright star-like core dominates the host, structure preserved (host x0.875).
    m_quasarShader->setUniform1f("u_QCoreK",
                                 (m_currentStation->id == "qso-3c273" ? 1.5f : 1.0f));
    // Close-up zoom from camera distance: spawn -> 1.0 (neutral framing),
    // closer -> magnified shadow for inspection (qso_close helper frame).
    {
      double qd = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
      double qs = m_currentStation->spawnDistanceRs;
      if (qs <= 0.0) qs = 25.0;
      float qzoom =
          static_cast<float>(std::clamp(qs / std::max(qd, 1e-3), 1.0, 40.0));
      m_quasarShader->setUniform1f("u_QZoom", qzoom);
    }

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
    m_starShader->setUniform1f("u_Time", time);
    m_bgStars.draw();
    m_starShader->unbind();

    m_linesShader->bind();
    m_linesShader->setUniformMat4f("u_MVP", mvp);
    m_linesShader->setUniformMat4f("u_Model", glm::mat4(1.0f));
    m_linesShader->setUniform1f("u_Time", time);

    if (m_currentStation->id == "ori") {
      // Orion: bright cyan lines
      m_linesShader->setUniform3f("u_LineColor", 0.45f, 0.85f, 1.0f);
      m_orionMesh.drawLines();
    } else {
      // Big Dipper: cyan-amber lines
      m_linesShader->setUniform3f("u_LineColor", 0.50f, 0.90f, 1.0f);
      m_umaMesh.drawLines();
    }

    m_linesShader->unbind();

    m_starShader->bind();
    m_starShader->setUniformMat4f("u_MVP", mvp);
    m_starShader->setUniformMat4f("u_MV", view);
    m_starShader->setUniform1f("u_PixelScale", pixelScale);
    m_starShader->setUniform1f("u_SizeK", 0.22f);
    m_starShader->setUniform1f("u_Time", time);
    if (m_currentStation->id == "ori") {
      m_orionStars.draw();
    } else {
      m_umaStars.draw();
    }
    m_starShader->unbind();
  } else if (m_currentStation->type == StationType::Galaxy) {
    // M31 Andromeda Galaxy impostor sprite
    tickSimTime(time);
    m_galaxyShader->bind();
    m_galaxyShader->setUniform1f("u_Time", time);
    m_galaxyShader->setUniform1f("u_SpinAngle", m_simTime * kM31SpinRate);
    m_galaxyShader->setUniform1f("u_Aspect", aspect);

    double dist = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
    double spawn = m_currentStation->spawnDistanceRs;
    float zoom = static_cast<float>(std::clamp(spawn / std::max(dist, 1e-3), 0.2, 4.0));
    m_galaxyShader->setUniform1f("u_Zoom", zoom);
    m_galaxyShader->setUniform1f("u_M31Dist", static_cast<float>(dist));
    m_galaxyShader->setUniform1f("u_Exposure", m_exposure);

    // Per-galaxy look by station id. Defaults = M31 constants 1:1, so an
    // unknown id (and m31 itself) renders exactly as before.
    float galSeed = 0.0f, galArms = 2.0f, galPitch = 3.5f;
    float galBulge = 8.5f, galDust = 1.0f, galArmGain = 1.0f;
    const std::string& galId = m_currentStation->id;
    if (galId == "m51") {
      galSeed = 3.7f; galArms = 2.0f; galPitch = 2.2f;
      galBulge = 4.5f; galDust = 0.7f; galArmGain = 1.2f;
    } else if (galId == "m104") {
      galSeed = 9.1f; galArms = 2.0f; galPitch = 4.2f;
      galBulge = 12.0f; galDust = 1.6f; galArmGain = 0.45f;
    }
    m_galaxyShader->setUniform1f("u_GalSeed", galSeed);
    m_galaxyShader->setUniform1f("u_GalArms", galArms);
    m_galaxyShader->setUniform1f("u_GalPitch", galPitch);
    m_galaxyShader->setUniform1f("u_GalBulge", galBulge);
    m_galaxyShader->setUniform1f("u_GalDust", galDust);
    m_galaxyShader->setUniform1f("u_GalArmGain", galArmGain);

    m_screenQuad->draw();
    m_galaxyShader->unbind();

    // M31 LOD only: NEAR (18 -> 6) volumetric disk starfield, alpha = near.
    // Additive points, no depth write, drawn over the faded impostor.
    if (m_currentStation->id == "m31") {
      float dF = static_cast<float>(dist);
      float near = 1.0f - glm::smoothstep(6.0f, 18.0f, dF);
      if (near > 0.003f) {
        glm::mat4 proj = glm::perspective(glm::radians(m_settings.fovDegrees), aspect, 0.1f, 1000.0f);
        glm::vec3 eye(static_cast<float>(cp.x), static_cast<float>(cp.y),
                      static_cast<float>(cp.z));
        glm::vec3 fwd(static_cast<float>(cf.x), static_cast<float>(cf.y),
                      static_cast<float>(cf.z));
        glm::vec3 up(static_cast<float>(cu.x), static_cast<float>(cu.y),
                     static_cast<float>(cu.z));
        glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
        glm::mat4 mvp = proj * view;

        const float pixelScale = static_cast<float>(fbHeight) /
            (2.0f * std::tan(glm::radians(m_settings.fovDegrees) * 0.5f));
        m_m31StarShader->bind();
        m_m31StarShader->setUniformMat4f("u_MVP", mvp);
        m_m31StarShader->setUniformMat4f("u_MV", view);
        m_m31StarShader->setUniform1f("u_PixelScale", pixelScale);
        m_m31StarShader->setUniform1f("u_SizeK", 0.08f);
        m_m31StarShader->setUniform1f("u_GlobalAlpha", near);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        m_m31Stars.draw();
        glDepthMask(GL_TRUE);
        m_m31StarShader->unbind();
      }
    }
  } else if (m_currentStation->type == StationType::Nebula) {
    // M1 Crab Nebula impostor sprite
    m_nebulaShader->bind();
    m_nebulaShader->setUniform1f("u_Time", time);
    m_nebulaShader->setUniform1f("u_Aspect", aspect);
    m_nebulaShader->setUniform1f("u_Exposure", m_exposure);
    m_screenQuad->draw();
    m_nebulaShader->unbind();
  } else if (m_currentStation->type == StationType::SolarSystem) {
    float dt = tickSimTime(time);
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
    // === Relativity === абберация направления на звёзды при быстром полёте.
    m_skyShader->setUniform3f("u_VelDir", static_cast<float>(m_relDir.x),
                              static_cast<float>(m_relDir.y),
                              static_cast<float>(m_relDir.z));
    m_skyShader->setUniform1f("u_Beta", m_relBeta);
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
      // Belt shimmer: subtle per-frame glint of the whole belt
      // (fixed phase at fixed time, screenshots stay deterministic).
      float beltTw = 0.85f + 0.15f * std::sin(time * 2.3f);
      m_linesShader->setUniform3f("u_LineColor", 0.45f * beltTw, 0.38f * beltTw, 0.30f * beltTw);
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
      m_planetShader->setUniform1i("u_Body", def->detail);
      m_planetShader->setUniform1f("u_Seed", def->seed);
      m_planetShader->setUniform3f("u_ColA", static_cast<float>(def->colA.x),
                                   static_cast<float>(def->colA.y),
                                   static_cast<float>(def->colA.z));
      m_planetShader->setUniform3f("u_ColB", static_cast<float>(def->colB.x),
                                   static_cast<float>(def->colB.y),
                                   static_cast<float>(def->colB.z));
      m_planetShader->setUniform1i("u_Spot", (def->id == "jupiter" ? 1 : 0));
      m_planetShader->setUniform1i("u_Clouds", (def->id == "earth" ? 1 : 0));
      m_planetShader->setUniform1f("u_SpinAngle", def->spinRate * m_simTime);
      m_sphere.draw();
      if (def->hasRing) {
        m_ringShader->bind();
        m_ringShader->setUniformMat4f("u_MVP", mvp);
        glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), bpos) *
                              glm::rotate(glm::mat4(1.0f), 0.45f, glm::vec3(1.0f, 0.0f, 0.0f));
        // Rings ride with the planet spin (purely visual: stripes are
        // radial). Skipped at zero angle to keep the matrix bit-exact.
        float ringSpin = def->spinRate * m_simTime;
        if (ringSpin != 0.0f) {
          ringModel = ringModel *
                      glm::rotate(glm::mat4(1.0f), ringSpin, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        m_ringShader->setUniformMat4f("u_Model", ringModel);
        m_ringShader->setUniform1f("u_Seed", def->seed);
        m_ringShader->setUniform1f("u_Time", time);
        m_ringShader->setUniform3f("u_Center", static_cast<float>(bpos.x),
                                  static_cast<float>(bpos.y),
                                  static_cast<float>(bpos.z));
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
      m_planetShader->setUniform1i("u_Body", 0);
      m_planetShader->setUniform1f("u_Seed", custom.seed);
      m_planetShader->setUniform3f("u_ColA", static_cast<float>(custom.color.x),
                                   static_cast<float>(custom.color.y),
                                   static_cast<float>(custom.color.z));
      m_planetShader->setUniform3f("u_ColB", static_cast<float>(custom.color.x) * 0.6f,
                                   static_cast<float>(custom.color.y) * 0.6f,
                                   static_cast<float>(custom.color.z) * 0.6f);
      m_planetShader->setUniform1i("u_Spot", 0);
      m_planetShader->setUniform1i("u_Clouds", 0);
      m_planetShader->setUniform1f("u_SpinAngle", 0.0f);
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
        m_cometShader->bind();
        m_cometShader->setUniformMat4f("u_MVP", mvp);
        m_cometShader->setUniform1f("u_Seed", cometDef->seed);
        m_cometShader->setUniform1f("u_Exposure", m_exposure);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        // Ion tail: long straight thin blue spike anti-sunward.
        glm::mat4 ionModel = glm::translate(glm::mat4(1.0f), cometPos) *
                             basis *
                             glm::scale(glm::mat4(1.0f), glm::vec3(len, 0.5f, 0.5f));
        m_cometShader->setUniformMat4f("u_Model", ionModel);
        m_cometShader->setUniform3f("u_ColA", 0.45f, 0.70f, 1.0f);
        m_sphere.draw();
        // Dust tail: shorter warm-white fan, tilted off the ion axis.
        glm::vec3 dustDir = glm::normalize(tailDir * 0.92f + glm::vec3(0.0f, 0.28f, 0.14f));
        glm::vec3 dby = glm::normalize(glm::cross(tmpUp, dustDir));
        glm::vec3 dbz = glm::cross(dustDir, dby);
        glm::mat4 dustBasis(1.0f);
        dustBasis[0] = glm::vec4(dustDir, 0.0f);
        dustBasis[1] = glm::vec4(dby, 0.0f);
        dustBasis[2] = glm::vec4(dbz, 0.0f);
        glm::mat4 dustModel = glm::translate(glm::mat4(1.0f), cometPos) *
                              dustBasis *
                              glm::scale(glm::mat4(1.0f), glm::vec3(len * 0.45f, 1.6f, 0.9f));
        m_cometShader->setUniformMat4f("u_Model", dustModel);
        m_cometShader->setUniform3f("u_ColA", 1.0f, 0.93f, 0.80f);
        m_sphere.draw();
        glDisable(GL_BLEND);
        m_cometShader->unbind();
      }
    }
    // Sun corona shell: additive 1.5x-radius halo with prominence tongues
    // (sun_corona.frag, centre transparent so the disc shows through).
    {
      glm::vec3 sunPos(0.0f);
      float sunR = 0.0f;
      for (const auto& body : m_lastBodies) {
        if (body.def && body.def->id == "sun") {
          sunPos = glm::vec3(static_cast<float>(body.pos.x),
                             static_cast<float>(body.pos.y),
                             static_cast<float>(body.pos.z));
          sunR = body.def->radius;
          break;
        }
      }
      if (sunR > 0.0f) {
        static std::unique_ptr<Shader> s_sunCorona;
        if (!s_sunCorona) {
          s_sunCorona = std::make_unique<Shader>("shaders/planet.vert",
                                                 "shaders/sun_corona.frag");
        }
        glm::mat4 coronaModel =
            glm::translate(glm::mat4(1.0f), sunPos) *
            glm::scale(glm::mat4(1.0f), glm::vec3(sunR * 1.5f));
        s_sunCorona->bind();
        s_sunCorona->setUniformMat4f("u_MVP", mvp);
        s_sunCorona->setUniformMat4f("u_Model", coronaModel);
        s_sunCorona->setUniform1f("u_Time", time);
        s_sunCorona->setUniform3f("u_CamPos", static_cast<float>(cp.x),
                                  static_cast<float>(cp.y),
                                  static_cast<float>(cp.z));
        s_sunCorona->setUniform1f("u_Exposure", m_exposure);
        s_sunCorona->setUniform1f("u_Seed", 1.0f);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        m_sphere.draw();
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        s_sunCorona->unbind();
      }
    }
    glDisable(GL_DEPTH_TEST);
  }

  // === WarpDrive === (агент W: общий хвост — uniform u_WarpK + оверлей).
  // Свой warp.frag: радиальные стрики + виньетка + хроматическая кайма +
  // вспышка. При u_WarpK == 0 и u_Flash == 0 скипаем целиком (ни одного
  // GL-вызова), поэтому скриншоты байт-в-байт с HEAD. Физику bh/quasar,
  // m_exposure и ветки станций не трогаем.
  // === WarpJump === (агент J: +u_Burst и корабль-силуэт поверх оверлея).
  // Корабль виден ТОЛЬКО в Pullback/Travel/Arrival/Pushin (флаг скрыт по
  // дефолту — вне катсцены ни одного GL-вызова). Геометрия статична в
  // локальных координатах (нос -Z, ~1.5 ед), в мир — через u_Model
  // (translate + yaw из shipDir). Контур-дарт: 2 LineMesh (корпус +
  // кабина/пилоны, ~30 точек хардкодом), движок — StarPoints 1 шт крупный
  // пульсирующий (mag<1 = голубой в star.frag), трейл — LineMesh-лента
  // 20 сегментов за кормой (per-vertex alpha у LineMesh нет — равномерный
  // тусклый голубой). Инициализация ленивая внутри visible-ветви.
  {
    float warpK = warpdrive::warpK();
    float warpFlash = warpdrive::warpFlash();
    float burst = warpdrive::burst();
    if (warpK > 0.0005f || warpFlash > 0.0005f || burst > 0.0005f) {
      if (warpK < 0.0f) warpK = 0.0f;
      if (warpK > 1.0f) warpK = 1.0f;
      if (warpFlash < 0.0f) warpFlash = 0.0f;
      if (warpFlash > 1.0f) warpFlash = 1.0f;
      if (burst < 0.0f) burst = 0.0f;
      if (burst > 1.0f) burst = 1.0f;
      try {
        static bool s_warpFailed = false;
        static Shader* s_warpShader = nullptr;
        if (!s_warpFailed && !s_warpShader) {
          try {
            s_warpShader = new Shader("shaders/quad.vert", "shaders/warp.frag");
          } catch (...) {
            s_warpFailed = true;
          }
        }
        if (s_warpShader) {
          glDisable(GL_DEPTH_TEST);
          glDepthMask(GL_FALSE);
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          s_warpShader->bind();
          s_warpShader->setUniform1f("u_WarpK", warpK);
          s_warpShader->setUniform1f("u_Flash", warpFlash);
          s_warpShader->setUniform1f("u_Burst", burst);
          s_warpShader->setUniform1f("u_Aspect", aspect);
          s_warpShader->setUniform1f("u_Time", time);
          m_screenQuad->draw();
          s_warpShader->unbind();
          glDisable(GL_BLEND);
          glDepthMask(GL_TRUE);
        }
      } catch (...) {
        // Оверлей никогда не ломает кадр.
      }
    }
    // Корабль-силуэт: поверх оверлея, чтобы emissive-контур читался.
    if (warpdrive::shipVisible()) {
      try {
        static bool s_shipInit = false;
        static LineMesh s_hull;
        static LineMesh s_keel;
        static LineMesh s_trail;
        static StarPoints s_engine;
        if (!s_shipInit) {
          // Корпус-дарт: нос + крылья + хвост + киль (нос смотрит -Z).
          const std::vector<glm::vec3> hull = {
              // N-L, N-R
              {0.0f, 0.0f, -0.9f}, {-0.75f, 0.0f, 0.55f},
              {0.0f, 0.0f, -0.9f}, {0.75f, 0.0f, 0.55f},
              // L-T, R-T (хвостовая кромка через центр)
              {-0.75f, 0.0f, 0.55f}, {0.0f, 0.0f, 0.35f},
              {0.75f, 0.0f, 0.55f}, {0.0f, 0.0f, 0.35f},
              // L-R (задняя кромка)
              {-0.75f, 0.0f, 0.55f}, {0.75f, 0.0f, 0.55f},
              // N-K, K-T (киль)
              {0.0f, 0.0f, -0.9f}, {0.0f, 0.28f, 0.30f},
              {0.0f, 0.28f, 0.30f}, {0.0f, 0.0f, 0.35f},
              // K-L, K-R (киль к крыльям)
              {0.0f, 0.28f, 0.30f}, {-0.75f, 0.0f, 0.55f},
              {0.0f, 0.28f, 0.30f}, {0.75f, 0.0f, 0.55f},
          };
          std::vector<unsigned int> hullIdx(hull.size());
          for (unsigned int i = 0; i < hullIdx.size(); ++i) hullIdx[i] = i;
          s_hull.setGeometry(hull, hullIdx);
          // Кабина + пилоны к крыльям.
          const std::vector<glm::vec3> keel = {
              {0.0f, 0.02f, -0.45f}, {0.0f, 0.14f, -0.10f},
              {0.0f, 0.14f, -0.10f}, {0.0f, 0.14f, 0.30f},
              {0.0f, 0.14f, -0.10f}, {-0.75f, 0.0f, 0.55f},
              {0.0f, 0.14f, -0.10f}, {0.75f, 0.0f, 0.55f},
          };
          std::vector<unsigned int> keelIdx(keel.size());
          for (unsigned int i = 0; i < keelIdx.size(); ++i) keelIdx[i] = i;
          s_keel.setGeometry(keel, keelIdx);
          // Трейл-лента: 20 поперечных пар за кормой (z 0.55 -> 2.75,
          // полуширина 0.30 -> 0.03) + 2 продольных ребра.
          std::vector<glm::vec3> trail;
          trail.reserve(120);
          constexpr int kSeg = 20;
          for (int i = 0; i < kSeg; ++i) {
            const float f0 = static_cast<float>(i) / kSeg;
            const float z = 0.55f + f0 * 2.2f;
            const float hw = 0.30f + (0.03f - 0.30f) * f0;
            trail.emplace_back(-hw, 0.0f, z);
            trail.emplace_back(hw, 0.0f, z);
          }
          for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < kSeg; ++i) {
              const float f0 = static_cast<float>(i) / kSeg;
              const float f1 = static_cast<float>(i + 1) / kSeg;
              const float hw0 = 0.30f + (0.03f - 0.30f) * f0;
              const float hw1 = 0.30f + (0.03f - 0.30f) * f1;
              trail.emplace_back(side * hw0, 0.0f, 0.55f + f0 * 2.2f);
              trail.emplace_back(side * hw1, 0.0f, 0.55f + f1 * 2.2f);
            }
          }
          std::vector<unsigned int> trailIdx(trail.size());
          for (unsigned int i = 0; i < trailIdx.size(); ++i) trailIdx[i] = i;
          s_trail.setGeometry(trail, trailIdx);
          s_shipInit = true;
        }
        const glm::vec3 shipPos(warpdrive::shipX(), warpdrive::shipY(),
                                warpdrive::shipZ());
        const glm::mat4 shipModel =
            glm::translate(glm::mat4(1.0f), shipPos) *
            glm::rotate(glm::mat4(1.0f), warpdrive::shipYaw(),
                        glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(m_settings.fovDegrees),
                                          aspect, 0.1f, 1000.0f);
        glm::vec3 eye(cp.x, cp.y, cp.z);
        glm::vec3 fwd(cf.x, cf.y, cf.z);
        glm::vec3 up(cu.x, cu.y, cu.z);
        glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
        glm::mat4 mvp = proj * view;
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        m_linesShader->bind();
        m_linesShader->setUniformMat4f("u_MVP", mvp);
        m_linesShader->setUniformMat4f("u_Model", shipModel);
        m_linesShader->setUniform1f("u_Time", time);
        // Emissive-белый контур корпуса + голубоватая кабина.
        m_linesShader->setUniform3f("u_LineColor", 0.92f, 0.96f, 1.0f);
        s_hull.drawLines();
        m_linesShader->setUniform3f("u_LineColor", 0.55f, 0.78f, 1.0f);
        s_keel.drawLines();
        // Тусклый равномерный шлейф (per-vertex alpha у LineMesh нет).
        m_linesShader->setUniform3f("u_LineColor", 0.25f, 0.45f, 0.70f);
        s_trail.drawLines();
        m_linesShader->unbind();
        // Движок: 1 крупная пульсирующая голубая точка за кормой.
        {
          const float sy = warpdrive::shipYaw();
          const float ex = warpdrive::shipX() + 0.55f * std::sin(sy);
          const float ez = warpdrive::shipZ() + 0.55f * std::cos(sy);
          const float mag = 0.1f + 0.35f * std::sin(time * 18.0f);
          s_engine.setStars({glm::vec3(ex, warpdrive::shipY(), ez)}, {mag});
          const float pixelScale = static_cast<float>(fbHeight) /
              (2.0f * std::tan(glm::radians(m_settings.fovDegrees) * 0.5f));
          m_starShader->bind();
          m_starShader->setUniformMat4f("u_MVP", mvp);
          m_starShader->setUniformMat4f("u_MV", view);
          m_starShader->setUniform1f("u_PixelScale", pixelScale);
          m_starShader->setUniform1f("u_SizeK", 0.22f);
          m_starShader->setUniform1f("u_Time", time);
          s_engine.draw();
          m_starShader->unbind();
        }
        glDepthMask(GL_TRUE);
      } catch (...) {
        // Корабль никогда не ломает кадр.
      }
    }
  }

  // === Relativity === базис камеры и скорость для доплера-тинта в композите.
  {
    const float camFwd[3] = {static_cast<float>(cf.x), static_cast<float>(cf.y),
                             static_cast<float>(cf.z)};
    const float camRight[3] = {static_cast<float>(cr.x),
                               static_cast<float>(cr.y),
                               static_cast<float>(cr.z)};
    const float camUp[3] = {static_cast<float>(cu.x), static_cast<float>(cu.y),
                            static_cast<float>(cu.z)};
    const float velDir[3] = {static_cast<float>(m_relDir.x),
                             static_cast<float>(m_relDir.y),
                             static_cast<float>(m_relDir.z)};
    m_postFx.setFlight(camFwd, camRight, camUp,
                       glm::radians(m_settings.fovDegrees), aspect, velDir,
                       m_relBeta);
  }
  m_postFx.endScene(fbWidth, fbHeight);
}

}  // namespace astro
