#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

namespace astro {

constexpr int kColonyStageCount = 24;
using ColonyStages = std::map<std::string, int>;

enum class ColonyEpoch : int {
  Outpost = 0,     // Stages 1-5: Аванпост и Жизнеобеспечение
  Industry = 1,    // Stages 6-10: Промышленность и Ресурсы
  Society = 2,     // Stages 11-15: Социум и Исследования
  HighTech = 3,    // Stages 16-20: Высокие Технологии
  Civilization = 4 // Stages 21-24: Планетарная Цивилизация
};

struct WorldPerk {
  const char* titleRu;
  const char* titleEn;
  const char* descriptionRu;
  const char* descriptionEn;
  float warpRechargeBonus = 0.0f;   // Ускорение перезарядки варпа
  float shieldBonus = 0.0f;          // Усиление щита
  float costDiscount = 0.0f;         // Скидка на постройки
  float scienceMultiplier = 0.0f;    // Бонус к науке
  float passiveRpRate = 0.0f;        // Модификатор пассивных очков
};

struct ColonyWorld {
  const char* id;
  const char* nameRu;
  const char* nameEn;
  const char* stationId;
  const char* gravity;
  const char* atmosphereRu;
  const char* atmosphereEn;
  const char* temperatureRu;
  const char* temperatureEn;
  WorldPerk perk;
};

struct ColonyStage {
  const char* nameRu;
  const char* nameEn;
  const char* infrastructureRu;
  const char* infrastructureEn;
  const char* loreRu;
  const char* loreEn;
  const char* techUnlocksRu;
  const char* techUnlocksEn;
  int costRP;
  int requiredScans;
  int population;
  int researchBonusPercent;
  int passiveRpPerMin;
  bool allowsResupply; // Spaceport / fuel depot available
  ColonyEpoch epoch;
};

// One build item of a stage plan: which mesh and how many of them appear on
// the surface once the stage is complete. "power" is a placeholder resolved by
// colonyEnergyAsset() to the plant type that fits the world.
struct ColonyStructure {
  const char* asset;
  int count;
};

enum class ColonyDistrict : int {
  Civic = 0,
  Residential,
  Industrial,
  Logistics,
  Energy,
  Research,
  Medical,
  Recreation,
  Security,
  Count
};

struct ColonyAssetSpec {
  const char* key;
  float footprint;
  float modelLength;
  ColonyDistrict district;
};
inline constexpr int kColonyAssetCount = 28;

struct ColonyPlacement {
  const char* asset;
  ColonyDistrict district;
  float theta;
  float phi;
  float heading;
  int ordinal;
};

constexpr int kColonyPlanSlots = 6;
constexpr const char* kColonyPowerAsset = "power";

const std::array<ColonyWorld, 8>& colonyWorlds();
const ColonyWorld* colonyWorld(const std::string& id);
const std::array<ColonyStage, kColonyStageCount>& colonyStages();
int colonyStage(const ColonyStages& stages, const std::string& id);
ColonyEpoch colonyEpoch(int stage);
const char* colonyEpochNameRu(ColonyEpoch epoch);
const char* colonyEpochNameEn(ColonyEpoch epoch);

void normalizeColonies(std::vector<std::string>& founded, ColonyStages& stages);
const std::array<std::array<ColonyStructure, kColonyPlanSlots>, kColonyStageCount>&
colonyStagePlan();
// Power plant appropriate for the world: fission on airless bodies, hydro on
// icy/ocean worlds, thermal where sunlight or volatiles make it the honest pick.
const char* colonyEnergyAsset(const std::string& worldId);
// Cumulative surface build-out up to `stage`, with "power" resolved and counts
// merged per asset. Deterministic: same stage always yields the same layout.
std::vector<std::pair<std::string, int>> colonyBuildPlan(const std::string& worldId,
                                                        int stage);
// Terraforming / world-development track. Milestones are reached when a
// colony's stage passes `stage`; atmosphere and biosphere are 0..1 and drive
// the planet's surface tint, so development is visible from orbit.
struct ColonyMilestone {
  int stage;
  const char* nameRu;
  const char* nameEn;
  const char* detailRu;
  const char* detailEn;
  float atmosphere;
  float biosphere;
  int rewardRP;
};

const std::vector<ColonyMilestone>& colonyTerraformMilestones();
void colonyTerraformState(int stage, float& atmosphere, float& biosphere);
int colonyMilestonesReached(int stage);
// RP granted by the milestones crossed when a world moves from `previousStage`
// to `newStage`. Each milestone is paid once, so re-entering an already crossed
// stage must yield zero.
int colonyMilestoneReward(int previousStage, int newStage);

int colonyStructureCount(const std::string& worldId, int stage);
// Unit surface normal where this world's settlement sits. Shared by the scene
// renderer and the capture harness so a screenshot always faces the town.
const float* colonyAnchorNormal(const std::string& worldId);
// Orthonormal basis at the settlement anchor: the normal plus the two tangents
// the functional district layout is built from. The renderer and the capture
// harness both need it, and a close-up screenshot only shows what the player
// descends into if they agree on which way the town faces.
struct ColonySurfaceFrame {
  float anchor[3];
  float tangentU[3];
  float tangentV[3];
};
ColonySurfaceFrame colonySurfaceFrame(const std::string& worldId);
const std::array<ColonyAssetSpec, kColonyAssetCount>& colonyAssetSpecs();
const ColonyAssetSpec* colonyAssetSpec(const std::string& key);
std::vector<ColonyPlacement> colonySurfaceLayout(const std::string& worldId,
                                                 int stage);
float colonySurfaceLayoutRadius(const std::string& worldId, int stage);
void colonySurfaceDirection(const ColonySurfaceFrame& frame, float theta,
                            float phi, float out[3]);
int colonyResearchBonus(const ColonyStages& stages);
int colonyTotalPopulation(const ColonyStages& stages);
int colonyPassiveRatePerMin(const ColonyStages& stages);
int colonyUpgradeCost(int currentStage);
int colonyUpgradeCost(const ColonyStages& stages, const std::string& worldId, int currentStage);
bool colonyCanResupply(const ColonyStages& stages, const std::string& worldId);

// === Colony perks in the live simulation ===
// Stage and perk texts promise more than science income: helium-3 logistics
// speed up warp recharge, solar and magnetospheric arrays hold a tougher
// shield, and the late epochs unlock elite contracts, classified hypotheses
// and the galactic warp vector. Each aggregator folds the per-world tables the
// same way colonyResearchBonus() does - the best developed world wins - so
// physics, missions and the journal read one number instead of parsing text.
float colonyWarpRechargeBoost(const ColonyStages& stages);
float colonyShieldBoost(const ColonyStages& stages);
int colonyMaxStage(const ColonyStages& stages);

constexpr int kColonyEliteContractStage = 16;
constexpr int kColonyHiddenHypothesisStage = 23;
constexpr int kColonyGalacticVectorStage = 24;

bool colonyUnlocksEliteContracts(const ColonyStages& stages);
bool colonyRevealsHiddenHypotheses(const ColonyStages& stages);
bool colonyUnlocksGalacticVector(const ColonyStages& stages);

// The stage-24 beacon is a route, not a stat: once the civilization epoch is
// complete on any world, the jump to this station burns no fuel.
constexpr const char* kColonyGalacticVectorStation = "m13";
bool colonyGalacticVectorRoute(const ColonyStages& stages, const std::string& stationId);

enum class ColonyUpgradeResult {
  Success,
  UnknownWorld,
  NotFounded,
  Complete,
  NeedScans,
  NeedPoints
};

ColonyUpgradeResult advanceColony(ColonyStages& stages, const std::string& id,
                                 int scannedCount, int& researchPoints);

}  // namespace astro
