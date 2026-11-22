#include "PeopleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace astro {

namespace {

uint32_t lcgNext(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return s;
}

const char* kFirstNames[] = {
    "Elena", "Mia", "Alexey", "Dmitry", "Sarah", "Chen", "Marcus", "Hiroshi",
    "Anya", "Lucas", "Tatyana", "Liam", "Priya", "Kai", "Sophia", "Jamal",
    "Astrid", "Viktor", "Mei", "Gabriel", "Olga", "Nathan", "Ingrid", "Tariq"
};

const char* kLastNames[] = {
    "Vance", "Kowalski", "Chen", "Sterling", "Novak", "Thorne", "Mercer", "Zhang",
    "Lindqvist", "Ivanov", "Okonjo", "Tanaka", "Al-Mansoor", "Moreau", "Santos", "Petrov",
    "Gallagher", "Kim", "Becker", "Cross", "Dubois", "Sokolov", "Nakamura", "Volkov"
};

const char* kActivitiesRu[] = {
    "РљР°Р»РёР±СЂСѓРµС‚ СЃРµРЅСЃРѕСЂС‹ СЃСѓР±РїСЂРѕСЃС‚СЂР°РЅСЃС‚РІР°",
    "РџСЂРѕРІРѕРґРёС‚ РґРёР°РіРЅРѕСЃС‚РёРєСѓ РєРІР°РЅС‚РѕРІРѕРіРѕ СЂРµР°РєС‚РѕСЂР°",
    "РђРЅР°Р»РёР·РёСЂСѓРµС‚ СЃРїРµРєС‚СЂР°Р»СЊРЅС‹Рµ Р»РёРЅРёРё Р°РєРєСЂРµС†РёРѕРЅРЅРѕРіРѕ РґРёСЃРєР°",
    "РћРїС‚РёРјРёР·РёСЂСѓРµС‚ С†РёРєР»С‹ РіРёРґСЂРѕРїРѕРЅРЅРѕР№ РѕСЂР°РЅР¶РµСЂРµРё",
    "РћС‚РґС‹С…Р°РµС‚ РІ РіРµРѕРґРµР·РёС‡РµСЃРєРѕРј РїР°СЂРєРµ",
    "Р”РµР¶СѓСЂРёС‚ РЅР° РїРѕСЃС‚Сѓ СЂР°РґРёР°С†РёРѕРЅРЅРѕРіРѕ РєРѕРЅС‚СЂРѕР»СЏ",
    "РР·СѓС‡Р°РµС‚ Р°СЂС…РёРІРЅС‹Рµ Р·Р°РјРµС‚РєРё Zettelkasten",
    "РџСЂРѕРІРµСЂСЏРµС‚ РіРµСЂРјРµС‚РёС‡РЅРѕСЃС‚СЊ СЃС‚С‹РєРѕРІРѕС‡РЅРѕРіРѕ СѓР·Р»Р°"
};

const char* kActivitiesEn[] = {
    "Calibrating subspace sensor array",
    "Running quantum reactor diagnostics",
    "Analyzing accretion disk emission spectra",
    "Optimizing hydroponic greenhouse cycles",
    "Resting at the geodesic park dome",
    "On duty at cosmic radiation monitoring station",
    "Studying classified Zettelkasten logbook notes",
    "Inspecting docking collar seal integrity"
};

}  // namespace

Person generatePerson(const std::string& worldId, uint32_t seed, Profession preferred) {
  uint32_t s = seed ^ 0x9e3779b9u;
  Person p;
  p.id = "p_" + worldId + "_" + std::to_string(seed % 100000);
  p.firstName = kFirstNames[lcgNext(s) % (sizeof(kFirstNames) / sizeof(kFirstNames[0]))];
  p.lastName = kLastNames[lcgNext(s) % (sizeof(kLastNames) / sizeof(kLastNames[0]))];
  p.originWorldId = worldId.empty() ? "earth" : worldId;
  p.profession = preferred;
  p.quirk = static_cast<PersonQuirk>(lcgNext(s) % 6);
  p.level = 1 + static_cast<int>(lcgNext(s) % 3);  // 1..3 initial level
  p.xp = static_cast<int>(lcgNext(s) % 150);
  p.vitals.health = 85.0f + static_cast<float>(lcgNext(s) % 16);
  p.vitals.nourishment = 80.0f + static_cast<float>(lcgNext(s) % 21);
  p.vitals.morale = 75.0f + static_cast<float>(lcgNext(s) % 26);
  p.vitals.fatigue = static_cast<float>(lcgNext(s) % 25);

  size_t actIdx = lcgNext(s) % (sizeof(kActivitiesRu) / sizeof(kActivitiesRu[0]));
  p.currentActivityRu = kActivitiesRu[actIdx];
  p.currentActivityEn = kActivitiesEn[actIdx];
  return p;
}

