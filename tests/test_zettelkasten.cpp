#include <cassert>
#include <cmath>
#include <iostream>
#include "Zettelkasten.h"

using namespace astro;

static void test_init() {
  ZettelGraph zg;
  assert(zg.getTotalCount() == 13);
  assert(zg.getDiscoveredCount() == 1);
  assert(zg.getSynthesizedCount() == 0);
  assert(zg.getTotalLinksCount() == 0);

  const ZettelNote* paris = zg.getNote("z_paris_rain");
  assert(paris != nullptr);
  (void)paris;
  assert(paris->seedDiscovered == true);
  assert(paris->cluster == kClusterSolar);
  assert(!paris->titleRu.empty());
  assert(!paris->quoteRu.empty());

  const ZettelNote* casimir = zg.getNote("z_casimir_metric");
  assert(casimir != nullptr);
  (void)casimir;
  assert(casimir->seedDiscovered == false);

  const ZettelNote* voyager = zg.getNote("z_voyager_golden_record");
  assert(voyager != nullptr);
  (void)voyager;
  assert(voyager->seedDiscovered == false);
  assert(voyager->cluster == kClusterEden);

  std::cout << "PASS: test_init\n";
}

static void test_refresh_discovery() {
  ZettelGraph zg;
  std::vector<std::string> visited = {"sol", "dock-dragon"};
  zg.refresh(visited, 1e9, false);

  assert(zg.getNote("z_paris_rain")->seedDiscovered);
  assert(zg.getNote("z_casimir_metric")->seedDiscovered);
  assert(zg.getNote("z_mia_promise")->seedDiscovered);
  assert(zg.getNote("z_voyager_golden_record")->seedDiscovered);
  assert(zg.getNote("z_stasis_dampener")->seedDiscovered);

  // Black hole visit
  visited.push_back("sgra");
  zg.refresh(visited, 12.0, false);
  assert(zg.getNote("z_universe5_collapse")->seedDiscovered);
  assert(zg.getNote("z_sgra_tidal")->seedDiscovered);
  assert(zg.getNote("z_loop_transcend")->seedDiscovered);

  // M13 visit
  visited.push_back("m13");
  zg.refresh(visited, 12.0, false);
  assert(zg.getNote("z_m13_biosphere")->seedDiscovered);
  assert(zg.getNote("z_nomad_citadel")->seedDiscovered);

  // Relativistic quasar visit
  visited.push_back("qso-3c273");
  zg.refresh(visited, 12.0, false);
  assert(zg.getNote("z_solar_injection")->seedDiscovered);
  assert(zg.getNote("z_sixth_resonance")->seedDiscovered);
  assert(zg.getNote("z_phase_shard")->seedDiscovered);

  assert(zg.getDiscoveredCount() == 13);
  std::cout << "PASS: test_refresh_discovery\n";
}

static void test_linking_and_synthesis() {
  ZettelGraph zg;
  // Initially casimir is undiscovered, so linking should fail
  bool failedLink = zg.linkNotes("z_paris_rain", "z_casimir_metric");
  assert(!failedLink);
  (void)failedLink;

  zg.discoverSeed("z_casimir_metric");
  bool okLink = zg.linkNotes("z_paris_rain", "z_casimir_metric");
  assert(okLink);
  (void)okLink;
  assert(zg.areLinked("z_paris_rain", "z_casimir_metric"));
  assert(zg.areLinked("z_casimir_metric", "z_paris_rain"));
  assert(zg.getTotalLinksCount() == 1);

  // Both should now be synthesized
  assert(zg.getNote("z_paris_rain")->synthesized);
  assert(zg.getNote("z_casimir_metric")->synthesized);

  // Unlink
  zg.unlinkNotes("z_paris_rain", "z_casimir_metric");
  assert(!zg.areLinked("z_paris_rain", "z_casimir_metric"));
  assert(zg.getTotalLinksCount() == 0);

  std::cout << "PASS: test_linking_and_synthesis\n";
}

static void test_cluster_progress_and_true_endings() {
  ZettelGraph zg;
  // All solar notes
  zg.discoverSeed("z_paris_rain");
  zg.discoverSeed("z_casimir_metric");
  zg.discoverSeed("z_solar_injection");

  float p0 = zg.getClusterProgress(kClusterSolar);
  assert(p0 > 0.0f && p0 < 0.5f);
  (void)p0;
  assert(!zg.isClusterTrueEndingUnlocked(kClusterSolar));

  // Synthesize and link
  zg.linkNotes("z_paris_rain", "z_casimir_metric");
  zg.linkNotes("z_casimir_metric", "z_solar_injection");

  float p1 = zg.getClusterProgress(kClusterSolar);
  assert(std::fabs(p1 - 1.0f) < 0.001f);
  (void)p1;
  assert(zg.isClusterTrueEndingUnlocked(kClusterSolar));

  std::string epilogue = ZettelGraph::getTrueEndingEpilogueRu(kClusterSolar);
  assert(!epilogue.empty());
  assert(epilogue.find("ИСТИННЫЙ ФИНАЛ") != std::string::npos);

  // Check Stasis cluster
  zg.discoverSeed("z_stasis_dampener");
  zg.discoverSeed("z_universe5_collapse");
  zg.discoverSeed("z_sgra_tidal");
  zg.linkNotes("z_stasis_dampener", "z_universe5_collapse");
  zg.linkNotes("z_universe5_collapse", "z_sgra_tidal");
  assert(zg.isClusterTrueEndingUnlocked(kClusterStasis));

  std::cout << "PASS: test_cluster_progress_and_true_endings\n";
}

static void test_serialization_roundtrip() {
  ZettelGraph zg1;
  zg1.discoverSeed("z_paris_rain");
  zg1.discoverSeed("z_casimir_metric");
  zg1.linkNotes("z_paris_rain", "z_casimir_metric");

  ZettelNote* n = zg1.getNote("z_paris_rain");
  assert(n != nullptr);
  n->posX = 42.5f;
  n->posY = -100.25f;

  std::string data = zg1.serialize();
  assert(!data.empty());

  ZettelGraph zg2;
  bool ok = zg2.deserialize(data);
  assert(ok);
  (void)ok;
  assert(zg2.areLinked("z_paris_rain", "z_casimir_metric"));

  const ZettelNote* n2 = zg2.getNote("z_paris_rain");
  assert(n2 != nullptr);
  (void)n2;
  assert(n2->seedDiscovered);
  assert(n2->synthesized);
  assert(std::fabs(n2->posX - 42.5f) < 0.01f);
  assert(std::fabs(n2->posY - (-100.25f)) < 0.01f);

  std::cout << "PASS: test_serialization_roundtrip\n";
}

int main() {
  test_init();
  test_refresh_discovery();
  test_linking_and_synthesis();
  test_cluster_progress_and_true_endings();
  test_serialization_roundtrip();
  std::cout << "\nAll 5 Zettelkasten tests passed successfully.\n";
  return 0;
}
