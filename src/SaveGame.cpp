#include "SaveGame.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

namespace astro {

namespace {

// --- Мини-JSON: value-дерево + рекурсивный парсер + pretty-писатель.
// Свой, без зависимостей: файлов профиля мало и они наши — нужны
// терпимость к порядку полей, \u-эскейпы и версионирование схемы.
struct JVal {
  enum class Kind { Null, Bool, Num, Str, Arr, Obj };
  Kind kind = Kind::Null;
  bool b = false;
  double num = 0.0;
  std::string str;
  std::vector<JVal> arr;
  std::vector<std::pair<std::string, JVal>> obj;

  const JVal* find(const std::string& key) const {
    if (kind != Kind::Obj) return nullptr;
    for (const auto& kv : obj) {
      if (kv.first == key) return &kv.second;
    }
    return nullptr;
  }
};

void appendUtf8(std::string& out, unsigned int cp) {
  if (cp <= 0x7F) {
    out += static_cast<char>(cp);
  } else if (cp <= 0x7FF) {
    out += static_cast<char>(0xC0 | (cp >> 6));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp <= 0xFFFF) {
    out += static_cast<char>(0xE0 | (cp >> 12));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    out += static_cast<char>(0xF0 | (cp >> 18));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  }
}

class JParser {
 public:
  explicit JParser(const std::string& s) : m_s(s) {}

  bool parse(JVal& out) {
    skipWs();
    if (!parseValue(out, 0)) return false;
    skipWs();
    if (m_p != m_s.size()) return fail("trailing characters");
    return true;
  }

  const std::string& error() const { return m_err; }

 private:
  bool fail(const std::string& msg) {
    if (m_err.empty()) {
      m_err = msg + " (byte " + std::to_string(m_p) + ")";
    }
    return false;
  }

  void skipWs() {
    while (m_p < m_s.size() &&
           (m_s[m_p] == ' ' || m_s[m_p] == '\t' || m_s[m_p] == '\r' ||
            m_s[m_p] == '\n')) {
      ++m_p;
    }
  }

  bool parseValue(JVal& out, int depth) {
    if (depth > 64) return fail("nesting too deep");
    if (m_p >= m_s.size()) return fail("unexpected end of input");
    const char c = m_s[m_p];
    if (c == '{') return parseObject(out, depth);
    if (c == '[') return parseArray(out, depth);
    if (c == '"') {
      out.kind = JVal::Kind::Str;
      return parseString(out.str);
    }
    if (m_s.compare(m_p, 4, "true") == 0) {
      m_p += 4;
      out.kind = JVal::Kind::Bool;
      out.b = true;
      return true;
    }
    if (m_s.compare(m_p, 5, "false") == 0) {
      m_p += 5;
      out.kind = JVal::Kind::Bool;
      out.b = false;
      return true;
    }
    if (m_s.compare(m_p, 4, "null") == 0) {
      m_p += 4;
      out.kind = JVal::Kind::Null;
      return true;
    }
    if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(out);
    return fail("unexpected character");
  }

  bool parseObject(JVal& out, int depth) {
    out.kind = JVal::Kind::Obj;
    ++m_p;  // '{'
    skipWs();
    if (m_p < m_s.size() && m_s[m_p] == '}') {
      ++m_p;
      return true;
    }
    while (true) {
      skipWs();
      if (m_p >= m_s.size() || m_s[m_p] != '"') {
        return fail("expected key string");
      }
      std::string key;
      if (!parseString(key)) return false;
      skipWs();
      if (m_p >= m_s.size() || m_s[m_p] != ':') {
        return fail("expected ':' after key");
      }
      ++m_p;
      skipWs();
      JVal v;
      if (!parseValue(v, depth + 1)) return false;
      out.obj.emplace_back(std::move(key), std::move(v));
      skipWs();
      if (m_p >= m_s.size()) return fail("unterminated object");
      if (m_s[m_p] == ',') {
        ++m_p;
        continue;
      }
      if (m_s[m_p] == '}') {
        ++m_p;
        return true;
      }
      return fail("expected ',' or '}'");
    }
  }

