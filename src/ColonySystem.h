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

const std::array<ColonyWorld, 8>& colonyWorlds();
const ColonyWorld* colonyWorld(const std::string& id);
const std::array<ColonyStage, kColonyStageCount>& colonyStages();
int colonyStage(const ColonyStages& stages, const std::string& id);
ColonyEpoch colonyEpoch(int stage);
const char* colonyEpochNameRu(ColonyEpoch epoch);
const char* colonyEpochNameEn(ColonyEpoch epoch);

void normalizeColonies(std::vector<std::string>& founded, ColonyStages& stages);
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
