#include "BenchmarkSystem.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace astro {

// === helpers ===
static inline float extractNum(const std::string& json, const std::string& key,
                               float fallback = 0.0f) {
  std::size_t pos = json.find(key);
  if (pos == std::string::npos) return fallback;
  pos = json.find(':', pos);
  if (pos == std::string::npos) return fallback;
  std::size_t start = json.find_first_of("0123456789-", pos);
  if (start == std::string::npos) return fallback;
  try {
    return std::stof(json.substr(start));
  } catch (...) {
    return fallback;
  }
}

static inline int extractInt(const std::string& json, const std::string& key,
                             int fallback = 0) {
  return static_cast<int>(extractNum(json, key, static_cast<float>(fallback)));
}

inline float percentileFps(const std::vector<float>& frameMsSorted,
                           float percentile01) {
  if (frameMsSorted.empty()) return 0.0f;
  if (percentile01 <= 0.0f) percentile01 = 0.0f;
  if (percentile01 >= 100.0f) percentile01 = 100.0f;
  std::size_t idx = static_cast<std::size_t>(
      std::ceil((percentile01 / 100.0f) * static_cast<float>(frameMsSorted.size())));
  if (idx == 0) idx = 1;
  if (idx > frameMsSorted.size()) idx = frameMsSorted.size();
  float ms = frameMsSorted[idx - 1];
  if (ms <= 0.0f) return 0.0f;
  return 1000.0f / ms;
}

inline float percentileMs(const std::vector<float>& frameMsSorted,
                          float percentile01) {
  if (frameMsSorted.empty()) return 0.0f;
  if (percentile01 <= 0.0f) percentile01 = 0.0f;
  if (percentile01 >= 100.0f) percentile01 = 100.0f;
  std::size_t idx = static_cast<std::size_t>(
      std::ceil((percentile01 / 100.0f) * static_cast<float>(frameMsSorted.size())));
  if (idx == 0) idx = 1;
  if (idx > frameMsSorted.size()) idx = frameMsSorted.size();
  return frameMsSorted[idx - 1];
}

void BenchmarkCollector::clear() {
  m_samples.clear();
  m_elapsedSec = 0.0;
  m_totalDrawCalls = 0;
  m_culledDrawCalls = 0;
}

void BenchmarkCollector::recordTelemetry(int drawCalls, int culledDrawCalls) {
  m_totalDrawCalls += drawCalls;
  m_culledDrawCalls += culledDrawCalls;
}

void BenchmarkCollector::recordFrame(float dtSec,
                                     const std::string& currentStationId) {
  if (!(dtSec > 0.0f) || !std::isfinite(dtSec)) return;
  BenchmarkSample s;
  s.frameTimeMs = dtSec * 1000.0f;
  s.stationId = currentStationId;
  s.timestampSec = m_elapsedSec;
  m_samples.push_back(s);
  m_elapsedSec += static_cast<double>(dtSec);
}

BenchmarkResult BenchmarkCollector::computeResults() const {
  BenchmarkResult res;
  res.totalFrames = static_cast<int>(m_samples.size());
  res.totalDurationSec = m_elapsedSec;
  res.totalDrawCalls = m_totalDrawCalls;
  res.culledDrawCalls = m_culledDrawCalls;
  if (m_samples.empty()) return res;

  std::vector<float> ms;
  ms.reserve(m_samples.size());
  for (const auto& s : m_samples) ms.push_back(s.frameTimeMs);
  std::vector<float> sorted = ms;
  std::sort(sorted.begin(), sorted.end());

  double sum = 0.0;
  for (float v : ms) sum += static_cast<double>(v);
  double avgMs = sum / static_cast<double>(ms.size());
  res.avgFps = (avgMs > 0.0) ? static_cast<float>(1000.0 / avgMs) : 0.0f;
  res.avgFrameTimeMs = static_cast<float>(avgMs);
  res.minFps = percentileFps(sorted, 100.0f);
  res.maxFps = percentileFps(sorted, 0.0f);
  res.onePercentLowFps = percentileFps(sorted, 99.0f);
  res.pointOnePercentLowFps = percentileFps(sorted, 99.9f);
  res.p99FrameTimeMs = percentileMs(sorted, 99.0f);
  res.p95FrameTimeMs = percentileMs(sorted, 95.0f);

  double varSum = 0.0;
  for (float v : ms) varSum += (static_cast<double>(v) - avgMs) *
                               (static_cast<double>(v) - avgMs);
  res.frameTimeStdDevMs = static_cast<float>(std::sqrt(varSum / static_cast<double>(ms.size())));

  // per-station avg FPS
  std::unordered_map<std::string, double> stSum;
  std::unordered_map<std::string, int> stCount;
  for (const auto& s : m_samples) {
    stSum[s.stationId] += static_cast<double>(s.frameTimeMs);
    stCount[s.stationId] += 1;
  }
  for (const auto& kv : stCount) {
    double avg = stSum[kv.first] / static_cast<double>(kv.second);
    res.perStationAvgFps[kv.first] =
        (avg > 0.0) ? static_cast<float>(1000.0 / avg) : 0.0f;
  }
  return res;
}

