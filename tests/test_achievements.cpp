#include "../src/Achievements.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
  // Test 1: empty state -> empty result
  {
    astro::AchievementState s;
    auto res = astro::achievementsCheck(s);
    if (!res.empty()) {
      std::cerr << "Test 1 failed: expected empty, got " << res.size() << '\n';
      return 1;
    }
  }

  // Test 2: two visited -> only first_steps
  {
    astro::AchievementState s;
    s.visited = {"sgra", "m31"};
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"first_steps"};
    if (res != expected) {
      std::cerr << "Test 2 failed: expected first_steps\n";
      return 1;
    }
  }

  // Test 3: ori + uma visited -> first_steps + constellation_hunter
  {
    astro::AchievementState s;
    s.visited = {"ori", "uma"};
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"first_steps", "constellation_hunter"};
    if (res != expected) {
      std::cerr << "Test 3 failed: expected first_steps + constellation_hunter\n";
      return 1;
    }
  }

  // Test 4: 6 visited -> +deep_field
  {
    astro::AchievementState s;
    s.visited = {"sgra", "3c273", "ori", "uma", "m31", "m1"};
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"first_steps", "constellation_hunter", "deep_field"};
    if (res != expected) {
      std::cerr << "Test 4 failed: expected deep_field combo\n";
      return 1;
    }
  }

  // Test 5: routeSize 8 -> +wayfinder
  {
    astro::AchievementState s;
    s.routeSize = 8;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"wayfinder"};
    if (res != expected) {
      std::cerr << "Test 5 failed: expected wayfinder\n";
      return 1;
    }
  }

  // Test 6: minBhDistance 2.5 -> +horizon_watcher
  {
    astro::AchievementState s;
    s.minBhDistance = 2.5;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"horizon_watcher"};
    if (res != expected) {
      std::cerr << "Test 6 failed: expected horizon_watcher\n";
      return 1;
    }
  }

  // Test 7: exported + imported -> +messenger + cartographer
  {
    astro::AchievementState s;
    s.exported = true;
    s.imported = true;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"messenger", "cartographer"};
    if (res != expected) {
      std::cerr << "Test 7 failed: expected messenger + cartographer\n";
      return 1;
    }
  }

  // Test 8: metadata check for all 10 achievements
  {
    const auto& ids = astro::allAchievementIds();
    if (ids.size() != 10) {
      std::cerr << "Test 8 failed: expected 10 achievement IDs\n";
      return 1;
    }
    for (const auto& id : ids) {
      std::string title = astro::achievementTitle(id);
      std::string desc = astro::achievementDescription(id);
      if (title.empty() || desc.empty()) {
        std::cerr << "Test 8 failed: empty title or desc for " << id << '\n';
        return 1;
      }
    }
  }

  // Test 9 (v3.7.0): colonyCount 1 -> only colonist
  {
    astro::AchievementState s;
    s.colonyCount = 1;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"colonist"};
    if (res != expected) {
      std::cerr << "Test 9 failed: expected colonist\n";
      return 1;
    }
  }

  // Test 10 (v3.7.0): colonyCount 2 -> colonist + architect
  {
    astro::AchievementState s;
    s.colonyCount = 2;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"colonist", "architect"};
    if (res != expected) {
      std::cerr << "Test 10 failed: expected colonist + architect\n";
      return 1;
    }
  }

  // Test 11 (v3.7.0): asteroidDodged -> debris_dancer
  {
    astro::AchievementState s;
    s.asteroidDodged = true;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"debris_dancer"};
    if (res != expected) {
      std::cerr << "Test 11 failed: expected debris_dancer\n";
      return 1;
    }
  }

  // Test 12 (v3.7.0): zero dodge -> nothing
  {
    astro::AchievementState s;
    auto res = astro::achievementsCheck(s);
    std::vector<std::string> expected = {"colonist"};
    if (res == expected) {
      std::cerr << "Test 12 failed: empty state must not unlock colonist\n";
      return 1;
    }
  }

  std::cout << "achievement tests: ALL PASS\n";
  return 0;
}
