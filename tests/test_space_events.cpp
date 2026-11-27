#include "../src/SpaceEventsSystem.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/ScienceSystem.h"
#include "../src/Zettelkasten.h"

int main() {
  astro::SpaceEventsManager mgr;
  astro::ScienceCodex codex;
  astro::BridgeComms comms;
  astro::ZettelGraph zettel;

  // 1. Deterministic sector seeding spawns 2 balanced emergent events.
  const int seeded = mgr.ensureSectorEvents("sol", 12345ULL);
  assert(seeded == 2);
  assert(mgr.activeCount("sol") == 2);
  assert(mgr.ensureSectorEvents("sol", 12345ULL) == 0);

  // 2. Manual radar ping spawns an additional encounter and enforces scanner cooldown.
  assert(mgr.pingSectorScanner("sol", 99ULL));
  assert(mgr.activeCount("sol") == 3);
  assert(std::abs(mgr.scannerCooldownRemaining() - astro::SpaceEventsManager::kScannerCooldownSec) < 1e-6);
  assert(!mgr.pingSectorScanner("sol", 101ULL));  // blocked by cooldown and cap

  // 3. Serialize / deserialize round-trip preserves exact state, cooldown, & counters.
  const std::string saved = mgr.serialize();
  astro::SpaceEventsManager loaded;
  assert(loaded.deserialize(saved));
  assert(loaded.activeCount("sol") == 3);
  assert(std::abs(loaded.scannerCooldownRemaining() - astro::SpaceEventsManager::kScannerCooldownSec) < 1e-3);
  assert(loaded.events()[0].id == mgr.events()[0].id);
  assert(std::abs(loaded.events()[0].pos.x - mgr.events()[0].pos.x) < 1e-6);

  // 4. Locate SOS, RogueAsteroid, and RelicProbe behaviors cleanly in fresh manager.
  mgr.clear();
  assert(std::abs(mgr.scannerCooldownRemaining()) < 1e-6);
  for (uint64_t s = 1; s <= 12; ++s) {
    mgr.pingSectorScanner("sgra", s * 7919ULL, true);
    if (mgr.activeCount("sgra") >= 3) break;
  }

  // Ensure every event type works end-to-end across sectors.
  astro::SpaceEventsManager sim;
  for (uint64_t k = 0; k < 24ULL; ++k) {
    sim.pingSectorScanner(k % 3 == 0 ? "sol" : (k % 3 == 1 ? "alpha" : "m87"),
                          k * 97ULL + 1ULL, true);
  }

  bool testedAst = false;
  bool testedProbe = false;
  bool testedSos = false;
  for (size_t i = 0; i < sim.events().size(); ++i) {
    const auto evCopy = sim.events()[i];
    if (evCopy.status != astro::SpaceEventStatus::Active) continue;

    // Out-of-range engagement must fail cleanly
    const astro::Vec3 farPos{evCopy.pos.x + 100.0, evCopy.pos.y, evCopy.pos.z};
    assert(!sim.engageEvent(evCopy.id, farPos, 1.0, &codex, &comms, &zettel));

    if (evCopy.type == astro::SpaceEventType::RogueAsteroid && !testedAst) {
      testedAst = true;
      const double oldVx = evCopy.vel.x;
      const double oldVz = evCopy.vel.z;
      const int rpBefore = codex.getTotalResearchPoints();
      // Even exact overhead ship alignment (X/Z matching asteroid) deflects cleanly
      assert(sim.engageEvent(evCopy.id, evCopy.pos, 1.5, &codex, &comms, &zettel));
      assert(sim.events()[i].vel.x != oldVx || sim.events()[i].vel.z != oldVz);
      assert(sim.engageEvent(evCopy.id, evCopy.pos, 1.5, &codex, &comms, &zettel));
      assert(sim.events()[i].status == astro::SpaceEventStatus::Resolved);
      assert(codex.getTotalResearchPoints() == rpBefore + evCopy.rewardRP);
    } else if (evCopy.type == astro::SpaceEventType::RelicProbe && !testedProbe) {
      testedProbe = true;
      const int zBefore = zettel.getTotalCount();
      assert(sim.engageEvent(evCopy.id, evCopy.pos, 3.0, &codex, &comms, &zettel));
      assert(sim.events()[i].status == astro::SpaceEventStatus::Resolved);
      assert(zettel.getTotalCount() == zBefore + 1);
    } else if (evCopy.type == astro::SpaceEventType::SosBeacon && !testedSos) {
      testedSos = true;
      assert(sim.engageEvent(evCopy.id, evCopy.pos, 3.0, &codex, &comms, &zettel));
      assert(sim.events()[i].status == astro::SpaceEventStatus::Resolved);
    }
  }
  assert(testedAst && testedProbe && testedSos);
  assert(sim.resolvedCount() == 3);

  // 5. Countdown expiration updates position & transitions Active -> Expired cleanly.
  astro::SpaceEventsManager expMgr;
  expMgr.ensureSectorEvents("trappist", 42ULL);
  const auto firstBefore = expMgr.events()[0];
  expMgr.update(0.2, "trappist", true);
  assert(expMgr.events()[0].timeRemainingSec < firstBefore.timeRemainingSec);
  for (int step = 0; step < 1500; ++step) {
    expMgr.update(0.25, "trappist", true);
  }
  assert(expMgr.activeCount("trappist") == 0);
  assert(expMgr.expiredCount() == 2);

  // 6. Quote-aware JSON deserialization handles '}' characters embedded in strings.
  const std::string crafted =
      "{\n  \"schema\": 1,\n  \"nextSeq\": 5,\n  \"events\": [\n"
      "    {\"id\":\"evt_custom_1\",\"type\":0,\"status\":0,\"stationId\":\"sol\","
      "\"titleRu\":\"Маяк {Beta}\",\"titleEn\":\"Beacon {Beta}\","
      "\"descRu\":\"Строка } со скобкой\",\"descEn\":\"Text with } brace\","
      "\"px\":1.5,\"py\":0.0,\"pz\":-2.5,\"vx\":0,\"vy\":0,\"vz\":0,"
      "\"tx\":0,\"ty\":0,\"tz\":0,\"radius\":2.0,\"progress\":0.5,\"req\":2.0,"
      "\"rem\":120,\"dur\":180,\"rp\":210,\"seed\":7}\n  ]\n}\n";
  astro::SpaceEventsManager braceMgr;
  assert(braceMgr.deserialize(crafted));
  assert(braceMgr.events().size() == 1);
  assert(braceMgr.events()[0].rewardRP == 210);
  assert(braceMgr.events()[0].descEn == "Text with } brace");

  // 7. Cooldown decay via update() and bounded archived history (kMaxArchivedHistory).
  astro::SpaceEventsManager cdMgr;
  assert(cdMgr.pingSectorScanner("sol", 1ULL));
  assert(!cdMgr.pingSectorScanner("sol", 2ULL));  // on 30s cooldown
  for (int i = 0; i < 40; ++i) cdMgr.update(0.25, "sol", true);  // -10s
  assert(std::abs(cdMgr.scannerCooldownRemaining() - 20.0) < 1e-4);
  assert(!cdMgr.pingSectorScanner("sol", 3ULL));  // still on cooldown
  for (int i = 0; i < 100; ++i) cdMgr.update(0.25, "sol", true);  // -25s -> clamped to 0
  assert(cdMgr.scannerCooldownRemaining() == 0.0);
  assert(cdMgr.pingSectorScanner("sol", 4ULL));  // cooldown expired, spawns 2nd event

  astro::SpaceEventsManager trimMgr;
  for (uint64_t k = 0; k < 30ULL; ++k) {
    assert(trimMgr.pingSectorScanner("alpha", 100ULL + k, true));
    const auto ev = trimMgr.events().back();
    while (trimMgr.events().back().status == astro::SpaceEventStatus::Active) {
      assert(trimMgr.engageEvent(ev.id, ev.pos, 2.0, &codex, &comms, &zettel));
    }
  }
  assert(trimMgr.resolvedCount() == 30);
  assert(trimMgr.totalEventsCount() <= static_cast<int>(astro::SpaceEventsManager::kMaxArchivedHistory));
  const std::string trimSaved = trimMgr.serialize();
  astro::SpaceEventsManager trimLoaded;
  assert(trimLoaded.deserialize(trimSaved));
  assert(trimLoaded.resolvedCount() == 30);

  std::cout << "test_space_events PASSED (resolved=" << sim.resolvedCount()
            << ", expired=" << expMgr.expiredCount() << ")\n";
  return 0;
}
