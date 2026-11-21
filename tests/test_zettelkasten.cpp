#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "Zettelkasten.h"

// assert() выключен в Release (/DNDEBUG): классифицированный архив проверяется
// макросом, который валит процесс в любой конфигурации.
#define REQUIRE(cond)                                                          \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "[FAIL] " << #cond << " at " << __FILE__ << ":" << __LINE__ \
                << "\n";                                                       \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

using namespace astro;

static void test_init() {
  ZettelGraph zg;
  assert(zg.getTotalCount() == 17);
  assert(zg.getDiscoveredCount() == 1);
  assert(zg.getSynthesizedCount() == 0);
  assert(zg.getMasterNodesCount() == 0);
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

  const ZettelNote* masterSolar = zg.getNote("z_master_solar_dawn");
  assert(masterSolar != nullptr);
  (void)masterSolar;
  assert(masterSolar->isMasterNode == true);
  assert(masterSolar->seedDiscovered == false);
  assert(masterSolar->parentIds.size() == 3);

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

static void test_synthesis_recipes() {
  ZettelGraph zg;
  zg.discoverSeed("z_paris_rain");
  zg.discoverSeed("z_casimir_metric");
  zg.discoverSeed("z_solar_injection");

  assert(!zg.canSynthesizeRecipe("recipe_solar_dawn"));

  // Link notes into connected chain
  zg.linkNotes("z_paris_rain", "z_casimir_metric");
  zg.linkNotes("z_casimir_metric", "z_solar_injection");

  assert(zg.canSynthesizeRecipe("recipe_solar_dawn"));
  const ZettelSynthesisRecipe* pending = zg.checkPendingSynthesis();
  assert(pending != nullptr);
  assert(pending->id == "recipe_solar_dawn");
  (void)pending;

  // Trigger synthesis
  bool ok = zg.triggerSynthesisRecipe("recipe_solar_dawn");
  assert(ok);
  (void)ok;

  // Master node should now be discovered, synthesized, and in foreground (depthLevel == 0)
  const ZettelNote* master = zg.getNote("z_master_solar_dawn");
  assert(master != nullptr);
  assert(master->seedDiscovered);
  assert(master->synthesized);
  assert(master->depthLevel == 0);
  assert(master->isMasterNode);
  assert(zg.getMasterNodesCount() == 1);
  (void)master;

  // Component notes should be demoted to background layer (depthLevel == 1)
  assert(zg.getNote("z_paris_rain")->depthLevel == 1);
  assert(zg.getNote("z_casimir_metric")->depthLevel == 1);
  assert(zg.getNote("z_solar_injection")->depthLevel == 1);

  // Master note should be linked to all 3 parents
  assert(zg.areLinked("z_master_solar_dawn", "z_paris_rain"));
  assert(zg.areLinked("z_master_solar_dawn", "z_casimir_metric"));
  assert(zg.areLinked("z_master_solar_dawn", "z_solar_injection"));

  // Cluster progress should be 1.0f (100% resonance)
  assert(zg.getClusterProgress(kClusterSolar) >= 0.999f);
  assert(zg.isClusterTrueEndingUnlocked(kClusterSolar));

  std::cout << "PASS: test_synthesis_recipes\n";
}

static void test_master_layering_and_serialization() {
  ZettelGraph zg1;
  zg1.discoverSeed("z_stasis_dampener");
  zg1.discoverSeed("z_universe5_collapse");
  zg1.discoverSeed("z_sgra_tidal");
  zg1.linkNotes("z_stasis_dampener", "z_universe5_collapse");
  zg1.linkNotes("z_universe5_collapse", "z_sgra_tidal");
  zg1.triggerSynthesisRecipe("recipe_stasis_bastion");

  std::string data = zg1.serialize();
  assert(!data.empty());
  assert(data.find("recipe=recipe_stasis_bastion|synth=1") != std::string::npos);
  assert(data.find("depth=1") != std::string::npos);

  ZettelGraph zg2;
  bool ok = zg2.deserialize(data);
  assert(ok);
  (void)ok;

  const ZettelNote* m2 = zg2.getNote("z_master_stasis_bastion");
  assert(m2 != nullptr);
  assert(m2->seedDiscovered);
  assert(m2->synthesized);
  assert(zg2.getNote("z_stasis_dampener")->depthLevel == 1);
  assert(zg2.areLinked("z_master_stasis_bastion", "z_sgra_tidal"));
  (void)m2;

  std::cout << "PASS: test_master_layering_and_serialization\n";
}

static void test_custom_notes_and_grand_synthesis() {
  ZettelGraph zg;
  assert(!zg.isAllMasterNodesSynthesized());

  // 1. Add custom note
  bool added = zg.addCustomNote("Заметка пилота", "Проверить [[sol|Солнце]] и метрику", "Гипотеза", 10.0f, 20.0f);
  assert(added);
  (void)added;
  assert(zg.getTotalCount() == 18);

  const ZettelNote* custom = nullptr;
  for (const auto& n : zg.getNotes()) {
    if (n.isCustom) {
      custom = &n;
      break;
    }
  }
  assert(custom != nullptr);
  assert(custom->isCustom);
  assert(custom->titleRu == "Заметка пилота");
  assert(custom->textRu == "Проверить [[sol|Солнце]] и метрику");
  assert(custom->categoryRu == "Гипотеза");
  assert(custom->seedDiscovered);
  assert(custom->synthesized);
  std::string customId = custom->id;

  // 2. Link custom note with Paris Rain
  assert(zg.linkNotes(customId, "z_paris_rain"));
  assert(zg.areLinked(customId, "z_paris_rain"));

  // 3. Serialize & Deserialize
  std::string s = zg.serialize();
  assert(s.find("custom_note=" + customId) != std::string::npos);

  ZettelGraph zg2;
  bool ok = zg2.deserialize(s);
  assert(ok);
  (void)ok;
  const ZettelNote* c2 = zg2.getNote(customId);
  assert(c2 != nullptr);
  (void)c2;
  assert(c2->isCustom);
  assert(c2->titleRu == "Заметка пилота");
  assert(c2->textRu == "Проверить [[sol|Солнце]] и метрику");
  assert(zg2.areLinked(customId, "z_paris_rain"));

  // 4. Delete custom note
  assert(zg.deleteCustomNote(customId));
  assert(zg.getNote(customId) == nullptr);
  assert(zg.getTotalCount() == 17);
  assert(!zg.areLinked(customId, "z_paris_rain"));

  // 5. Test Grand Synthesis
  ZettelGraph zgGrand;
  assert(!zgGrand.isAllMasterNodesSynthesized());
  zgGrand.synthesizeNote("z_master_solar_dawn");
  zgGrand.synthesizeNote("z_master_stasis_bastion");
  zgGrand.synthesizeNote("z_master_eden_cradle");
  assert(!zgGrand.isAllMasterNodesSynthesized());
  zgGrand.synthesizeNote("z_master_omni_nexus");
  assert(zgGrand.isAllMasterNodesSynthesized());

  std::cout << "PASS: test_custom_notes_and_grand_synthesis\n";
}

static void test_backlinks_and_hypotheses() {
  ZettelGraph zg;
  // Link two seeds (both must be discovered)
  zg.discoverSeed("z_paris_rain");
  zg.discoverSeed("z_casimir_metric");
  assert(zg.linkNotes("z_paris_rain", "z_casimir_metric"));
  auto in = zg.getIncomingLinks("z_paris_rain");
  assert(in.size() >= 1);
  assert(zg.getNodeDegree("z_paris_rain") >= 1);
  auto unlinked = zg.getUnlinkedMentions("z_paris_rain");
  (void)unlinked;
  // Hypotheses seeded
  assert(!zg.getHypotheses().empty());
  const auto& hyps = zg.getHypotheses();
  bool found = false;
  for (const auto& h : hyps) {
    if (h.id == "hyp_casimir_shield") { found = true; break; }
  }
  assert(found);
  assert(zg.resolveHypothesis("hyp_casimir_shield"));
  assert(!zg.resolveHypothesis("hyp_casimir_shield"));  // already resolved
  zg.checkHypothesesProgression("venus", true, 100);
  std::cout << "PASS: test_backlinks_and_hypotheses\n";
}

static void test_classified_hypotheses() {
  ZettelGraph zg;
  // Два досье под грифом, и они под грифом, пока архив не открыт.
  REQUIRE(zg.hiddenHypothesisCount() == 2);
  bool sawTachyon = false, sawMetric = false;
  for (const auto& h : zg.getHypotheses()) {
    if (h.id == kHypothesisRelicTachyon) {
      sawTachyon = true;
      REQUIRE(h.hidden);
    }
    if (h.id == kHypothesisPrecursorMetric) {
      sawMetric = true;
      REQUIRE(h.hidden);
    }
  }
  REQUIRE(sawTachyon && sawMetric);

  const size_t notesBefore = zg.getNotes().size();
  REQUIRE(zg.revealHiddenHypotheses() == 2);
  REQUIRE(zg.hiddenHypothesisCount() == 0);
  REQUIRE(zg.revealHiddenHypotheses() == 0);  // повторное открытие ничего не делает
  REQUIRE(zg.getNotes().size() == notesBefore);  // снятие грифа - не новая заметка

  REQUIRE(zg.resolveHypothesis(kHypothesisRelicTachyon));
  REQUIRE(!zg.resolveHypothesis(kHypothesisRelicTachyon));

  // Элитный контракт отдаёт файл напрямую: гриф падает вместе с наградой.
  ZettelGraph fresh;
  REQUIRE(fresh.hiddenHypothesisCount() == 2);
  REQUIRE(fresh.resolveHypothesis(kHypothesisPrecursorMetric));
  REQUIRE(fresh.hiddenHypothesisCount() == 1);
  std::cout << "PASS: test_classified_hypotheses\n";
}

static void test_detective_investigation_cases() {
  ZettelGraph zg;
  REQUIRE(zg.getDetectiveCases().size() == 2);
  REQUIRE(!zg.isDetectiveCaseSolved(kDetectiveCaseArsenyShuttles));
  REQUIRE(!zg.isDetectiveCaseSolved(kDetectiveCaseMiaContraband));

  // Check physical clues and analytical discrepancies in notes
  const ZettelNote* damp = zg.getNote("z_stasis_dampener");
  REQUIRE(damp != nullptr);
  REQUIRE(!damp->evidenceRu.empty());
  REQUIRE(!damp->discrepancyRu.empty());
  REQUIRE(damp->evidenceRu.find("Мянь Лун") != std::string::npos);

  const ZettelNote* u5 = zg.getNote("z_universe5_collapse");
  REQUIRE(u5 != nullptr);
  REQUIRE(!u5->evidenceRu.empty());
  REQUIRE(!u5->discrepancyRu.empty());
  REQUIRE(u5->evidenceRu.find("14 дискретных импульсов") != std::string::npos);

  const ZettelNote* mia = zg.getNote("z_mia_promise");
  REQUIRE(mia != nullptr);
  REQUIRE(!mia->evidenceRu.empty());
  REQUIRE(!mia->discrepancyRu.empty());
  REQUIRE(mia->evidenceRu.find("404-B") != std::string::npos);

  const ZettelNote* m13 = zg.getNote("z_m13_biosphere");
  REQUIRE(m13 != nullptr);
  REQUIRE(!m13->evidenceRu.empty());
  REQUIRE(!m13->discrepancyRu.empty());
  REQUIRE(m13->discrepancyRu.find("не вошедших в списки") != std::string::npos);

  // Discover and link Arseny's case
  zg.discoverSeed("z_stasis_dampener");
  zg.discoverSeed("z_universe5_collapse");
  REQUIRE(zg.linkNotes("z_stasis_dampener", "z_universe5_collapse"));
  REQUIRE(zg.isDetectiveCaseSolved(kDetectiveCaseArsenyShuttles));

  auto evs1 = zg.pollNewDetectiveEvents();
  REQUIRE(evs1.size() == 1);
  REQUIRE(evs1[0] == kDetectiveCaseArsenyShuttles);
  REQUIRE(zg.pollNewDetectiveEvents().empty());

  // Discover and link Mia's case
  zg.discoverSeed("z_mia_promise");
  zg.discoverSeed("z_m13_biosphere");
  REQUIRE(zg.linkNotes("z_mia_promise", "z_m13_biosphere"));
  REQUIRE(zg.isDetectiveCaseSolved(kDetectiveCaseMiaContraband));

  auto evs2 = zg.pollNewDetectiveEvents();
  REQUIRE(evs2.size() == 1);
  REQUIRE(evs2[0] == kDetectiveCaseMiaContraband);
  REQUIRE(zg.pollNewDetectiveEvents().empty());

  // Serialization & deserialization roundtrip
  std::string saved = zg.serialize();
  REQUIRE(saved.find("detective_case=case_arseny_shuttles|solved=1") != std::string::npos);
  REQUIRE(saved.find("detective_case=case_mia_contraband|solved=1") != std::string::npos);

  ZettelGraph roundtrip;
  REQUIRE(!roundtrip.isDetectiveCaseSolved(kDetectiveCaseArsenyShuttles));
  REQUIRE(!roundtrip.isDetectiveCaseSolved(kDetectiveCaseMiaContraband));
  REQUIRE(roundtrip.deserialize(saved));
  REQUIRE(roundtrip.isDetectiveCaseSolved(kDetectiveCaseArsenyShuttles));
  REQUIRE(roundtrip.isDetectiveCaseSolved(kDetectiveCaseMiaContraband));

  std::cout << "PASS: test_detective_investigation_cases\n";
}

int main() {
  test_init();
  test_refresh_discovery();
  test_linking_and_synthesis();
  test_cluster_progress_and_true_endings();
  test_serialization_roundtrip();
  test_synthesis_recipes();
  test_master_layering_and_serialization();
  test_custom_notes_and_grand_synthesis();
  test_backlinks_and_hypotheses();
  test_classified_hypotheses();
  test_detective_investigation_cases();
  std::cout << "\nAll 11 Zettelkasten tests passed successfully.\n";
  return 0;
}