  bool parseArray(JVal& out, int depth) {
    out.kind = JVal::Kind::Arr;
    ++m_p;  // '['
    skipWs();
    if (m_p < m_s.size() && m_s[m_p] == ']') {
      ++m_p;
      return true;
    }
    while (true) {
      skipWs();
      JVal v;
      if (!parseValue(v, depth + 1)) return false;
      out.arr.push_back(std::move(v));
      skipWs();
      if (m_p >= m_s.size()) return fail("unterminated array");
      if (m_s[m_p] == ',') {
        ++m_p;
        continue;
      }
      if (m_s[m_p] == ']') {
        ++m_p;
        return true;
      }
      return fail("expected ',' or ']'");
    }
  }

  bool parseHex4(unsigned int& out) {
    if (m_p + 4 > m_s.size()) return fail("truncated \\u escape");
    unsigned int v = 0;
    for (int i = 0; i < 4; ++i) {
      const char c = m_s[m_p++];
      v <<= 4;
      if (c >= '0' && c <= '9') {
        v |= static_cast<unsigned int>(c - '0');
      } else if (c >= 'a' && c <= 'f') {
        v |= static_cast<unsigned int>(c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        v |= static_cast<unsigned int>(c - 'A' + 10);
      } else {
        return fail("bad hex digit in \\u escape");
      }
    }
    out = v;
    return true;
  }

  bool parseString(std::string& out) {
    ++m_p;  // opening quote
    out.clear();
    while (m_p < m_s.size()) {
      const unsigned char c = static_cast<unsigned char>(m_s[m_p]);
      if (c == '"') {
        ++m_p;
        return true;
      }
      if (c == '\\') {
        ++m_p;
        if (m_p >= m_s.size()) return fail("unterminated escape");
        const char e = m_s[m_p++];
        switch (e) {
          case '"': out += '"'; break;
          case '\\': out += '\\'; break;
          case '/': out += '/'; break;
          case 'b': out += '\b'; break;
          case 'f': out += '\f'; break;
          case 'n': out += '\n'; break;
          case 'r': out += '\r'; break;
          case 't': out += '\t'; break;
          case 'u': {
            unsigned int cp = 0;
            if (!parseHex4(cp)) return false;
            if (cp >= 0xD800 && cp <= 0xDBFF) {  // суррогатная пара
              if (m_p + 1 < m_s.size() && m_s[m_p] == '\\' && m_s[m_p + 1] == 'u') {
                m_p += 2;
                unsigned int lo = 0;
                if (!parseHex4(lo)) return false;
                if (lo < 0xDC00 || lo > 0xDFFF) {
                  return fail("invalid low surrogate");
                }
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
              } else {
                return fail("unpaired high surrogate");
              }
            }
            appendUtf8(out, cp);
            break;
          }
          default:
            return fail("bad escape character");
        }
      } else if (c < 0x20) {
        return fail("raw control character in string");
      } else {
        out += static_cast<char>(c);
        ++m_p;
      }
    }
    return fail("unterminated string");
  }

  bool parseNumber(JVal& out) {
    const char* begin = m_s.c_str() + m_p;
    char* end = nullptr;
    const double v = std::strtod(begin, &end);
    if (end == begin) return fail("invalid number");
    m_p += static_cast<size_t>(end - begin);
    out.kind = JVal::Kind::Num;
    out.num = v;
    return true;
  }

  const std::string& m_s;
  size_t m_p = 0;
  std::string m_err;
};

// --- Писатель (pretty, 2 пробела) ---
std::string ind(int n) { return std::string(static_cast<size_t>(n) * 2, ' '); }

void writeEscaped(std::string& out, const std::string& s) {
  out += '"';
  for (const char ch : s) {
    const unsigned char c = static_cast<unsigned char>(ch);
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  out += '"';
}

std::string numToJson(double v) {
  if (v >= -9.0e15 && v <= 9.0e15) {
    const long long ll = static_cast<long long>(v);
    if (static_cast<double>(ll) == v) return std::to_string(ll);
  }
  char buf[40];
  std::snprintf(buf, sizeof(buf), "%.9g", v);
  return std::string(buf);
}

void writeVec3(std::string& out, const Vec3& v) {
  out += '[';
  out += numToJson(v.x);
  out += ", ";
  out += numToJson(v.y);
  out += ", ";
  out += numToJson(v.z);
  out += ']';
}

void writeStrArray(std::string& out, const std::vector<std::string>& v) {
  out += '[';
  for (size_t i = 0; i < v.size(); ++i) {
    if (i > 0) out += ", ";
    writeEscaped(out, v[i]);
  }
  out += ']';
}

// --- Чтение полей с дефолтами (терпимость к отсутствию/типу) ---
const JVal* findOf(const JVal& obj, const std::string& key, JVal::Kind kind) {
  const JVal* v = obj.find(key);
  if (!v || v->kind != kind) return nullptr;
  return v;
}

int readInt(const JVal& obj, const std::string& key, int def) {
  const JVal* v = findOf(obj, key, JVal::Kind::Num);
  return v ? static_cast<int>(v->num) : def;
}

double readNum(const JVal& obj, const std::string& key, double def) {
  const JVal* v = findOf(obj, key, JVal::Kind::Num);
  return v ? v->num : def;
}

float readFloat(const JVal& obj, const std::string& key, float def) {
  return static_cast<float>(readNum(obj, key, static_cast<double>(def)));
}

bool readBool(const JVal& obj, const std::string& key, bool def) {
  const JVal* v = findOf(obj, key, JVal::Kind::Bool);
  return v ? v->b : def;
}

std::string readStr(const JVal& obj, const std::string& key,
                    const std::string& def) {
  const JVal* v = findOf(obj, key, JVal::Kind::Str);
  return v ? v->str : def;
}

void readStrArray(const JVal& obj, const std::string& key,
                  std::vector<std::string>& out) {
  const JVal* v = findOf(obj, key, JVal::Kind::Arr);
  if (!v) return;
  for (const JVal& e : v->arr) {
    if (e.kind == JVal::Kind::Str) out.push_back(e.str);
  }
}

void readVec3(const JVal& obj, const std::string& key, Vec3& out) {
  const JVal* v = findOf(obj, key, JVal::Kind::Arr);
  if (!v || v->arr.size() < 3) return;
  if (v->arr[0].kind == JVal::Kind::Num) out.x = v->arr[0].num;
  if (v->arr[1].kind == JVal::Kind::Num) out.y = v->arr[1].num;
  if (v->arr[2].kind == JVal::Kind::Num) out.z = v->arr[2].num;
}

bool readSchema(const JVal& root, int& out, std::string& err) {
  const JVal* sc = findOf(root, "schema", JVal::Kind::Num);
  if (!sc) {
    err = "missing schema";
    return false;
  }
  const int schema = static_cast<int>(sc->num);
  if (schema < 1) {
    err = "invalid schema " + std::to_string(schema);
    return false;
  }
  if (schema > kSaveSchema) {
    err = "save schema " + std::to_string(schema) +
          " is newer than supported " + std::to_string(kSaveSchema);
    return false;
  }
  out = schema;
  return true;
}

// Безопасное чтение env-переменной: на MSVC _dupenv_s (без C4996),
// на остальных платформах обычный getenv.
std::string envVar(const char* name) {
#if defined(_MSC_VER)
  char* buf = nullptr;
  size_t len = 0;
  if (_dupenv_s(&buf, &len, name) == 0 && buf) {
    std::string v(buf);
    std::free(buf);
    return v;
  }
  return std::string();
#else
  const char* v = std::getenv(name);
  return v ? std::string(v) : std::string();
#endif
}

}  // namespace

std::string saveSerialize(const SaveGame& g) {
  std::string out;
  out += "{\n";
  out += ind(1) + "\"schema\": " + std::to_string(g.schema) + ",\n";
  out += ind(1) + "\"mode\": ";
  writeEscaped(out, saveModeName(g.mode));
  out += ",\n";
  out += ind(1) + "\"savedUtc\": ";
  writeEscaped(out, g.savedUtc);
  out += ",\n";

  out += ind(1) + "\"progress\": {\n";
  out += ind(2) + "\"visited\": ";
  writeStrArray(out, g.progress.visited);
  out += ",\n";
  out += ind(2) + "\"colonies\": ";
  writeStrArray(out, g.progress.colonies);
  out += ",\n";
  out += ind(2) + "\"unlocked\": ";
  writeStrArray(out, g.progress.unlocked);
  out += ",\n";
  out += ind(2) + "\"route\": ";
  writeStrArray(out, g.progress.route);
  out += ",\n";
  out += ind(2) + "\"minBhDistance\": " + numToJson(g.progress.minBhDistance) + ",\n";
  out += ind(2) + std::string("\"exported\": ") + (g.progress.exported ? "true" : "false") + ",\n";
  out += ind(2) + std::string("\"imported\": ") + (g.progress.imported ? "true" : "false") + ",\n";
  out += ind(2) + std::string("\"capturedHorizon\": ") + (g.progress.capturedHorizon ? "true" : "false") + ",\n";
  out += ind(2) + "\"asteroidDodges\": " + std::to_string(g.progress.asteroidDodges) + "\n";
  out += ind(1) + "},\n";

  out += ind(1) + "\"customs\": [";
  for (size_t i = 0; i < g.customs.size(); ++i) {
    const SaveCustom& c = g.customs[i];
    out += "\n";
    out += ind(2) + "{\n";
    out += ind(3) + "\"id\": ";
    writeEscaped(out, c.id);
    out += ",\n";
    out += ind(3) + "\"name\": ";
    writeEscaped(out, c.name);
    out += ",\n";
    out += ind(3) + "\"pos\": ";
    writeVec3(out, c.pos);
    out += ",\n";
    out += ind(3) + "\"vel\": ";
    writeVec3(out, c.vel);
    out += ",\n";
    out += ind(3) + "\"radius\": " + numToJson(c.radius) + ",\n";
    out += ind(3) + "\"color\": ";
    writeVec3(out, c.color);
    out += ",\n";
    out += ind(3) + "\"colorB\": ";
    writeVec3(out, c.colorB);
    out += ",\n";
    out += ind(3) + "\"type\": " + std::to_string(c.type) + ",\n";
    out += ind(3) + "\"detail\": " + std::to_string(c.detail) + ",\n";
    out += ind(3) + "\"ringOuter\": " + numToJson(c.ringOuter) + ",\n";
    out += ind(3) + "\"seed\": " + numToJson(c.seed) + "\n";
    out += ind(2) + "}";
    if (i + 1 < g.customs.size()) out += ",";
  }
  if (!g.customs.empty()) out += "\n";
  out += ind(1) + "],\n";

  out += ind(1) + "\"session\": {\n";
  out += ind(2) + "\"station\": ";
  writeEscaped(out, g.session.station);
  out += ",\n";
  out += ind(2) + "\"camPos\": ";
  writeVec3(out, g.session.camPos);
  out += ",\n";
  out += ind(2) + "\"yaw\": " + numToJson(g.session.yaw) + ",\n";
  out += ind(2) + "\"pitch\": " + numToJson(g.session.pitch) + ",\n";
  out += ind(2) + "\"timeScale\": " + numToJson(g.session.timeScale) + ",\n";
  out += ind(2) + "\"fuel\": " + numToJson(g.session.fuel) + ",\n";
  out += ind(2) + "\"shield\": " + numToJson(g.session.shield) + ",\n";
  out += ind(2) + "\"hull\": " + numToJson(g.session.hull) + ",\n";
  out += ind(2) + "\"shipStyle\": " + std::to_string(g.session.shipStyle) + ",\n";
  out += ind(2) + "\"hullTint\": ";
  writeVec3(out, Vec3(g.session.custom.primaryColor[0], g.session.custom.primaryColor[1], g.session.custom.primaryColor[2]));
  out += ",\n";
  out += ind(2) + "\"accentTint\": ";
  writeVec3(out, Vec3(g.session.custom.accentColor[0], g.session.custom.accentColor[1], g.session.custom.accentColor[2]));
  out += ",\n";
  out += ind(2) + "\"emissiveTint\": ";
  writeVec3(out, Vec3(g.session.custom.emissiveColor[0], g.session.custom.emissiveColor[1], g.session.custom.emissiveColor[2]));
  out += ",\n";
  out += ind(2) + "\"ringSpin\": " + numToJson(g.session.custom.ringSpinSpeed) + ",\n";
  out += ind(2) + "\"warpGlow\": " + numToJson(g.session.custom.warpGlowIntensity) + ",\n";
  out += ind(2) + "\"engineUpgrade\": " + std::to_string(g.session.custom.engineUpgrade) + ",\n";
  out += ind(2) + "\"shieldUpgrade\": " + std::to_string(g.session.custom.shieldUpgrade) + ",\n";
  out += ind(2) + "\"warpUpgrade\": " + std::to_string(g.session.custom.warpUpgrade) + "\n";
  out += ind(1) + "}\n";

  out += "}\n";
  return out;
}

bool saveDeserialize(const std::string& text, SaveGame& out, std::string& err) {
  JVal root;
  JParser p(text);
  if (!p.parse(root)) {
    err = p.error();
    return false;
  }
  if (root.kind != JVal::Kind::Obj) {
    err = "root is not an object";
    return false;
  }
  int schema = 0;
  if (!readSchema(root, schema, err)) return false;

  out = SaveGame{};
  out.schema = schema;
  out.mode = (readStr(root, "mode", "sandbox") == "adventure")
                 ? SaveMode::Adventure
                 : SaveMode::Sandbox;
  out.savedUtc = readStr(root, "savedUtc", "");

  if (const JVal* pr = findOf(root, "progress", JVal::Kind::Obj)) {
    readStrArray(*pr, "visited", out.progress.visited);
    readStrArray(*pr, "colonies", out.progress.colonies);
    readStrArray(*pr, "unlocked", out.progress.unlocked);
    readStrArray(*pr, "route", out.progress.route);
    out.progress.minBhDistance = readNum(*pr, "minBhDistance", out.progress.minBhDistance);
    out.progress.exported = readBool(*pr, "exported", false);
    out.progress.imported = readBool(*pr, "imported", false);
    out.progress.capturedHorizon = readBool(*pr, "capturedHorizon", false);
    out.progress.asteroidDodges = readInt(*pr, "asteroidDodges", 0);
  }

  if (const JVal* cu = findOf(root, "customs", JVal::Kind::Arr)) {
    for (const JVal& e : cu->arr) {
      if (e.kind != JVal::Kind::Obj) continue;
      SaveCustom c;
      c.id = readStr(e, "id", "");
      c.name = readStr(e, "name", "");
      readVec3(e, "pos", c.pos);
      readVec3(e, "vel", c.vel);
      c.radius = readFloat(e, "radius", c.radius);
      readVec3(e, "color", c.color);
      readVec3(e, "colorB", c.colorB);
      c.type = readInt(e, "type", c.type);
      c.detail = readInt(e, "detail", c.detail);
      c.ringOuter = readFloat(e, "ringOuter", c.ringOuter);
      c.seed = readFloat(e, "seed", c.seed);
      out.customs.push_back(std::move(c));
    }
  }

  if (const JVal* se = findOf(root, "session", JVal::Kind::Obj)) {
    out.session.station = readStr(*se, "station", out.session.station);
    readVec3(*se, "camPos", out.session.camPos);
    out.session.yaw = readNum(*se, "yaw", out.session.yaw);
    out.session.pitch = readNum(*se, "pitch", out.session.pitch);
    out.session.timeScale = readFloat(*se, "timeScale", out.session.timeScale);
    out.session.fuel = readFloat(*se, "fuel", out.session.fuel);
    out.session.shield = readFloat(*se, "shield", out.session.shield);
    out.session.hull = readFloat(*se, "hull", out.session.hull);
    out.session.shipStyle = readInt(*se, "shipStyle", out.session.shipStyle);
    out.session.custom.style = out.session.shipStyle;
    Vec3 ht(out.session.custom.primaryColor[0], out.session.custom.primaryColor[1], out.session.custom.primaryColor[2]);
    readVec3(*se, "hullTint", ht);
    out.session.custom.primaryColor[0] = static_cast<float>(ht.x);
    out.session.custom.primaryColor[1] = static_cast<float>(ht.y);
    out.session.custom.primaryColor[2] = static_cast<float>(ht.z);
    Vec3 at(out.session.custom.accentColor[0], out.session.custom.accentColor[1], out.session.custom.accentColor[2]);
    readVec3(*se, "accentTint", at);
    out.session.custom.accentColor[0] = static_cast<float>(at.x);
    out.session.custom.accentColor[1] = static_cast<float>(at.y);
    out.session.custom.accentColor[2] = static_cast<float>(at.z);
    Vec3 et(out.session.custom.emissiveColor[0], out.session.custom.emissiveColor[1], out.session.custom.emissiveColor[2]);
    readVec3(*se, "emissiveTint", et);
    out.session.custom.emissiveColor[0] = static_cast<float>(et.x);
    out.session.custom.emissiveColor[1] = static_cast<float>(et.y);
    out.session.custom.emissiveColor[2] = static_cast<float>(et.z);
    out.session.custom.ringSpinSpeed = readFloat(*se, "ringSpin", out.session.custom.ringSpinSpeed);
    out.session.custom.warpGlowIntensity = readFloat(*se, "warpGlow", out.session.custom.warpGlowIntensity);
    out.session.custom.engineUpgrade = readInt(*se, "engineUpgrade", out.session.custom.engineUpgrade);
    out.session.custom.shieldUpgrade = readInt(*se, "shieldUpgrade", out.session.custom.shieldUpgrade);
    out.session.custom.warpUpgrade = readInt(*se, "warpUpgrade", out.session.custom.warpUpgrade);
  }
  return true;
}

std::string saveSerializeSettings(const SaveSettings& s) {
  std::string out;
  out += "{\n";
  out += ind(1) + "\"schema\": " + std::to_string(kSaveSchema) + ",\n";
  out += ind(1) + "\"perfPreset\": " + std::to_string(s.perfPreset) + ",\n";
  out += ind(1) + "\"audioVolume\": " + std::to_string(s.audioVolume) + ",\n";
  out += ind(1) + std::string("\"muted\": ") + (s.muted ? "true" : "false") + ",\n";
  out += ind(1) + "\"viewMode\": " + std::to_string(s.viewMode) + ",\n";
  out += ind(1) + "\"difficulty\": " + std::to_string(s.difficulty) + ",\n";
  out += ind(1) + "\"aaMode\": " + std::to_string(s.aaMode) + ",\n";
  out += ind(1) + std::string("\"shipReflections\": ") +
         (s.shipReflections ? "true" : "false") + ",\n";
  out += ind(1) + "\"enginePalette\": " + std::to_string(s.enginePalette) + ",\n";
  out += ind(1) + "\"renderScale\": " + numToJson(s.renderScale) + ",\n";
  out += ind(1) + "\"shellRadius\": " + numToJson(s.shellRadius) + ",\n";
  out += ind(1) + "\"stepSize\": " + numToJson(s.stepSize) + ",\n";
  out += ind(1) + "\"maxSteps\": " + std::to_string(s.maxSteps) + ",\n";
  out += ind(1) + "\"mouseSensitivity\": " + numToJson(s.mouseSensitivity) + ",\n";
  out += ind(1) + "\"invertY\": " + std::to_string(s.invertY) + ",\n";
  out += ind(1) + "\"keyColony\": " + std::to_string(s.keyColony) + ",\n";
  out += ind(1) + "\"keyView\": " + std::to_string(s.keyView) + ",\n";
  out += ind(1) + "\"keyMap\": " + std::to_string(s.keyMap) + ",\n";
  out += ind(1) + "\"keyJournal\": " + std::to_string(s.keyJournal) + ",\n";
  out += ind(1) + "\"keyTour\": " + std::to_string(s.keyTour) + "\n";
  out += "}\n";
  return out;
}

bool saveDeserializeSettings(const std::string& text, SaveSettings& out,
                             std::string& err) {
  JVal root;
  JParser p(text);
  if (!p.parse(root)) {
    err = p.error();
    return false;
  }
  if (root.kind != JVal::Kind::Obj) {
    err = "root is not an object";
    return false;
  }
  int schema = 0;
  if (!readSchema(root, schema, err)) return false;

  out = SaveSettings{};
  out.perfPreset = readInt(root, "perfPreset", out.perfPreset);
  out.audioVolume = readInt(root, "audioVolume", out.audioVolume);
  out.muted = readBool(root, "muted", out.muted);
  out.viewMode = readInt(root, "viewMode", out.viewMode);
  out.difficulty = readInt(root, "difficulty", out.difficulty);
  out.aaMode = readInt(root, "aaMode", out.aaMode);
  out.shipReflections = readBool(root, "shipReflections", out.shipReflections);
  out.enginePalette = readInt(root, "enginePalette", out.enginePalette);
  out.renderScale = readFloat(root, "renderScale", out.renderScale);
  out.shellRadius = readFloat(root, "shellRadius", out.shellRadius);
  out.stepSize = readFloat(root, "stepSize", out.stepSize);
  out.maxSteps = readInt(root, "maxSteps", out.maxSteps);
  out.mouseSensitivity = readFloat(root, "mouseSensitivity", out.mouseSensitivity);
  out.invertY = readInt(root, "invertY", out.invertY);
  out.keyColony = readInt(root, "keyColony", out.keyColony);
  out.keyView = readInt(root, "keyView", out.keyView);
  out.keyMap = readInt(root, "keyMap", out.keyMap);
  out.keyJournal = readInt(root, "keyJournal", out.keyJournal);
  out.keyTour = readInt(root, "keyTour", out.keyTour);
  return true;
}

std::string saveProfileDir() {
  std::string root;
#if defined(_WIN32)
  const std::string appdata = envVar("APPDATA");
  root = appdata.empty() ? "." : appdata;
#elif defined(__APPLE__)
  const std::string home = envVar("HOME");
  root = home.empty() ? "." : home + "/Library/Application Support";
#else
  const std::string xdg = envVar("XDG_CONFIG_HOME");
  if (!xdg.empty()) {
    root = xdg;
  } else {
    const std::string home = envVar("HOME");
    root = home.empty() ? "." : home + "/.config";
  }
#endif
  return root + "/astro-voyager";
}

std::string saveFilePathFor(const std::string& dir, SaveMode mode) {
  return dir + "/" + (mode == SaveMode::Adventure ? "save-adventure.json"
                                                  : "save-sandbox.json");
}

std::string saveSettingsPathFor(const std::string& dir) {
  return dir + "/settings.json";
}

std::string saveFilePath(SaveMode mode) {
  return saveFilePathFor(saveProfileDir(), mode);
}

std::string saveSettingsFilePath() {
  return saveSettingsPathFor(saveProfileDir());
}

const char* saveModeName(SaveMode mode) {
  return mode == SaveMode::Adventure ? "adventure" : "sandbox";
}

bool saveWriteFile(const std::string& path, const std::string& text,
                   std::string& err) {
  err.clear();
  const std::filesystem::path p(path);
  if (p.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec) {
      err = "cannot create directory " + p.parent_path().string() + ": " +
            ec.message();
      return false;
    }
  }
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) {
    err = "cannot open for write: " + path;
    return false;
  }
  f.write(text.data(), static_cast<std::streamsize>(text.size()));
  if (!f) {
    err = "write failed: " + path;
    return false;
  }
  return true;
}

bool saveReadFile(const std::string& path, std::string& out, std::string& err) {
  err.clear();
  out.clear();
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    err = "cannot open for read: " + path;
    return false;
  }
  std::string data((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
  if (f.bad()) {
    err = "read failed: " + path;
    return false;
  }
  out = std::move(data);
  return true;
}

bool saveFileExists(const std::string& path) {
  std::error_code ec;
  return std::filesystem::exists(path, ec);
}

std::string saveUtcNowISO8601() {
  const std::time_t t = std::time(nullptr);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
  return std::string(buf);
}

}  // namespace astro