std::vector<Person> generateColonyNotableCandidates(const std::string& worldId,
                                                    int stage,
                                                    uint32_t seed) {
  std::vector<Person> candidates;
  if (stage <= 0) return candidates;

  Profession profs[] = {
      Profession::WarpEngineer,
      Profession::Astrophysicist,
      Profession::Navigator,
      Profession::EclssMedic,
      Profession::Botanist,
      Profession::Geologist
  };

  int count = std::min(4, 1 + stage / 5);
  for (int i = 0; i < count; ++i) {
    uint32_t candSeed = seed + static_cast<uint32_t>(i * 1013 + stage * 71);
    Profession pref = profs[(i + stage) % (sizeof(profs) / sizeof(profs[0]))];
    Person p = generatePerson(worldId, candSeed, pref);
    p.level = std::clamp(1 + stage / 7, 1, 5);
    candidates.push_back(p);
  }
  return candidates;
}

bool ShipCrewRoster::assign(CrewPost post, const Person& person) {
  if (post == CrewPost::Count) return false;
  m_assigned[post] = person;
  return true;
}

void ShipCrewRoster::unassign(CrewPost post) {
  m_assigned.erase(post);
}

bool ShipCrewRoster::hasPost(CrewPost post) const {
  return m_assigned.find(post) != m_assigned.end();
}

const Person* ShipCrewRoster::getPost(CrewPost post) const {
  auto it = m_assigned.find(post);
  return (it != m_assigned.end()) ? &it->second : nullptr;
}

Person* ShipCrewRoster::getPost(CrewPost post) {
  auto it = m_assigned.find(post);
  return (it != m_assigned.end()) ? &it->second : nullptr;
}

float ShipCrewRoster::warpCooldownMultiplier() const {
  auto it = m_assigned.find(CrewPost::ChiefEngineer);
  if (it == m_assigned.end()) return 1.0f;
  float mult = 1.0f - (0.05f + 0.05f * static_cast<float>(it->second.level));
  if (it->second.quirk == PersonQuirk::Workaholic) {
    mult -= 0.05f;
  }
  return std::clamp(mult, 0.65f, 1.00f);
}

float ShipCrewRoster::shieldRegenMultiplier() const {
  auto it = m_assigned.find(CrewPost::ChiefEngineer);
  if (it == m_assigned.end()) return 1.0f;
  float bonus = 1.0f + 0.06f * static_cast<float>(it->second.level);
  if (it->second.quirk == PersonQuirk::RadiationResistant) {
    bonus += 0.05f;
  }
  return std::clamp(bonus, 1.00f, 1.40f);
}

float ShipCrewRoster::hullAutoRepairRate() const {
  auto it = m_assigned.find(CrewPost::ChiefEngineer);
  if (it == m_assigned.end()) return 0.0f;
  float rate = 0.04f * static_cast<float>(it->second.level); // % per sec
  if (it->second.quirk == PersonQuirk::Workaholic) {
    rate *= 1.25f;
  }
  return rate;
}

float ShipCrewRoster::scienceYieldMultiplier() const {
  auto it = m_assigned.find(CrewPost::ScienceOfficer);
  if (it == m_assigned.end()) return 1.0f;
  float mult = 1.0f + 0.10f * static_cast<float>(it->second.level);
  if (it->second.quirk == PersonQuirk::Visionary) {
    mult += 0.15f;
  }
  return std::clamp(mult, 1.00f, 1.75f);
}

