#include "Catalog.h"

#include <cstdio>

namespace astro {

const std::vector<Station>& builtinCatalog() {
  static const std::vector<Station> kStations = {
      // v3.6.0: последнее поле — distanceLy (св. годы от Солнца, упрощённо).
      {"sgra", StationType::BlackHole, "Sgr A*",
       "Galactic center black hole. Schwarzschild lensing, Rs=1.0.", 10.0,
       {1.0, 30.0, 0.05, 600}, 1.05, 40.0, 26000.0},
      {"qso-3c273", StationType::Quasar, "3C 273",
       "Bright quasar: disk + jets + glow billboard.", 25.0, {}, 2.0, 15.0,
       2.44e9},
      {"ori", StationType::Constellation, "Orion",
       "Orion lines (Betelgeuse-Rigel belt).", 50.0, {}, 0.0, 0.0, 1344.0},
      {"uma", StationType::Constellation, "Ursa Major",
       "Big Dipper lines.", 50.0, {}, 0.0, 0.0, 80.0},
      {"m31", StationType::Galaxy, "Andromeda (M31)",
       "Spiral galaxy impostor sprite.", 80.0, {}, 0.0, 0.0, 2.54e6},
      {"m51", StationType::Galaxy, "Whirlpool (M51)",
       "Whirlpool spiral: two open arms.", 90.0, {}, 0.0, 0.0, 31.6e6},
      {"m104", StationType::Galaxy, "Sombrero (M104)",
       "Sombrero: bright bulge, dark dust lane.", 85.0, {}, 0.0, 0.0, 29.3e6},
      {"m1", StationType::Nebula, "Crab Nebula (M1)",
       "Supernova remnant impostor sprite.", 60.0, {}, 0.0, 0.0, 6500.0},
      {"sol", StationType::SolarSystem, "Solar System",
       "Sun + 8 planets + Moon, rings, belt.", 70.0, {}, 0.0, 0.0, 0.0},
      {"m87", StationType::BlackHole, "M87*",
       "Supermassive black hole with relativistic jet.", 14.0,
       {1.5, 30.0, 0.05, 600}, 1.575, 60.0, 53.5e6},
      {"psr-vela", StationType::Pulsar, "Vela Pulsar",
       "Young neutron star: tilted dipole beams, lighthouse pulse.", 35.0,
       {}, 0.5, 3.0, 958.0},
      {"psr-crab", StationType::Pulsar, "Crab Pulsar",
       "33 ms rotator in supernova remnant M1. Pair with station m1.",
       35.0, {}, 0.5, 3.0, 6500.0},
      {"sgr-1806", StationType::Pulsar, "SGR 1806-20",
       "Magnetar: 1e15 G field, starquake giant flares.", 40.0, {}, 0.5, 3.0,
       30000.0},
      {"psr-b1913", StationType::Pulsar, "PSR B1913+16",
       "Hulse-Taylor binary: orbital decay proved gravitational waves.",
       38.0, {}, 0.5, 3.0, 21000.0},
      {"mw", StationType::Galaxy, "Milky Way",
       "Our galaxy from outside. Find the Sun, fly close to go home.",
       45.0, {}, 0.0, 0.0, 0.0},
      {"m13", StationType::StarCluster, "Hercules Cluster (M13)",
       "Globular cluster: 300000 ancient stars. Fly between them.", 20.0,
       {}, 0.5, 0.0, 22200.0},
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

// v3.6.0: ETA по классической варп-шкале v = f^3 * c (упрощённо, как в
// старом добром каноне). Домен: 1..9; защита от нуля.
double warpEtaYears(double distanceLy, int warpFactor) {
  if (distanceLy <= 0.0) return 0.0;
  if (warpFactor < 1) warpFactor = 1;
  if (warpFactor > 9) warpFactor = 9;
  const double f = static_cast<double>(warpFactor);
  return distanceLy / (f * f * f);
}

std::string formatDistanceLy(double ly) {
  char buf[48];
  if (ly <= 0.0) return "local";
  if (ly >= 1.0e9) {
    std::snprintf(buf, sizeof(buf), "%.2f Gly", ly / 1.0e9);
  } else if (ly >= 1.0e6) {
    std::snprintf(buf, sizeof(buf), "%.2f Mly", ly / 1.0e6);
  } else if (ly >= 1.0e3) {
    std::snprintf(buf, sizeof(buf), "%.1f kly", ly / 1.0e3);
  } else {
    std::snprintf(buf, sizeof(buf), "%.0f ly", ly);
  }
  return buf;
}

std::string formatEtaYears(double years) {
  char buf[48];
  if (years <= 0.0) return "-";
  if (years >= 1.0e9) {
    std::snprintf(buf, sizeof(buf), "%.1f Gy", years / 1.0e9);
  } else if (years >= 1.0e6) {
    std::snprintf(buf, sizeof(buf), "%.1f My", years / 1.0e6);
  } else if (years >= 1.0e3) {
    std::snprintf(buf, sizeof(buf), "%.1f ky", years / 1.0e3);
  } else if (years >= 1.0) {
    std::snprintf(buf, sizeof(buf), "%.1f y", years);
  } else if (years >= 1.0 / 12.0) {
    std::snprintf(buf, sizeof(buf), "%.1f mo", years * 12.0);
  } else {
    std::snprintf(buf, sizeof(buf), "%.1f d", years * 365.25);
  }
  return buf;
}

}  // namespace astro
