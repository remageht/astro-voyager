#pragma once

#include <string>
#include <vector>

#include "Camera.h"

namespace astro {

// Planet definition: pure data, no GL, no GLM (core-safe, like Camera).
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
  Vec3 colA{1.0, 1.0, 1.0};
  Vec3 colB{1.0, 1.0, 1.0};
  bool hasRing = false;
  float ringInner = 0.0f;
  float ringOuter = 0.0f;
  std::string parentId;  // "" = orbits sun, else body id (moon->earth)
  std::vector<std::string> facts;  // 1-2 short ASCII lines each
  float ecc = 0.0f;   // eccentricity, 0 = circular
  float incl = 0.0f;  // inclination radians, tilt of orbital plane about X
  float spinRate = 0.0f;  // axial spin, rad per sim-second (retrograde if < 0)
};

struct BodyState {
  const PlanetDef* def = nullptr;
  Vec3 pos;
};

class SolarSystem {
 public:
  SolarSystem();  // builds table below, angles at angle0
  void update(float timeSec);  // angle = angle0 + angleSpeed*timeSec
  const std::vector<PlanetDef>& defs() const;
  std::vector<BodyState> bodies() const;  // world positions (parent-relative composed)
  std::vector<Vec3> orbitCircle(float radius, int segments = 128) const;

 private:
  std::vector<PlanetDef> m_defs;
  float m_timeSec = 0.0f;
};

}  // namespace astro
