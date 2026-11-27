#include "SpaceEventsSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>

#include "ScienceSystem.h"
#include "Zettelkasten.h"

namespace astro {
namespace {

constexpr double kPi = 3.14159265358979323846;

uint64_t splitMix64(uint64_t x) {
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

double hashUnit(uint64_t h) {
  return static_cast<double>(h & 0xFFFFFFULL) / static_cast<double>(0x1000000ULL);
}

uint64_t fnv1a(const std::string& s) {
  uint64_t h = 14695981039346656037ULL;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  return h;
}

double vecDist(const Vec3& a, const Vec3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::string jsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else {
      out += static_cast<char>(c);
    }
  }
  return out;
}

std::string trimCopy(const std::string& s) {
  size_t a = 0;
  while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
  size_t b = s.size();
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
  return s.substr(a, b - a);
}

bool extractJsonString(const std::string& obj, const std::string& key, std::string& out) {
  const std::string pat = "\"" + key + "\":\"";
  size_t pos = obj.find(pat);
  if (pos == std::string::npos) return false;
  pos += pat.size();
  std::string val;
  while (pos < obj.size()) {
    const char ch = obj[pos++];
    if (ch == '\\' && pos < obj.size()) {
      const char esc = obj[pos++];
      if (esc == 'n') val += '\n';
      else if (esc == 'r') val += '\r';
      else if (esc == 't') val += '\t';
      else val += esc;
    } else if (ch == '"') {
      out = val;
      return true;
    } else {
      val += ch;
    }
  }
  return false;
}

bool extractJsonDouble(const std::string& obj, const std::string& key, double& out) {
  const std::string pat = "\"" + key + "\":";
  size_t pos = obj.find(pat);
  if (pos == std::string::npos) return false;
  pos += pat.size();
  size_t end = pos;
  while (end < obj.size() && obj[end] != ',' && obj[end] != '}') ++end;
  try {
    out = std::stod(trimCopy(obj.substr(pos, end - pos)));
    return std::isfinite(out);
  } catch (...) {
    return false;
  }
}

bool extractJsonInt(const std::string& obj, const std::string& key, int& out) {
  double v = 0.0;
  if (!extractJsonDouble(obj, key, v)) return false;
  out = static_cast<int>(std::llround(v));
  return true;
}

size_t findJsonObjectEnd(const std::string& data, size_t openBracePos, size_t limit) {
  bool inString = false;
  bool escaped = false;
  int depth = 0;
  for (size_t i = openBracePos; i < limit; ++i) {
    const char c = data[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (c == '\\' && inString) {
      escaped = true;
      continue;
    }
    if (c == '"') {
      inString = !inString;
      continue;
    }
    if (!inString) {
      if (c == '{') {
        ++depth;
      } else if (c == '}') {
        --depth;
        if (depth == 0) return i;
      }
    }
  }
  return std::string::npos;
}

}  // namespace

DeepSpaceEvent SpaceEventsManager::createEventForType(SpaceEventType type,
                                                      const std::string& stationId,
                                                      uint64_t seed) {
  const uint64_t h0 = splitMix64(seed ^ fnv1a(stationId) ^ (m_nextSeq * 0x9E3779B1ULL));
  const uint64_t h1 = splitMix64(h0 ^ 0xA5A5A5A5A5A5A5A5ULL);
  const uint64_t h2 = splitMix64(h1 ^ 0x5A5A5A5A5A5A5A5AULL);

  DeepSpaceEvent ev;
  ev.seed = static_cast<uint32_t>((h0 & 0x7FFFFFFFULL) | 1u);
  ev.stationId = stationId.empty() ? "sol" : stationId;
  ev.type = type;
  ev.status = SpaceEventStatus::Active;

  char idBuf[64];
  std::snprintf(idBuf, sizeof(idBuf), "evt_%s_%llu_%u",
                ev.stationId.c_str(),
                static_cast<unsigned long long>(m_nextSeq++),
                ev.seed % 997u);
  ev.id = idBuf;

  const double az = hashUnit(h0) * 2.0 * kPi;
  const double elev = (hashUnit(h1) - 0.5) * 0.6;
  const double dist = 5.5 + hashUnit(h2) * 5.0;
  const Vec3 basePos{
      dist * std::cos(az) * std::cos(elev),
      dist * std::sin(elev),
      dist * std::sin(az) * std::cos(elev)};

  if (type == SpaceEventType::SosBeacon) {
    ev.titleRu = "Аварийный маяк SOS-7";
    ev.titleEn = "Distress Beacon SOS-7";
    ev.descRu = "Капсула научного курьера потеряла стабилизацию. Подлетите ближе и синхронизируйте маяк [F].";
    ev.descEn = "Science courier pod lost attitude lock. Approach within range and stabilize beacon [F].";
    ev.pos = basePos;
    ev.vel = {0.0, 0.02 * std::sin(az), 0.0};
    ev.targetPos = basePos;
    ev.interactRadius = 2.2;
    ev.requiredProgress = 2.0;
    ev.totalDurationSec = 210.0;
    ev.timeRemainingSec = 210.0;
    ev.rewardRP = 160 + static_cast<int>(h1 % 61ULL);
  } else if (type == SpaceEventType::RogueAsteroid) {
    ev.titleRu = "Болид на траектории перехвата";
    ev.titleEn = "Rogue Asteroid Trajectory";
    ev.descRu = "Железоникелевый болид сближается с секторной орбитой. Сблизьтесь и включите дефлектор [F].";
    ev.descEn = "Iron-nickel rogue asteroid approaching sector orbit. Intercept and fire deflector pulse [F].";
    const double astDist = 9.0 + hashUnit(h2) * 3.5;
    ev.pos = {astDist * std::cos(az), (hashUnit(h1) - 0.5) * 2.0, astDist * std::sin(az)};
    ev.targetPos = {0.0, 0.0, 0.0};
    const double speed = 0.045;
    const double len = std::max(vecDist(ev.pos, ev.targetPos), 1e-4);
    ev.vel = {
        (ev.targetPos.x - ev.pos.x) / len * speed,
        (ev.targetPos.y - ev.pos.y) / len * speed,
        (ev.targetPos.z - ev.pos.z) / len * speed};
    ev.interactRadius = 2.6;
    ev.requiredProgress = 2.5;
    ev.totalDurationSec = 180.0;
    ev.timeRemainingSec = 180.0;
    ev.rewardRP = 220 + static_cast<int>(h2 % 81ULL);
  } else {
    ev.titleRu = "Дрейфующий зонд-реликт";
    ev.titleEn = "Drifting Relic Probe";
    ev.descRu = "Древний квантовый зонд хранит архив метрики. Подлетите вплотную и скачайте телеметрию [F].";
    ev.descEn = "Ancient quantum probe carries metric archives. Move close to download telemetry [F].";
    ev.pos = {basePos.x * 1.15, basePos.y + 0.8, basePos.z * 1.15};
    ev.vel = {-0.012 * std::sin(az), 0.004, 0.012 * std::cos(az)};
    ev.targetPos = ev.pos;
    ev.interactRadius = 1.9;
    ev.requiredProgress = 2.2;
    ev.totalDurationSec = 240.0;
    ev.timeRemainingSec = 240.0;
    ev.rewardRP = 190 + static_cast<int>(h0 % 71ULL);
  }
  return ev;
}

void SpaceEventsManager::update(double dt, const std::string& stationId, bool activeGameplay) {
  if (!activeGameplay || dt <= 0.0) return;
  const double stepDt = std::min(dt, 0.25);

  for (auto& ev : m_events) {
    if (ev.status != SpaceEventStatus::Active) continue;
    if (!stationId.empty() && ev.stationId != stationId) continue;

    ev.pos.x += ev.vel.x * stepDt;
    ev.pos.y += ev.vel.y * stepDt;
    ev.pos.z += ev.vel.z * stepDt;

    ev.timeRemainingSec -= stepDt;
    if (ev.totalDurationSec > 0.0) {
      ev.integrityPct = static_cast<float>(
          std::clamp((ev.timeRemainingSec / ev.totalDurationSec) * 100.0, 0.0, 100.0));
    }

    if (ev.timeRemainingSec <= 0.0) {
      ev.timeRemainingSec = 0.0;
      ev.status = SpaceEventStatus::Expired;
    } else if (ev.type == SpaceEventType::RogueAsteroid &&
               vecDist(ev.pos, ev.targetPos) < 0.75) {
      ev.timeRemainingSec = 0.0;
      ev.status = SpaceEventStatus::Expired;
    }
  }
}

int SpaceEventsManager::ensureSectorEvents(const std::string& stationId, uint64_t seed) {
  const std::string st = stationId.empty() ? "sol" : stationId;
  if (activeCount(st) > 0) return 0;

  // Avoid spawning a second automated wave if this sector already resolved/expired events in-session,
  // unless player explicitly triggers pingSectorScanner().
  for (const auto& ev : m_events) {
    if (ev.stationId == st) return 0;
  }

  const uint64_t h = splitMix64(seed ^ fnv1a(st));
  const SpaceEventType firstType = static_cast<SpaceEventType>(h % 3ULL);
  const SpaceEventType secondType = static_cast<SpaceEventType>((h + 1ULL) % 3ULL);
  m_events.push_back(createEventForType(firstType, st, seed));
  m_events.push_back(createEventForType(secondType, st, seed + 17ULL));
  return 2;
}

bool SpaceEventsManager::pingSectorScanner(const std::string& stationId, uint64_t seed) {
  const std::string st = stationId.empty() ? "sol" : stationId;
  if (activeCount(st) >= 3) return false;

  const uint64_t h = splitMix64(seed ^ fnv1a(st) ^ (m_nextSeq * 31337ULL));
  const SpaceEventType type = static_cast<SpaceEventType>(h % 3ULL);
  m_events.push_back(createEventForType(type, st, h));
  return true;
}

const DeepSpaceEvent* SpaceEventsManager::findNearestActive(const Vec3& shipPos,
                                                            const std::string& stationId,
                                                            double* outDist) const {
  const DeepSpaceEvent* best = nullptr;
  double bestDist = 1e18;
  for (const auto& ev : m_events) {
    if (ev.status != SpaceEventStatus::Active) continue;
    if (!stationId.empty() && ev.stationId != stationId) continue;
    const double d = vecDist(shipPos, ev.pos);
    if (d < bestDist) {
      bestDist = d;
      best = &ev;
    }
  }
  if (outDist) *outDist = (best != nullptr) ? bestDist : 0.0;
  return best;
}

void SpaceEventsManager::rewardEvent(DeepSpaceEvent& ev,
                                     ScienceCodex* codex,
                                     BridgeComms* comms,
                                     ZettelGraph* zettel) {
  if (codex) {
    codex->addResearchPoints(ev.rewardRP);
  }
  if (comms) {
    RadioMessage msg;
    msg.id = "ev_" + ev.id;
    msg.callsignRu = "ТАКТИЧЕСКИЙ РАДАР";
    msg.callsignEn = "TACTICAL RADAR";
    if (ev.type == SpaceEventType::SosBeacon) {
      msg.textRu = "Маяк SOS стабилизирован! Экипаж капсулы в безопасности (+" +
                   std::to_string(ev.rewardRP) + " RP).";
      msg.textEn = "Distress beacon stabilized! Courier crew safe (+" +
                   std::to_string(ev.rewardRP) + " RP).";
    } else if (ev.type == SpaceEventType::RogueAsteroid) {
      msg.textRu = "Угроза столкновения снята: болид отклонён гравитационным импульсом (+" +
                   std::to_string(ev.rewardRP) + " RP).";
      msg.textEn = "Impact averted: rogue asteroid deflected off orbital path (+" +
                   std::to_string(ev.rewardRP) + " RP).";
    } else {
      msg.textRu = "Телеметрия древнего зонда расшифрована и добавлена в архив (+" +
                   std::to_string(ev.rewardRP) + " RP).";
      msg.textEn = "Ancient probe telemetry decoded and archived (+" +
                   std::to_string(ev.rewardRP) + " RP).";
    }
    msg.priority = 1.8f;
    msg.durationSec = 7.0;
    comms->queueMessage(msg, 0.0);
  }
  if (zettel && ev.type == SpaceEventType::RelicProbe) {
    zettel->addCustomNote(
        ev.titleRu + " [" + ev.stationId + "]",
        "Телеметрия дрейфующего зонда-реликта: обнаружены аномалии фазы метрики (+" +
            std::to_string(ev.rewardRP) + " RP).",
        "Эврика");
  }
}

bool SpaceEventsManager::engageEvent(const std::string& eventId,
                                     const Vec3& shipPos,
                                     double dt,
                                     ScienceCodex* codex,
                                     BridgeComms* comms,
                                     ZettelGraph* zettel) {
  if (dt <= 0.0) return false;
  for (auto& ev : m_events) {
    if (ev.id != eventId || ev.status != SpaceEventStatus::Active) continue;
    const double d = vecDist(shipPos, ev.pos);
    if (d > ev.interactRadius) return false;

    ev.progress = std::min(ev.requiredProgress, ev.progress + dt);
    if (ev.type == SpaceEventType::RogueAsteroid) {
      // Deflector pulse steers asteroid orthogonally around target orbit + radially outward
      const double tx = ev.pos.x - ev.targetPos.x;
      const double tz = ev.pos.z - ev.targetPos.z;
      const double tLen = std::sqrt(tx * tx + tz * tz);
      const double dirX = (tLen > 1e-4) ? (-tz / tLen) : 1.0;
      const double dirZ = (tLen > 1e-4) ? (tx / tLen) : 0.0;
      const double radX = (tLen > 1e-4) ? (tx / tLen) : 0.0;
      const double radZ = (tLen > 1e-4) ? (tz / tLen) : 1.0;
      ev.vel.x += (dirX * 0.075 + radX * 0.045) * dt;
      ev.vel.z += (dirZ * 0.075 + radZ * 0.045) * dt;
    }
    if (ev.progress >= ev.requiredProgress) {
      ev.status = SpaceEventStatus::Resolved;
      rewardEvent(ev, codex, comms, zettel);
    }
    return true;
  }
  return false;
}

int SpaceEventsManager::activeCount(const std::string& stationId) const {
  int count = 0;
  for (const auto& ev : m_events) {
    if (ev.status != SpaceEventStatus::Active) continue;
    if (!stationId.empty() && ev.stationId != stationId) continue;
    ++count;
  }
  return count;
}

int SpaceEventsManager::resolvedCount() const {
  int count = 0;
  for (const auto& ev : m_events) {
    if (ev.status == SpaceEventStatus::Resolved) ++count;
  }
  return count;
}

int SpaceEventsManager::expiredCount() const {
  int count = 0;
  for (const auto& ev : m_events) {
    if (ev.status == SpaceEventStatus::Expired) ++count;
  }
  return count;
}

std::string SpaceEventsManager::serialize() const {
  std::ostringstream os;
  os << std::setprecision(10);
  os << "{\n  \"schema\": 1,\n  \"nextSeq\": " << m_nextSeq << ",\n  \"events\": [\n";
  for (size_t i = 0; i < m_events.size(); ++i) {
    const auto& ev = m_events[i];
    os << "    {"
       << "\"id\":\"" << jsonEscape(ev.id) << "\","
       << "\"type\":" << static_cast<int>(ev.type) << ","
       << "\"status\":" << static_cast<int>(ev.status) << ","
       << "\"stationId\":\"" << jsonEscape(ev.stationId) << "\","
       << "\"titleRu\":\"" << jsonEscape(ev.titleRu) << "\","
       << "\"titleEn\":\"" << jsonEscape(ev.titleEn) << "\","
       << "\"descRu\":\"" << jsonEscape(ev.descRu) << "\","
       << "\"descEn\":\"" << jsonEscape(ev.descEn) << "\","
       << "\"px\":" << ev.pos.x << ",\"py\":" << ev.pos.y << ",\"pz\":" << ev.pos.z << ","
       << "\"vx\":" << ev.vel.x << ",\"vy\":" << ev.vel.y << ",\"vz\":" << ev.vel.z << ","
       << "\"tx\":" << ev.targetPos.x << ",\"ty\":" << ev.targetPos.y << ",\"tz\":" << ev.targetPos.z << ","
       << "\"radius\":" << ev.interactRadius << ","
       << "\"progress\":" << ev.progress << ","
       << "\"req\":" << ev.requiredProgress << ","
       << "\"rem\":" << ev.timeRemainingSec << ","
       << "\"dur\":" << ev.totalDurationSec << ","
       << "\"rp\":" << ev.rewardRP << ","
       << "\"seed\":" << ev.seed
       << "}" << (i + 1 < m_events.size() ? "," : "") << "\n";
  }
  os << "  ]\n}\n";
  return os.str();
}

bool SpaceEventsManager::deserialize(const std::string& data) {
  if (data.find("\"events\"") == std::string::npos) return false;

  std::vector<DeepSpaceEvent> loaded;
  uint64_t nextSeq = 1;
  double seqVal = 1.0;
  if (extractJsonDouble(data, "nextSeq", seqVal) && seqVal >= 1.0) {
    nextSeq = static_cast<uint64_t>(seqVal);
  }

  const size_t arrStart = data.find('[');
  const size_t arrEnd = data.rfind(']');
  if (arrStart == std::string::npos || arrEnd == std::string::npos || arrEnd <= arrStart) {
    return false;
  }

  size_t cur = arrStart;
  while (cur < arrEnd) {
    const size_t ob = data.find('{', cur);
    if (ob == std::string::npos || ob >= arrEnd) break;
    const size_t cb = findJsonObjectEnd(data, ob, arrEnd);
    if (cb == std::string::npos || cb > arrEnd) break;
    const std::string obj = data.substr(ob, cb - ob + 1);

    DeepSpaceEvent ev;
    int tInt = 0;
    int sInt = 0;
    double seedVal = 1.0;
    if (extractJsonString(obj, "id", ev.id) &&
        extractJsonString(obj, "stationId", ev.stationId)) {
      extractJsonInt(obj, "type", tInt);
      extractJsonInt(obj, "status", sInt);
      ev.type = static_cast<SpaceEventType>(std::clamp(tInt, 0, 2));
      ev.status = static_cast<SpaceEventStatus>(std::clamp(sInt, 0, 2));
      extractJsonString(obj, "titleRu", ev.titleRu);
      extractJsonString(obj, "titleEn", ev.titleEn);
      extractJsonString(obj, "descRu", ev.descRu);
      extractJsonString(obj, "descEn", ev.descEn);
      extractJsonDouble(obj, "px", ev.pos.x);
      extractJsonDouble(obj, "py", ev.pos.y);
      extractJsonDouble(obj, "pz", ev.pos.z);
      extractJsonDouble(obj, "vx", ev.vel.x);
      extractJsonDouble(obj, "vy", ev.vel.y);
      extractJsonDouble(obj, "vz", ev.vel.z);
      extractJsonDouble(obj, "tx", ev.targetPos.x);
      extractJsonDouble(obj, "ty", ev.targetPos.y);
      extractJsonDouble(obj, "tz", ev.targetPos.z);
      extractJsonDouble(obj, "radius", ev.interactRadius);
      extractJsonDouble(obj, "progress", ev.progress);
      extractJsonDouble(obj, "req", ev.requiredProgress);
      extractJsonDouble(obj, "rem", ev.timeRemainingSec);
      extractJsonDouble(obj, "dur", ev.totalDurationSec);
      extractJsonInt(obj, "rp", ev.rewardRP);
      if (extractJsonDouble(obj, "seed", seedVal) && seedVal >= 1.0) {
        ev.seed = static_cast<uint32_t>(seedVal);
      }
      if (ev.totalDurationSec > 0.0) {
        ev.integrityPct = static_cast<float>(
            std::clamp((ev.timeRemainingSec / ev.totalDurationSec) * 100.0, 0.0, 100.0));
      }
      loaded.push_back(std::move(ev));
    }
    cur = cb + 1;
  }

  m_events = std::move(loaded);
  m_nextSeq = std::max<uint64_t>(nextSeq, m_events.size() + 1);
  return true;
}

void SpaceEventsManager::clear() {
  m_events.clear();
  m_nextSeq = 1;
}

}  // namespace astro
