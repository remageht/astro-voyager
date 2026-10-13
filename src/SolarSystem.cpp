#include "SolarSystem.h"

#include <cmath>

namespace astro {

namespace {

// Pi constant (ASCII only).
constexpr float kPi = 3.14159265358979323846f;

// Vec3 arithmetic for parent-relative composition (local to this file;
// Camera.h keeps the minimal core vector without operators).
inline Vec3 operator+(const Vec3& a, const Vec3& b) {
  return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

PlanetDef makeDef(const char* id, const char* name, float radius, float orbitRadius,
                  float angleSpeed, float angle0, int type, float seed,
                  Vec3 colA, Vec3 colB) {
  PlanetDef d;
  d.id = id;
  d.name = name;
  d.radius = radius;
  d.orbitRadius = orbitRadius;
  d.angleSpeed = angleSpeed;
  d.angle0 = angle0;
  d.type = type;
  d.seed = seed;
  d.colA = colA;
  d.colB = colB;
  return d;
}

std::vector<PlanetDef> initDefs() {
  std::vector<PlanetDef> list;
  list.reserve(11);

  // Index i gives angle0 = i * 0.8 rad and seed = i + 1.
  // spinRate = 2*pi / (rotation period in days * 10 sim-sec per day),
  // so Earth spins once per 10 sim-sec at 1x; retrograde bodies < 0.
  // 0: Sun
  {
    PlanetDef d = makeDef("sun", "Sun", 4.0f, 0.0f, 0.0f, 0.0f * 0.8f, 0, 1.0f,
                          Vec3(1.0f, 0.95f, 0.85f), Vec3(1.0f, 0.60f, 0.20f));
    d.facts = {"G2V main-sequence star", "Diameter 1,392,700 km"};
    d.spinRate = 2.0f * kPi / (27.0f * 10.0f);
    d.detail = 1;
    list.push_back(d);
  }
  // 1: Mercury
  {
    PlanetDef d = makeDef("mercury", "Mercury", 0.40f, 8.0f, 0.150f, 1.0f * 0.8f, 2, 2.0f,
                          Vec3(0.55f, 0.50f, 0.45f), Vec3(0.35f, 0.30f, 0.27f));
    d.facts = {"Diameter 4,879 km", "88-day year, no moons"};
    d.spinRate = 2.0f * kPi / (58.6f * 10.0f);
    d.detail = 2;
    list.push_back(d);
  }
  // 2: Venus
  {
    PlanetDef d = makeDef("venus", "Venus", 0.55f, 11.0f, 0.110f, 2.0f * 0.8f, 2, 3.0f,
                          Vec3(0.95f, 0.85f, 0.60f), Vec3(0.75f, 0.60f, 0.35f));
    d.facts = {"Diameter 12,104 km", "Hottest planet 465 C, spins backwards"};
    d.spinRate = -2.0f * kPi / (243.0f * 10.0f);
    d.detail = 3;
    list.push_back(d);
  }
  // 3: Earth
  {
    PlanetDef d = makeDef("earth", "Earth", 0.58f, 14.0f, 0.090f, 3.0f * 0.8f, 2, 4.0f,
                          Vec3(0.20f, 0.50f, 0.90f), Vec3(0.20f, 0.70f, 0.35f));
    d.facts = {"Diameter 12,742 km", "One moon, liquid water"};
    d.spinRate = 2.0f * kPi / (1.0f * 10.0f);
    d.detail = 4;
    list.push_back(d);
  }
  // 4: Moon (orbits Earth)
  {
    PlanetDef d = makeDef("moon", "Moon", 0.16f, 1.3f, 0.60f, 4.0f * 0.8f, 2, 5.0f,
                          Vec3(0.80f, 0.80f, 0.80f), Vec3(0.50f, 0.50f, 0.50f));
    d.parentId = "earth";
    d.facts = {"Diameter 3,474 km", "Tidally locked to Earth"};
    d.spinRate = d.angleSpeed;  // tidally locked: spin matches orbital rate
    d.detail = 5;
    list.push_back(d);
  }
  // 5: Mars
  {
    PlanetDef d = makeDef("mars", "Mars", 0.42f, 17.0f, 0.072f, 5.0f * 0.8f, 2, 6.0f,
                          Vec3(0.90f, 0.40f, 0.25f), Vec3(0.60f, 0.25f, 0.15f));
    d.facts = {"Diameter 6,779 km", "Olympus Mons, tallest volcano"};
    d.spinRate = 2.0f * kPi / (1.03f * 10.0f);
    d.detail = 6;
    list.push_back(d);
  }
  // 6: Jupiter
  {
    PlanetDef d = makeDef("jupiter", "Jupiter", 1.60f, 24.0f, 0.040f, 6.0f * 0.8f, 1, 7.0f,
                          Vec3(0.90f, 0.80f, 0.65f), Vec3(0.70f, 0.45f, 0.30f));
    d.facts = {"Diameter 139,820 km", "Great Red Spot storm"};
    d.spinRate = 2.0f * kPi / (0.41f * 10.0f);
    d.detail = 7;
    list.push_back(d);
  }
  // 7: Saturn
  {
    PlanetDef d = makeDef("saturn", "Saturn", 1.35f, 31.0f, 0.030f, 7.0f * 0.8f, 1, 8.0f,
                          Vec3(0.95f, 0.88f, 0.70f), Vec3(0.75f, 0.65f, 0.45f));
    d.hasRing = true;
    d.ringInner = 1.7f;
    d.ringOuter = 2.7f;
    d.facts = {"Diameter 116,460 km", "Rings: ice, Cassini Division"};
    d.spinRate = 2.0f * kPi / (0.44f * 10.0f);
    d.detail = 8;
    list.push_back(d);
  }
  // 8: Uranus
  {
    PlanetDef d = makeDef("uranus", "Uranus", 0.90f, 38.0f, 0.020f, 8.0f * 0.8f, 3, 9.0f,
                          Vec3(0.70f, 0.95f, 0.95f), Vec3(0.45f, 0.75f, 0.80f));
    d.facts = {"Diameter 50,724 km", "Tilted 98 degrees, rolls along"};
    d.spinRate = -2.0f * kPi / (0.72f * 10.0f);
    d.detail = 9;
    list.push_back(d);
  }
  // 9: Neptune
  {
    PlanetDef d = makeDef("neptune", "Neptune", 0.92f, 44.0f, 0.016f, 9.0f * 0.8f, 3, 10.0f,
                          Vec3(0.30f, 0.50f, 0.95f), Vec3(0.15f, 0.25f, 0.70f));
    d.facts = {"Diameter 49,244 km", "Supersonic methane winds"};
    d.spinRate = 2.0f * kPi / (0.67f * 10.0f);
    d.detail = 10;
    list.push_back(d);
  }
  // 10: Halley (eccentric inclined comet, type 4 coma ball)
  {
    PlanetDef d = makeDef("halley", "Halley", 0.28f, 30.0f, 0.055f, 2.0f, 4, 11.0f,
                          Vec3(0.75f, 1.0f, 0.85f), Vec3(1.0f, 1.0f, 1.0f));
    d.ecc = 0.75f;
    d.incl = 0.32f;
    d.facts = {"Periodic comet, 75-year orbit", "Tail grows near the Sun"};
    d.detail = 11;
    list.push_back(d);
  }

  return list;
}

// Kepler local position for def at timeSec.
// orbitRadius = semi-major axis a. ecc=0,incl=0 reproduces circular math.
Vec3 keplerLocal(const PlanetDef& d, float timeSec) {
  float M = d.angle0 + d.angleSpeed * timeSec;
  float E = M;
  for (int i = 0; i < 3; ++i) {
    E = M + d.ecc * std::sin(E);
  }
  float halfE = E * 0.5f;
  float nu = 2.0f * std::atan2(std::sqrt(1.0f + d.ecc) * std::sin(halfE),
                               std::sqrt(1.0f - d.ecc) * std::cos(halfE));
  float r = d.orbitRadius * (1.0f - d.ecc * std::cos(E));
  float cn = std::cos(nu);
  float sn = std::sin(nu);
  return Vec3(r * cn, r * sn * std::sin(d.incl), r * sn * std::cos(d.incl));
}

// Find def index by id, or -1.
int findIndex(const std::vector<PlanetDef>& defs, const std::string& id) {
  for (size_t i = 0; i < defs.size(); ++i) {
    if (defs[i].id == id) return static_cast<int>(i);
  }
  return -1;
}

}  // namespace

SolarSystem::SolarSystem() : m_defs(initDefs()), m_timeSec(0.0f) {}

void SolarSystem::update(float timeSec) { m_timeSec = timeSec; }

const std::vector<PlanetDef>& SolarSystem::defs() const { return m_defs; }

std::vector<BodyState> SolarSystem::bodies() const {
  std::vector<BodyState> out;
  out.reserve(m_defs.size());
  // World position cache by def index. Parents come before children in
  // the table, but resolve via lookup to stay robust.
  std::vector<Vec3> world(m_defs.size(), Vec3());
  std::vector<bool> done(m_defs.size(), false);

  for (size_t i = 0; i < m_defs.size(); ++i) {
    const PlanetDef& d = m_defs[i];
    Vec3 local = keplerLocal(d, m_timeSec);
    Vec3 w = local;
    if (!d.parentId.empty()) {
      int pi = findIndex(m_defs, d.parentId);
      if (pi >= 0) {
        // Parent is earlier in the table in practice; if not done yet,
        // compute its world position first (one level is enough here,
        // loop handles deeper chains).
        if (!done[static_cast<size_t>(pi)]) {
          const PlanetDef& p = m_defs[static_cast<size_t>(pi)];
          Vec3 plocal = keplerLocal(p, m_timeSec);
          Vec3 pw = plocal;
          if (!p.parentId.empty()) {
            int gi = findIndex(m_defs, p.parentId);
            if (gi >= 0 && done[static_cast<size_t>(gi)]) {
              pw = world[static_cast<size_t>(gi)] + plocal;
            }
          }
          world[static_cast<size_t>(pi)] = pw;
          done[static_cast<size_t>(pi)] = true;
        }
        w = world[static_cast<size_t>(pi)] + local;
      }
    }
    world[i] = w;
    done[i] = true;
  }

  for (size_t i = 0; i < m_defs.size(); ++i) {
    BodyState s;
    s.def = &m_defs[i];
    s.pos = world[i];
    out.push_back(s);
  }
  return out;
}

std::vector<Vec3> SolarSystem::orbitCircle(float radius, int segments) const {
  std::vector<Vec3> pts;
  if (segments <= 0 || radius < 0.0f) return pts;
  pts.reserve(static_cast<size_t>(segments));
  for (int i = 0; i < segments; ++i) {
    float a = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
    pts.emplace_back(std::cos(a) * radius, 0.0f, std::sin(a) * radius);
  }
  return pts;
}

}  // namespace astro