float ShipCrewRoster::fuelConsumptionMultiplier() const {
  auto it = m_assigned.find(CrewPost::Navigator);
  if (it == m_assigned.end()) return 1.0f;
  float mult = 1.0f - 0.05f * static_cast<float>(it->second.level);
  return std::clamp(mult, 0.75f, 1.00f);
}

float ShipCrewRoster::tidalStressTolerance() const {
  auto it = m_assigned.find(CrewPost::Navigator);
  if (it == m_assigned.end()) return 1.0f;
  float tol = 1.0f + 0.06f * static_cast<float>(it->second.level);
  if (it->second.quirk == PersonQuirk::CalmUnderPressure) {
    tol += 0.18f;
  }
  return std::clamp(tol, 1.00f, 1.50f);
}

float ShipCrewRoster::evaOxygenDrainMultiplier() const {
  auto it = m_assigned.find(CrewPost::LifeSupportMedic);
  if (it == m_assigned.end()) return 1.0f;
  float drain = 1.0f - 0.07f * static_cast<float>(it->second.level);
  if (it->second.quirk == PersonQuirk::VoidBorn) {
    drain -= 0.10f;
  }
  return std::clamp(drain, 0.55f, 1.00f);
}

void ShipCrewRoster::addExperience(int amount, const std::string& reason) {
  (void)reason;
  if (amount <= 0) return;
  for (auto& pair : m_assigned) {
    pair.second.xp += amount;
    int needed = pair.second.level * 200;
    while (pair.second.xp >= needed && pair.second.level < 5) {
      pair.second.xp -= needed;
      pair.second.level++;
      needed = pair.second.level * 200;
    }
  }
}

void ShipCrewRoster::gainXp(CrewPost post, int amount) {
  Person* p = getPost(post);
  if (!p || amount <= 0) return;
  p->xp += amount;
  int needed = p->level * 200;
  while (p->xp >= needed && p->level < 5) {
    p->xp -= needed;
    p->level++;
    needed = p->level * 200;
  }
}

ColonyVitals calculateColonyVitals(const std::string& worldId, int stage,
                                   const std::vector<std::pair<std::string, int>>& buildPlan) {
  (void)worldId;
  ColonyVitals v;
  if (stage <= 0) {
    v.avgHealth = 0.0f;
    v.avgNourishment = 0.0f;
    v.avgMorale = 0.0f;
    v.avgCompetency = 0.0f;
    v.scienceMultiplier = 1.0f;
    return v;
  }

  int greenhouses = 0;
  int waters = 0;
  int hospitals = 0;
  int schools = 0;
  int parks = 0;
  int markets = 0;

  for (const auto& item : buildPlan) {
    if (item.first == "colony_greenhouse") greenhouses += item.second;
    else if (item.first == "colony_water") waters += item.second;
    else if (item.first == "colony_hospital" || item.first == "colony_clinic") hospitals += item.second;
    else if (item.first == "colony_school") schools += item.second;
    else if (item.first == "colony_park") parks += item.second;
    else if (item.first == "colony_market") markets += item.second;
  }

  v.avgHealth = std::clamp(65.0f + static_cast<float>(hospitals * 10 + waters * 5), 40.0f, 100.0f);
  v.avgNourishment = std::clamp(60.0f + static_cast<float>(greenhouses * 12 + waters * 6), 35.0f, 100.0f);
  v.avgMorale = std::clamp(55.0f + static_cast<float>(parks * 12 + markets * 10 + greenhouses * 3), 30.0f, 100.0f);
  v.avgCompetency = std::clamp(40.0f + static_cast<float>(schools * 15 + stage), 20.0f, 100.0f);

  if (v.avgHealth >= 75.0f && v.avgNourishment >= 70.0f && v.avgMorale >= 70.0f) {
    v.inTechnologicalSurge = true;
    v.scienceMultiplier = 1.50f;
    v.buildDiscount = 0.15f;
    v.activeBreakthroughRu = "РљРІР°РЅС‚РѕРІР°СЏ СЃРёРЅРµСЂРіРёСЏ Р±РёРѕСЃС„РµСЂС‹ Рё СЃРёРЅС‚РµР·Р°";
    v.activeBreakthroughEn = "Quantum Biosphere & Fusion Synergy";
  } else {
    v.inTechnologicalSurge = false;
    v.scienceMultiplier = 1.0f + 0.003f * (v.avgMorale - 50.0f);
    v.buildDiscount = 0.0f;
    v.activeBreakthroughRu = "РЎС‚Р°РЅРґР°СЂС‚РЅС‹Р№ РёСЃСЃР»РµРґРѕРІР°С‚РµР»СЊСЃРєРёР№ РїСЂРѕС‚РѕРєРѕР»";
    v.activeBreakthroughEn = "Standard Research Protocol";
  }

  return v;
}

