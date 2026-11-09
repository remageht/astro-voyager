#include "../src/MissionSystem.h"
#include "../src/ScienceSystem.h"
#include "../src/Zettelkasten.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

namespace {

void testBoardGeneration() {
  astro::MissionManager mgr;
  mgr.generateBoard("sol", 4, 42);
  const auto& available = mgr.getAvailableContracts();
  assert(available.size() == 4);

  for (const auto& c : available) {
    assert(!c.id.empty());
    assert(c.status == astro::MissionStatus::Available);
    assert(!c.titleRu.empty());
    assert(!c.titleEn.empty());
    assert(!c.clientRu.empty());
    assert(!c.clientEn.empty());
    assert(!c.descRu.empty());
    assert(!c.descEn.empty());
    assert(c.rewardRP >= 150 && c.rewardRP <= 700);
    assert(c.targetValue > 0.0f);
    (void)c;
  }
  std::cout << "[PASS] Mission board generation & contract fields\n";
}

void testContractAcceptAndLimits() {
  astro::MissionManager mgr;
  mgr.generateBoard("sol", 5, 100);
  const auto& avail = mgr.getAvailableContracts();
  assert(avail.size() >= 4);

  std::string id0 = avail[0].id;
  std::string id1 = avail[1].id;
  std::string id2 = avail[2].id;
  std::string id3 = avail[3].id;

  assert(mgr.acceptContract(id0));
  assert(mgr.acceptContract(id1));
  assert(mgr.acceptContract(id2));
  assert(mgr.getActiveContracts().size() == 3);

  // 4th contract should fail due to kMaxActive = 3
  assert(!mgr.acceptContract(id3));
  assert(mgr.getActiveContracts().size() == 3);

  // Abandon one and accept the 4th
  assert(mgr.abandonContract(id1));
  assert(mgr.getActiveContracts().size() == 2);
  assert(mgr.acceptContract(id3));
  assert(mgr.getActiveContracts().size() == 3);

  std::cout << "[PASS] Contract acceptance and concurrency limits\n";
}

void testSpectroscopySurveyLifecycle() {
  astro::MissionManager mgr;
  // Artificially insert a known SpectralSurvey contract
  astro::MissionContract c;
  c.id = "test_spec_sn1987a";
  c.type = astro::MissionType::SpectralSurvey;
  c.status = astro::MissionStatus::Active;
  c.targetStationId = "sn1987a";
  c.targetValue = 1.0f;
  c.rewardRP = 320;
  c.clientRu = "Земной Научный Директорат";
  c.clientEn = "Earth Scientific Directorate";
  mgr.getActiveContractsMut().push_back(c);

  assert(mgr.getCompletedUnclaimedCount() == 0);

  // Irrelevant scan should not complete
  assert(!mgr.onSpectroscopyScan("mars"));
  assert(mgr.getCompletedUnclaimedCount() == 0);

  // Correct scan triggers completion
  assert(mgr.onSpectroscopyScan("sn1987a"));
  assert(mgr.getCompletedUnclaimedCount() == 1);
  assert(mgr.getActiveContracts()[0].status == astro::MissionStatus::Completed);

  // Claim reward
  astro::ScienceCodex codex;
  astro::BridgeComms comms;
  assert(codex.getTotalResearchPoints() == 0);

  assert(mgr.claimReward("test_spec_sn1987a", codex, &comms));
  assert(codex.getTotalResearchPoints() == 320);
  assert(comms.hasActiveMessage());
  assert(mgr.getActiveContracts().empty());

  std::cout << "[PASS] Spectral survey contract lifecycle & RP reward\n";
}

void testSurfaceSamplingLifecycle() {
  astro::MissionManager mgr;
  astro::MissionContract c;
  c.id = "test_drill_titan";
  c.type = astro::MissionType::SurfaceSampling;
  c.status = astro::MissionStatus::Active;
  c.targetStationId = "titan";
  c.targetValue = 35.0f; // 35 cm depth
  c.rewardRP = 280;
  mgr.getActiveContractsMut().push_back(c);

  // Drill on wrong planet
  assert(!mgr.onSurfaceSample("moon", 50.0f));
  assert(mgr.getActiveContracts()[0].currentValue == 0.0f);

  // Partial depth on Titan
  assert(!mgr.onSurfaceSample("titan", 20.0f));
  assert(mgr.getActiveContracts()[0].currentValue == 20.0f);
  assert(mgr.getActiveContracts()[0].status == astro::MissionStatus::Active);

  // Full depth reaches target
  assert(mgr.onSurfaceSample("titan", 40.0f));
  assert(mgr.getActiveContracts()[0].currentValue == 40.0f);
  assert(mgr.getActiveContracts()[0].status == astro::MissionStatus::Completed);

  astro::ScienceCodex codex;
  assert(mgr.claimReward("test_drill_titan", codex));
  assert(codex.getTotalResearchPoints() == 280);

  std::cout << "[PASS] Surface core sampling lifecycle\n";
}

void testGravitationalProbeAndDeepSpace() {
  astro::MissionManager mgr;
  astro::MissionContract cGrav;
  cGrav.id = "test_grav_sgra";
  cGrav.type = astro::MissionType::GravitationalProbe;
  cGrav.status = astro::MissionStatus::Active;
  cGrav.targetStationId = "sgra";
  cGrav.targetValue = 3.5f; // r <= 3.5 Rs
  cGrav.rewardRP = 500;
  mgr.getActiveContractsMut().push_back(cGrav);

  astro::MissionContract cDeep;
  cDeep.id = "test_deep_probe";
  cDeep.type = astro::MissionType::DeepSpaceProbe;
  cDeep.status = astro::MissionStatus::Active;
  cDeep.targetValue = 45.0f; // Dist >= 45u
  cDeep.rewardRP = 350;
  mgr.getActiveContractsMut().push_back(cDeep);

  // Test gravitational probe: r = 5.0 is too far
  assert(!mgr.onProximityObservation("sgra", 5.0));
  // r = 2.8 Rs triggers success
  assert(mgr.onProximityObservation("sgra", 2.8));
  assert(mgr.getActiveContracts()[0].status == astro::MissionStatus::Completed);

  // Test deep space probe: 30u is too close
  assert(!mgr.onDeepSpacePosition(30.0));
  // 52u triggers success
  assert(mgr.onDeepSpacePosition(52.0));
  assert(mgr.getActiveContracts()[1].status == astro::MissionStatus::Completed);

  std::cout << "[PASS] Gravitational probe and deep space telemetry\n";
}

void testSerializationRoundTrip() {
  astro::MissionManager mgr;
  astro::MissionContract c;
  c.id = "cnt_persisted_01";
  c.type = astro::MissionType::CargoExpress;
  c.status = astro::MissionStatus::Active;
  c.titleRu = "Срочный рейс Sol -> M13";
  c.titleEn = "Priority Transit Sol -> M13";
  c.clientRu = "Служба Снабжения";
  c.clientEn = "Logistics Bureau";
  c.targetStationId = "sol";
  c.destinationStationId = "m13";
  c.targetValue = 1.0f;
  c.currentValue = 0.5f;
  c.rewardRP = 450;
  c.timeLimitSec = 180.0;
  c.timeRemainingSec = 120.0;
  mgr.getActiveContractsMut().push_back(c);

  std::string json = mgr.serialize();
  assert(!json.empty());
  assert(json.find("cnt_persisted_01") != std::string::npos);

  astro::MissionManager restored;
  assert(restored.deserialize(json));
  assert(restored.getActiveContracts().size() == 1);
  const auto& rc = restored.getActiveContracts()[0];
  assert(rc.id == "cnt_persisted_01");
  assert(rc.type == astro::MissionType::CargoExpress);
  assert(rc.status == astro::MissionStatus::Active);
  assert(rc.rewardRP == 450);
  assert(rc.targetStationId == "sol");
  assert(rc.destinationStationId == "m13");
  assert(std::fabs(rc.timeRemainingSec - 120.0) < 0.1);
  (void)rc;

  std::cout << "[PASS] Mission state serialization & round-trip\n";
}

}  // namespace

int main() {
  std::cout << "=== Running Astro-Voyager Mission System Tests ===\n";
  testBoardGeneration();
  testContractAcceptAndLimits();
  testSpectroscopySurveyLifecycle();
  testSurfaceSamplingLifecycle();
  testGravitationalProbeAndDeepSpace();
  testSerializationRoundTrip();
  std::cout << "mission tests: ALL PASS\n";
  return 0;
}
