#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace astro {

// === Perf Benchmark Harness === (Stage 4): чистый C++17, без GL-зависимостей.
struct BenchmarkSample {
  float frameTimeMs = 0.0f;
  std::string stationId;
  double timestampSec = 0.0;
};

struct BenchmarkResult {
  int totalFrames = 0;
  double totalDurationSec = 0.0;
  float avgFps = 0.0f;
  float minFps = 0.0f;
  float maxFps = 0.0f;
  float onePercentLowFps = 0.0f;
  float pointOnePercentLowFps = 0.0f;
  float frameTimeStdDevMs = 0.0f;
  // v4.5.0: Enhanced frame-time telemetry & culling metrics
  float avgFrameTimeMs = 0.0f;
  float p99FrameTimeMs = 0.0f;
  float p95FrameTimeMs = 0.0f;
  int totalDrawCalls = 0;
  int culledDrawCalls = 0;
  std::unordered_map<std::string, float> perStationAvgFps;
};

class BenchmarkCollector {
 public:
  void recordFrame(float dtSec, const std::string& currentStationId);
  void recordTelemetry(int drawCalls, int culledDrawCalls);
  void clear();
  std::size_t size() const { return m_samples.size(); }
  BenchmarkResult computeResults() const;
  std::string serializeJson(const BenchmarkResult& res) const;
  std::string formatAsciiReport(const BenchmarkResult& res) const;
  bool compareWithBaseline(const BenchmarkResult& current,
                            const std::string& baselineJsonPath,
                            float maxRegressionPercent,
                            std::string& outReport);

 private:
  std::vector<BenchmarkSample> m_samples;
  double m_elapsedSec = 0.0;
  int m_totalDrawCalls = 0;
  int m_culledDrawCalls = 0;
};

inline float percentileFps(const std::vector<float>& frameMsSorted,
                           float percentile01);  // e.g. 99.0f, 99.9f
inline float percentileMs(const std::vector<float>& frameMsSorted,
                          float percentile01);

}  // namespace astro
