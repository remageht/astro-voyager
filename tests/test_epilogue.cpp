#include <cassert>
#include <iostream>
#include <string>

#include "EpilogueSystem.h"
#include "Expedition.h"
#include "Zettelkasten.h"

using namespace astro;

void testSolarRebirthTrueEnding() {
  std::cout << "[Test 1] Solar Rebirth True Ending (kClusterSolar unlocked)... ";
  ZettelGraph zg;
  zg.init();
  // Discover and synthesize solar notes to unlock true ending
  zg.discoverSeed("z_paris_rain");
  zg.discoverSeed("z_casimir_sun");
  zg.synthesizeNote("z_paris_rain");
  zg.synthesizeNote("z_casimir_sun");
  zg.triggerSynthesisRecipe("recipe_solar_rekindle");

  assert(zg.isClusterTrueEndingUnlocked(kClusterSolar));

  ExpeditionState exp;
  exp.chapterIndex = 6;
  exp.maxUnlockedChapter = 6;
  exp.introSeen = true;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleSolarRebirth, exp, zg, 4, 1500, 18, 92.5f);

  assert(c.isTrueEnding);
  assert(c.clusterKey == kClusterSolar);
  assert(c.anchorStationId == "sol");
  assert(c.titleRu.find("ВЕЧНЫЙ РАССВЕТ") != std::string::npos);
  assert(c.titleEn.find("ETERNAL DAWN") != std::string::npos);
  assert(c.subtitleRu.find("Елена") != std::string::npos);
  assert(c.narrativeParagraphsRu.size() >= 3);
  assert(c.crewFates.size() >= 4);
  assert(c.stats.coreDrillSamplesCount == 4);
  assert(c.stats.researchPoints == 1500);
  assert(c.stats.warpJumpsCount == 18);
  assert(c.stats.hullIntegrityPercent > 90.0f);
  assert(!c.closingQuoteRu.empty());
  std::cout << "PASSED\n";
}

void testEdenColonyTrueEnding() {
  std::cout << "[Test 2] Eden Colony True Ending (kClusterEden unlocked)... ";
  ZettelGraph zg;
  zg.init();
  zg.discoverSeed("z_spores_titan");
  zg.discoverSeed("z_greenhouse_nomad");
  zg.synthesizeNote("z_spores_titan");
  zg.synthesizeNote("z_greenhouse_nomad");
  zg.triggerSynthesisRecipe("recipe_eden_genesis");

  assert(zg.isClusterTrueEndingUnlocked(kClusterEden));

  ExpeditionState exp;
  exp.chapterIndex = 6;
  exp.maxUnlockedChapter = 6;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleColony, exp, zg, 5, 2000, 22, 85.0f);

  assert(c.isTrueEnding);
  assert(c.clusterKey == kClusterEden);
  assert(c.anchorStationId == "hercules_b");
  assert(c.titleRu.find("ЦВЕТУЩИЙ ЭДЕМ") != std::string::npos);
  assert(c.titleEn.find("BLOSSOMING EDEN") != std::string::npos);
  assert(c.subtitleRu.find("Мия Сато") != std::string::npos);
  assert(c.narrativeParagraphsRu.size() >= 3);
  assert(c.stats.coreDrillSamplesCount == 5);
  std::cout << "PASSED\n";
}

void testMultiverseTrueEnding() {
  std::cout << "[Test 3] Multiverse True Ending (kClusterMultiverse unlocked)... ";
  ZettelGraph zg;
  zg.init();
  zg.discoverSeed("z_egorka_six_echo");
  zg.discoverSeed("z_cauchy_horizon");
  zg.synthesizeNote("z_egorka_six_echo");
  zg.synthesizeNote("z_cauchy_horizon");
  zg.triggerSynthesisRecipe("recipe_multiverse_bridge");

  assert(zg.isClusterTrueEndingUnlocked(kClusterMultiverse));

  ExpeditionState exp;
  exp.chapterIndex = 6;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleLoopTranscendence, exp, zg, 2, 3500, 30, 99.0f);

  assert(c.isTrueEnding);
  assert(c.clusterKey == kClusterMultiverse);
  assert(c.anchorStationId == "sgra");
  assert(c.titleRu.find("АРХИТЕКТОРЫ БЕСКОНЕЧНОСТИ") != std::string::npos);
  assert(c.titleEn.find("ARCHITECTS OF INFINITY") != std::string::npos);
  assert(c.subtitleRu.find("Егорка-6") != std::string::npos);
  std::cout << "PASSED\n";
}

