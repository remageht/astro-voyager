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

constexpr int kColonyPlanSlots = 3;
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
int colonyResearchBonus(const ColonyStages& stages);
int colonyTotalPopulation(const ColonyStages& stages);
int colonyPassiveRatePerMin(const ColonyStages& stages);
int colonyUpgradeCost(int currentStage);
int colonyUpgradeCost(const ColonyStages& stages, const std::string& worldId, int currentStage);
bool colonyCanResupply(const ColonyStages& stages, const std::string& worldId);

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
