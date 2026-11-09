#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace astro {

class ScienceCodex;
class BridgeComms;
class ZettelGraph;

// Archetypes of procedural scientific and flight directives
enum class MissionType {
  SpectralSurvey = 0,     // Spectroscopy scan / anomaly analysis
  SurfaceSampling = 1,    // Planetary landing & core drill sampling
  GravitationalProbe = 2, // Extreme proximity probe near black hole (r <= 4.0 Rs)
  DeepSpaceProbe = 3,     // Deep space sensor telemetry (> 45.0u from origin)
  CargoExpress = 4        // Timed relativistic courier delivery between stations
};

// Lifecycle state of a contract
enum class MissionStatus {
  Available = 0,  // Posted on orbital station terminal
  Active = 1,     // Accepted into ship's mission log (max 3 concurrent)
  Completed = 2,  // Objectives fulfilled, reward ready to claim
  Claimed = 3,    // Reward collected (RP awarded, radio msg sent)
  Expired = 4     // Timer elapsed or contract abandoned
};

// Procedural contract definition
struct MissionContract {
  std::string id;
  MissionType type = MissionType::SpectralSurvey;
  MissionStatus status = MissionStatus::Available;

  std::string titleRu;
  std::string titleEn;
  std::string clientRu;
  std::string clientEn;
  std::string descRu;
  std::string descEn;

  std::string targetStationId;
  std::string targetStationName;
  std::string destinationStationId;
  std::string destinationStationName;

  float targetValue = 0.0f;     // e.g. required core depth (cm), observation hold (s), or distance (u)
  float currentValue = 0.0f;   // Current progress toward targetValue
  int rewardRP = 200;           // Research Points reward
  std::string rewardHypothesisId; // Optional Zettelkasten hypothesis id to unlock

  double timeLimitSec = 0.0;    // 0.0 = untimed open contract
  double timeRemainingSec = 0.0;
  uint32_t seed = 0;

  bool isFinished() const {
    return status == MissionStatus::Completed || status == MissionStatus::Claimed;
  }
};

// Manages the procedural mission board and active contract tracking
class MissionManager {
 public:
  MissionManager();

  // Procedurally populates available contracts for the docked station
  void generateBoard(const std::string& currentStationId, int count = 4, uint32_t seed = 0);

  const std::vector<MissionContract>& getAvailableContracts() const { return m_available; }
  const std::vector<MissionContract>& getActiveContracts() const { return m_active; }
  std::vector<MissionContract>& getActiveContractsMut() { return m_active; }

  // Contract management (max 3 concurrent active contracts)
  bool acceptContract(const std::string& contractId);
  bool abandonContract(const std::string& contractId);

  // Time and proximity update loop
  void update(double dt, const std::string& currentStationId, double shipDistToTarget,
              bool isDocked, float shipSpeed);

  // Event hooks triggered by flight actions
  bool onSpectroscopyScan(const std::string& stationId);
  bool onSurfaceSample(const std::string& bodyId, float depthCm);
  bool onProximityObservation(const std::string& stationId, double distRs);
  bool onDeepSpacePosition(double distFromOrigin);
  bool onStationDocked(const std::string& stationId);

  // Claim reward for a completed contract
  bool claimReward(const std::string& contractId, ScienceCodex& codex,
                   BridgeComms* comms = nullptr, ZettelGraph* zettel = nullptr);

  // Query helpers
  int getCompletedUnclaimedCount() const;
  const MissionContract* getPrimaryActiveContract() const;
  int getMaxActiveContracts() const { return kMaxActive; }

  // Serialization to/from JSON
  std::string serialize() const;
  bool deserialize(const std::string& json);

 private:
  static constexpr int kMaxActive = 3;
  std::vector<MissionContract> m_available;
  std::vector<MissionContract> m_active;
  uint32_t m_lastSeed = 1337;
  std::string m_lastStationId;
};

}  // namespace astro
