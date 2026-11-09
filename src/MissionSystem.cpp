#include "MissionSystem.h"

#include "ScienceSystem.h"
#include "Zettelkasten.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace astro {

namespace {

struct ClientInfo {
  std::string ru;
  std::string en;
};

const ClientInfo kClients[] = {
  {"Земной Научный Директорат", "Earth Scientific Directorate"},
  {"Академия Экзофизики и Релятивизма", "Exophysics & Relativistic Academy"},
  {"Колониальный Корпус M13", "M13 Colonial Corps"},
  {"Фонд Исследования Сингулярностей", "Singularity Research Foundation"},
  {"Институт Глубокого Космоса", "Deep Space Exploration Institute"},
  {"Служба Снабжения Орбитального Флота", "Orbital Fleet Logistics Bureau"}
};
constexpr size_t kClientCount = sizeof(kClients) / sizeof(kClients[0]);

struct BodyTarget {
  std::string id;
  std::string nameRu;
  std::string nameEn;
};

const BodyTarget kTerrestrialBodies[] = {
  {"mars", "Марс", "Mars"},
  {"moon", "Луна", "Moon"},
  {"titan", "Титан", "Titan"},
  {"europa", "Европа", "Europa"},
  {"pluto", "Плутон", "Pluto"}
};
constexpr size_t kTerrestrialCount = sizeof(kTerrestrialBodies) / sizeof(kTerrestrialBodies[0]);

const BodyTarget kSpectralTargets[] = {
  {"psr-vela", "Пульсар Вела", "Vela Pulsar"},
  {"gw170817", "Килинова GW170817", "Kilonova GW170817"},
  {"sn1987a", "Сверхновая SN 1987A", "Supernova SN 1987A"},
  {"m16", "Столпы Творения M16", "Pillars of Creation"},
  {"at2019qiz", "TDE AT2019qiz", "TDE AT2019qiz"},
  {"saturn", "Сатурн", "Saturn"},
  {"m87", "M87*", "M87*"}
};
constexpr size_t kSpectralCount = sizeof(kSpectralTargets) / sizeof(kSpectralTargets[0]);

const BodyTarget kGravityTargets[] = {
  {"sgra", "Стрелец A*", "Sagittarius A*"},
  {"m87", "M87*", "M87*"}
};
constexpr size_t kGravityCount = sizeof(kGravityTargets) / sizeof(kGravityTargets[0]);

const std::pair<std::string, std::string> kStationPairs[] = {
  {"sol", "m13"},
  {"sol", "sgra"},
  {"sgra", "m87"},
  {"m13", "sol"}
};
constexpr size_t kPairCount = sizeof(kStationPairs) / sizeof(kStationPairs[0]);

uint32_t stepLcg(uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

std::string escapeJson(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else out += c;
  }
  return out;
}

}  // namespace

MissionManager::MissionManager() {
  m_lastSeed = 1337;
  generateBoard("sol", 4, 1337);
}

