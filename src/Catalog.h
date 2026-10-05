#pragma once
#include <string>
#include <vector>

namespace astro {

enum class StationType { BlackHole, Quasar, Constellation, Galaxy, Nebula };

struct Station {
  std::string id;
  StationType type;
  std::string name;
  std::string description;
  double spawnDistanceRs = 10.0;
};

const std::vector<Station>& builtinCatalog();
const Station* findStation(const std::string& id);
std::string typeToString(StationType t);

}  // namespace astro