void simulateColonyVitalsTick(ColonyVitals& vitals, float simDtSeconds) {
  float drift = std::sin(simDtSeconds * 0.1f) * 0.5f;
  vitals.avgMorale = std::clamp(vitals.avgMorale + drift, 20.0f, 100.0f);
}

const char* professionNameRu(Profession p) {
  switch (p) {
    case Profession::Astrophysicist: return "РќР°СѓС‡РЅС‹Р№ РћС„РёС†РµСЂ (РђСЃС‚СЂРѕС„РёР·РёРє)";
    case Profession::WarpEngineer: return "Р“Р»Р°РІРЅС‹Р№ РРЅР¶РµРЅРµСЂ (Р’Р°СЂРї & Р©РёС‚С‹)";
    case Profession::Navigator: return "Р“Р»Р°РІРЅС‹Р№ РЁС‚СѓСЂРјР°РЅ";
    case Profession::EclssMedic: return "РћС„РёС†РµСЂ Р–РёР·РЅРµРѕР±РµСЃРїРµС‡РµРЅРёСЏ (ECLSS)";
    case Profession::Botanist: return "Р‘РёРѕРёРЅР¶РµРЅРµСЂ-РђРіСЂРѕРЅРѕРј";
    case Profession::Geologist: return "РџР»Р°РЅРµС‚Р°СЂРЅС‹Р№ Р“РµРѕР»РѕРі";
  }
  return "РЎРїРµС†РёР°Р»РёСЃС‚";
}

const char* professionNameEn(Profession p) {
  switch (p) {
    case Profession::Astrophysicist: return "Science Officer (Astrophysicist)";
    case Profession::WarpEngineer: return "Chief Engineer (Warp & Shields)";
    case Profession::Navigator: return "Chief Navigator";
    case Profession::EclssMedic: return "Life Support Officer (ECLSS)";
    case Profession::Botanist: return "Bioengineer-Botanist";
    case Profession::Geologist: return "Planetary Geologist";
  }
  return "Specialist";
}

const char* quirkNameRu(PersonQuirk q) {
  switch (q) {
    case PersonQuirk::RadiationResistant: return "РЎС‚РѕР№РєРёР№ Рє Р Р°РґРёР°С†РёРё";
    case PersonQuirk::VoidBorn: return "Р”РµРІСЏС‚С‹Р№ Р’Р°Р» (РџСѓСЃС‚РѕС‚РЅРёРє)";
    case PersonQuirk::Workaholic: return "РўСЂСѓРґРѕРіРѕР»РёРє";
    case PersonQuirk::Visionary: return "Р’РёР·РёРѕРЅРµСЂ РћС‚РєСЂС‹С‚РёР№";
    case PersonQuirk::CalmUnderPressure: return "РҐР»Р°РґРЅРѕРєСЂРѕРІРёРµ РЎРёРЅРіСѓР»СЏСЂРЅРѕСЃС‚Рё";
    case PersonQuirk::Harmonizer: return "Р­РјРїР°С‚-Р“Р°СЂРјРѕРЅРёР·Р°С‚РѕСЂ";
  }
  return "РћР±С‹С‡РЅС‹Р№";
}

const char* quirkNameEn(PersonQuirk q) {
  switch (q) {
    case PersonQuirk::RadiationResistant: return "Radiation Resistant";
    case PersonQuirk::VoidBorn: return "Void Born";
    case PersonQuirk::Workaholic: return "Workaholic";
    case PersonQuirk::Visionary: return "Visionary Explorer";
    case PersonQuirk::CalmUnderPressure: return "Singularity Composure";
    case PersonQuirk::Harmonizer: return "Crew Harmonizer";
  }
  return "Standard";
}