void MissionManager::generateBoard(const std::string& currentStationId, int count, uint32_t seed) {
  if (seed == 0) {
    seed = m_lastSeed + 17u;
  }
  m_lastSeed = seed;
  m_lastStationId = currentStationId;

  m_available.clear();
  uint32_t rng = seed;

  for (int i = 0; i < count; ++i) {
    MissionContract c;
    c.seed = rng;
    const uint32_t typeRoll = stepLcg(rng) % 5;
    c.type = static_cast<MissionType>(typeRoll);
    c.status = MissionStatus::Available;

    const ClientInfo& client = kClients[stepLcg(rng) % kClientCount];
    c.clientRu = client.ru;
    c.clientEn = client.en;

    std::ostringstream idStream;
    idStream << "cnt_" << (currentStationId.empty() ? "sol" : currentStationId)
             << "_" << static_cast<int>(c.type) << "_" << (rng & 0xFFFF);
    c.id = idStream.str();

    switch (c.type) {
      case MissionType::SpectralSurvey: {
        const BodyTarget& tgt = kSpectralTargets[stepLcg(rng) % kSpectralCount];
        c.targetStationId = tgt.id;
        c.targetStationName = tgt.nameRu;
        c.targetValue = 1.0f;
        c.currentValue = 0.0f;
        c.rewardRP = 200 + static_cast<int>(stepLcg(rng) % 150);

        c.titleRu = "Спектрометрия: " + tgt.nameRu;
        c.titleEn = "Spectrometry: " + tgt.nameEn;
        c.descRu = "Проведите спектральный анализ излучения и зарегистрируйте линии поглощения/эмиссии объекта " + tgt.nameRu + ".";
        c.descEn = "Perform spectrometry observation and log characteristic emission/absorption lines of " + tgt.nameEn + ".";
        break;
      }
      case MissionType::SurfaceSampling: {
        const BodyTarget& tgt = kTerrestrialBodies[stepLcg(rng) % kTerrestrialCount];
        c.targetStationId = tgt.id;
        c.targetStationName = tgt.nameRu;
        const float depth = 20.0f + static_cast<float>(stepLcg(rng) % 4) * 10.0f; // 20, 30, 40, 50 cm
        c.targetValue = depth;
        c.currentValue = 0.0f;
        c.rewardRP = 250 + static_cast<int>(stepLcg(rng) % 200);

        std::ostringstream dStr;
        dStr << static_cast<int>(depth);
        c.titleRu = "Георазведка: Керн " + tgt.nameRu + " (" + dStr.str() + " см)";
        c.titleEn = "Core Sampling: " + tgt.nameEn + " (" + dStr.str() + " cm)";
        c.descRu = "Совершите посадку на " + tgt.nameRu + " и произведите забор образца породы буром CoreSampler на глубину от " + dStr.str() + " см.";
        c.descEn = "Execute a planetary touchdown on " + tgt.nameEn + " and extract geological core sample to depth >= " + dStr.str() + " cm.";
        break;
      }
      case MissionType::GravitationalProbe: {
        const BodyTarget& tgt = kGravityTargets[stepLcg(rng) % kGravityCount];
        c.targetStationId = tgt.id;
        c.targetStationName = tgt.nameRu;
        c.targetValue = 3.5f; // r <= 3.5 Rs
        c.currentValue = 0.0f;
        c.rewardRP = 400 + static_cast<int>(stepLcg(rng) % 200);

        c.titleRu = "Сингулярность: Зондирование " + tgt.nameRu;
        c.titleEn = "Singularity Probe: " + tgt.nameEn;
        c.descRu = "Подойдите к границе фотонной сферы " + tgt.nameRu + " (r <= 3.5 Rs), зафиксируйте метрику пространства-времени и безопасно уйдите на орбиту.";
        c.descEn = "Approach the photon sphere boundary of " + tgt.nameEn + " (r <= 3.5 Rs), capture spacetime curvature metric and return safely.";
        break;
      }
      case MissionType::DeepSpaceProbe: {
        c.targetStationId = "deep_space";
        c.targetStationName = "Глубокий космос";
        c.targetValue = 45.0f; // Dist >= 45u
        c.currentValue = 0.0f;
        c.rewardRP = 300 + static_cast<int>(stepLcg(rng) % 150);

        c.titleRu = "Дальний дозор: Калибровка за эклиптикой";
        c.titleEn = "Deep Space Probe: Off-Ecliptic Telemetry";
        c.descRu = "Выйдите в глубокий космос за пределы эклиптики (дистанция > 45.0u от центра) для регистрации реликтового фона.";
        c.descEn = "Navigate into deep interstellar space (> 45.0u from origin) to collect cosmic microwave background telemetry.";
        break;
      }
      case MissionType::CargoExpress: {
        const auto& pair = kStationPairs[stepLcg(rng) % kPairCount];
        c.targetStationId = pair.first;
        c.destinationStationId = pair.second;
        c.targetStationName = pair.first;
        c.destinationStationName = pair.second;
        c.targetValue = 1.0f;
        c.currentValue = 0.0f;
        c.timeLimitSec = 240.0; // 4 minutes
        c.timeRemainingSec = 240.0;
        c.rewardRP = 350 + static_cast<int>(stepLcg(rng) % 150);

        c.titleRu = "Срочный рейс: Доставка " + pair.first + " -> " + pair.second;
        c.titleEn = "Priority Transit: Delivery " + pair.first + " -> " + pair.second;
        c.descRu = "Доставьте контейнер с квантовыми кристаллами на станцию " + pair.second + " до истечения времени.";
        c.descEn = "Courier quantum crystal telemetry pack to station " + pair.second + " before the timer expires.";
        break;
      }
    }

    // Avoid adding duplicates of already active contracts
    bool alreadyActive = false;
    for (const auto& a : m_active) {
      if (a.targetStationId == c.targetStationId && a.type == c.type) {
        alreadyActive = true;
        break;
      }
    }
    if (!alreadyActive) {
      m_available.push_back(c);
    }
  }
}

