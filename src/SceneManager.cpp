#include "SceneManager.h"
#include "Constellation.h"

#include "GLCompat.h"
#include "ObjModel.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <charconv>
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
float g_shipThruster = 0.7f;  // v3.3.0: тяга 0..1 (шлейф/движок от скорости)
int g_shipStyle = 0;  // 0 Dart, 1 Ring Ship (IXS), 2 Shuttle, 3 Model (OBJ)
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
void setShipStyle(int s) { g_shipStyle = (s < 0 || s > 3) ? 0 : s; }
// v3.6.0 (пасс D): астероидное поле в Travel. Дефолт 0: харнесс не видит
// катсцену и ни одного GL-вызова не получает — PNG остаются байт-в-байт.
float g_astK = 0.0f;
float g_astLane = 0.0f;
void setAsteroids(float k, float lane) {
  g_astK = (k < 0.0f) ? 0.0f : ((k > 1.0f) ? 1.0f : k);
  g_astLane = (lane < -1.0f) ? -1.0f : ((lane > 1.0f) ? 1.0f : lane);
}
float astK() { return g_astK; }
float astLane() { return g_astLane; }
int shipStyle() { return g_shipStyle; }
void setThruster(float k) {
  if (k < 0.0f) k = 0.0f;
  if (k > 1.0f) k = 1.0f;
  g_shipThruster = k;
}
float thruster() { return g_shipThruster; }
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
  m_pulsarShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/pulsar.frag");
  m_planetShader = std::make_unique<Shader>("shaders/planet.vert", "shaders/planet.frag");
  m_cometShader = std::make_unique<Shader>("shaders/planet.vert", "shaders/comet.frag");
  m_ringShader = std::make_unique<Shader>("shaders/ring.vert", "shaders/ring.frag");
  m_skyShader = std::make_unique<Shader>("shaders/quad.vert", "shaders/sky.frag");
  m_skyboxDeep = std::make_unique<Cubemap>();
  m_skyboxLocal = std::make_unique<Cubemap>();
  // Mip-цепочка только у видимого скайбокса (IBL корабля); deep-кубемап
  // (линзирование в bh.frag) остаётся без мипов — эталоны не меняются.
  m_skyboxLocal->setMipmaps(true);
  m_skyboxLocal->regenerate(0.9985f, 0.40f, 1234u);

  initConstellationOrion();
  initConstellationUma();
    initM31Disk();
    initGalaxyDisks();
    initCluster();

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
  // v3.2.0: denser, dimmer belt reads as a dust band instead of speckles.
  m_belt.setBelt(5200, 19.5f, 22.5f);

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

static const char* customIdPrefix(const CustomBody& b) {
  if (b.type == 0 || b.detail == 1) return "star";
  if (b.detail >= 7) return "gas";
  if (b.detail == 5) return "moon";
  if (b.detail == 2) return "planet";
  return "rock";
}

static const char* customNamePrefix(const CustomBody& b) {
  if (b.type == 0 || b.detail == 1) return "Star";
  if (b.detail >= 7) return "Gas giant";
  if (b.detail == 5) return "Moon";
  if (b.detail == 2) return "Planet";
  return "Rock";
}

void SceneManager::spawnRockAt(const Vec3& p, const Vec3& v) {
  CustomBody b;
  b.radius = 0.3f;
  b.color = Vec3(0.62, 0.58, 0.52);
  b.colorB = Vec3(0.62 * 0.6, 0.58 * 0.6, 0.52 * 0.6);
  b.type = 2;
  b.detail = 0;
  b.seed = 0.0f;  // auto: sequence number (legacy rock behaviour)
  spawnCustom(b, p, v);
}

void SceneManager::spawnCustom(const CustomBody& templ, const Vec3& p, const Vec3& v) {
  ++m_customSeq;
  CustomBody b = templ;
  b.id = std::string(customIdPrefix(b)) + "-" + std::to_string(m_customSeq);
  b.name = std::string(customNamePrefix(b)) + " " + std::to_string(m_customSeq);
  b.pos = p;
  b.vel = v;
  if (b.seed == 0.0f) b.seed = static_cast<float>(m_customSeq);
  m_custom.push_back(b);
}

