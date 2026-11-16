#include "ColonySystem.h"

#include <cassert>
#include <iostream>

using namespace astro;

int main() {
  std::cout << "[test_colony] Starting ColonySystem 2.0 unit tests...\n";

  // Test 1: Worlds count and perks
  {
    const auto& worlds = colonyWorlds();
    if (worlds.size() != 8) {
      std::cerr << "Test 1 failed: expected 8 worlds, got " << worlds.size() << "\n";
      return 1;
    }
    const auto* mars = colonyWorld("mars");
    if (!mars || mars->perk.costDiscount < 0.10f) {
      std::cerr << "Test 1 failed: mars perk discount missing\n";
      return 1;
    }
    const auto* moon = colonyWorld("moon");
    if (!moon || moon->perk.warpRechargeBonus < 0.20f) {
      std::cerr << "Test 1 failed: moon perk warp bonus missing\n";
      return 1;
    }
    const auto* europa = colonyWorld("europa");
    if (!europa || europa->perk.scienceMultiplier < 0.30f) {
      std::cerr << "Test 1 failed: europa perk science bonus missing\n";
      return 1;
    }
    std::cout << "PASS: Test 1 worlds and perks\n";
  }

  // Test 2: 24 stages progression and epochs
  {
    const auto& stages = colonyStages();
    if (stages.size() != 24) {
      std::cerr << "Test 2 failed: expected 24 stages, got " << stages.size() << "\n";
      return 1;
    }
    if (colonyEpoch(1) != ColonyEpoch::Outpost ||
        colonyEpoch(5) != ColonyEpoch::Outpost ||
        colonyEpoch(6) != ColonyEpoch::Industry ||
        colonyEpoch(10) != ColonyEpoch::Industry ||
        colonyEpoch(11) != ColonyEpoch::Society ||
        colonyEpoch(14) != ColonyEpoch::Society ||
        colonyEpoch(16) != ColonyEpoch::HighTech ||
        colonyEpoch(21) != ColonyEpoch::Civilization ||
        colonyEpoch(24) != ColonyEpoch::Civilization) {
      std::cerr << "Test 2 failed: epoch mapping incorrect\n";
      return 1;
    }
    // Check monotonic population growth
    for (size_t i = 1; i < stages.size(); ++i) {
      if (stages[i].population < stages[i - 1].population) {
        std::cerr << "Test 2 failed: population not monotonic at stage " << i + 1 << "\n";
        return 1;
      }
    }
    std::cout << "PASS: Test 2 24 stages and epoch categorization\n";
  }

  // Test 3: Advance colony lifecycle
  {
    ColonyStages st;
    int rp = 500;
    int scans = 0;

    // Not founded yet
    if (advanceColony(st, "mars", scans, rp) != ColonyUpgradeResult::NotFounded) {
      std::cerr << "Test 3 failed: should be NotFounded\n";
      return 1;
    }

    // Found stage 1
    st["mars"] = 1;
    if (colonyStage(st, "mars") != 1) {
      std::cerr << "Test 3 failed: stage should be 1\n";
      return 1;
    }

    // Upgrade to stage 2 requires 1 scan
    if (advanceColony(st, "mars", scans, rp) != ColonyUpgradeResult::NeedScans) {
      std::cerr << "Test 3 failed: should require scans\n";
      return 1;
    }

    scans = 5;
    int beforeRp = rp;
    int expectedCost = colonyUpgradeCost(st, "mars", 1);
    auto res = advanceColony(st, "mars", scans, rp);
    if (res != ColonyUpgradeResult::Success) {
      std::cerr << "Test 3 failed: upgrade should succeed\n";
      return 1;
    }
    if (colonyStage(st, "mars") != 2) {
      std::cerr << "Test 3 failed: stage should be 2\n";
      return 1;
    }
    if (rp != beforeRp - expectedCost) {
      std::cerr << "Test 3 failed: RP deduction incorrect\n";
      return 1;
    }

    std::cout << "PASS: Test 3 advance colony lifecycle\n";
  }

  // Test 4: Population, passive generation rate, and resupply
  {
    ColonyStages st;
    st["mars"] = 5;    // Outpost tier 5: 75 pop
    st["moon"] = 14;   // Spaceport tier: allows resupply, 5000 pop

    int totalPop = colonyTotalPopulation(st);
    if (totalPop != 75 + 5000) {
      std::cerr << "Test 4 failed: total pop expected 5075, got " << totalPop << "\n";
      return 1;
    }

    if (colonyCanResupply(st, "mars")) {
      std::cerr << "Test 4 failed: mars at stage 5 cannot resupply\n";
      return 1;
    }
    if (!colonyCanResupply(st, "moon")) {
      std::cerr << "Test 4 failed: moon at stage 14 must allow resupply\n";
      return 1;
    }

    int passiveRate = colonyPassiveRatePerMin(st);
    if (passiveRate <= 0) {
      std::cerr << "Test 4 failed: passive rate should be positive\n";
      return 1;
    }

    std::cout << "PASS: Test 4 population, passive rates, and resupply\n";
  }

  // Test 5: Science bonus with world perk
  {
    ColonyStages st;
    st["mars"] = 12; // Stage 12: base +20%
    int bonusMars = colonyResearchBonus(st);
    if (bonusMars != 20) {
      std::cerr << "Test 5 failed: expected 20% bonus for Mars stage 12, got " << bonusMars << "\n";
      return 1;
    }

    st["europa"] = 12; // Europa has +40% perk -> 20 * 1.4 = 28%
    int bonusEuropa = colonyResearchBonus(st);
    if (bonusEuropa < 28) {
      std::cerr << "Test 5 failed: expected >= 28% for Europa stage 12, got " << bonusEuropa << "\n";
      return 1;
    }

    std::cout << "PASS: Test 5 science bonus with world perk multipliers\n";
  }

  // Test 6: Deep engineering lore, tech unlocks and environmental telemetry
  {
    for (const auto& w : colonyWorlds()) {
      if (!w.gravity || !w.atmosphereRu || !w.atmosphereEn || !w.temperatureRu || !w.temperatureEn) {
        std::cerr << "Test 6 failed: missing environmental telemetry for " << w.id << "\n";
        return 1;
      }
    }

    const auto& stages = colonyStages();
    for (size_t i = 0; i < stages.size(); ++i) {
      const auto& s = stages[i];
      if (!s.loreRu || !s.loreEn || !s.techUnlocksRu || !s.techUnlocksEn) {
        std::cerr << "Test 6 failed: stage " << i + 1 << " missing deep lore or tech unlocks\n";
        return 1;
      }
      if (std::string(s.loreRu).empty() || std::string(s.loreEn).empty() ||
          std::string(s.techUnlocksRu).empty() || std::string(s.techUnlocksEn).empty()) {
        std::cerr << "Test 6 failed: stage " << i + 1 << " has empty lore or tech unlocks\n";
        return 1;
      }
      if (s.epoch != colonyEpoch(static_cast<int>(i + 1))) {
        std::cerr << "Test 6 failed: epoch mismatch at stage " << i + 1 << "\n";
        return 1;
      }
    }

    std::cout << "PASS: Test 6 deep engineering lore, tech unlocks, and environmental telemetry\n";
  }

  std::cout << "[test_colony] ALL 6 TESTS PASSED!\n";
  return 0;
}