bool MissionManager::acceptContract(const std::string& contractId) {
  if (static_cast<int>(m_active.size()) >= kMaxActive) return false;

  auto it = std::find_if(m_available.begin(), m_available.end(),
                         [&](const MissionContract& c) { return c.id == contractId; });
  if (it == m_available.end()) return false;

  MissionContract contract = *it;
  contract.status = MissionStatus::Active;
  m_available.erase(it);
  m_active.push_back(contract);
  return true;
}

bool MissionManager::abandonContract(const std::string& contractId) {
  auto it = std::find_if(m_active.begin(), m_active.end(),
                         [&](const MissionContract& c) { return c.id == contractId; });
  if (it == m_active.end()) return false;

  m_active.erase(it);
  return true;
}

void MissionManager::update(double dt, const std::string& currentStationId,
                            double shipDistToTarget, bool isDocked, float shipSpeed) {
  (void)currentStationId;
  (void)shipDistToTarget;
  (void)isDocked;
  (void)shipSpeed;

  for (auto& c : m_active) {
    if (c.status != MissionStatus::Active) continue;

    // Timed missions countdown
    if (c.timeLimitSec > 0.0) {
      c.timeRemainingSec -= dt;
      if (c.timeRemainingSec <= 0.0) {
        c.timeRemainingSec = 0.0;
        c.status = MissionStatus::Expired;
      }
    }
  }
}

bool MissionManager::onSpectroscopyScan(const std::string& stationId) {
  bool triggered = false;
  for (auto& c : m_active) {
    if (c.status == MissionStatus::Active && c.type == MissionType::SpectralSurvey) {
      if (c.targetStationId == stationId) {
        c.currentValue = c.targetValue;
        c.status = MissionStatus::Completed;
        triggered = true;
      }
    }
  }
  return triggered;
}

bool MissionManager::onSurfaceSample(const std::string& bodyId, float depthCm) {
  bool triggered = false;
  for (auto& c : m_active) {
    if (c.status == MissionStatus::Active && c.type == MissionType::SurfaceSampling) {
      if (c.targetStationId == bodyId) {
        c.currentValue = std::max(c.currentValue, depthCm);
        if (c.currentValue >= c.targetValue) {
          c.status = MissionStatus::Completed;
          triggered = true;
        }
      }
    }
  }
  return triggered;
}

bool MissionManager::onProximityObservation(const std::string& stationId, double distRs) {
  bool triggered = false;
  for (auto& c : m_active) {
    if (c.status == MissionStatus::Active && c.type == MissionType::GravitationalProbe) {
      if (c.targetStationId == stationId) {
        if (distRs > 0.0 && distRs <= static_cast<double>(c.targetValue)) {
          c.currentValue = static_cast<float>(distRs);
          c.status = MissionStatus::Completed;
          triggered = true;
        }
      }
    }
  }
  return triggered;
}

