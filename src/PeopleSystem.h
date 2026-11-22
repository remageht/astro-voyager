#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace astro {

// === РџСЂРѕС„РµСЃСЃРёРё СЃРїРµС†РёР°Р»РёСЃС‚РѕРІ Рё РєРѕР»РѕРЅРёСЃС‚РѕРІ ===
enum class Profession : int {
  Astrophysicist = 0,    // РќР°СѓС‡РЅС‹Р№ РѕС„РёС†РµСЂ: СѓРјРЅРѕР¶РµРЅРёРµ RP, РіР»СѓР±РѕРєРёР№ Р°РЅР°Р»РёР· Р°РЅРѕРјР°Р»РёР№
  WarpEngineer = 1,      // Р‘РѕСЂС‚РёРЅР¶РµРЅРµСЂ: СѓСЃРєРѕСЂРµРЅРёРµ РІР°СЂРї-РєСѓР»РґР°СѓРЅР°, СЂРµРјРѕРЅС‚ С‰РёС‚РѕРІ/РєРѕСЂРїСѓСЃР°
  Navigator = 2,         // РЁС‚СѓСЂРјР°РЅ: СЃС‚Р°Р±РёР»РёР·Р°С†РёСЏ Сѓ С‡РµСЂРЅС‹С… РґС‹СЂ, СЃРЅРёР¶РµРЅРёРµ СЂР°СЃС…РѕРґР° С‚РѕРїР»РёРІР°
  EclssMedic = 3,        // РћС„РёС†РµСЂ Р¶РёР·РЅРµРѕР±РµСЃРїРµС‡РµРЅРёСЏ: СЌРєРѕРЅРѕРјРёСЏ O2 РІ EVA, Р·Р°С‰РёС‚Р° Р·РґРѕСЂРѕРІСЊСЏ
  Botanist = 4,          // РђРіСЂРѕРЅРѕРј-Р±РѕС‚Р°РЅРёРє: РіРёРґСЂРѕРїРѕРЅРёРєР°, Р±РёРѕРјР°СЃСЃР° Рё СЃС‹С‚РѕСЃС‚СЊ
  Geologist = 5          // Р“РµРѕР»РѕРі-РёСЃСЃР»РµРґРѕРІР°С‚РµР»СЊ: СЂР°Р·РІРµРґРєР° РїРѕР»РµР·РЅС‹С… РёСЃРєРѕРїР°РµРјС‹С…
};

// === РЈРЅРёРєР°Р»СЊРЅС‹Рµ С‡РµСЂС‚С‹ С…Р°СЂР°РєС‚РµСЂР° (Quirks) ===
enum class PersonQuirk : int {
  RadiationResistant = 0, // РЈСЃС‚РѕР№С‡РёРІ Рє РєРѕСЃРјРёС‡РµСЃРєРѕР№ СЂР°РґРёР°С†РёРё Рё РІСЃРїС‹С€РєР°Рј
  VoidBorn = 1,           // РџСЂРёРІС‹С‡РµРЅ Рє РѕС‚РєСЂС‹С‚РѕРјСѓ РєРѕСЃРјРѕСЃСѓ Рё РЅРµРІРµСЃРѕРјРѕСЃС‚Рё (Р±РѕРЅСѓСЃ Рє EVA)
  Workaholic = 2,         // Р Р°Р±РѕС‚Р°РµС‚ РЅР° +25% Р±С‹СЃС‚СЂРµРµ, РЅРѕ С‚СЂРµР±СѓРµС‚ СЂРµРіСѓР»СЏСЂРЅРѕРіРѕ РѕС‚РґС‹С…Р°
  Visionary = 3,          // РџРѕРІС‹С€РµРЅРЅС‹Р№ С€Р°РЅСЃ РЅР°СѓС‡РЅРѕРіРѕ РїСЂРѕСЂС‹РІР° Рё РѕС‚РєСЂС‹С‚РёСЏ РіРёРїРѕС‚РµР·
  CalmUnderPressure = 4,  // РќРµ РїР°РЅРёРєСѓРµС‚ РїСЂРё СЌРєСЃС‚СЂРµРјР°Р»СЊРЅРѕРј РїСЂРёР»РёРІРЅРѕРј СЃС‚СЂРµСЃСЃРµ СЃРёРЅРіСѓР»СЏСЂРЅРѕСЃС‚РµР№
  Harmonizer = 5          // РџРѕРґРґРµСЂР¶РёРІР°РµС‚ Р±РѕРµРІРѕР№ РґСѓС… РєРѕРјР°РЅРґС‹ (+15% Рє РјРѕСЂР°Р»Рё)
};

