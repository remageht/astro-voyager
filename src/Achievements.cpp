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

  // 5. horizon_watcher: minBhDistance <= 3.0
  if (s.minBhDistance <= 3.0) {
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

  return result;
}

const char* achievementTitle(const std::string& id) {
  if (id == "first_steps") return "First Steps";
  if (id == "constellation_hunter") return "Constellation Hunter";
  if (id == "deep_field") return "Deep Field";
  if (id == "wayfinder") return "Wayfinder";
  if (id == "horizon_watcher") return "Horizon Watcher";
  if (id == "messenger") return "Messenger";
  if (id == "cartographer") return "Cartographer";
  return "";
}

const char* achievementDescription(const std::string& id) {
  if (id == "first_steps") return "Visit two different stations";
  if (id == "constellation_hunter") return "Visit Orion and Ursa Major";
  if (id == "deep_field") return "Visit all six stations";
  if (id == "wayfinder") return "Record a route of 8 jumps";
  if (id == "horizon_watcher") return "Approach the black hole within 3 Rs";
  if (id == "messenger") return "Export your journey to a file";
  if (id == "cartographer") return "Import a shared journey";
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
    "cartographer"
  };
  return kIds;
}

}  // namespace astro
