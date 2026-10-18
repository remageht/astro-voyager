#include "Achievements.h"

namespace astro {

std::vector<std::string> achievementsCheck(const AchievementState& s) {
  std::vector<std::string> result;

  // 1. first_steps: visited.size() >= 2
  if (s.visited.size() >= 2) {
    result.push_back("first_steps");
  }

  // 2. constellation_hunter: visited contains both "ori" and "uma"
  bool hasOri = false;
  bool hasUma = false;
  for (const auto& id : s.visited) {
    if (id == "ori") hasOri = true;
    if (id == "uma") hasUma = true;
  }
  if (hasOri && hasUma) {
    result.push_back("constellation_hunter");
  }

  // 3. deep_field: visited.size() >= 6
  if (s.visited.size() >= 6) {
    result.push_back("deep_field");
  }

  // 4. wayfinder: routeSize >= 8
  if (s.routeSize >= 8) {
    result.push_back("wayfinder");
  }

  // 5. horizon_watcher ("Горизонт событий"): minBhDistance <= 3.0
  // (close approach) OR explicit event-horizon capture (rs*1.05 crossed,
  // set by AppGL capture cutscene). Old saves/tests without the new flag
  // keep working: default false, old condition unchanged.
  if (s.minBhDistance <= 3.0 || s.capturedHorizon) {
    result.push_back("horizon_watcher");
  }

  // 6. messenger: exported == true
  if (s.exported) {
    result.push_back("messenger");
  }

  // 7. cartographer: imported == true
  if (s.imported) {
    result.push_back("cartographer");
  }

  // 8. colonist (v3.7.0): colonyCount >= 1
  if (s.colonyCount >= 1) {
    result.push_back("colonist");
  }

  // 9. architect (v3.7.0): colonyCount >= 2
  if (s.colonyCount >= 2) {
    result.push_back("architect");
  }

  // 10. debris_dancer (v3.7.0): успешный маневр в полосе обломков
  if (s.asteroidDodged) {
    result.push_back("debris_dancer");
  }

  return result;
}

const char* achievementTitle(const std::string& id) {
  if (id == "first_steps") return "First Steps";
  if (id == "constellation_hunter") return "Constellation Hunter";
  if (id == "deep_field") return "Deep Field";
  if (id == "wayfinder") return "Wayfinder";
  if (id == "horizon_watcher") return "Горизонт событий";
  if (id == "messenger") return "Messenger";
  if (id == "cartographer") return "Cartographer";
  if (id == "colonist") return "Первопроходец";
  if (id == "architect") return "Архитектор сети";
  if (id == "debris_dancer") return "Танцор обломков";
  return "";
}

const char* achievementDescription(const std::string& id) {
  if (id == "first_steps") return "Visit two different stations";
  if (id == "constellation_hunter") return "Visit Orion and Ursa Major";
  if (id == "deep_field") return "Visit all six stations";
  if (id == "wayfinder") return "Record a route of 8 jumps";
  if (id == "horizon_watcher") return "Захвачен горизонтом событий";
  if (id == "messenger") return "Export your journey to a file";
  if (id == "cartographer") return "Import a shared journey";
  if (id == "colonist") return "Основать первую колонию";
  if (id == "architect") return "Основать колонии на обеих планетах";
  if (id == "debris_dancer") return "Пройти маневром полосу обломков";
  return "";
}

const std::vector<std::string>& allAchievementIds() {
  static const std::vector<std::string> kIds = {
    "first_steps",
    "constellation_hunter",
    "deep_field",
    "wayfinder",
    "horizon_watcher",
    "messenger",
    "cartographer",
    "colonist",
    "architect",
    "debris_dancer"
  };
  return kIds;
}

}  // namespace astro
