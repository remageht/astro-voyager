#pragma once
#include <string>
#include <vector>

namespace astro {

enum class StationType { BlackHole, Quasar, Constellation, Galaxy, Nebula };

struct StationParams {
  double rs = 1.0;
  double shellRadius = 30.0;
  double step = 0.05;
  int maxSteps = 600;
};

struct Station {
  std::string id;
  StationType type;
  std::string name;
  std::string description;
  double spawnDistanceRs = 10.0;
  StationParams params;
};

const std::vector<Station>& builtinCatalog();
const Station* findStation(const std::string& id);
std::string typeToString(StationType t);

}  // namespace astro
