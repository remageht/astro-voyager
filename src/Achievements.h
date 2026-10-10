#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace astro {

struct AchievementState {
  std::vector<std::string> visited;  // unique ids of visited stations
  size_t routeSize = 0;              // m_route.size()
  double minBhDistance = 1e9;        // min distance from camera to center at BlackHole station
  bool exported = false;             // successful route export
  bool imported = false;             // successful route import
};

std::vector<std::string> achievementsCheck(const AchievementState& s);
const char* achievementTitle(const std::string& id);
const char* achievementDescription(const std::string& id);
const std::vector<std::string>& allAchievementIds();

}  // namespace astro
