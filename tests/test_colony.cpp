#include "ColonySystem.h"
#include "Types.h"

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

  // Test 7: surface build-out plans and per-world power plants
  {
    static const char* kKnownAssets[] = {"dome", "factory", "pad", "nuclear",
                                         "hydro", "thermal", "datacenter",
                                         "townhall", "parliament", "police",
                                         "warehouse", "military", "houses",
                                         "apartments", "pipeline", "powerline",
                                         "solar", "diesel", "inverter", "wind"};
    const auto isKnown = [](const std::string& asset) {
      for (const char* known : kKnownAssets)
        if (asset == known) return true;
      return false;
    };

    // Every stage contributes at least one structure, so progression is visible.
    const auto& plan = colonyStagePlan();
    for (size_t i = 0; i < plan.size(); ++i) {
      int items = 0;
      for (int s = 0; s < kColonyPlanSlots; ++s) {
        const ColonyStructure& entry = plan[i][s];
        if (entry.count <= 0) continue;
        ++items;
        const bool known = entry.asset == std::string(kColonyPowerAsset) ||
                           isKnown(entry.asset);
        if (!known) {
          std::cerr << "Test 7 failed: stage " << i + 1 << " uses unknown asset "
                    << entry.asset << "\n";
          return 1;
        }
      }
      if (items == 0) {
        std::cerr << "Test 7 failed: stage " << i + 1 << " builds nothing\n";
        return 1;
      }
    }

    // Airless bodies run fission, icy worlds hydro, Mercury/Titan thermal.
    if (std::string(colonyEnergyAsset("mars")) != "nuclear" ||
        std::string(colonyEnergyAsset("moon")) != "nuclear" ||
        std::string(colonyEnergyAsset("europa")) != "hydro" ||
        std::string(colonyEnergyAsset("hercules-b")) != "hydro" ||
        std::string(colonyEnergyAsset("mercury")) != "thermal" ||
        std::string(colonyEnergyAsset("titan")) != "thermal") {
      std::cerr << "Test 7 failed: world power plant mapping\n";
      return 1;
    }

    // The "power" placeholder must be resolved, never rendered literally.
    const auto stage3 = colonyBuildPlan("europa", 3);
    for (const auto& entry : stage3) {
      if (entry.first == kColonyPowerAsset || !isKnown(entry.first)) {
        std::cerr << "Test 7 failed: unresolved asset " << entry.first << "\n";
        return 1;
      }
    }

    // Build-out grows monotonically and merges duplicates per asset.
    int previous = -1;
    for (int stage = 1; stage <= kColonyStageCount; ++stage) {
      const auto buildPlan = colonyBuildPlan("mars", stage);
      int total = 0;
      for (const auto& entry : buildPlan) {
        total += entry.second;
        for (const auto& other : buildPlan) {
          if (&entry == &other) continue;
          if (entry.first == other.first) {
            std::cerr << "Test 7 failed: duplicate asset rows at stage " << stage << "\n";
            return 1;
          }
        }
      }
      if (total != colonyStructureCount("mars", stage)) {
        std::cerr << "Test 7 failed: structure count mismatch at stage " << stage << "\n";
        return 1;
      }
      if (total <= previous) {
        std::cerr << "Test 7 failed: build-out did not grow at stage " << stage << "\n";
        return 1;
      }
      previous = total;
    }

    // Stage 24 must show the full civilization footprint.
    const auto capital = colonyBuildPlan("mars", kColonyStageCount);
    bool hasPower = false, hasData = false, hasDome = false;
    for (const auto& entry : capital) {
      if (entry.first == "nuclear") hasPower = true;
      if (entry.first == "datacenter") hasData = true;
      if (entry.first == "dome") hasDome = true;
    }
    if (!hasPower || !hasData || !hasDome) {
      std::cerr << "Test 7 failed: capital build-out incomplete\n";
      return 1;
    }

    // Civic, housing and utility layers must appear in technological order:
    // backup gensets with the first grid, private plots before storey blocks,
    // administration before the parliament, and the capital keeps all of them.
    const auto has = [](const std::vector<std::pair<std::string, int>>& p,
                        const char* key) {
      for (const auto& e : p) if (e.first == key) return true;
      return false;
    };
    if (!has(colonyBuildPlan("mars", 3), "diesel") ||
        !has(colonyBuildPlan("mars", 5), "houses") ||
        has(colonyBuildPlan("mars", 5), "apartments") ||
        !has(colonyBuildPlan("mars", 8), "powerline") ||
        !has(colonyBuildPlan("mars", 11), "townhall") ||
        !has(colonyBuildPlan("mars", 13), "apartments") ||
        !has(colonyBuildPlan("mars", 16), "wind") ||
        !has(colonyBuildPlan("mars", 21), "parliament")) {
      std::cerr << "Test 7 failed: civic/housing/utility progression out of order\n";
      return 1;
    }
    for (const char* key : {"townhall", "parliament", "police", "warehouse",
                            "military", "houses", "apartments", "pipeline",
                            "powerline", "solar", "diesel", "inverter", "wind"}) {
      if (!has(capital, key)) {
        std::cerr << "Test 7 failed: capital missing " << key << "\n";
        return 1;
      }
    }

    std::cout << "PASS: Test 7 surface build-out plans and per-world power plants\n";
  }

  // Test 8: world-development milestones
  {
    const auto& ms = colonyTerraformMilestones();
    if (ms.size() < 8) {
      std::cerr << "Test 8 failed: expected a full terraforming ladder\n";
      return 1;
    }
    int lastStage = 0;
    float lastAtmo = -1.0f, lastBio = -1.0f;
    for (const auto& m : ms) {
      if (m.stage <= lastStage || m.stage > kColonyStageCount || m.rewardRP <= 0 ||
          m.atmosphere < lastAtmo || m.biosphere < lastBio) {
        std::cerr << "Test 8 failed: milestones must rise monotonically\n";
        return 1;
      }
      lastStage = m.stage;
      lastAtmo = m.atmosphere;
      lastBio = m.biosphere;
    }
    if (colonyMilestonesReached(0) != 0 || colonyMilestonesReached(4) != 1 ||
        colonyMilestonesReached(kColonyStageCount) != static_cast<int>(ms.size())) {
      std::cerr << "Test 8 failed: milestone counting\n";
      return 1;
    }
    float atmo = 1.0f, bio = 1.0f;
    colonyTerraformState(0, atmo, bio);
    if (atmo != 0.0f || bio != 0.0f) {
      std::cerr << "Test 8 failed: barren world must have no atmosphere or life\n";
      return 1;
    }
    colonyTerraformState(kColonyStageCount, atmo, bio);
    if (atmo < 0.99f || bio < 0.99f) {
      std::cerr << "Test 8 failed: a stage-24 world should be a full biosphere\n";
      return 1;
    }
    int totalReward = 0;
    for (const auto& m : ms) totalReward += m.rewardRP;
    if (colonyMilestoneReward(0, 0) != 0 || colonyMilestoneReward(kColonyStageCount, kColonyStageCount) != 0 ||
        colonyMilestoneReward(ms[0].stage, ms[0].stage + 1) != 0 ||
        colonyMilestoneReward(ms[0].stage - 1, ms[0].stage) != ms[0].rewardRP ||
        colonyMilestoneReward(0, kColonyStageCount) != totalReward) {
      std::cerr << "Test 8 failed: milestone rewards must be paid once, inside the crossed window only\n";
      return 1;
    }
    std::cout << "PASS: Test 8 world-development milestones\n";
  }

  // Test 9: perks feed the simulation, not only the panel text
  {
    auto near = [](float a, float b) { return a > b - 0.01f && a < b + 0.01f; };

    ColonyStages empty;
    if (colonyWarpRechargeBoost(empty) != 0.0f || colonyShieldBoost(empty) != 0.0f ||
        colonyMaxStage(empty) != 0 || colonyUnlocksEliteContracts(empty) ||
        colonyRevealsHiddenHypotheses(empty) || colonyGalacticVectorRoute(empty, "m13")) {
      std::cerr << "Test 9 failed: an empty federation must grant nothing\n";
      return 1;
    }

    // Founding is enough to light the world perk, same as scienceMultiplier.
    ColonyStages moon;
    moon["moon"] = 1;
    if (!near(colonyWarpRechargeBoost(moon), 0.25f) || colonyShieldBoost(moon) != 0.0f) {
      std::cerr << "Test 9 failed: lunar He-3 must speed warp recharge only\n";
      return 1;
    }
    if (!near(ship::shieldCapacity(colonyShieldBoost(moon)), 100.0f)) {
      std::cerr << "Test 9 failed: no shield perk means baseline capacity\n";
      return 1;
    }

    // The best developed world wins, so Mercury outclasses the M13 anchor.
    ColonyStages shielded;
    shielded["mercury"] = 5;
    shielded["hercules-b"] = 5;
    if (!near(colonyShieldBoost(shielded), 0.20f) ||
        !near(ship::shieldCapacity(colonyShieldBoost(shielded)), 120.0f)) {
      std::cerr << "Test 9 failed: shield perk must raise capacity to 120\n";
      return 1;
    }

    // Stage gates promised by the tech text, checked on both sides.
    ColonyStages gates;
    gates["mars"] = kColonyEliteContractStage - 1;
    if (colonyUnlocksEliteContracts(gates)) {
      std::cerr << "Test 9 failed: elite contracts need stage 16\n";
      return 1;
    }
    gates["mars"] = kColonyEliteContractStage;
    if (!colonyUnlocksEliteContracts(gates) || colonyRevealsHiddenHypotheses(gates)) {
      std::cerr << "Test 9 failed: elite stage must not open the archive\n";
      return 1;
    }
    gates["mars"] = kColonyHiddenHypothesisStage;
    if (!colonyRevealsHiddenHypotheses(gates) ||
        colonyGalacticVectorRoute(gates, "m13")) {
      std::cerr << "Test 9 failed: archive opens at 23, the vector not before 24\n";
      return 1;
    }
    gates["mars"] = kColonyGalacticVectorStage;
    if (!colonyUnlocksGalacticVector(gates) || !colonyGalacticVectorRoute(gates, "m13")) {
      std::cerr << "Test 9 failed: stage 24 must open the M13 route\n";
      return 1;
    }
    if (colonyGalacticVectorRoute(gates, "sol")) {
      std::cerr << "Test 9 failed: the beacon is a route to M13, not a free ride\n";
      return 1;
    }
    if (colonyMaxStage(gates) != kColonyStageCount) {
      std::cerr << "Test 9 failed: max stage must follow the federation\n";
      return 1;
    }
    std::cout << "PASS: Test 9 perks applied to warp, shields, unlocks and routes\n";
  }

  std::cout << "[test_colony] ALL 9 TESTS PASSED!\n";
  return 0;
}
