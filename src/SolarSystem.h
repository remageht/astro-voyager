#pragma once

#include <string>
#include <vector>

#include "vendor/glm/glm/glm.hpp"

namespace astro {

// Planet definition: pure data, no GL.
// type: 0 sun, 1 gas giant, 2 rocky, 3 ice giant, 4 coma ball.
struct PlanetDef {
  std::string id;
  std::string name;
  float radius = 0.0f;
  float orbitRadius = 0.0f;
  float angleSpeed = 0.0f;
  float angle0 = 0.0f;
  int type = 2;
  float seed = 1.0f;
  glm::vec3 colA{1.0f, 1.0f, 1.0f};
  glm::vec3 colB{1.0f, 1.0f, 1.0f};
  bool hasRing = false;
  float ringInner = 0.0f;
  float ringOuter = 0.0f;
  std::string parentId;  // "" = orbits sun, else body id (moon->earth)
  std::vector<std::string> facts;  // 1-2 short ASCII lines each
  float ecc = 0.0f;   // eccentricity, 0 = circular
  float incl = 0.0f;  // inclination radians, tilt of orbital plane about X
};

struct BodyState {
  const PlanetDef* def = nullptr;
  glm::vec3 pos{0.0f, 0.0f, 0.0f};
};

class SolarSystem {
 public:
  SolarSystem();  // builds table below, angles at angle0
  void update(float timeSec);  // angle = angle0 + angleSpeed*timeSec
  const std::vector<PlanetDef>& defs() const;
  std::vector<BodyState> bodies() const;  // world positions (parent-relative composed)
  std::vector<glm::vec3> orbitCircle(float radius, int segments = 128) const;

 private:
  std::vector<PlanetDef> m_defs;
  float m_timeSec = 0.0f;
};

}  // namespace astro
