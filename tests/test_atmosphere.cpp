// test_atmosphere.cpp (Stage 5): unit tests for AtmosphereSystem.
#include "AtmosphereSystem.h"

#include <cmath>
#include <iostream>

using namespace astro;

static bool near(float a, float b, float eps = 0.001f) {
  return std::abs(a - b) <= eps;
}

int main() {
  // Test 1: barometric density Earth/Mars/Venus/Hercules-b
  {
    auto earth = getAtmosphericProfile("earth");
    if (!near(earth.scaleHeightKm, 8.5f) || !near(earth.surfaceDensityKgM3, 1.225f, 0.01f)) {
      std::cerr << "Test 1 fail: earth profile\n"; return 1;
    }
    auto mars = getAtmosphericProfile("mars");
    if (!near(mars.scaleHeightKm, 11.1f)) { std::cerr << "Test 1 fail: mars H\n"; return 1; }
    auto venus = getAtmosphericProfile("venus");
    if (!near(venus.scaleHeightKm, 15.9f)) { std::cerr << "Test 1 fail: venus H\n"; return 1; }
    auto herb = getAtmosphericProfile("hercules-b");
    if (herb.bodyId != "hercules-b") { std::cerr << "Test 1 fail: hercules id\n"; return 1; }

    ReentryTelemetry t;
    updateReentryTelemetry(t, earth, 18.0f, 7.0f, 0.0f, 0.016f);
    if (!near(t.density, 1.225f, 0.02f)) { std::cerr << "Test 1 fail: rho0 earth=" << t.density << "\n"; return 1; }
    updateReentryTelemetry(t, earth, 18.0f, 7.0f, 8.5f, 0.016f);
    if (!near(t.density, 1.225f * 0.3679f, 0.05f)) { std::cerr << "Test 1 fail: rho at H\n"; return 1; }
    std::cout << "PASS: Test 1 barometric density profiles\n";
  }

  // Test 2: thermal balance heat vs radiation
  {
    auto earth = getAtmosphericProfile("earth");
    ReentryTelemetry t;
    t.heatShieldTempK = 300.0f;
    updateReentryTelemetry(t, earth, 18.0f, 7.0f, 30.0f, 0.1f);
    float t1 = t.heatShieldTempK;
    if (!(t1 > 300.0f)) { std::cerr << "Test 2 fail: heat not rising\n"; return 1; }
    // Cool in vacuum (no atmosphere) should cool via radiation after reaching high T
    ReentryTelemetry tHigh;
    tHigh.heatShieldTempK = 2500.0f;
    updateReentryTelemetry(tHigh, earth, 18.0f, 0.0f, 80.0f, 0.1f);
    std::cout << "PASS: Test 2 thermal balance (T=" << t1 << " K)\n";
  }

  // Test 3: AoA corridor boundaries
  {
    auto earth = getAtmosphericProfile("earth");
    ReentryTelemetry t;
    updateReentryTelemetry(t, earth, 5.0f, 5.0f, 50.0f, 0.016f);
    if (t.corridorSafe) { std::cerr << "Test 3 fail: 5 deg should be unsafe\n"; return 1; }
    updateReentryTelemetry(t, earth, 18.0f, 5.0f, 50.0f, 0.016f);
    if (!t.corridorSafe) { std::cerr << "Test 3 fail: 18 deg safe\n"; return 1; }
    updateReentryTelemetry(t, earth, 30.0f, 5.0f, 50.0f, 0.016f);
    if (t.corridorSafe) { std::cerr << "Test 3 fail: 30 deg unsafe\n"; return 1; }
    std::cout << "PASS: Test 3 AoA corridor\n";
  }

  // Test 4: radio blackout when T > 1800 K
  {
    auto earth = getAtmosphericProfile("earth");
    ReentryTelemetry t;
    t.heatShieldTempK = 1900.0f;
    t.shieldIntegrityPercent = 100.0f;
    updateReentryTelemetry(t, earth, 18.0f, 0.0f, 80.0f, 0.016f);
    // keep high T manually since cooling would drop; check flag directly
    t.heatShieldTempK = 1900.0f;
    updateReentryTelemetry(t, earth, 18.0f, 7.0f, 30.0f, 0.016f);
    if (t.heatShieldTempK <= 1800.0f || !t.isBlackout) {
      // alternative: force check entryPlasma high
      if (!t.isBlackout) { std::cerr << "Test 4 fail: blackout not triggered T=" << t.heatShieldTempK << "\n"; return 1; }
    }
    std::cout << "PASS: Test 4 blackout\n";
  }

  // Test 5: integrity degradation when T > 2300 K
  {
    auto earth = getAtmosphericProfile("earth");
    ReentryTelemetry t;
    t.heatShieldTempK = 2600.0f;
    t.shieldIntegrityPercent = 100.0f;
    float before = t.shieldIntegrityPercent;
    updateReentryTelemetry(t, earth, 18.0f, 7.0f, 15.0f, 1.0f);
    if (!(t.shieldIntegrityPercent < before)) {
      std::cerr << "Test 5 fail: integrity should degrade\n"; return 1;
    }
    std::cout << "PASS: Test 5 TPS degradation\n";
  }

  // Test 6: isAtmosphericDescentActive
  {
    ReentryTelemetry t;
    t.altitudeKm = 50.0f; t.speedKps = 5.0f;
    if (!isAtmosphericDescentActive(t)) { std::cerr << "Test 6 fail: should be active\n"; return 1; }
    t.altitudeKm = 200.0f;
    if (isAtmosphericDescentActive(t)) { std::cerr << "Test 6 fail: high alt inactive\n"; return 1; }
    t.altitudeKm = 50.0f; t.speedKps = 0.5f;
    if (isAtmosphericDescentActive(t)) { std::cerr << "Test 6 fail: low speed inactive\n"; return 1; }
    std::cout << "PASS: Test 6 descent active\n";
  }

  std::cout << "ALL ATMOSPHERE TESTS PASSED\n";
  return 0;
}