const char* quirkDescRu(PersonQuirk q) {
  switch (q) {
    case PersonQuirk::RadiationResistant: return "+15% СЃС‚РѕР№РєРѕСЃС‚СЊ Рє СЃРѕР»РЅРµС‡РЅС‹Рј РІСЃРїС‹С€РєР°Рј Рё СЂР°РґРёР°С†РёРё";
    case PersonQuirk::VoidBorn: return "-10% СЂР°СЃС…РѕРґ РєРёСЃР»РѕСЂРѕРґР° РІ РѕС‚РєСЂС‹С‚РѕРј РєРѕСЃРјРѕСЃРµ (EVA)";
    case PersonQuirk::Workaholic: return "+25% СЃРєРѕСЂРѕСЃС‚СЊ СЂРµРјРѕРЅС‚Р° Рё РѕС…Р»Р°Р¶РґРµРЅРёСЏ, РЅРѕ С‚СЂРµР±СѓРµС‚ СЂРµРіСѓР»СЏСЂРЅРѕРіРѕ РѕС‚РґС‹С…Р°";
    case PersonQuirk::Visionary: return "+15% Рє РїРѕР»СѓС‡РµРЅРёСЋ РЅР°СѓС‡РЅС‹С… РѕС‡РєРѕРІ (RP) Рё С€Р°РЅСЃ РЅР°СѓС‡РЅРѕРіРѕ РїСЂРѕСЂС‹РІР°";
    case PersonQuirk::CalmUnderPressure: return "+18% Рє РїРѕСЂРѕРіСѓ Р±РµР·РѕРїР°СЃРЅРѕРіРѕ РїСЂРёР»РёРІРЅРѕРіРѕ СЃС‚СЂРµСЃСЃР° С‡РµСЂРЅРѕР№ РґС‹СЂС‹";
    case PersonQuirk::Harmonizer: return "+15% Рє РЅР°СЃС‚СЂРѕРµРЅРёСЋ Рё РјРѕСЂР°Р»Рё РІСЃРµРіРѕ СЌРєРёРїР°Р¶Р°";
  }
  return "";
}

const char* quirkDescEn(PersonQuirk q) {
  switch (q) {
    case PersonQuirk::RadiationResistant: return "+15% resistance to radiation and solar flares";
    case PersonQuirk::VoidBorn: return "-10% oxygen consumption during EVA spacewalks";
    case PersonQuirk::Workaholic: return "+25% repair and cooldown speed; accumulates fatigue";
    case PersonQuirk::Visionary: return "+15% bonus to scientific RP yield and breakthrough chance";
    case PersonQuirk::CalmUnderPressure: return "+18% black hole tidal stress tolerance";
    case PersonQuirk::Harmonizer: return "+15% morale booster for the entire crew roster";
  }
  return "";
}

const char* crewPostNameRu(CrewPost cp) {
  switch (cp) {
    case CrewPost::ChiefEngineer: return "Р“Р»Р°РІРЅС‹Р№ РРЅР¶РµРЅРµСЂ";
    case CrewPost::ScienceOfficer: return "РќР°СѓС‡РЅС‹Р№ РћС„РёС†РµСЂ";
    case CrewPost::Navigator: return "Р“Р»Р°РІРЅС‹Р№ РЁС‚СѓСЂРјР°РЅ";
    case CrewPost::LifeSupportMedic: return "РћС„РёС†РµСЂ Р–РёР·РЅРµРѕР±РµСЃРїРµС‡РµРЅРёСЏ";
    default: return "РџРѕСЃС‚";
  }
}

const char* crewPostNameEn(CrewPost cp) {
  switch (cp) {
    case CrewPost::ChiefEngineer: return "Chief Engineer";
    case CrewPost::ScienceOfficer: return "Science Officer";
    case CrewPost::Navigator: return "Chief Navigator";
    case CrewPost::LifeSupportMedic: return "Life Support Officer";
    default: return "Post";
  }
}

}  // namespace astro