bool MissionManager::onDeepSpacePosition(double distFromOrigin) {
  bool triggered = false;
  for (auto& c : m_active) {
    if (c.status == MissionStatus::Active && c.type == MissionType::DeepSpaceProbe) {
      c.currentValue = static_cast<float>(distFromOrigin);
      if (c.currentValue >= c.targetValue) {
        c.status = MissionStatus::Completed;
        triggered = true;
      }
    }
  }
  return triggered;
}

bool MissionManager::onStationDocked(const std::string& stationId) {
  bool triggered = false;
  for (auto& c : m_active) {
    if (c.status == MissionStatus::Active && c.type == MissionType::CargoExpress) {
      if (c.destinationStationId == stationId) {
        c.currentValue = c.targetValue;
        c.status = MissionStatus::Completed;
        triggered = true;
      }
    }
  }
  return triggered;
}

bool MissionManager::claimReward(const std::string& contractId, ScienceCodex& codex,
                                 BridgeComms* comms, ZettelGraph* zettel) {
  auto it = std::find_if(m_active.begin(), m_active.end(),
                         [&](const MissionContract& c) { return c.id == contractId; });
  if (it == m_active.end() || it->status != MissionStatus::Completed) return false;

  // Award Research Points
  codex.addResearchPoints(it->rewardRP);
  it->status = MissionStatus::Claimed;

  // Radio transmission congratulations
  if (comms) {
    RadioMessage msg;
    msg.id = "radio_" + it->id;
    msg.callsignRu = it->clientRu;
    msg.callsignEn = it->clientEn;
    msg.textRu = "Контракт выполнен! Данные успешно переданы в научный реестр. Начислено +" +
                 std::to_string(it->rewardRP) + " RP.";
    msg.textEn = "Directive fulfilled! Telemetry logged to science database. Awarded +" +
                 std::to_string(it->rewardRP) + " RP.";
    msg.priority = 2.0f;
    msg.durationSec = 6.0;
    comms->queueMessage(msg, 0.0);
  }

  // Unlock hypothesis if specified
  if (zettel && !it->rewardHypothesisId.empty()) {
    zettel->resolveHypothesis(it->rewardHypothesisId);
  }

  // Remove claimed contract from active list
  m_active.erase(it);
  return true;
}

int MissionManager::getCompletedUnclaimedCount() const {
  int count = 0;
  for (const auto& c : m_active) {
    if (c.status == MissionStatus::Completed) {
      count++;
    }
  }
  return count;
}

const MissionContract* MissionManager::getPrimaryActiveContract() const {
  for (const auto& c : m_active) {
    if (c.status == MissionStatus::Active || c.status == MissionStatus::Completed) {
      return &c;
    }
  }
  return nullptr;
}