void testStasisAllianceTrueEnding() {
  std::cout << "[Test 4] Stasis Alliance True Ending (kClusterStasis unlocked)... ";
  ZettelGraph zg;
  zg.init();
  zg.discoverSeed("z_stasis_trauma");
  zg.discoverSeed("z_fifth_accord");
  zg.synthesizeNote("z_stasis_trauma");
  zg.synthesizeNote("z_fifth_accord");
  zg.triggerSynthesisRecipe("recipe_stasis_harmony");

  assert(zg.isClusterTrueEndingUnlocked(kClusterStasis));

  ExpeditionState exp;
  exp.chapterIndex = 6;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleFifthAlliance, exp, zg, 3, 1800, 14, 75.0f);

  assert(c.isTrueEnding);
  assert(c.clusterKey == kClusterStasis);
  assert(c.anchorStationId == "m13");
  assert(c.titleRu.find("БРАТСТВО ПЯТИ МИРОВ") != std::string::npos);
  assert(c.titleEn.find("BROTHERHOOD OF FIVE REALITIES") != std::string::npos);
  assert(c.subtitleRu.find("Арсений") != std::string::npos);
  std::cout << "PASSED\n";
}

void testFallbackNonTrueEnding() {
  std::cout << "[Test 5] Fallback Non-True Ending... ";
  ZettelGraph zg;
  zg.init();
  // No synthesis triggered

  ExpeditionState exp;
  exp.chapterIndex = 6;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleHorizon, exp, zg, 1, 500, 10, 60.0f);

  assert(!c.isTrueEnding);
  assert(!c.titleRu.empty());
  assert(!c.narrativeParagraphsRu.empty());
  assert(!c.crewFates.empty());
  assert(c.stats.coreDrillSamplesCount == 1);
  std::cout << "PASSED\n";
}

void testChronicleLogbookFormatting() {
  std::cout << "[Test 6] Chronicle Logbook ASCII formatting... ";
  ZettelGraph zg;
  zg.init();
  ExpeditionState exp;
  exp.chapterIndex = 6;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleSolarRebirth, exp, zg, 3, 1200, 12, 88.0f);

  std::string logRu = formatChronicleLogbook(c, true);
  std::string logEn = formatChronicleLogbook(c, false);

  assert(logRu.find("СВИТОК ХРОНИКИ ЭКСПЕДИЦИИ") != std::string::npos);
  assert(logRu.find("СТАТИСТИКА И ДОСТИЖЕНИЯ") != std::string::npos);
  assert(logRu.find("1200") != std::string::npos);

  assert(logEn.find("EXPEDITION CHRONICLE SCROLL") != std::string::npos);
  assert(logEn.find("EXPEDITION METRICS") != std::string::npos);
  assert(logEn.find("1200") != std::string::npos);
  std::cout << "PASSED\n";
}

void testAnchorStationResolution() {
  std::cout << "[Test 7] Anchor station resolution... ";
  assert(getAnchorStationForEnding(kFinaleSolarRebirth, true) == "sol");
  assert(getAnchorStationForEnding(kFinaleColony, true) == "hercules_b");
  assert(getAnchorStationForEnding(kFinaleLoopTranscendence, true) == "sgra");
  assert(getAnchorStationForEnding(kFinaleHyperBreakthrough, true) == "sgra");
  assert(getAnchorStationForEnding(kFinaleFifthAlliance, true) == "m13");
  assert(getAnchorStationForEnding(kFinaleHorizon, false) == "sgra");
  assert(getAnchorStationForEnding(kFinaleArkCitadel, false) == "sol");
  std::cout << "PASSED\n";
}

void testClampingAndBoundaries() {
  std::cout << "[Test 8] Clamping and boundaries... ";
  ZettelGraph zg;
  zg.init();
  ExpeditionState exp;

  EpilogueChronicle c = generateEpilogueChronicle(
      kFinaleSolarRebirth, exp, zg, -10, -500, -2, 150.0f);

  assert(c.stats.coreDrillSamplesCount == 0);
  assert(c.stats.researchPoints == 0);
  assert(c.stats.warpJumpsCount == 0);
  assert(c.stats.hullIntegrityPercent == 100.0f);
  assert(c.stats.discoveryPercent >= 10.0f && c.stats.discoveryPercent <= 100.0f);
  std::cout << "PASSED\n";
}

int main() {
  std::cout << "=== Running test_epilogue suite ===\n";
  testSolarRebirthTrueEnding();
  testEdenColonyTrueEnding();
  testMultiverseTrueEnding();
  testStasisAllianceTrueEnding();
  testFallbackNonTrueEnding();
  testChronicleLogbookFormatting();
  testAnchorStationResolution();
  testClampingAndBoundaries();
  std::cout << "=== All 8 test_epilogue checks passed successfully! ===\n";
  return 0;
}