// === РџР°СЂР°РјРµС‚СЂС‹ Р¶РёР·РЅРµРґРµСЏС‚РµР»СЊРЅРѕСЃС‚Рё (Vitals) ===
struct PersonVitals {
  float health = 100.0f;       // 0..100 (Р±РёРѕ-С†РµР»РѕСЃС‚РЅРѕСЃС‚СЊ, СЂР°РґРёР°С†РёРѕРЅРЅР°СЏ СЃС‚РѕР№РєРѕСЃС‚СЊ)
  float nourishment = 100.0f;  // 0..100 (СЃС‹С‚РѕСЃС‚СЊ Рё РіРёРґСЂР°С‚Р°С†РёСЏ РѕС‚ С‚РµРїР»РёС† Рё РІРѕРґРѕРѕС‡РёСЃС‚РєРё)
  float morale = 100.0f;       // 0..100 (РїСЃРёС…РѕР»РѕРіРёС‡РµСЃРєРёР№ РєРѕРјС„РѕСЂС‚ РѕС‚ РїР°СЂРєРѕРІ Рё СЂС‹РЅРєРѕРІ)
  float fatigue = 0.0f;        // 0..100 (СѓСЃС‚Р°Р»РѕСЃС‚СЊ; РЅР°РєР°РїР»РёРІР°РµС‚СЃСЏ РїСЂРё СЂР°Р±РѕС‚Рµ, СЃР±СЂР°СЃС‹РІР°РµС‚СЃСЏ РІ Р¶РёР»СЊРµ)
};

// === РџРµСЂСЃРѕРЅР° (РљРѕР»РѕРЅРёСЃС‚ / РћС„РёС†РµСЂ СЌРєРёРїР°Р¶Р°) ===
struct Person {
  std::string id;
  std::string firstName;
  std::string lastName;
  std::string originWorldId;  // "earth", "moon", "mars", "europa", "titan", etc.
  Profession profession = Profession::Astrophysicist;
  PersonQuirk quirk = PersonQuirk::Visionary;
  int level = 1;              // 1..5
  int xp = 0;                 // 0..1000
  PersonVitals vitals;
  std::string currentActivityRu;
  std::string currentActivityEn;
  std::string fullName() const { return firstName + " " + lastName; }
};

// === Р”РѕР»Р¶РЅРѕСЃС‚Рё РІ СЌРєРёРїР°Р¶Рµ РєРѕСЂР°Р±Р»СЏ (Ship Crew Posts) ===
enum class CrewPost : int {
  ChiefEngineer = 0,    // Р“Р»Р°РІРЅС‹Р№ РёРЅР¶РµРЅРµСЂ (РІР°СЂРї, С‰РёС‚С‹, РєРѕСЂРїСѓСЃ)
  ScienceOfficer = 1,   // РќР°СѓС‡РЅС‹Р№ РѕС„РёС†РµСЂ (РёСЃСЃР»РµРґРѕРІР°РЅРёСЏ, Р°РЅРѕРјР°Р»РёРё, РіРёРїРѕС‚РµР·С‹)
  Navigator = 2,        // Р“Р»Р°РІРЅС‹Р№ С€С‚СѓСЂРјР°РЅ (С‚СЂР°РµРєС‚РѕСЂРёРё, С‚РѕРїР»РёРІРѕ, РїСЂРёР»РёРІРЅС‹Рµ СЃРёР»С‹)
  LifeSupportMedic = 3, // РћС„РёС†РµСЂ Р¶РёР·РЅРµРѕР±РµСЃРїРµС‡РµРЅРёСЏ (ECLSS, РєРёСЃР»РѕСЂРѕРґ, Р·РґРѕСЂРѕРІСЊРµ)
  Count = 4
};

// === Р РµРµСЃС‚СЂ СЌРєРёРїР°Р¶Р° РєРѕСЂР°Р±Р»СЏ (Ship Crew Roster) ===
class ShipCrewRoster {
 public:
  bool assign(CrewPost post, const Person& person);
  void unassign(CrewPost post);
  bool hasPost(CrewPost post) const;
  const Person* getPost(CrewPost post) const;
  Person* getPost(CrewPost post);
  const std::map<CrewPost, Person>& allAssigned() const { return m_assigned; }
  void clear() { m_assigned.clear(); }