std::string MissionManager::serialize() const {
  std::ostringstream ss;
  ss << "{\n  \"version\": 1,\n  \"active\": [\n";
  for (size_t i = 0; i < m_active.size(); ++i) {
    const auto& c = m_active[i];
    ss << "    {\n"
       << "      \"id\": \"" << escapeJson(c.id) << "\",\n"
       << "      \"type\": " << static_cast<int>(c.type) << ",\n"
       << "      \"status\": " << static_cast<int>(c.status) << ",\n"
       << "      \"titleRu\": \"" << escapeJson(c.titleRu) << "\",\n"
       << "      \"titleEn\": \"" << escapeJson(c.titleEn) << "\",\n"
       << "      \"clientRu\": \"" << escapeJson(c.clientRu) << "\",\n"
       << "      \"clientEn\": \"" << escapeJson(c.clientEn) << "\",\n"
       << "      \"descRu\": \"" << escapeJson(c.descRu) << "\",\n"
       << "      \"descEn\": \"" << escapeJson(c.descEn) << "\",\n"
       << "      \"targetStationId\": \"" << escapeJson(c.targetStationId) << "\",\n"
       << "      \"targetStationName\": \"" << escapeJson(c.targetStationName) << "\",\n"
       << "      \"destinationStationId\": \"" << escapeJson(c.destinationStationId) << "\",\n"
       << "      \"destinationStationName\": \"" << escapeJson(c.destinationStationName) << "\",\n"
       << "      \"targetValue\": " << c.targetValue << ",\n"
       << "      \"currentValue\": " << c.currentValue << ",\n"
       << "      \"rewardRP\": " << c.rewardRP << ",\n"
       << "      \"timeLimitSec\": " << c.timeLimitSec << ",\n"
       << "      \"timeRemainingSec\": " << c.timeRemainingSec << "\n"
       << "    }" << (i + 1 < m_active.size() ? ",\n" : "\n");
  }
  ss << "  ]\n}";
  return ss.str();
}

bool MissionManager::deserialize(const std::string& json) {
  if (json.empty() || json.find("\"version\"") == std::string::npos) return false;

  std::vector<MissionContract> loaded;
  size_t pos = 0;
  while ((pos = json.find("\"id\":", pos)) != std::string::npos) {
    MissionContract c;
    auto readStr = [&](const std::string& key) -> std::string {
      size_t kPos = json.find("\"" + key + "\":", pos);
      if (kPos == std::string::npos || kPos > pos + 1200) return "";
      size_t startQ = json.find('"', kPos + key.size() + 2);
      if (startQ == std::string::npos) return "";
      size_t endQ = json.find('"', startQ + 1);
      if (endQ == std::string::npos) return "";
      return json.substr(startQ + 1, endQ - startQ - 1);
    };

    auto readFloat = [&](const std::string& key, float defVal) -> float {
      size_t kPos = json.find("\"" + key + "\":", pos);
      if (kPos == std::string::npos || kPos > pos + 1200) return defVal;
      size_t numStart = json.find_first_of("0123456789.-", kPos + key.size() + 2);
      if (numStart == std::string::npos) return defVal;
      return std::stof(json.substr(numStart));
    };

    auto readInt = [&](const std::string& key, int defVal) -> int {
      size_t kPos = json.find("\"" + key + "\":", pos);
      if (kPos == std::string::npos || kPos > pos + 1200) return defVal;
      size_t numStart = json.find_first_of("0123456789-", kPos + key.size() + 2);
      if (numStart == std::string::npos) return defVal;
      return std::stoi(json.substr(numStart));
    };

    c.id = readStr("id");
    c.type = static_cast<MissionType>(readInt("type", 0));
    c.status = static_cast<MissionStatus>(readInt("status", 1));
    c.titleRu = readStr("titleRu");
    c.titleEn = readStr("titleEn");
    c.clientRu = readStr("clientRu");
    c.clientEn = readStr("clientEn");
    c.descRu = readStr("descRu");
    c.descEn = readStr("descEn");
    c.targetStationId = readStr("targetStationId");
    c.targetStationName = readStr("targetStationName");
    c.destinationStationId = readStr("destinationStationId");
    c.destinationStationName = readStr("destinationStationName");
    c.targetValue = readFloat("targetValue", 0.0f);
    c.currentValue = readFloat("currentValue", 0.0f);
    c.rewardRP = readInt("rewardRP", 200);
    c.timeLimitSec = static_cast<double>(readFloat("timeLimitSec", 0.0f));
    c.timeRemainingSec = static_cast<double>(readFloat("timeRemainingSec", 0.0f));

    if (!c.id.empty()) {
      loaded.push_back(c);
    }
    pos += 10;
  }

  m_active = std::move(loaded);
  return true;
}

}  // namespace astro