std::string BenchmarkCollector::serializeJson(const BenchmarkResult& r) const {
  std::ostringstream os;
  os << std::fixed << std::setprecision(3);
  os << "{\n";
  os << "  \"totalFrames\": " << r.totalFrames << ",\n";
  os << "  \"totalDurationSec\": " << r.totalDurationSec << ",\n";
  os << "  \"avgFps\": " << r.avgFps << ",\n";
  os << "  \"minFps\": " << r.minFps << ",\n";
  os << "  \"maxFps\": " << r.maxFps << ",\n";
  os << "  \"onePercentLowFps\": " << r.onePercentLowFps << ",\n";
  os << "  \"pointOnePercentLowFps\": " << r.pointOnePercentLowFps << ",\n";
  os << "  \"avgFrameTimeMs\": " << r.avgFrameTimeMs << ",\n";
  os << "  \"p99FrameTimeMs\": " << r.p99FrameTimeMs << ",\n";
  os << "  \"p95FrameTimeMs\": " << r.p95FrameTimeMs << ",\n";
  os << "  \"totalDrawCalls\": " << r.totalDrawCalls << ",\n";
  os << "  \"culledDrawCalls\": " << r.culledDrawCalls << ",\n";
  os << "  \"frameTimeStdDevMs\": " << r.frameTimeStdDevMs << ",\n";
  os << "  \"perStationAvgFps\": {";
  std::vector<std::string> keys;
  for (const auto& kv : r.perStationAvgFps) keys.push_back(kv.first);
  std::sort(keys.begin(), keys.end());
  bool first = true;
  for (const auto& k : keys) {
    if (!first) os << ", ";
    os << "\"" << k << "\": " << r.perStationAvgFps.at(k);
    first = false;
  }
  os << "}\n";
  os << "}\n";
  return os.str();
}

std::string BenchmarkCollector::formatAsciiReport(const BenchmarkResult& r) const {
  std::ostringstream os;
  os << std::fixed << std::setprecision(2);
  os << "==== Astro Voyager Benchmark Report ===\n";
  os << "Frames: " << r.totalFrames << "  Duration: " << r.totalDurationSec << " s\n";
  os << "Avg FPS: " << r.avgFps << "\n";
  os << "Min FPS: " << r.minFps << "\n";
  os << "Max FPS: " << r.maxFps << "\n";
  os << "1% Low FPS: " << r.onePercentLowFps << "\n";
  os << "0.1% Low FPS: " << r.pointOnePercentLowFps << "\n";
  os << "Avg Frame Time: " << r.avgFrameTimeMs << " ms  (P95: " << r.p95FrameTimeMs << " ms, P99: " << r.p99FrameTimeMs << " ms)\n";
  os << "Draw calls: " << r.totalDrawCalls << " (Culled: " << r.culledDrawCalls << ")\n";
  os << "Frame time stddev: " << r.frameTimeStdDevMs << " ms\n";
  if (!r.perStationAvgFps.empty()) {
    os << "Per-station Avg FPS:\n";
    std::vector<std::string> keys;
    for (const auto& kv : r.perStationAvgFps) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());
    for (const auto& k : keys) {
      os << "  " << k << ": " << r.perStationAvgFps.at(k) << "\n";
    }
  }
  os << "======================================\n";
  return os.str();
}

bool BenchmarkCollector::compareWithBaseline(const BenchmarkResult& current,
                                             const std::string& baselineJsonPath,
                                             float maxRegressionPercent,
                                             std::string& outReport) {
  std::ifstream in(baselineJsonPath);
  if (!in) {
    outReport = "baseline not found: " + baselineJsonPath;
    return false;
  }
  std::string base((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  float baseAvg = extractNum(base, "\"avgFps\"");
  if (baseAvg <= 0.0f) {
    outReport = "baseline avgFps missing or zero";
    return false;
  }
  float curAvg = current.avgFps;
  float regression = (curAvg >= baseAvg)
                         ? 0.0f
                         : (baseAvg - curAvg) / baseAvg * 100.0f;
  std::ostringstream os;
  os << std::fixed << std::setprecision(2);
  os << "baseline avgFps=" << baseAvg << " current avgFps=" << curAvg
     << " regression=" << regression << "% (limit " << maxRegressionPercent
     << "%)";
  outReport = os.str();
  return regression <= maxRegressionPercent;
}

}  // namespace astro
