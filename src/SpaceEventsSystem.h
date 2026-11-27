#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Types.h"

namespace astro {

class ScienceCodex;
class BridgeComms;
class ZettelGraph;

// === Deep Space Emergent Events === (v5.2.0)
// Dynamic sector encounters: SOS distress beacons, rogue asteroid intercept/deflection,
// and drifting ancient relic probes. Pure C++17 (no GL/ImGui dependencies).

enum class SpaceEventType {
  SosBeacon = 0,
  RogueAsteroid = 1,
  RelicProbe = 2
};

enum class SpaceEventStatus {
  Active = 0,
  Resolved = 1,
  Expired = 2
};

struct DeepSpaceEvent {
  std::string id;
  SpaceEventType type = SpaceEventType::SosBeacon;
  SpaceEventStatus status = SpaceEventStatus::Active;
  std::string stationId;
  std::string titleRu;
  std::string titleEn;
  std::string descRu;
  std::string descEn;
  Vec3 pos{0.0, 0.0, 0.0};
  Vec3 vel{0.0, 0.0, 0.0};
  Vec3 targetPos{0.0, 0.0, 0.0};
  double interactRadius = 2.0;
  double progress = 0.0;
  double requiredProgress = 2.0;
  double timeRemainingSec = 180.0;
  double totalDurationSec = 180.0;
  int rewardRP = 160;
  float integrityPct = 100.0f;
  uint32_t seed = 1u;
};

class SpaceEventsManager {
 public:
  static constexpr double kScannerCooldownSec = 30.0;
  static constexpr size_t kMaxArchivedHistory = 24;

  // Advance active event kinematics (rogue asteroids / drifting probes) and countdown timers.
  void update(double dt, const std::string& stationId, bool activeGameplay);

  // Deterministically populates initial emergent encounters for a sector if none exist.
  // Returns the number of newly spawned events.
  int ensureSectorEvents(const std::string& stationId, uint64_t seed);

  // Trigger a deep-space radar ping to spawn an emergent anomaly in the current sector.
  // Enforces kScannerCooldownSec unless ignoreCooldown is passed (used in seeded tests).
  bool pingSectorScanner(const std::string& stationId,
                         uint64_t seed,
                         bool ignoreCooldown = false);
  double scannerCooldownRemaining() const { return m_scannerCooldownSec; }

  // Find the nearest active event in stationId (optionally returning Euclidean distance).
  const DeepSpaceEvent* findNearestActive(const Vec3& shipPos,
                                          const std::string& stationId,
                                          double* outDist = nullptr) const;

  // Apply ship proximity action (stabilize SOS, fire gravitational deflector on rogue asteroid,
  // or decode telemetry from a relic probe). Returns true if engagement succeeded or resolved.
  bool engageEvent(const std::string& eventId,
                   const Vec3& shipPos,
                   double dt,
                   ScienceCodex* codex = nullptr,
                   BridgeComms* comms = nullptr,
                   ZettelGraph* zettel = nullptr);

  int activeCount(const std::string& stationId = "") const;
  int resolvedCount() const { return m_resolvedTotal; }
  int expiredCount() const { return m_expiredTotal; }
  int totalEventsCount() const { return static_cast<int>(m_events.size()); }
  const std::vector<DeepSpaceEvent>& events() const { return m_events; }

  std::string serialize() const;
  bool deserialize(const std::string& data);
  void clear();

 private:
  DeepSpaceEvent createEventForType(SpaceEventType type,
                                    const std::string& stationId,
                                    uint64_t seed);
  void rewardEvent(DeepSpaceEvent& ev,
                   ScienceCodex* codex,
                   BridgeComms* comms,
                   ZettelGraph* zettel);
  void trimHistory();

  std::vector<DeepSpaceEvent> m_events;
  uint64_t m_nextSeq = 1;
  double m_scannerCooldownSec = 0.0;
  int m_resolvedTotal = 0;
  int m_expiredTotal = 0;
};

}  // namespace astro
