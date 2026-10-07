#pragma once

#include <string>
#include <vector>
#include <utility>

namespace astro {

struct StarInfo {
  std::string name;
  std::string bayer;
  int hip = 0;
  float ra = 0.0f;    // Right Ascension (degrees)
  float dec = 0.0f;   // Declination (degrees)
  float mag = 0.0f;   // Visual magnitude
  float x = 0.0f;     // Projected 3D X
  float y = 0.0f;     // Projected 3D Y
  float z = 0.0f;     // Projected 3D Z
};

struct ConstellationInfo {
  std::string id;
  std::string name;
  float centerRa = 0.0f;
  float centerDec = 0.0f;
  std::vector<StarInfo> stars;
  std::vector<std::pair<unsigned int, unsigned int>> segments;
};

// Returns built-in constellation data (mirrors res/constellations.json)
const std::vector<ConstellationInfo>& builtinConstellations();
const ConstellationInfo* findConstellation(const std::string& id);

}  // namespace astro
