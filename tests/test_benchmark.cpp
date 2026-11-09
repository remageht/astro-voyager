// test_benchmark.cpp (Stage 4): unit tests for BenchmarkCollector.
#include "BenchmarkSystem.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

using namespace astro;

static float near(float a, float b, float eps = 0.5f) {
  return std::abs(a - b) <= eps;
}

int main() {
  // Test 1: avg/min/max FPS
  {
    BenchmarkCollector cc;
    for (int i = 0; i < 100; ++i) cc.recordFrame(0.016f, "sol");  // 62.5 FPS
    BenchmarkResult r = cc.computeResults();
    if (r.totalFrames != 100) { std::cerr << "PASS_FAIL: totalFrames\n"; return 1; }
    if (!near(r.avgFps, 62.5f, 0.1f)) { std::cerr << "PASS_FAIL: avgFps " << r.avgFps << "\n"; return 1; }
    if (!near(r.minFps, 62.5f, 0.2f)) { std::cerr << "PASS_FAIL: minFps\n"; return 1; }
    if (!near(r.maxFps, 62.5f, 0.2f)) { std::cerr << "PASS_FAIL: maxFps\n"; return 1; }
    if (!near(r.frameTimeStdDevMs, 0.0f, 0.1f)) { std::cerr << "PASS_FAIL: stddev\n"; return 1; }
    if (r.perStationAvgFps.find("sol") == r.perStationAvgFps.end()) {
      std::cerr << "PASS_FAIL: perStation\n"; return 1;
    }
    std::cout << "PASS: Test 1 avg/min/max FPS\n";
  }

  // Test 2: percentile 1% low / 0.1% low with stutters
  {
    BenchmarkCollector cc;
    for (int i = 0; i < 1000; ++i) cc.recordFrame(0.016f, "sgra");  // 16 ms
    for (int i = 0; i < 100; ++i) cc.recordFrame(0.100f, "sgra");    // 100 ms stutters (~9%)
    BenchmarkResult r = cc.computeResults();
    // 1% low should be pulled down substantially vs avg (kind of stutter effect)
    if (!(r.onePercentLowFps < r.avgFps)) {
      std::cerr << "PASS_FAIL: 1% low should be < avg\n"; return 1;
    }
    if (!(r.pointOnePercentLowFps <= r.onePercentLowFps)) {
      std::cerr << "PASS_FAIL: 0.1% low should be <= 1% low\n"; return 1;
    }
    if (!near(r.minFps, 10.0f, 0.5f)) {  // min should be from worst frame
      std::cerr << "PASS_FAIL: minFps stutter " << r.minFps << "\n"; return 1;
    }
    std::cout << "PASS: Test 2 percentile 1% / 0.1% lows\n";
  }

  // Test 3: JSON serialize + ascii + baseline compare
  {
    BenchmarkCollector cc;
    for (int i = 0; i < 60; ++i) cc.recordFrame(0.016f, "m13");
    BenchmarkResult r = cc.computeResults();
    std::string json = cc.serializeJson(r);
    if (json.find("\"avgFps\"") == std::string::npos) { std::cerr << "PASS_FAIL: json avgFps\n"; return 1; }
    std::string ascii = cc.formatAsciiReport(r);
    if (ascii.find("Benchmark Report") == std::string::npos) { std::cerr << "PASS_FAIL: ascii\n"; return 1; }

    // Write baseline 100 FPS, current ~62.5 FPS -> regression ~37%
    const char* path = "bench_baseline_test.json";
    {
      std::ofstream out(path);
      out << "{\"avgFps\": 100.0, \"totalFrames\": 100}";
    }
    std::string rep;
    bool ok = cc.compareWithBaseline(r, path, 10.0f, rep);
    if (ok) { std::cerr << "PASS_FAIL: regression should be detected\n"; return 1; }
    bool ok2 = cc.compareWithBaseline(r, path, 50.0f, rep);
    if (!ok2) { std::cerr << "PASS_FAIL: tolerant compare should pass\n"; return 1; }
    std::remove(path);
    // Improvement case: current > baseline
    {
      BenchmarkCollector cc2;
      for (int i = 0; i < 60; ++i) cc2.recordFrame(0.010f, "m13");  // 100 FPS
      BenchmarkResult r2 = cc2.computeResults();
      const char* path2 = "bench_baseline_test2.json";
      {
        std::ofstream out(path2);
        out << "{\"avgFps\": 50.0}";
      }
      std::string rep2;
      if (!cc2.compareWithBaseline(r2, path2, 10.0f, rep2)) {
        std::cerr << "PASS_FAIL: improvement should be accepted\n"; return 1;
      }
      std::remove(path2);
    }
    std::cout << "PASS: Test 3 JSON serialize + baseline compare\n";
  }

  // Test 4: Extended telemetry metrics (avgFrameTimeMs, p95, p99, draw calls, culling)
  {
    BenchmarkCollector cc;
    for (int i = 0; i < 95; ++i) cc.recordFrame(0.016f, "sol");   // 16 ms (62.5 FPS)
    for (int i = 0; i < 4; ++i)  cc.recordFrame(0.033f, "sol");   // 33 ms (~30 FPS)
    cc.recordFrame(0.050f, "sol");                                 // 50 ms spike (20 FPS)
    cc.recordTelemetry(120, 45);

    BenchmarkResult r = cc.computeResults();
    if (r.totalFrames != 100) { std::cerr << "PASS_FAIL: Test 4 totalFrames\n"; return 1; }
    if (!near(r.avgFrameTimeMs, 17.0f, 1.0f)) { std::cerr << "PASS_FAIL: Test 4 avgFrameTimeMs " << r.avgFrameTimeMs << "\n"; return 1; }
    if (r.p99FrameTimeMs < r.p95FrameTimeMs) { std::cerr << "PASS_FAIL: Test 4 p99 should be >= p95\n"; return 1; }
    if (!near(r.p95FrameTimeMs, 16.0f, 0.5f)) { std::cerr << "PASS_FAIL: Test 4 p95 should be ~16ms\n"; return 1; }
    if (!near(r.p99FrameTimeMs, 33.0f, 0.5f)) { std::cerr << "PASS_FAIL: Test 4 p99 should be ~33ms\n"; return 1; }
    if (r.totalDrawCalls != 120) { std::cerr << "PASS_FAIL: Test 4 totalDrawCalls\n"; return 1; }
    if (r.culledDrawCalls != 45) { std::cerr << "PASS_FAIL: Test 4 culledDrawCalls\n"; return 1; }

    std::string json = cc.serializeJson(r);
    if (json.find("\"p99FrameTimeMs\"") == std::string::npos) { std::cerr << "PASS_FAIL: Test 4 json p99FrameTimeMs\n"; return 1; }
    if (json.find("\"culledDrawCalls\": 45") == std::string::npos) { std::cerr << "PASS_FAIL: Test 4 json culledDrawCalls\n"; return 1; }

    std::string rep = cc.formatAsciiReport(r);
    if (rep.find("Culled: 45") == std::string::npos) { std::cerr << "PASS_FAIL: Test 4 rep culled\n"; return 1; }

    std::cout << "PASS: Test 4 extended telemetry metrics (p95/p99 frame time & draw culling)\n";
  }

  std::cout << "ALL BENCHMARK TESTS PASSED\n";
  return 0;
}