CustomBody* SceneManager::findCustom(const std::string& id) {
  for (auto& b : m_custom) {
    if (b.id == id) return &b;
  }
  return nullptr;
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

void SceneManager::addLoadedCustom(const CustomBody& b) {
  m_custom.push_back(b);
  const size_t dash = b.id.rfind('-');
  if (dash != std::string::npos && dash + 1 < b.id.size()) {
    int n = 0;
    const char* first = b.id.data() + dash + 1;
    const char* last = b.id.data() + b.id.size();
    const auto res = std::from_chars(first, last, n);
    if (res.ec == std::errc() && n > m_customSeq) {
      m_customSeq = n;
    }
  }
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

void SceneManager::genDiskPoints(uint32_t seed, int count, float rMin, float rMax,
                                 float thick, float armPull,
                                 const std::vector<M31BrightStar>& heroes,
                                 std::vector<glm::vec3>& pos,
                                 std::vector<float>& mags) {
  // Shared deterministic disk generator. LCG call order mirrors the legacy
  // initM31Disk body exactly, so m31 output is bit-identical for its
  // constants (seed 0xA531A531, 4500, 6..30, thick 0.53, pull 0.30).
  const float kPi = 3.14159265358979323846f;
  uint32_t state = seed;
  auto next01 = [&state]() -> float {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
  };
  pos.reserve(static_cast<size_t>(count) + heroes.size());
  mags.reserve(static_cast<size_t>(count) + heroes.size());
  const float rMin2 = rMin * rMin;
  const float rMax2 = rMax * rMax;
  for (int i = 0; i < count; ++i) {
    float u = next01();
    float v = next01();
    float w1 = next01();
    float w2 = next01();
    float w3 = next01();
    float m = next01();
    // Uniform-area radius in annulus rMin..rMax.
    float r = std::sqrt(rMin2 + (rMax2 - rMin2) * u);
    float th = 2.0f * kPi * v;
    // Pull toward spiral arms (galaxy.frag phase: 2*phi - log(r)*3.5).
    float armPhase = 2.0f * th - std::log(std::max(r, 0.01f)) * 3.5f;
    float pull = (w3 < 0.30f) ? 0.0f : 1.0f;
    th -= pull * armPull * std::sin(armPhase);
    // Thickness: sum of 3 uniforms ~= gaussian around the plane.
    float y = (w1 + w2 + next01() - 1.5f) * thick;
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
  for (const auto& b : heroes) {
    pos.push_back(b.pos);
    mags.push_back(b.mag);
  }
}

void SceneManager::initM31Disk() {
  // M31 volumetric disk starfield: ~4500 points in the disk plane (XZ),
  // radius 6..30 (uniform area), thickness ~0.8 in Y, ~70% pulled toward
  // the two logarithmic spiral arms (same phase as galaxy.frag).
  // Deterministic LCG so screenshots stay stable run to run.
  // Plus 4 bright named stars near the nucleus for close-range labels.
  const int kDiskCount = 4500;

  m_m31Bright = {
      {{2.5f, 0.2f, 4.0f}, "AE Andromedae", "Luminous blue variable in M31"},
      {{-2.2f, -0.3f, 3.5f}, "AF Andromedae", "Luminous blue variable in M31"},
      {{0.8f, 0.3f, 5.0f}, "S Andromedae", "SN 1885, Type Ia supernova"},
      {{-0.8f, 0.5f, 6.0f}, "M31N 2008-12a", "Recurrent nova in M31"},
  };

  std::vector<glm::vec3> pos;
  std::vector<float> mags;
  genDiskPoints(0xA531A531u, kDiskCount, 6.0f, 30.0f, 0.53f, 0.30f,
                m_m31Bright, pos, mags);
  m_m31Stars.setStars(pos, mags);
}

// Sun position inside the Milky Way disk (disk units). Single source:
// initGalaxyDisks places the Sol hero here, sunWorldPos reports it.
static const glm::vec3 kMwSunPos(9.0f, 0.2f, 5.5f);

void SceneManager::initGalaxyDisks() {
  // v3.1.0 Deep Field: approach LOD for m51/m104/mw (M31 pattern:
  // volumetric disk points fading in on approach; see galaxy branch).
  // Distinct deterministic seeds; heroes double as fly-to targets.
  {
    m_m51Bright = {
        {{14.0f, 0.4f, 9.0f}, "SN 2011dh", "Type IIb supernova, June 2011"},
        {{-11.0f, -0.5f, 12.0f}, "M51 X-7", "Ultraluminous X-ray source"},
        {{0.6f, 0.1f, 0.4f}, "Whirlpool Nucleus", "Seyfert 2 nucleus"},
    };
    std::vector<glm::vec3> pos;
    std::vector<float> mags;
    genDiskPoints(0x51A551A5u, 4200, 7.0f, 34.0f, 0.55f, 0.30f, m_m51Bright,
                  pos, mags);
    m_m51Stars.setStars(pos, mags);
  }
  {
    m_m104Bright = {
        {{0.4f, 0.1f, 0.3f}, "Sombrero Nucleus", "Black hole, 10^9 solar masses"},
        {{10.0f, 0.5f, -8.0f}, "Dust-lane knot", "Bright star cloud on the dust lane"},
        {{-6.0f, 2.5f, 9.0f}, "Halo globular", "Bright globular cluster in the halo"},
    };
    std::vector<glm::vec3> pos;
    std::vector<float> mags;
    genDiskPoints(0x10410410u, 3800, 6.0f, 26.0f, 0.50f, 0.30f, m_m104Bright,
                  pos, mags);
    m_m104Stars.setStars(pos, mags);
  }
  {
    M31BrightStar sol;
    sol.pos = kMwSunPos;
    sol.name = "Sol";
    sol.fact = "G2V, our star. Fly within 3 units to go home.";
    sol.mag = -1.5f;
    m_mwBright = {
        sol,
        {{-7.0f, 0.3f, -6.0f}, "Cygnus X-1", "Stellar black hole, HMXB"},
        {{0.0f, 0.0f, 0.0f}, "Galactic Center", "Sgr A* lives here"},
    };
    std::vector<glm::vec3> pos;
    std::vector<float> mags;
    genDiskPoints(0xA577A577u, 4000, 5.0f, 22.0f, 0.45f, 0.30f, m_mwBright,
                  pos, mags);
    m_mwStars.setStars(pos, mags);
  }
}

void SceneManager::initCluster() {
  // v3.6.0: M13-like globular cluster — 5200 points in a puffy Plummer
  // sphere (scale 2.2, capped at 8) so the additive pile-up stays a bright
  // but resolvable core. Magnitudes dimmed vs disk stars. Deterministic LCG.
  // Plus 140 bright giants in a warm tone, drawn separately after the core.
  const float kPi = 3.14159265358979323846f;
  uint32_t state = 0xC131C131u;
  auto next01 = [&state]() -> float {
    state = state * 1664525u + 1013904223u;
    return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
  };
  std::vector<glm::vec3> pos;
  std::vector<float> mags;
  pos.reserve(5200);
  mags.reserve(5200);
  for (int i = 0; i < 5200; ++i) {
    float u = std::max(next01(), 1e-4f);
    float v = next01();
    float w = next01();
    float m = next01();
    float r = 2.2f / std::sqrt(std::pow(u, -2.0f / 3.0f) - 1.0f);
    r = std::min(r, 8.0f);
    float th = 2.0f * kPi * v;
    float ph = std::acos(std::clamp(2.0f * w - 1.0f, -1.0f, 1.0f));
    pos.emplace_back(r * std::sin(ph) * std::cos(th), r * std::cos(ph),
                     r * std::sin(ph) * std::sin(th));
    float mag = 2.0f + 3.0f * m - 0.8f * (1.0f - r / 8.0f);
    mags.push_back(std::max(mag, 0.0f));
  }
  m_clusterStars.setStars(pos, mags);
  // v3.6.0: 140 ярких гигантов ядра в тёплом тоне (размер 0.26, clamp 34px в
  // шейдере через u_MaxPx). OC: они не попадают в другие станции благодаря
  // m_clusterBright.clear() в начале render().
  std::vector<glm::vec3> bpos;
  std::vector<float> bmags;
  bpos.reserve(140);
  bmags.reserve(140);
  uint32_t stateB = 0xB884B884u;
  auto nextB = [&stateB]() -> float {
    stateB = stateB * 1664525u + 1013904223u;
    return static_cast<float>(stateB >> 8) * (1.0f / 16777216.0f);
  };
  for (int i = 0; i < 140; ++i) {
    float u = std::max(nextB(), 1e-4f);
    float v = nextB();
    float w = nextB();
    float r = 2.2f / std::sqrt(std::pow(u, -2.0f / 3.0f) - 1.0f);
    r = std::min(r, 8.0f);
    float th = 2.0f * kPi * v;
    float ph = std::acos(std::clamp(2.0f * w - 1.0f, -1.0f, 1.0f));
    bpos.emplace_back(r * std::sin(ph) * std::cos(th), r * std::cos(ph),
                      r * std::sin(ph) * std::sin(th));
    float mag = 0.4f + 1.4f * nextB() - 0.8f * (1.0f - r / 8.0f);
    bmags.push_back(std::max(mag, 0.0f));
  }
  // Hercules Prime — ярчайшая звезда ядра кластера в центре (origin);
  // рисуется билбордом (как остальные гиганты), а не сферой-планетой,
  // чтобы не раздуваться в «боке»-пузырь при подлёте.
  bpos.emplace_back(0.0f, 0.0f, 0.0f);
  bmags.push_back(-0.5f);
  m_clusterBright.setStars(bpos, bmags);
}

bool SceneManager::hasColony(const std::string& id) const {
  return std::find(m_colonies.begin(), m_colonies.end(), id) !=
         m_colonies.end();
}

const std::vector<SceneManager::M31BrightStar>& SceneManager::galaxyBrightStars() const {
  if (m_currentStation) {
    const std::string& id = m_currentStation->id;
    if (id == "m31") return m_m31Bright;
    if (id == "m51") return m_m51Bright;
    if (id == "m104") return m_m104Bright;
    if (id == "mw") return m_mwBright;
  }
  static const std::vector<M31BrightStar> kEmpty;
  return kEmpty;
}

bool SceneManager::sunWorldPos(glm::vec3& out) const {
  if (!m_currentStation || m_currentStation->id != "mw") return false;
  out = kMwSunPos;
  return true;
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

  // v3.6.0: сайты колоний и яркие звёзды кластера пересчитываются с нуля
  // каждый кадр, чтобы не просачиваться между станциями.
  m_colonySites.clear();
  m_clusterBright.clear();

  // === Speed FX === (v3.3.0): игровой разгон/варп — камера в варпе почти не
  // движется, поэтому доплер/абберацию масштабируем внешним boost'ом.
  // По умолчанию 0 => betaEff == m_relBeta (харнесс байт-в-байт).
  float betaEff = m_relBeta;
  Vec3 dirEff = m_relDir;
  if (m_speedBeta > betaEff) {
    betaEff = m_speedBeta;
    dirEff = m_speedDir;
  }

  // v3.6.0: импостеры привязаны к миру — NDC-центр объекта и ракурс камеры к
  // нему. На спавне (камера в (0,0,d) смотрит в origin) ndc = (0,0), par = 0,
  // => шейдерные ветки ровно как до привязки, эталонные PNG байт-в-байт.
  float impNdcX = 0.0f, impNdcY = 0.0f;
  float impParX = 0.0f, impParY = 0.0f, impParZ = 0.0f;
  {
    glm::mat4 projI =
        glm::perspective(glm::radians(m_settings.fovDegrees), aspect, 0.1f,
                         1000.0f);
    glm::vec3 eyeI(cp.x, cp.y, cp.z);
    glm::vec3 fwdI(cf.x, cf.y, cf.z);
    glm::vec3 upI(cu.x, cu.y, cu.z);
    glm::mat4 viewI = glm::lookAt(eyeI, eyeI + fwdI, upI);
    glm::mat4 mvpI = projI * viewI;
    glm::vec4 clipI = mvpI * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (clipI.w > 1e-6f) {
      impNdcX = static_cast<float>(clipI.x / clipI.w);
      impNdcY = static_cast<float>(clipI.y / clipI.w);
      double cl = std::sqrt(cp.x * cp.x + cp.y * cp.y + cp.z * cp.z);
      impParX = static_cast<float>(cp.x / cl);
      impParY = static_cast<float>(cp.y / cl);
      impParZ = static_cast<float>(cp.z / cl - 1.0);
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
    m_bhShader->setUniform1f("u_BandK", isSgra ? 1.3f : 1.0f);
    m_bhShader->setUniform1f("u_RedshiftK", m_settings.gravRedshift ? 1.0f : 0.0f);
    // v3.9.0: грав. красное смещение скайбокса (0 = legacy, точно x1.0).
    m_bhShader->setUniform1f("u_RedshiftK", m_settings.gravRedshift ? 1.0f : 0.0f);
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
    m_bhShader->setUniform3f("u_VelDir", static_cast<float>(dirEff.x),
                             static_cast<float>(dirEff.y),
                             static_cast<float>(dirEff.z));
    m_bhShader->setUniform1f("u_Beta", betaEff);

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
      m_starShader->setUniform3f("u_Tint", 1.0f, 1.0f, 1.0f);
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
      m_quasarShader->setUniform2f("u_ObjNdc", impNdcX, impNdcY);
      m_quasarShader->setUniform3f("u_CamPar", impParX, impParY, impParZ);
    }

    m_screenQuad->draw();
    m_quasarShader->unbind();
  } else if (m_currentStation->type == StationType::Constellation) {
    // Orion or Ursa Major Constellation lines
    // v3.0.0: clear color is linear (sRGB 0.01/0.015/0.03 decoded with
    // pow(c, 2.2)); the composite applies the single gamma on top.
    glClearColor(0.00004f, 0.00010f, 0.00045f, 1.0f);
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
    m_starShader->setUniform3f("u_Tint", 1.0f, 1.0f, 1.0f);
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
    m_starShader->setUniform3f("u_Tint", 1.0f, 1.0f, 1.0f);
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
    } else if (galId == "mw") {
      // Milky Way from outside: tighter dusty arms, modest bulge.
      galSeed = 5.3f; galArms = 2.0f; galPitch = 2.5f;
      galBulge = 5.0f; galDust = 1.8f; galArmGain = 0.8f;
    }
    m_galaxyShader->setUniform1f("u_GalSeed", galSeed);
    m_galaxyShader->setUniform1f("u_GalArms", galArms);
    m_galaxyShader->setUniform1f("u_GalPitch", galPitch);
    m_galaxyShader->setUniform1f("u_GalBulge", galBulge);
    m_galaxyShader->setUniform1f("u_GalDust", galDust);
    m_galaxyShader->setUniform1f("u_GalArmGain", galArmGain);

    m_screenQuad->draw();
    m_galaxyShader->unbind();

    // Galaxy LOD: NEAR (18 -> 6) volumetric disk starfield, alpha = near.
    // Additive points, no depth write, drawn over the faded impostor.
    // v3.1.0: every galaxy (m31 pattern); buffers are per-station seeds.
    StarPoints* lodStars = nullptr;
    const std::string& lodId = m_currentStation->id;
    if (lodId == "m31") lodStars = &m_m31Stars;
    else if (lodId == "m51") lodStars = &m_m51Stars;
    else if (lodId == "m104") lodStars = &m_m104Stars;
    else if (lodId == "mw") lodStars = &m_mwStars;
    if (lodStars) {
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
        lodStars->draw();
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
    m_nebulaShader->setUniform2f("u_ObjNdc", impNdcX, impNdcY);
    m_nebulaShader->setUniform1f("u_ObjZoom", 1.0f);
    m_nebulaShader->setUniform3f("u_CamPar", impParX, impParY, impParZ);
    m_screenQuad->draw();
    m_nebulaShader->unbind();
  } else if (m_currentStation->type == StationType::Pulsar) {
    // Pulsar impostor sprite + per-pulsar preset. Defaults = Vela 1:1, so
    // psr-vela renders exactly as before (harness byte-identical).
    m_pulsarShader->bind();
    m_pulsarShader->setUniform1f("u_Time", time);
    m_pulsarShader->setUniform1f("u_Aspect", aspect);
    m_pulsarShader->setUniform1f("u_Exposure", m_exposure);
    float psrSpin = 2.5f, psrTilt = 0.38f, psrHalo = 1.0f, psrArcs = 1.0f;
    float psrTintR = 1.0f, psrTintG = 1.0f, psrTintB = 1.0f;
    const std::string& psrId = m_currentStation->id;
    if (psrId == "psr-crab") {
      // Crab: faster, tighter beams, cooler tint (pairs with station m1).
      psrSpin = 6.0f; psrTilt = 0.30f; psrHalo = 1.2f; psrArcs = 0.9f;
      psrTintR = 0.85f; psrTintG = 0.95f; psrTintB = 1.10f;
    } else if (psrId == "sgr-1806") {
      // Magnetar SGR 1806-20: slow rotator, huge loops, amber flare halo.
      psrSpin = 1.1f; psrTilt = 0.55f; psrHalo = 2.2f; psrArcs = 1.7f;
      psrTintR = 1.15f; psrTintG = 0.75f; psrTintB = 0.45f;
    } else if (psrId == "psr-b1913") {
      // Hulse-Taylor binary: brisk spin, rose-violet tint, wide loops.
      psrSpin = 4.2f; psrTilt = 0.62f; psrHalo = 1.3f; psrArcs = 1.2f;
      psrTintR = 1.08f; psrTintG = 0.82f; psrTintB = 1.18f;
    }
    m_pulsarShader->setUniform1f("u_SpinRate", psrSpin);
    m_pulsarShader->setUniform1f("u_MagTilt", psrTilt);
    m_pulsarShader->setUniform3f("u_BeamTint", psrTintR, psrTintG, psrTintB);
    m_pulsarShader->setUniform1f("u_HaloGain", psrHalo);
    m_pulsarShader->setUniform1f("u_ArcScale", psrArcs);
    m_screenQuad->draw();
    m_pulsarShader->unbind();
  } else if (m_currentStation->type == StationType::StarCluster) {
    // v3.1.0 Deep Field: globular cluster ball (m13). Points only over a
    // linear-space dark clear (same triple as the constellation branch);
    // bloom gives the core its glow. Fly between the stars by hand.
    glClearColor(0.00004f, 0.00010f, 0.00045f, 1.0f);
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
    m_starShader->setUniform1f("u_MaxPx", 24.0f);
    m_starShader->setUniform1f("u_SizeK", 0.14f);
    m_starShader->setUniform3f("u_Tint", 1.0f, 1.0f, 1.0f);
    m_clusterStars.draw();
    // v3.6.0: яркие гиганты ядра (тёплый оттенок, поменьше).
    m_starShader->setUniform1f("u_MaxPx", 34.0f);
    m_starShader->setUniform1f("u_SizeK", 0.26f);
    m_starShader->setUniform3f("u_Tint", 1.0f, 0.88f, 0.70f);
    m_clusterBright.draw();
      // v3.6.0: Hercules Prime (M13) — основная звезда + планета на орбите.
      // Масштаб согласован с кластером: орбита 4.6, планета 0.22 — мелкая
      // экзопланета, а не «тело крупнее скопления».
      if (m_currentStation->id == "m13") {
        double sim = m_simTime;
        double a = sim * 0.06 + 0.4;
        Vec3 hbPos(4.6 * std::cos(a), 0.9 * std::sin(a * 0.7),
                   4.6 * std::sin(a));
      auto drawPlanet = [&](const Vec3& p, double radius, int type, int body,
                            float seed, const Vec3& ca, const Vec3& cb) {
        glm::vec3 bpos(static_cast<float>(p.x), static_cast<float>(p.y),
                       static_cast<float>(p.z));
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), bpos) *
            glm::scale(glm::mat4(1.0f), glm::vec3(static_cast<float>(radius)));
        m_planetShader->setUniformMat4f("u_Model", model);
        m_planetShader->setUniform1i("u_Type", type);
        m_planetShader->setUniform1i("u_Body", body);
        m_planetShader->setUniform1f("u_Seed", seed);
        m_planetShader->setUniform3f("u_ColA", static_cast<float>(ca.x),
                                     static_cast<float>(ca.y),
                                     static_cast<float>(ca.z));
        m_planetShader->setUniform3f("u_ColB", static_cast<float>(cb.x),
                                     static_cast<float>(cb.y),
                                     static_cast<float>(cb.z));
        m_planetShader->setUniform1i("u_Spot", 0);
        m_planetShader->setUniform1i("u_Clouds", 0);
        m_planetShader->setUniform1f("u_SpinAngle", 0.0f);
        m_planetShader->setUniform1f("u_Radius", static_cast<float>(radius));
        m_planetShader->setUniform1i("u_RingOn", 0);
        m_planetShader->setUniform1f("u_RingInner", 0.0f);
        m_planetShader->setUniform1f("u_RingOuter", 0.0f);
        m_planetShader->setUniform1f("u_RingTilt", 0.0f);
        m_planetShader->setUniform1i("u_MoonOccN", 0);
        m_sphere.draw();
      };
      m_planetShader->bind();
      m_planetShader->setUniformMat4f("u_MVP", mvp);
      m_planetShader->setUniform1f("u_Time", time);
      m_planetShader->setUniform1f("u_Exposure", m_exposure);
      m_planetShader->setUniform3f("u_CamPos", static_cast<float>(cp.x),
                                   static_cast<float>(cp.y),
                                   static_cast<float>(cp.z));
      // v3.6.0: Hercules Prime — звезда-ядро (рисуется билбордом в
      // m_clusterBright), здесь только планета Hercules b + кольцо колонии.
      drawPlanet(hbPos, 0.22, 2, 2, 3.0f, Vec3(0.85, 0.8, 1.0),
                 Vec3(0.7, 0.6, 0.9));
      // v3.6.0: сайт колонии на Hercules b — каталог + кольцо-маркер.
      m_colonySites.push_back(ColonySite{"hercules-b", "Hercules b", "m13", hbPos,
                                         1.5f});
      if (hasColony("hercules-b")) {
        m_ringShader->bind();
        m_ringShader->setUniformMat4f("u_MVP", mvp);
        glm::vec3 hbp(static_cast<float>(hbPos.x), static_cast<float>(hbPos.y),
                      static_cast<float>(hbPos.z));
        glm::mat4 ringModel =
            glm::translate(glm::mat4(1.0f), hbp) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / 2.7f));
        m_ringShader->setUniformMat4f("u_Model", ringModel);
        m_ringShader->setUniform1f("u_Seed", 2.0f);
        m_ringShader->setUniform1f("u_Time", time);
        m_ringShader->setUniform3f("u_Center", hbp.x, hbp.y, hbp.z);
        m_ringShader->setUniform3f("u_ColA", 1.0f, 0.8f, 0.5f);
        m_ringShader->setUniform1f("u_PlanetR", 0.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_saturnRing.draw();
        glDisable(GL_BLEND);
        m_planetShader->bind();
      }
      m_planetShader->unbind();
    }
    m_starShader->unbind();
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
    m_skyShader->setUniform3f("u_VelDir", static_cast<float>(dirEff.x),
                              static_cast<float>(dirEff.y),
                              static_cast<float>(dirEff.z));
    m_skyShader->setUniform1f("u_Beta", betaEff);
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
      // v3.2.0: per-point colour dimmed to match the 5200-point density.
      float beltTw = 0.85f + 0.15f * std::sin(time * 2.3f);
      m_linesShader->setUniform3f("u_LineColor", 0.30f * beltTw, 0.25f * beltTw, 0.20f * beltTw);
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
      // v3.2.0 ring-shadow set (only Saturn has u_RingOn == 1).
      m_planetShader->setUniform1f("u_Radius", def->radius);
      m_planetShader->setUniform1i("u_RingOn", def->hasRing ? 1 : 0);
      m_planetShader->setUniform1f("u_RingInner", def->ringInner);
      m_planetShader->setUniform1f("u_RingOuter", def->ringOuter);
      m_planetShader->setUniform1f("u_RingTilt", def->ringTilt);
      // v3.2.0: moon transit shadows — collect this body's moons from the
      // same snapshot the moon spheres are drawn from (Jupiter: 4
      // Galileans, Saturn: Titan). Sun-at-origin convention, see
      // planet.frag moonShadow().
      int occN = 0;
      for (const auto& mb : m_lastBodies) {
        if (occN >= 5) break;
        if (!mb.def || mb.def->parentId != def->id) continue;
        const std::string occName = "u_MoonOcc[" + std::to_string(occN) + "]";
        m_planetShader->setUniform4f(occName.c_str(),
                                     static_cast<float>(mb.pos.x - bpos.x),
                                     static_cast<float>(mb.pos.y - bpos.y),
                                     static_cast<float>(mb.pos.z - bpos.z),
                                     mb.def->radius);
        ++occN;
      }
      m_planetShader->setUniform1i("u_MoonOccN", occN);
      m_sphere.draw();
      if (def->hasRing) {
        m_ringShader->bind();
        m_ringShader->setUniformMat4f("u_MVP", mvp);
        // v3.2.0: per-body ring tilt (Saturn 0.45 default, Uranus ~vertical
        // at 1.5) and mesh scale so ringOuter lands on the def value:
        // Saturn 2.7/2.7 = 1.0 (bit-identical), Uranus 1.80/2.7 = 0.667.
        glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), bpos) *
                              glm::rotate(glm::mat4(1.0f), def->ringTilt, glm::vec3(1.0f, 0.0f, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(def->ringOuter / 2.7f));
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
        if (def->id == "uranus") {
          m_ringShader->setUniform3f("u_ColA", 0.55f, 0.62f, 0.68f);
        } else {
          m_ringShader->setUniform3f("u_ColA", 0.8f, 0.72f, 0.6f);
        }
        // v3.2.0: planet eclipse on the ring (sun at the world origin).
        m_ringShader->setUniform1f("u_PlanetR", def->radius);
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
      // v3.2.0 editor: surface type from the palette (rock/rocky/moon/gas/
      // star), secondary colour, world radius (ring shadow + close detail).
      m_planetShader->setUniform1i("u_Type", custom.type);
      m_planetShader->setUniform1i("u_Body", custom.detail);
      m_planetShader->setUniform1f("u_Seed", custom.seed);
      m_planetShader->setUniform3f("u_ColA", static_cast<float>(custom.color.x),
                                   static_cast<float>(custom.color.y),
                                   static_cast<float>(custom.color.z));
      m_planetShader->setUniform3f("u_ColB", static_cast<float>(custom.colorB.x),
                                   static_cast<float>(custom.colorB.y),
                                   static_cast<float>(custom.colorB.z));
      m_planetShader->setUniform1f("u_Radius", custom.radius);
      m_planetShader->setUniform1i("u_Spot", 0);
      m_planetShader->setUniform1i("u_Clouds", 0);
      m_planetShader->setUniform1f("u_SpinAngle", 0.0f);
      m_planetShader->setUniform1i("u_MoonOccN", 0);
      bool customRing = custom.ringOuter > 0.0f;
      m_planetShader->setUniform1i("u_RingOn", customRing ? 1 : 0);
      if (customRing) {
        float ringScale = custom.ringOuter / 2.7f;
        m_planetShader->setUniform1f("u_RingInner", 1.7f * ringScale);
        m_planetShader->setUniform1f("u_RingOuter", custom.ringOuter);
        m_planetShader->setUniform1f("u_RingTilt", 0.45f);
      }
      m_sphere.draw();
      if (customRing) {
        m_ringShader->bind();
        m_ringShader->setUniformMat4f("u_MVP", mvp);
        glm::mat4 ringModel = glm::translate(glm::mat4(1.0f), cpos) *
                              glm::rotate(glm::mat4(1.0f), 0.45f, glm::vec3(1.0f, 0.0f, 0.0f)) *
                              glm::scale(glm::mat4(1.0f), glm::vec3(custom.ringOuter / 2.7f));
        m_ringShader->setUniformMat4f("u_Model", ringModel);
        m_ringShader->setUniform1f("u_Seed", custom.seed);
        m_ringShader->setUniform1f("u_Time", time);
        m_ringShader->setUniform3f("u_Center", cpos.x, cpos.y, cpos.z);
        m_ringShader->setUniform3f("u_ColA",
                                   static_cast<float>(custom.color.x) * 0.9f + 0.08f,
                                   static_cast<float>(custom.color.y) * 0.9f + 0.08f,
                                   static_cast<float>(custom.color.z) * 0.9f + 0.08f);
        m_ringShader->setUniform1f("u_PlanetR", custom.radius);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        m_saturnRing.draw();
        glDisable(GL_BLEND);
        m_planetShader->bind();
      }
    }
    m_planetShader->unbind();

    // v3.6.0: сайт колонии на Марсе — кольцо-маркер (виден при сближении;
    // AppGL показывает подсказку [C] в hint-радиусе radius*8+2.5).
    if (hasColony("mars")) {
      for (const auto& body : m_lastBodies) {
        if (body.def && body.def->id == "mars") {
          glm::vec3 mpos(static_cast<float>(body.pos.x),
                         static_cast<float>(body.pos.y),
                         static_cast<float>(body.pos.z));
          m_colonySites.push_back(ColonySite{"mars", "Mars", "sol",
                                             Vec3(mpos.x, mpos.y, mpos.z),
                                             1.5f});
          m_ringShader->bind();
          m_ringShader->setUniformMat4f("u_MVP", mvp);
          glm::mat4 ringModel =
              glm::translate(glm::mat4(1.0f), mpos) *
              glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / 2.7f));
          m_ringShader->setUniformMat4f("u_Model", ringModel);
          m_ringShader->setUniform1f("u_Seed", 1.0f);
          m_ringShader->setUniform1f("u_Time", time);
          m_ringShader->setUniform3f("u_Center", mpos.x, mpos.y, mpos.z);
          m_ringShader->setUniform3f("u_ColA", 1.0f, 0.8f, 0.5f);
          m_ringShader->setUniform1f("u_PlanetR", 0.0f);
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          m_saturnRing.draw();
          glDisable(GL_BLEND);
          break;
        }
      }
    }

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
  // === WarpJump === (агент J: +u_Burst и сплошной корабль поверх оверлея).
  // Корабль виден ТОЛЬКО в Pullback/Travel/Arrival/Pushin (флаг скрыт по
  // дефолту — вне катсцены ни одного GL-вызова). Сплошные корпуса (v2.7):
  // SphereMesh-эллипсоиды + RingMesh-кольца через ship.vert/ship.frag
  // (ламберт + френель), переключение стилей Hull в warp panel. Трейл —
  // LineMesh-лента, движок — StarPoints с красным u_Tint. Инициализация
  // ленивая внутри visible-ветви. История: v2.6 — wireframe-силуэт (LineMesh).
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
    // Сплошной корабль: поверх оверлея, с depth-тестом между частями.
    if (warpdrive::shipVisible()) {
      try {
        static bool s_shipFailed = false;
        static Shader* s_shipShader = nullptr;
        static SphereMesh s_shipSphere;
        static RingMesh s_shipRing;
        static LineMesh s_trail;
        static StarPoints s_engine;
        static bool s_shipInit = false;
        if (!s_shipFailed && !s_shipShader) {
          try {
            s_shipShader = new Shader("shaders/ship.vert", "shaders/ship.frag");
          } catch (const std::exception& e) {
            std::cerr << "[ship] shader error: " << e.what() << "\n";
            s_shipFailed = true;
          }
        }
        if (!s_shipInit) {
          s_shipSphere.setSphere(22, 44);  // S4: гладкий силуэт корпуса
          s_shipRing.setRing(0.30f, 0.44f, 64);
          // Трейл-лента: 44 поперечных пары за кормой (z 0.55 -> 2.75,
          // полуширина 0.16 -> 0.02) + 2 продольных ребра. Плотные узкие
          // рунги читаются гладким следом вместо «лестницы» на малой тяге.
          std::vector<glm::vec3> trail;
          trail.reserve(280);
          constexpr int kSeg = 44;
          for (int i = 0; i < kSeg; ++i) {
            const float f0 = static_cast<float>(i) / kSeg;
            const float z = 0.55f + f0 * 2.2f;
            const float hw = 0.16f + (0.02f - 0.16f) * f0;
            trail.emplace_back(-hw, 0.0f, z);
            trail.emplace_back(hw, 0.0f, z);
          }
          for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < kSeg; ++i) {
              const float f0 = static_cast<float>(i) / kSeg;
              const float f1 = static_cast<float>(i + 1) / kSeg;
              const float hw0 = 0.16f + (0.02f - 0.16f) * f0;
              const float hw1 = 0.16f + (0.02f - 0.16f) * f1;
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
        // Анимация корпуса: на тяге и в варпе корабль «вытягивается» вдоль
        // курса (эффект двигателя), кольца Ring Ship при этом ещё и крутятся.
        const float thrW = warpdrive::thruster();
        const float warpVis = std::min(std::max(warpdrive::warpK(), 0.0f), 1.0f);
        const float warpStretch = 1.0f + 0.06f * thrW + 0.45f * warpVis;
        const glm::mat4 shipHullModel =
            shipModel * glm::scale(glm::mat4(1.0f),
                                   glm::vec3(1.0f, 1.0f, warpStretch));
        glm::mat4 proj = glm::perspective(glm::radians(m_settings.fovDegrees),
                                          aspect, 0.1f, 1000.0f);
        glm::vec3 eye(cp.x, cp.y, cp.z);
        glm::vec3 fwd(cf.x, cf.y, cf.z);
        glm::vec3 up(cu.x, cu.y, cu.z);
        glm::mat4 view = glm::lookAt(eye, eye + fwd, up);
        glm::mat4 mvp = proj * view;
        // Сплошные части: depth тест + запись, ламберт+френель через ship.frag.
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        // Часть корпуса: {pos, scale, baseColor, ring?, metal, rough, emissive?}
        // S5/S8: emissive — линейный HDR-довесок (сопла, навигационные огни).
        struct ShipPart {
          glm::vec3 pos;
          glm::vec3 scale;
          glm::vec3 color;
          int ring;
          float metal;
          float rough;
          glm::vec3 emissive{0.0f, 0.0f, 0.0f};
        };
        const glm::vec3 hullCol(0.84f, 0.88f, 0.94f);
        const glm::vec3 glassCol(0.35f, 0.55f, 0.80f);
        const glm::vec3 wingCol(0.62f, 0.70f, 0.82f);
        const glm::vec3 nozzleCol(0.16f, 0.17f, 0.20f);  // тёмный металл сопла
        const glm::vec3 navRed(1.80f, 0.16f, 0.12f);
        const glm::vec3 navGreen(0.16f, 1.60f, 0.55f);
        // v3.6.0: палитра выхлопа из настроек. Индекс 0 — исторический
        // красно-оранжевый (эталон ship.png не меняется), далее ион/изумруд/плазма.
        static const glm::vec3 kEngineEmi[4] = {
            {2.40f, 0.50f, 0.20f},
            {0.55f, 1.30f, 2.60f},
            {0.55f, 2.20f, 1.00f},
            {1.90f, 0.85f, 2.60f}};
        static const glm::vec3 kGlowTint[4] = {
            {1.00f, 0.22f, 0.10f},
            {0.35f, 0.60f, 1.00f},
            {0.25f, 1.00f, 0.50f},
            {0.90f, 0.55f, 1.00f}};
        static const glm::vec3 kTrailBase[4] = {
            {0.25f, 0.45f, 0.70f},
            {0.30f, 0.50f, 1.00f},
            {0.25f, 0.85f, 0.55f},
            {0.70f, 0.45f, 1.00f}};
        const int pal = std::min(std::max(m_settings.enginePalette, 0), 3);
        const glm::vec3 engineEmi = kEngineEmi[pal];
        std::vector<ShipPart> parts;
        const int style = warpdrive::shipStyle();
        // Стиль 3 (Model OBJ) при недоступном файле откатывается на процедурный
        // Ring Ship: разделы ниже строят тот же parts-набор, что и style 1.
        const int styleParts = (style == 3) ? 1 : style;
        if (styleParts == 1) {
          // Ring Ship v2 (v3.6.0): по образцу владельца (Tripo) — три кольца,
          // носовое копьё, многодюзовый кластер кормы. Всё из единичных сфер
          // и плоских колец: детерминизм, без новых мешей и текстур.
          parts.push_back({{0.0f, 0.0f, -0.02f}, {0.095f, 0.095f, 0.72f}, hullCol, 0, 0.90f, 0.32f});
          // Носовое копьё: база, стержень, острие (нос = -z).
          parts.push_back({{0.0f, 0.0f, -0.82f}, {0.075f, 0.075f, 0.20f}, hullCol, 0, 0.90f, 0.32f});
          parts.push_back({{0.0f, 0.0f, -1.10f}, {0.030f, 0.030f, 0.30f}, hullCol, 0, 0.90f, 0.32f});
          parts.push_back({{0.0f, 0.0f, -1.38f}, {0.013f, 0.013f, 0.10f}, wingCol, 0, 0.72f, 0.45f});
          // Три кольца, каждое на кресте из 4 пилонов (±x/±y, без поворотов).
          for (float ringZ : {-0.44f, 0.0f, 0.44f}) {
            parts.push_back({{0.0f, 0.0f, ringZ}, {1.0f, 1.0f, 1.0f}, wingCol, 1, 0.78f, 0.28f});
            parts.push_back({{0.20f, 0.0f, ringZ}, {0.13f, 0.014f, 0.035f}, wingCol, 0, 0.72f, 0.45f});
            parts.push_back({{-0.20f, 0.0f, ringZ}, {0.13f, 0.014f, 0.035f}, wingCol, 0, 0.72f, 0.45f});
            parts.push_back({{0.0f, 0.20f, ringZ}, {0.014f, 0.13f, 0.035f}, wingCol, 0, 0.72f, 0.45f});
            parts.push_back({{0.0f, -0.20f, ringZ}, {0.014f, 0.13f, 0.035f}, wingCol, 0, 0.72f, 0.45f});
          }
          // Корма: обтекатель + центральное сопло с горлом + венец из 6 сопел.
          parts.push_back({{0.0f, 0.0f, 0.62f}, {0.135f, 0.135f, 0.16f}, hullCol, 0, 0.90f, 0.32f});
          parts.push_back({{0.0f, 0.0f, 0.76f}, {0.060f, 0.060f, 0.10f}, nozzleCol, 0, 0.95f, 0.30f});
          parts.push_back({{0.0f, 0.0f, 0.85f}, {0.040f, 0.040f, 0.022f}, engineEmi, 0, 0.0f, 0.60f, engineEmi});
          for (int k = 0; k < 6; ++k) {
            const float a = static_cast<float>(k) * 1.0471976f;  // 60 градусов
            const float cx = 0.088f * std::cos(a);
            const float cy = 0.088f * std::sin(a);
            parts.push_back({{cx, cy, 0.74f}, {0.030f, 0.030f, 0.055f}, nozzleCol, 0, 0.95f, 0.30f});
            parts.push_back({{cx, cy, 0.80f}, {0.019f, 0.019f, 0.014f}, engineEmi, 0, 0.0f, 0.60f, engineEmi});
          }
          // Навогни на кормовом кольце (z+ = корма).
          parts.push_back({{-0.44f, 0.0f, 0.44f}, {0.020f, 0.020f, 0.020f}, navRed, 0, 0.0f, 0.60f, navRed});
          parts.push_back({{0.44f, 0.0f, 0.44f}, {0.020f, 0.020f, 0.020f}, navGreen, 0, 0.0f, 0.60f, navGreen});
        } else if (styleParts == 2) {
          // Shuttle: широкий корпус + кабина + 2 мотогондолы.
          parts.push_back({{0.0f, 0.0f, 0.0f}, {0.48f, 0.10f, 0.55f}, hullCol, 0, 0.90f, 0.32f});
          parts.push_back({{0.0f, 0.07f, -0.32f}, {0.11f, 0.08f, 0.15f}, glassCol, 0, 0.70f, 0.08f});
          parts.push_back({{-0.52f, 0.0f, 0.10f}, {0.07f, 0.07f, 0.28f}, wingCol, 0, 0.72f, 0.45f});
          parts.push_back({{0.52f, 0.0f, 0.10f}, {0.07f, 0.07f, 0.28f}, wingCol, 0, 0.72f, 0.45f});
          // S5: сопла мотогондол + S8 навогни на законцовках.
          parts.push_back({{-0.52f, 0.0f, 0.40f}, {0.058f, 0.058f, 0.055f}, nozzleCol, 0, 0.95f, 0.30f});
          parts.push_back({{0.52f, 0.0f, 0.40f}, {0.058f, 0.058f, 0.055f}, nozzleCol, 0, 0.95f, 0.30f});
          parts.push_back({{-0.52f, 0.0f, 0.46f}, {0.036f, 0.036f, 0.018f}, engineEmi, 0, 0.0f, 0.60f, engineEmi});
          parts.push_back({{0.52f, 0.0f, 0.46f}, {0.036f, 0.036f, 0.018f}, engineEmi, 0, 0.0f, 0.60f, engineEmi});
          parts.push_back({{-0.585f, 0.0f, -0.10f}, {0.018f, 0.018f, 0.018f}, navRed, 0, 0.0f, 0.60f, navRed});
          parts.push_back({{0.585f, 0.0f, -0.10f}, {0.018f, 0.018f, 0.018f}, navGreen, 0, 0.0f, 0.60f, navGreen});
          // Shuttle v2 (v3.6.0): боковые окна кабины (слабое свечение),
          // хвостовой киль, входные патрубки гондол, шов грузового люка.
          parts.push_back({{0.0f, 0.14f, 0.40f}, {0.018f, 0.12f, 0.16f}, wingCol, 0, 0.72f, 0.45f});
          for (int s = -1; s <= 1; s += 2) {
            for (float wz : {-0.34f, -0.22f}) {
              parts.push_back({{0.35f * static_cast<float>(s), 0.05f, wz},
                               {0.012f, 0.020f, 0.045f}, glassCol, 0, 0.70f, 0.08f,
                               glm::vec3(0.03f, 0.05f, 0.08f)});
            }
            parts.push_back({{0.52f * static_cast<float>(s), 0.0f, -0.16f},
                             {0.045f, 0.045f, 0.030f}, nozzleCol, 0, 0.95f, 0.30f});
          }
          parts.push_back({{0.0f, 0.055f, 0.50f}, {0.30f, 0.010f, 0.018f}, nozzleCol, 0, 0.95f, 0.30f});
        } else {
          // Dart: заострённый фюзеляж + крыло + киль + кабина.
          parts.push_back({{0.0f, 0.0f, -0.10f}, {0.13f, 0.11f, 0.82f}, hullCol, 0, 0.90f, 0.32f});
          parts.push_back({{0.0f, 0.02f, -0.48f}, {0.09f, 0.08f, 0.16f}, glassCol, 0, 0.70f, 0.08f, glm::vec3(0.045f, 0.06f, 0.09f)});
          parts.push_back({{0.0f, -0.02f, 0.22f}, {0.78f, 0.035f, 0.34f}, wingCol, 0, 0.72f, 0.45f});
          parts.push_back({{0.0f, 0.15f, 0.28f}, {0.035f, 0.22f, 0.30f}, wingCol, 0, 0.72f, 0.45f});
          // S5: сопло фюзеляжа + раскалённое горло; S8 навогни на законцовках.
          parts.push_back({{0.0f, 0.0f, 0.74f}, {0.085f, 0.085f, 0.10f}, nozzleCol, 0, 0.95f, 0.30f});
          parts.push_back({{0.0f, 0.0f, 0.83f}, {0.050f, 0.050f, 0.022f}, engineEmi, 0, 0.0f, 0.60f, engineEmi});
          parts.push_back({{-0.79f, -0.02f, 0.22f}, {0.018f, 0.018f, 0.018f}, navRed, 0, 0.0f, 0.60f, navRed});
          parts.push_back({{0.79f, -0.02f, 0.22f}, {0.018f, 0.018f, 0.018f}, navGreen, 0, 0.0f, 0.60f, navGreen});
          // Dart v2 (v3.6.0): дорсальный гребень, подфюзеляжный
          // воздухозаборник, хвостовые стабилизаторы на крыле.
          parts.push_back({{0.0f, 0.095f, -0.04f}, {0.028f, 0.030f, 0.34f}, hullCol, 0, 0.90f, 0.32f});
          parts.push_back({{0.0f, -0.10f, -0.34f}, {0.055f, 0.038f, 0.14f}, nozzleCol, 0, 0.95f, 0.30f});
          parts.push_back({{-0.15f, 0.03f, 0.50f}, {0.012f, 0.12f, 0.16f}, wingCol, 0, 0.72f, 0.45f});
          parts.push_back({{0.15f, 0.03f, 0.50f}, {0.012f, 0.12f, 0.16f}, wingCol, 0, 0.72f, 0.45f});
        }
        // === Ship lighting pass (S3): ключ — доминирующий источник сцены.
        // Якорь: у mw — Солнце (sunWorldPos), иначе центр станции (origin);
        // цвет/сила зависят от типа станции. Rim — холодный контр от
        // обратного направления (в ship.frag), fill — IBL скайбокса.
        glm::vec3 keyDir(0.42f, 0.70f, 0.56f);
        glm::vec3 keyCol(1.00f, 0.95f, 0.88f);
        if (const Station* st = getCurrentStation()) {
          glm::vec3 anchor(0.0f);
          glm::vec3 sunPos;
          if (sunWorldPos(sunPos)) anchor = sunPos;
          const glm::vec3 d = anchor - shipPos;
          const float len = glm::length(d);
          if (len > 1e-4f) keyDir = d / len;
          switch (st->type) {
            case StationType::BlackHole:  // раскалённый аккреционный диск
              keyCol = glm::vec3(1.00f, 0.70f, 0.42f) * 1.25f; break;
            case StationType::Quasar:  // сине-белое пекло
              keyCol = glm::vec3(0.85f, 0.92f, 1.18f) * 1.45f; break;
            case StationType::Pulsar:  // циановый маяк
              keyCol = glm::vec3(0.78f, 0.90f, 1.12f) * 1.05f; break;
            case StationType::Galaxy:
            case StationType::StarCluster:  // суммарный звёздный свет
              keyCol = glm::vec3(1.00f, 0.94f, 0.84f) * 0.70f; break;
            case StationType::Nebula:
              keyCol = glm::vec3(0.92f, 0.86f, 0.98f) * 0.60f; break;
            case StationType::SolarSystem:  // солнце G2V
              keyCol = glm::vec3(1.00f, 0.94f, 0.82f) * 1.45f; break;
            case StationType::Constellation:
              keyCol = glm::vec3(0.95f, 0.97f, 1.00f) * 0.50f; break;
          }
          // Лёгкий верхний уклон: плоский свет читается хуже.
          keyDir = glm::normalize(keyDir + glm::vec3(0.0f, 0.30f, 0.0f));
        }
        if (s_shipShader) {
          s_shipShader->bind();
          s_shipShader->setUniformMat4f("u_MVP", mvp);
          // Скин Ring Ship живёт в пространстве корабля: передаём обратную
          // матрицу один раз на кадр (для остальных стилей она не нужна).
          s_shipShader->setUniformMat4f("u_ShipInv", glm::inverse(shipModel));
          s_shipShader->setUniform3f("u_CamPos", eye);
          s_shipShader->setUniform3f("u_KeyDir", keyDir.x, keyDir.y, keyDir.z);
          s_shipShader->setUniform3f("u_KeyColor", keyCol.x, keyCol.y, keyCol.z);
          // IBL: мипованный локальный скайбокс (S1), 512 => 9 мипов.
          s_shipShader->setUniform1i("u_Env", 0);
          s_shipShader->setUniform1f("u_EnvMaxLod", 9.0f);
          // v3.6.0: тумблер зеркальных отражений из настроек (по умолчанию 1.0
          // — эталон ship.png не меняется, умножение на 1.0f точное).
          s_shipShader->setUniform1f("u_ReflK", m_settings.shipReflections ? 1.0f : 0.0f);
          glActiveTexture(GL_TEXTURE0);
          m_skyboxLocal->bind(0);
          // Фаза спина интегрируется раз за кадр: кольца процедурного
          // корпуса и кольца OBJ-модели (u_RingSpin) делят одну фазу.
          static float s_spinVar = 0.0f;
          static float s_spinPrev = time;
          const float dtS = time - s_spinPrev;
          s_spinPrev = time;
          if (dtS > 0.0f && dtS < 0.5f)
            s_spinVar += (2.6f * thrW + 8.0f * warpVis) * dtS;
          const float spinPhase = 0.55f * time + s_spinVar;
          bool meshDrawn = false;
          if (style == 3) {
            // Model hull (агент M): меш из res/models/warp_ship.obj рядом с
            // exe. Лениво, раз за процесс; ~15 МБ текстовый парс — единичный
            // хитч кадра. При отсутствии файла meshDrawn останется false:
            // процедурный Ring Ship уже собран в parts (styleParts).
            static ObjModel s_shipModel;
            static bool s_modelTried = false;
            if (!s_modelTried) {
              s_modelTried = true;
              s_shipModel.load("res/models/warp_ship.obj");
              // PBR-набор рядом с мешом (albedo/normal/rough/metal BMP).
              // Нет карт — остаёмся на плоском металле, ошибки не фатальны.
              s_shipModel.loadTextures("res/models/warp_ship");
            }
            if (s_shipModel.ready()) {
              s_shipShader->setUniformMat4f("u_Model", shipHullModel);
              s_shipShader->setUniform3f("u_InvScale", 1.0f, 1.0f, 1.0f);
              s_shipShader->setUniform1i("u_FlatNormal", 0);
              // Кручение колец Model-корабля (окраска едет с ободом —
              // скин сэмплится в некрутёной позиции). Окна читаются и
              // в простое: база 0.55*time + переменная фаза.
              s_shipShader->setUniform1f("u_RingSpin", spinPhase);
              s_shipShader->setUniform1i("u_SkinOn", 0);
              // Окраску несёт livery-альбедо (запечён в экспортёре), поэтому
              // множитель нейтральный; метал/шершавость — фолбэки, когда карт нет.
              s_shipShader->setUniform3f("u_BaseColor", glm::vec3(1.0f));
              s_shipShader->setUniform1f("u_Metal", 0.78f);
              s_shipShader->setUniform1f("u_Rough", 0.42f);
              // Текстурированный путь: карты в юнитах 1..4 (0 — скайбокс).
              const bool texOn = s_shipModel.textured();
              s_shipShader->setUniform1i("u_TexOn", texOn ? 1 : 0);
              if (texOn) {
                s_shipShader->setUniform1i("u_Albedo", 1);
                s_shipShader->setUniform1i("u_NormalTex", 2);
                s_shipShader->setUniform1i("u_RoughTex", 3);
                s_shipShader->setUniform1i("u_MetalTex", 4);
                s_shipModel.bindTextures(1);
              }
              // Emissive-карта livery (кольцевой неон, окна, выхлоп): юнит 5.
              // Пинним БЕЗ emOn: без карты юнит 0 с samplerCube IBL — конфликт
              // типов сэмплеров, драйвер может отбросить весь draw.
              const bool emOn = s_shipModel.emissive();
              s_shipShader->setUniform1i("u_EmissiveOn", emOn ? 1 : 0);
              s_shipShader->setUniform1i("u_EmissiveTex", 5);
              // Star Trek-подсветка в варпе: линейный HDR-довесок, умеренный.
              const float wk =
                  std::min(std::max(warpdrive::warpK(), 0.0f), 1.0f);
              s_shipShader->setUniform3f(
                  "u_Emissive", glm::vec3(0.010f, 0.028f, 0.052f) * wk);
              s_shipModel.draw();
              if (texOn) glActiveTexture(GL_TEXTURE0);
              meshDrawn = true;
            }
          }
          if (!meshDrawn) {
          // Процедурные части: плоский u_BaseColor, текстуры выключены
          // (у них нет атрибута UV — location 2 не включён).
          s_shipShader->setUniform1i("u_TexOn", 0);
          // Процедурным частям спин OBJ не нужен: кольца крутятся на CPU.
          s_shipShader->setUniform1f("u_RingSpin", 0.0f);
          s_shipShader->setUniform1i("u_EmissiveOn", 0);  // не утекать из Model-пути
          // Сэмплеры карт всё равно обязаны смотреть в СВОИ юниты: если два
          // сэмплера разного типа делят юнит 0 (кубмап скайбокса), драйвер
          // вправе отбросить draw — корпус рисовался чёрным.
          s_shipShader->setUniform1i("u_Albedo", 1);
          s_shipShader->setUniform1i("u_NormalTex", 2);
          s_shipShader->setUniform1i("u_RoughTex", 3);
          s_shipShader->setUniform1i("u_MetalTex", 4);
          // И emissive-сэмплер прочь от юнита 0: его дефолт делит кубмапу
          // IBL (юнит 0) с samplerCube → драйвер отбрасывает silhouette.
          s_shipShader->setUniform1i("u_EmissiveTex", 5);
          // Тайловый скин Ring Ship (панели, заклёпки, потёртости):
          // tools/make_ring_skin.mjs -> res/models/ring_skin_*.bmp.
          static unsigned int s_skinAlbedo = 0;
          static unsigned int s_skinRough = 0;
          static bool s_skinTried = false;
          if (!s_skinTried) {
            s_skinTried = true;
            s_skinAlbedo = loadBmpTexture("res/models/ring_skin_albedo.bmp");
            s_skinRough = loadBmpTexture("res/models/ring_skin_rough.bmp");
          }
          // Панельный скин — только у процедурного Ring Ship (стиль 1);
          // Dart и Shuttle сохраняют эталонный вид (ship.png не меняется).
          const bool skinOn = (style == 1) && s_skinAlbedo != 0;
          s_shipShader->setUniform1i("u_SkinOn", skinOn ? 1 : 0);
          s_shipShader->setUniform1i("u_SkinAlbedo", 5);
          s_shipShader->setUniform1i("u_SkinRough", 6);
          if (skinOn) {
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, s_skinAlbedo);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, s_skinRough);
            glActiveTexture(GL_TEXTURE0);
          }
          int ringIdx = 0;
          for (const ShipPart& p : parts) {
            glm::mat4 model = shipHullModel *
                              glm::translate(glm::mat4(1.0f), p.pos);
            if (p.ring == 1) {
              // RingMesh лежит в XZ (нормаль +Y) — поворот в плоскость XY.
              model = model * glm::rotate(glm::mat4(1.0f),
                                          glm::radians(90.0f),
                                          glm::vec3(1.0f, 0.0f, 0.0f));
              // Спин строго В ПЛОСКОСТИ КОЛЬЦА: ось mesh-Y (нормаль кольца),
              // а не mesh-Z — прежняя ось после rotX(90°) ложилась на
              // вертикаль корабля, и кольца кувыркались сквозь корпус
              // как гироскоп вместо «катания по окружности».
              const float spinDir = (ringIdx % 2 == 0) ? 1.0f : -1.0f;
              model = model * glm::rotate(glm::mat4(1.0f),
                                          spinDir * (0.55f * time + s_spinVar),
                                          glm::vec3(0.0f, 1.0f, 0.0f));
              // Бегущие по ободу огни: радиальная симметрия RingMesh не
              // показывает вращение — скорость читают маркеры с той же
              // фазой (пер-кольцо offset, направление как у спина).
              const float ang = spinDir * (0.55f * time + s_spinVar) +
                                static_cast<float>(ringIdx) * 2.0944f;
              const glm::mat4 ringCenter = model;
              for (int m = 0; m < 3; ++m) {
                const float a = ang + static_cast<float>(m) * 2.0944f;
                // Плоскость кольца после rotX(90°) — локальные XY: маркер
                // катится по ободу вокруг продольной оси. Масштаб обязателен:
                // единичная сфера иначе перекрывает весь корабль.
                glm::mat4 dotM = ringCenter *
                    glm::translate(glm::mat4(1.0f),
                                   glm::vec3(0.37f * std::cos(a),
                                             0.37f * std::sin(a), 0.0f)) *
                    glm::scale(glm::mat4(1.0f),
                               glm::vec3(0.018f, 0.018f, 0.018f));
                s_shipShader->setUniformMat4f("u_Model", dotM);
                s_shipShader->setUniform3f("u_InvScale", 1.0f, 1.0f, 1.0f);
                s_shipShader->setUniform1f("u_FlatNormal", 0.0f);
                s_shipShader->setUniform3f("u_BaseColor", 0.85f, 0.90f, 1.0f);
                s_shipShader->setUniform1f("u_Metal", 0.0f);
                s_shipShader->setUniform1f("u_Rough", 0.60f);
                // Маркеры варпа: цвет палитры выхлопа × (0.55 idle / 1.0+ warp)
                const float glow = 0.55f + 1.45f * warpVis;
                s_shipShader->setUniform3f(
                    "u_Emissive",
                    engineEmi.x * glow, engineEmi.y * glow, engineEmi.z * glow);
                s_shipSphere.draw();
              }
              ++ringIdx;
              // Далее общий хвост цикла: масштаб, юниформы и draw кольца.
            }
            model = model * glm::scale(glm::mat4(1.0f), p.scale);
            s_shipShader->setUniformMat4f("u_Model", model);
            s_shipShader->setUniform3f("u_InvScale", 1.0f / p.scale.x,
                                       1.0f / p.scale.y, 1.0f / p.scale.z);
            s_shipShader->setUniform1i("u_FlatNormal", p.ring);
            s_shipShader->setUniform3f("u_BaseColor", p.color);
            s_shipShader->setUniform1f("u_Metal", p.metal);
            s_shipShader->setUniform1f("u_Rough", p.rough);
            s_shipShader->setUniform3f("u_Emissive", p.emissive.x, p.emissive.y,
                                       p.emissive.z);
            if (p.ring == 1) {
              s_shipRing.draw();
            } else {
              s_shipSphere.draw();
            }
          }
          }  // !meshDrawn (fallback procedural parts)
          s_shipShader->unbind();
        }
        // Трейл + движок: без depth, аддитивно поверх корпуса.
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        // v3.3.0: тяга растягивает ленту за кормой и разжигает её цвет.
        const float thr = warpdrive::thruster();
        const glm::mat4 trailModel =
            shipModel *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f + 2.2f * thr));
        m_linesShader->bind();
        m_linesShader->setUniformMat4f("u_MVP", mvp);
        m_linesShader->setUniformMat4f("u_Model", trailModel);
        m_linesShader->setUniform1f("u_Time", time);
        // Цвет следа — база палитры * тяга (пал 0 — исторические коэффициенты).
        const glm::vec3 trailThrK(1.70f, 1.50f, 1.20f);
        const glm::vec3 trailCol = kTrailBase[pal] * (0.55f + trailThrK * thr);
        m_linesShader->setUniform3f("u_LineColor", trailCol.x, trailCol.y,
                                    trailCol.z);
        s_trail.drawLines();
        m_linesShader->unbind();
        // Движок: 1 крупная пульсирующая КРАСНАЯ точка за кормой.
        {
          const float sy = warpdrive::shipYaw();
          // Model hull (стиль 3): хвост длиннее (полудлина 1.15), выносим
          // точку дальше за корму, чем у процедурных корпусов (0.55).
          const float engineBack = (style == 3) ? 1.45f : 0.55f;
          const float ex = warpdrive::shipX() + engineBack * std::sin(sy);
          const float ez = warpdrive::shipZ() + engineBack * std::cos(sy);
          // v3.3.0: яркость и размер движка растут с тягой.
          const float thr2 = warpdrive::thruster();
          const float mag = (0.1f + 0.35f * std::sin(time * 18.0f)) *
                            (0.55f + 1.45f * thr2);
          s_engine.setStars({glm::vec3(ex, warpdrive::shipY(), ez)}, {mag});
          const float pixelScale = static_cast<float>(fbHeight) /
              (2.0f * std::tan(glm::radians(m_settings.fovDegrees) * 0.5f));
          m_starShader->bind();
          m_starShader->setUniformMat4f("u_MVP", mvp);
          m_starShader->setUniformMat4f("u_MV", view);
          m_starShader->setUniform1f("u_PixelScale", pixelScale);
          m_starShader->setUniform1f("u_SizeK", 0.22f * (0.75f + 0.75f * thr2));
          m_starShader->setUniform1f("u_Time", time);
          m_starShader->setUniform3f("u_Tint", kGlowTint[pal].x, kGlowTint[pal].y,
                                     kGlowTint[pal].z);
          s_engine.draw();
          m_starShader->unbind();
        }
        // === v3.6.0 (пасс D): полоса обломков в Travel === конвейер
        // из 24 «камней»: короткие LineMesh-стриики вдоль курса от носа
        // корабля (-4.6 по локальному Z) до точки у ближней кромки (-0.2),
        // где камень «пролетает мимо» и пересортируется на новый пролёт.
        // На старте стриик тонок (около 10+ ед. от камеры), читается как
        // точка глубины; скипается целиком при astK = 0 (харнесс чистый).
        {
          const float astK = warpdrive::astK();
          if (astK > 0.0005f) {
            static LineMesh s_astField;
            std::vector<glm::vec3> ast;
            ast.reserve(48);
            std::vector<unsigned int> astIdx;
            astIdx.reserve(48);
            constexpr int kAstN = 24;
            const float ay = warpdrive::shipYaw();
            // Нос = -localZ: мирный forward (-sin, 0, -cos); правый борт =
            // разворот localX вокруг Y: (cos, 0, -sin).
            const float fwX = -std::sin(ay);
            const float fwZ = -std::cos(ay);
            const float rtX = std::cos(ay);
            const float rtZ = -std::sin(ay);
            const float s0 = warpdrive::shipX();
            const float s1 = warpdrive::shipY();
            const float s2 = warpdrive::shipZ();
            auto hOF = [](float n) -> float {
              return std::fmod(std::sin(n) * 43758.5453f, 1.0f) < 0.0f
                         ? std::fmod(std::sin(n) * 43758.5453f, 1.0f) + 1.0f
                         : std::fmod(std::sin(n) * 43758.5453f, 1.0f);
            };
            const float lane = warpdrive::astLane();  // -1..1 сглаженный
            for (int i = 0; i < kAstN; ++i) {
              const float fi = static_cast<float>(i);
              const float off = hOF(fi * 13.79f + 17.0f);
              const float rate = 0.8f + 0.6f * hOF(fi * 21.31f + 5.0f);
              const float len = 2.0f + 1.5f * hOF(fi * 7.23f + 29.0f);
              const float lat0 = (hOF(fi * 9.77f + 41.0f) - 0.5f) * 4.0f;
              const float yOff = (hOF(fi * 11.47f + 61.0f) - 0.5f) * 1.8f;
              // Уклонение D (lane > 0) сдвигает камни влево: проплевший
              // между ними манёвр читается как прыжок в сторону.
              const float lat = lat0 - lane * 1.35f;
              const float ph = std::fmod(time * 0.36f * rate + off, 1.0f);
              if (ph < 0.04f || ph > 0.96f) continue;  // «миг» на краях скип
              const float zHead = -4.6f + 4.4f * ph;
              const float zTail = zHead + len;  // хвост дальше от носа
              const glm::vec3 head(s0 + fwX * zHead + rtX * lat,
                                   s1 + yOff, s2 + fwZ * zHead + rtZ * lat);
              const glm::vec3 tail(s0 + fwX * zTail + rtX * lat,
                                   s1 + yOff, s2 + fwZ * zTail + rtZ * lat);
              astIdx.push_back(static_cast<unsigned int>(ast.size()));
              ast.push_back(head);
              astIdx.push_back(static_cast<unsigned int>(ast.size()));
              ast.push_back(tail);
            }
            m_linesShader->bind();
            m_linesShader->setUniformMat4f("u_MVP", mvp);
            m_linesShader->setUniformMat4f("u_Model", glm::mat4(1.0f));
            m_linesShader->setUniform1f("u_Time", time);
            const glm::vec3 astCol(0.62f * astK, 0.50f * astK, 0.38f * astK);
            m_linesShader->setUniform3f("u_LineColor", astCol.x, astCol.y,
                                        astCol.z);
            s_astField.setGeometry(ast, astIdx);
            s_astField.drawLines();
            m_linesShader->unbind();
          }
        }
        glDisable(GL_BLEND);
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
    const float velDir[3] = {static_cast<float>(dirEff.x),
                             static_cast<float>(dirEff.y),
                             static_cast<float>(dirEff.z)};
    m_postFx.setFlight(camFwd, camRight, camUp,
                       glm::radians(m_settings.fovDegrees), aspect, velDir,
                       betaEff);
    // === Warp field === (v3.6.0): экранная позиция корабля + экранное
    // направление курса для линз-эффекта в композите. Скрытый корабль или
    // нулевой варп => u_WarpK = 0 (точный проход, эталоны без изменений).
    // v3.6.0: линза пространства — ЯВЛЕНИЕ ТОЛЬКО ВАРП-ДВИГАТЕЛЯ; обычное
    // ускорение ограничено стриками (u_SpeedK) — намеренно разные эффекты.
    const float lensK = m_warpFx;
    if (warpdrive::shipVisible() && lensK > 0.001f) {
      const glm::vec3 eyeF(static_cast<float>(cp.x), static_cast<float>(cp.y),
                           static_cast<float>(cp.z));
      const glm::vec3 fwdF(static_cast<float>(cf.x), static_cast<float>(cf.y),
                           static_cast<float>(cf.z));
      const glm::vec3 upF(static_cast<float>(cu.x), static_cast<float>(cu.y),
                          static_cast<float>(cu.z));
      const glm::vec3 shipPos(static_cast<float>(warpdrive::shipX()),
                              static_cast<float>(warpdrive::shipY()),
                              static_cast<float>(warpdrive::shipZ()));
      const glm::mat4 lensProj = glm::perspective(
          glm::radians(m_settings.fovDegrees), aspect, 0.1f, 1000.0f);
      const glm::vec4 clip = lensProj * glm::lookAt(eyeF, eyeF + fwdF, upF) *
                             glm::vec4(shipPos, 1.0f);
      const float sdx = static_cast<float>(cr.x * dirEff.x + cr.y * dirEff.y +
                                           cr.z * dirEff.z);
      const float sdy = static_cast<float>(cu.x * dirEff.x + cu.y * dirEff.y +
                                           cu.z * dirEff.z);
      if (clip.w > 0.001f) {
        m_postFx.setWarpLens(clip.x / clip.w * 0.5f + 0.5f,
                             clip.y / clip.w * 0.5f + 0.5f, sdx, sdy, lensK);
      } else {
        m_postFx.setWarpLens(0.5f, 0.5f, sdx, sdy, 0.0f);
      }
    } else {
      m_postFx.setWarpLens(0.5f, 0.5f, 0.0f, 1.0f, 0.0f);
    }
  }
  // === Ship survival === (v3.3.0): 0/0/0 keeps the composite byte-exact.
  m_postFx.setShipFx(m_shipChroma, m_shipStatic, m_shipFlash);
  // === Speed FX === (v3.3.0): стрики скорости в композите (0 = выкл).
  m_postFx.setSpeedFx(m_speedFx);
  m_postFx.setTime(time);
  m_postFx.endScene(fbWidth, fbHeight);
}

}  // namespace astro