  // РђРєС‚РёРІРЅС‹Рµ Р±РѕРЅСѓСЃС‹ Рё С„РёР·РёС‡РµСЃРєРёРµ РјРЅРѕР¶РёС‚РµР»Рё РґР»СЏ РєРѕСЂР°Р±Р»СЏ:
  float warpCooldownMultiplier() const;     // РњРЅРѕР¶РёС‚РµР»СЊ РІСЂРµРјРµРЅРё РѕС‚РєР°С‚Р° РІР°СЂРїР° (0.70..1.00)
  float shieldRegenMultiplier() const;      // РЈСЃРєРѕСЂРµРЅРёРµ РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ С‰РёС‚Р° (1.00..1.40)
  float hullAutoRepairRate() const;         // РџСЂРѕС†РµРЅС‚РѕРІ РїСЂРѕС‡РЅРѕСЃС‚Рё РєРѕСЂРїСѓСЃР°/СЃРµРє (0.00..0.50)
  float scienceYieldMultiplier() const;     // РњРЅРѕР¶РёС‚РµР»СЊ РїРѕР»СѓС‡Р°РµРјС‹С… РѕС‡РєРѕРІ РЅР°СѓРєРё RP (1.00..1.75)
  float fuelConsumptionMultiplier() const;  // РњРЅРѕР¶РёС‚РµР»СЊ СЂР°СЃС…РѕРґР° С‚РѕРїР»РёРІР° (0.75..1.00)
  float tidalStressTolerance() const;       // РљРѕСЌС„С„РёС†РёРµРЅС‚ Р·Р°РїР°СЃР° РїСЂРёР»РёРІРЅРѕРіРѕ СЃС‚СЂРµСЃСЃР° (1.00..1.50)
  float evaOxygenDrainMultiplier() const;   // РњРЅРѕР¶РёС‚РµР»СЊ СЂР°СЃС…РѕРґР° РєРёСЃР»РѕСЂРѕРґР° РІ EVA (0.60..1.00)

  // Начисление опыта экипажу за действия (сканы, прыжки, ремонт):
  void addExperience(int amount, const std::string& reason = "");
  void gainXp(CrewPost post, int amount);
  int assignedCount() const { return static_cast<int>(m_assigned.size()); }

  // Резерв специалистов корабля:
  std::vector<Person> reserve;

 private:
  std::map<CrewPost, Person> m_assigned;
};

inline int nextLevelXp(int level) { return level * 200; }

// === Р”РµРјРѕРіСЂР°С„РёС‡РµСЃРєРѕРµ СЃРѕСЃС‚РѕСЏРЅРёРµ Рё Р±Р»Р°РіРѕРїРѕР»СѓС‡РёРµ РєРѕР»РѕРЅРёРё (Colony Vitality) ===
struct ColonyVitals {
  float avgHealth = 100.0f;          // Р—РґРѕСЂРѕРІСЊРµ Р¶РёС‚РµР»РµР№ (0..100)
  float avgNourishment = 100.0f;     // РЎС‹С‚РѕСЃС‚СЊ Рё РіРёРґСЂРѕР±Р°Р»Р°РЅСЃ (0..100)
  float avgMorale = 100.0f;          // РњРѕСЂР°Р»СЊ Рё СЃС‡Р°СЃС‚СЊРµ (0..100)
  float avgCompetency = 50.0f;       // РЈСЂРѕРІРµРЅСЊ РєРІР°Р»РёС„РёРєР°С†РёРё СЃРїРµС†РёР°Р»РёСЃС‚РѕРІ (0..100)
  bool inTechnologicalSurge = false; // РЎС‚Р°С‚СѓСЃ В«РўРµС…РЅРѕР»РѕРіРёС‡РµСЃРєРѕРіРѕ РїРѕРґСЉС‘РјР°В»
  float scienceMultiplier = 1.0f;    // РС‚РѕРіРѕРІС‹Р№ РјРЅРѕР¶РёС‚РµР»СЊ RP (1.0..1.8)
  float buildDiscount = 0.0f;        // РЎРєРёРґРєР° РЅР° РїРѕСЃС‚СЂРѕР№РєСѓ (0.00..0.25)
  std::string activeBreakthroughRu;  // РўРµРєСѓС‰РёР№ РЅР°СѓС‡РЅС‹Р№ РїСЂРѕСЂС‹РІ РєРѕР»РѕРЅРёРё
  std::string activeBreakthroughEn;
};

// === РџСЂРѕС†РµРґСѓСЂРЅР°СЏ РіРµРЅРµСЂР°С†РёСЏ Рё СЃРёРјСѓР»СЏС†РёСЏ ===
Person generatePerson(const std::string& worldId, uint32_t seed,
                      Profession preferred = Profession::Astrophysicist);

std::vector<Person> generateColonyNotableCandidates(const std::string& worldId,
                                                    int stage,
                                                    uint32_t seed = 42);

ColonyVitals calculateColonyVitals(const std::string& worldId, int stage,
                                   const std::vector<std::pair<std::string, int>>& buildPlan);

void simulateColonyVitalsTick(ColonyVitals& vitals, float simDtSeconds);

// Р›РѕРєР°Р»РёР·Р°С†РёСЏ Рё С„РѕСЂРјР°С‚РёСЂРѕРІР°РЅРёРµ:
const char* professionNameRu(Profession p);
const char* professionNameEn(Profession p);
const char* quirkNameRu(PersonQuirk q);
const char* quirkNameEn(PersonQuirk q);
const char* quirkDescRu(PersonQuirk q);
const char* quirkDescEn(PersonQuirk q);
const char* crewPostNameRu(CrewPost cp);
const char* crewPostNameEn(CrewPost cp);

}  // namespace astro