#pragma once
#include <string>
#include <vector>

namespace astro {

enum class StationType { BlackHole, Quasar, Constellation, Galaxy, Nebula, SolarSystem, Pulsar, StarCluster, Kilonova };

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
  // PH: station-level collision radius in world units (0.0 = disabled,
  // per-body rules apply instead). BlackHole: rs*1.05 (capture shell),
  // quasar: core-touch radius for slide + flash hint.
  double collisionRadius = 0.0;
  double mass = 0.0;  // GM game units for ship gravity (v3.3.0)
  // v3.6.0: расстояние от Солнца в световых годах (упрощённо, из
  // астрономических справочников) для дистанции/ETA в варп-панели.
  // 0 = локальная цель (ETA не показывается).
  double distanceLy = 0.0;
};

const std::vector<Station>& builtinCatalog();
const Station* findStation(const std::string& id);
std::string typeToString(StationType t);

// v3.6.0: упрощённая оценка времени перелёта по классической варп-шкале
// (v = f^3 * c): годы пути при данном факторе. 0/r==0 => 0.
double warpEtaYears(double distanceLy, int warpFactor);
// Человекочитаемые форматы для варп-панели: "26.0 kly", "2.54 Mly",
// "35.7 y", "4.3 mo". Отдельные функции — их проверяет core-тест.
std::string formatDistanceLy(double ly);
std::string formatEtaYears(double years);

}  // namespace astro
