#include "Catalog.h"

namespace astro {

const std::vector<Station>& builtinCatalog() {
  static const std::vector<Station> kStations = {
      {"sgra", StationType::BlackHole, "Sgr A*",
       "Galactic center black hole. Schwarzschild lensing, Rs=1.0.", 10.0,
       {1.0, 30.0, 0.05, 600}, 1.05, 40.0},
      {"qso-3c273", StationType::Quasar, "3C 273",
       "Bright quasar: disk + jets + glow billboard.", 25.0, {}, 2.0, 15.0},
      {"ori", StationType::Constellation, "Orion",
       "Orion lines (Betelgeuse-Rigel belt).", 50.0, {}},
      {"uma", StationType::Constellation, "Ursa Major",
       "Big Dipper lines.", 50.0, {}},
      {"m31", StationType::Galaxy, "Andromeda (M31)",
       "Spiral galaxy impostor sprite.", 80.0, {}},
      {"m51", StationType::Galaxy, "Whirlpool (M51)",
       "Whirlpool spiral: two open arms.", 90.0, {}},
      {"m104", StationType::Galaxy, "Sombrero (M104)",
       "Sombrero: bright bulge, dark dust lane.", 85.0, {}},
      {"m1", StationType::Nebula, "Crab Nebula (M1)",
       "Supernova remnant impostor sprite.", 60.0, {}},
      {"sol", StationType::SolarSystem, "Solar System",
       "Sun + 8 planets + Moon, rings, belt.", 70.0, {}},
      {"m87", StationType::BlackHole, "M87*",
       "Supermassive black hole with relativistic jet.", 14.0,
       {1.5, 30.0, 0.05, 600}, 1.575, 60.0},
      {"psr-vela", StationType::Pulsar, "Vela Pulsar",
       "Young neutron star: tilted dipole beams, lighthouse pulse.", 35.0,
       {}, 0.5, 3.0},
      {"psr-crab", StationType::Pulsar, "Crab Pulsar",
       "33 ms rotator in supernova remnant M1. Pair with station m1.",
       35.0, {}, 0.5, 3.0},
      {"sgr-1806", StationType::Pulsar, "SGR 1806-20",
       "Magnetar: 1e15 G field, starquake giant flares.", 40.0, {}, 0.5, 3.0},
      {"psr-b1913", StationType::Pulsar, "PSR B1913+16",
       "Hulse-Taylor binary: orbital decay proved gravitational waves.",
       38.0, {}, 0.5, 3.0},
      {"mw", StationType::Galaxy, "Milky Way",
       "Our galaxy from outside. Find the Sun, fly close to go home.",
       45.0, {}},
      {"m13", StationType::StarCluster, "Hercules Cluster (M13)",
       "Globular cluster: 300000 ancient stars. Fly between them.", 20.0,
       {}, 0.5},
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
    case StationType::SolarSystem: return "solar_system";
    case StationType::Pulsar: return "pulsar";
    case StationType::StarCluster: return "star_cluster";
  }
  return "unknown";
}

}  // namespace astro
