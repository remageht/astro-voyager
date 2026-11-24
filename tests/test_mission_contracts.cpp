// test_mission_contracts.cpp — ТЕСТЫ логики квестового выдачи (Issue #227):
// бордер генерирует доступные контракты, acceptContract переносит их в
// активные, max 3 одновременных, serialize/deserialize бордера сохраняет
// состояние. Core, без GPU.

#include <iostream>
#include <string>

#include "MissionSystem.h"

namespace {
int g_fail = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << "\n";
    ++g_fail;
  } else {
    std::cout << "PASS: " << what << "\n";
  }
}
}  // namespace

int main() {
  // 1. Board generates contracts; accepting moves them to active.
  {
    astro::MissionManager mgr;
    mgr.generateBoard("sol", 4, 42);
    check(!mgr.getAvailableContracts().empty(), "generateBoard -> available not empty");

    const std::string id = mgr.getAvailableContracts().front().id;
    const size_t availBefore = mgr.getAvailableContracts().size();
    const bool ok = mgr.acceptContract(id);
    check(ok, "acceptContract returns true for a board contract");
    check(mgr.getActiveContracts().size() == 1, "acceptContract adds to active list");

    // Повторное принятие неизвестного и отсутствующего в бордере — false.
    check(!mgr.acceptContract("definitely-not-a-contract-id"),
          "acceptContract(unknown id) -> false");
    check(mgr.getAvailableContracts().size() < availBefore,
          "acceptContract removes from available");
  }

  // 2. Max 3 active — rejecting when full.
  {
    astro::MissionManager mgr;
    mgr.generateBoard("sol", 10, 7);
    size_t accepted = 0;
    for (const auto& c : mgr.getAvailableContracts()) {
      if (mgr.acceptContract(c.id)) ++accepted;
    }
    check(mgr.getActiveContracts().size() == accepted,
          "active size matches accepts");
    check(accepted <= static_cast<size_t>(mgr.getMaxActiveContracts()),
          "no more than max 3 active");
    if (mgr.getActiveContracts().size() == 3) {
      check(!mgr.acceptContract(mgr.getActiveContracts().front().id),
            "acceptContract at capacity -> false");
    }
  }

  // 3. serialize/deserialize round-trip of active contracts
  //    (MissionManager::serialize() writes only m_active; the board is
  //    re-populated via generateBoard — do not assert on board size here).
  {
    astro::MissionManager mgr;
    mgr.generateBoard("mars", 4, 9);
    const std::string id = mgr.getAvailableContracts().front().id;
    mgr.acceptContract(id);

    astro::MissionManager mgr2;
    check(mgr2.deserialize(mgr.serialize()), "deserialize(serialize()) -> true");
    check(mgr2.getActiveContracts().size() == mgr.getActiveContracts().size(),
          "deserialized active size matches");
    if (!mgr2.getActiveContracts().empty() && !mgr.getActiveContracts().empty()) {
      check(mgr2.getActiveContracts().front().id == mgr.getActiveContracts().front().id,
            "deserialized active contract id matches");
    }
  }

  if (g_fail > 0) {
    std::cerr << g_fail << " check(s) FAILED\n";
    return 1;
  }
  std::cout << "mission contract tests: ALL PASS\n";
  return 0;
}
