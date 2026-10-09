#include "Journey.h"

#include <cctype>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace astro {

namespace {

static bool isValidToken(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s) {
    if (!((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') ||
          c == '_' || c == '-')) {
      return false;
    }
  }
  return true;
}

static size_t skipWs(const std::string& s, size_t pos) {
  while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n')) {
    ++pos;
  }
  return pos;
}

}  // namespace

std::string journeySerialize(const std::vector<std::string>& stations) {
  std::string out = "{\"version\":1,\"stations\":[";
  for (size_t i = 0; i < stations.size(); ++i) {
    if (i > 0) out += ",";
    out += "\"" + stations[i] + "\"";
  }
  out += "]}";
  return out;
}

bool journeyDeserialize(const std::string& text,
                        std::vector<std::string>& outStations,
                        std::string& outError) {
  outStations.clear();
  outError.clear();

  size_t startPos = skipWs(text, 0);
  if (startPos >= text.size()) {
    outError = "empty text";
    return false;
  }
  if (text[startPos] != '{') {
    outError = "expected '{' at root";
    return false;
  }

  size_t endPos = text.find_last_not_of(" \t\r\n");
  if (endPos == std::string::npos || text[endPos] != '}') {
    outError = "expected '}' at end";
    return false;
  }

  // Parse version
  size_t vPos = text.find("\"version\"");
  if (vPos == std::string::npos) {
    outError = "missing version";
    return false;
  }
  vPos += 9;
  vPos = skipWs(text, vPos);
  if (vPos >= text.size() || text[vPos] != ':') {
    outError = "expected ':' after version";
    return false;
  }
  vPos = skipWs(text, vPos + 1);
  if (vPos >= text.size()) {
    outError = "missing version value";
    return false;
  }
  size_t numStart = vPos;
  while (vPos < text.size() && (text[vPos] >= '0' && text[vPos] <= '9')) {
    ++vPos;
  }
  if (vPos == numStart) {
    outError = "malformed version number";
    return false;
  }
  std::string verStr = text.substr(numStart, vPos - numStart);
  if (verStr != "1") {
    outError = "unsupported version";
    return false;
  }
  if (vPos < text.size() && text[vPos] != ',' && text[vPos] != '}' &&
      text[vPos] != ' ' && text[vPos] != '\t' && text[vPos] != '\r' && text[vPos] != '\n') {
    outError = "malformed version value";
    return false;
  }

  // Parse stations array
  size_t sPos = text.find("\"stations\"");
  if (sPos == std::string::npos) {
    outError = "missing stations";
    return false;
  }
  sPos += 10;
  sPos = skipWs(text, sPos);
  if (sPos >= text.size() || text[sPos] != ':') {
    outError = "expected ':' after stations";
    return false;
  }
  sPos = skipWs(text, sPos + 1);
  if (sPos >= text.size() || text[sPos] != '[') {
    outError = "expected '[' for stations array";
    return false;
  }
  ++sPos;

  std::vector<std::string> parsed;
  bool inArray = true;
  bool expectValue = false;

  while (inArray) {
    sPos = skipWs(text, sPos);
    if (sPos >= text.size()) {
      outError = "unclosed stations array";
      return false;
    }
    if (text[sPos] == ']') {
      if (expectValue) {
        outError = "trailing comma in stations array";
        return false;
      }
      ++sPos;
      inArray = false;
      break;
    }
    if (text[sPos] != '"') {
      outError = "expected string in stations array";
      return false;
    }
    ++sPos;
    size_t startToken = sPos;
    while (sPos < text.size() && text[sPos] != '"') {
      ++sPos;
    }
    if (sPos >= text.size()) {
      outError = "unclosed quote in stations array";
      return false;
    }
    std::string token = text.substr(startToken, sPos - startToken);
    ++sPos;

    if (!isValidToken(token)) {
      outError = "invalid station token: " + token;
      return false;
    }
    parsed.push_back(token);

    sPos = skipWs(text, sPos);
    if (sPos >= text.size()) {
      outError = "unclosed stations array";
      return false;
    }
    if (text[sPos] == ',') {
      ++sPos;
      expectValue = true;
    } else if (text[sPos] == ']') {
      ++sPos;
      inArray = false;
      break;
    } else {
      outError = "expected ',' or ']' after station token";
      return false;
    }
  }

  outStations = std::move(parsed);
  return true;
}

std::string journeySanitizeFileName(const std::string& name) {
  if (name.empty()) {
    return "journey";
  }
  std::string res;
  res.reserve(name.size());
  for (char c : name) {
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-') {
      res.push_back(c);
    } else {
      res.push_back('_');
    }
  }
  if (res.empty()) {
    return "journey";
  }
  if (res.size() > 48) {
    res = res.substr(0, 48);
  }
  return res;
}

}  // namespace astro
