#include "Catalog.h"

namespace astro {

const std::vector<Station>& builtinCatalog() {
  static const std::vector<Station> kStations = {
      {"sgra", StationType::BlackHole, "Sgr A*",
       "Galactic center black hole. Schwarzschild lensing, Rs=1.0.", 10.0},
      {"qso-3c273", StationType::Quasar, "3C 273",
       "Bright quasar: disk + jets + glow billboard.", 25.0},
      {"ori", StationType::Constellation, "Orion",
       "Orion lines (Betelgeuse-Rigel belt).", 50.0},
      {"uma", StationType::Constellation, "Ursa Major",
       "Big Dipper lines.", 50.0},
      {"m31", StationType::Galaxy, "Andromeda (M31)",
       "Spiral galaxy impostor sprite.", 80.0},
      {"m1", StationType::Nebula, "Crab Nebula (M1)",
       "Supernova remnant impostor sprite.", 60.0},
  };
  return kStations;
}

const Station* findStation(const std::string& id) {
  for (const auto& s : builtinCatalog()) {
    if (s.id == id) return &s;
  }
  return nullptr;
}

std::string typeToString(StationType t) {
  switch (t) {
    case StationType::BlackHole: return "black_hole";
    case StationType::Quasar: return "quasar";
    case StationType::Constellation: return "constellation";
    case StationType::Galaxy: return "galaxy";
    case StationType::Nebula: return "nebula";
  }
  return "unknown";
}

}  // namespace astro
