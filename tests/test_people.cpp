#include "../src/PeopleSystem.h"
#include "../src/SaveGame.h"
#include "../src/ColonySystem.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

namespace {

bool close(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps;
}

}  // namespace

int main() {
  std::cout << "[test_people] Starting PeopleSystem & Crew test suite..." << std::endl;

  // Test 1: Procedural person generation
  {
    astro::Person p1 = astro::generatePerson("mars", 12345, astro::Profession::WarpEngineer);
    assert(!p1.id.empty());
    assert(!p1.firstName.empty());
    assert(!p1.lastName.empty());
    assert(p1.originWorldId == "mars");
    assert(p1.profession == astro::Profession::WarpEngineer);
    assert(p1.level >= 1 && p1.level <= 3);
    assert(p1.vitals.health >= 80.0f && p1.vitals.health <= 100.0f);
    assert(p1.vitals.nourishment >= 70.0f && p1.vitals.nourishment <= 100.0f);
    assert(p1.vitals.morale >= 70.0f && p1.vitals.morale <= 100.0f);
    assert(!p1.currentActivityRu.empty());
    assert(!p1.currentActivityEn.empty());

    // Deterministic with same seed
    astro::Person p2 = astro::generatePerson("mars", 12345, astro::Profession::WarpEngineer);
    assert(p1.firstName == p2.firstName);
    assert(p1.lastName == p2.lastName);
    assert(p1.quirk == p2.quirk);
    std::cout << "PASS: Test 1 - Deterministic person generation (" << p1.firstName << " " << p1.lastName << ")" << std::endl;
  }

  // Test 2: Colony notable candidates generation
  {
    auto candidates = astro::generateColonyNotableCandidates("europa", 14, 999);
    assert(candidates.size() >= 2);
    for (const auto& c : candidates) {
      assert(c.originWorldId == "europa");
      assert(c.level >= 2); // Stage 14 produces experienced specialists
    }
    std::cout << "PASS: Test 2 - Colony notable candidates generation (count: " << candidates.size() << ")" << std::endl;
  }

  // Test 3: Crew roster assignments and multipliers
  {
    astro::ShipCrewRoster roster;
    assert(!roster.hasPost(astro::CrewPost::ChiefEngineer));
    assert(close(roster.warpCooldownMultiplier(), 1.0f));
    assert(close(roster.shieldRegenMultiplier(), 1.0f));
    assert(close(roster.scienceYieldMultiplier(), 1.0f));
    assert(close(roster.fuelConsumptionMultiplier(), 1.0f));
    assert(close(roster.evaOxygenDrainMultiplier(), 1.0f));

    // Assign Chief Engineer
    astro::Person eng = astro::generatePerson("moon", 42, astro::Profession::WarpEngineer);
    eng.level = 3;
    eng.quirk = astro::PersonQuirk::RadiationResistant;
    roster.assign(astro::CrewPost::ChiefEngineer, eng);

    assert(roster.hasPost(astro::CrewPost::ChiefEngineer));
    assert(roster.getPost(astro::CrewPost::ChiefEngineer)->firstName == eng.firstName);
    // Warp cooldown multiplier should be < 1.0f
    assert(roster.warpCooldownMultiplier() < 0.85f);
    // Shield regen bonus should be > 1.10f
    assert(roster.shieldRegenMultiplier() > 1.15f);
    // Hull auto repair rate > 0
    assert(roster.hullAutoRepairRate() > 0.10f);

    // Assign Science Officer with Visionary quirk
    astro::Person sci = astro::generatePerson("earth", 777, astro::Profession::Astrophysicist);
    sci.level = 4;
    sci.quirk = astro::PersonQuirk::Visionary;
    roster.assign(astro::CrewPost::ScienceOfficer, sci);

    assert(roster.scienceYieldMultiplier() > 1.40f);

    // Assign Navigator
    astro::Person nav = astro::generatePerson("titan", 888, astro::Profession::Navigator);
    nav.level = 3;
    nav.quirk = astro::PersonQuirk::CalmUnderPressure;
    roster.assign(astro::CrewPost::Navigator, nav);

    assert(roster.fuelConsumptionMultiplier() < 0.90f);
    assert(roster.tidalStressTolerance() > 1.25f);

    // Assign Life Support Medic
    astro::Person med = astro::generatePerson("europa", 555, astro::Profession::EclssMedic);
    med.level = 2;
    med.quirk = astro::PersonQuirk::VoidBorn;
    roster.assign(astro::CrewPost::LifeSupportMedic, med);

    assert(roster.evaOxygenDrainMultiplier() < 0.80f);

    std::cout << "PASS: Test 3 - Crew roster bonuses and multipliers" << std::endl;
  }

  // Test 4: Crew experience and level up progression
  {
    astro::ShipCrewRoster roster;
    astro::Person p = astro::generatePerson("sol", 101, astro::Profession::WarpEngineer);
    p.level = 1;
    p.xp = 0;
    roster.assign(astro::CrewPost::ChiefEngineer, p);

    roster.addExperience(250, "Warp transit jump");
    const auto* post = roster.getPost(astro::CrewPost::ChiefEngineer);
    assert(post != nullptr);
    assert(post->level == 2);
    assert(post->xp == 50);

    std::cout << "PASS: Test 4 - Crew XP progression & level up" << std::endl;
  }

  // Test 5: Colony vitals and Technological Surge
  {
    // High stage build plan with multiple services
    std::vector<std::pair<std::string, int>> buildPlan = {
        {"colony_greenhouse", 3},
        {"colony_water", 2},
        {"colony_hospital", 2},
        {"colony_school", 2},
        {"colony_park", 3},
        {"colony_market", 2}
    };

    astro::ColonyVitals vitals = astro::calculateColonyVitals("mars", 16, buildPlan);
    assert(vitals.avgHealth >= 75.0f);
    assert(vitals.avgNourishment >= 70.0f);
    assert(vitals.avgMorale >= 70.0f);
    assert(vitals.inTechnologicalSurge == true);
    assert(vitals.scienceMultiplier >= 1.50f);
    assert(vitals.buildDiscount >= 0.15f);
    assert(!vitals.activeBreakthroughRu.empty());

    std::cout << "PASS: Test 5 - Colony vitals & Technological Surge (" << vitals.activeBreakthroughRu << ")" << std::endl;
  }

  // Test 6: Round-trip savegame serialization
  {
    astro::SaveGame g;
    g.mode = astro::SaveMode::Sandbox;
    g.savedUtc = "2026-11-22T16:00:00Z";
    g.progress.colonies = {"mars"};
    g.progress.colonyStages["mars"] = 12;

    astro::Person eng = astro::generatePerson("mars", 111, astro::Profession::WarpEngineer);
    eng.level = 3;
    g.progress.crew.assign(astro::CrewPost::ChiefEngineer, eng);

    astro::Person sci = astro::generatePerson("europa", 222, astro::Profession::Astrophysicist);
    sci.level = 2;
    g.progress.crew.assign(astro::CrewPost::ScienceOfficer, sci);

    std::string serialized = astro::saveSerialize(g);
    assert(serialized.find("\"crew\":") != std::string::npos);
    assert(serialized.find(eng.firstName) != std::string::npos);
    assert(serialized.find(sci.firstName) != std::string::npos);

    astro::SaveGame loaded;
    std::string err;
    bool ok = astro::saveDeserialize(serialized, loaded, err);
    assert(ok);
    assert(err.empty());
    assert(loaded.progress.crew.hasPost(astro::CrewPost::ChiefEngineer));
    assert(loaded.progress.crew.hasPost(astro::CrewPost::ScienceOfficer));
    assert(!loaded.progress.crew.hasPost(astro::CrewPost::Navigator));

    const auto* loadedEng = loaded.progress.crew.getPost(astro::CrewPost::ChiefEngineer);
    assert(loadedEng != nullptr);
    assert(loadedEng->firstName == eng.firstName);
    assert(loadedEng->level == 3);

    std::cout << "PASS: Test 6 - SaveGame crew round-trip serialization" << std::endl;
  }

  // Test 7: Localization and titles
  {
    assert(astro::professionNameRu(astro::Profession::Astrophysicist) != nullptr);
    assert(std::string(astro::professionNameEn(astro::Profession::Astrophysicist)).find("Science") != std::string::npos);
    assert(std::string(astro::professionNameEn(astro::Profession::WarpEngineer)).find("Engineer") != std::string::npos);
    assert(astro::quirkNameRu(astro::PersonQuirk::VoidBorn) != nullptr);
    assert(std::string(astro::quirkNameEn(astro::PersonQuirk::VoidBorn)).find("Void") != std::string::npos);
    assert(std::string(astro::quirkDescEn(astro::PersonQuirk::RadiationResistant)).find("radiation") != std::string::npos);
    assert(std::string(astro::crewPostNameEn(astro::CrewPost::ChiefEngineer)).find("Chief Engineer") != std::string::npos);
    std::cout << "PASS: Test 7 - Localization strings and formatting" << std::endl;
  }

  std::cout << "\nAll 7 test suites in test_people PASSED successfully!" << std::endl;
  return 0;
}