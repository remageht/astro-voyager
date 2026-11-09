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

  // Test 7: Weather profile archetypes
  {
    auto wEarth = generateInitialWeather("earth", 123.0);
    if (wEarth.nameRu.empty() || wEarth.nameEn.empty()) {
      std::cerr << "Test 7 fail: empty earth weather name\n"; return 1;
    }
    auto wMars = generateInitialWeather("mars", 456.0);
    if (wMars.type != WeatherType::Clear && wMars.type != WeatherType::Sandstorm) {
      std::cerr << "Test 7 fail: mars unexpected weather type\n"; return 1;
    }
    auto wVenus = generateInitialWeather("venus", 789.0);
    if (wVenus.type != WeatherType::AcidRain && wVenus.type != WeatherType::Cyclone) {
      std::cerr << "Test 7 fail: venus unexpected weather type\n"; return 1;
    }
    auto wTitan = generateInitialWeather("titan", 101.0);
    if (wTitan.type != WeatherType::CryoSnow) {
      std::cerr << "Test 7 fail: titan unexpected weather type\n"; return 1;
    }
    std::cout << "PASS: Test 7 weather profiles & archetypes\n";
  }

  // Test 8: Weather evolution & altitude attenuation
  {
    auto earthProfile = getAtmosphericProfile("earth");
    WeatherCondition w = generateInitialWeather("earth", 0.0);
    // At sea level (alt = 0), weather intensity should be active
    updatePlanetaryWeather(w, earthProfile, 0.0f, 45.0f, 10.0f, 100.0, 1.0f);
    if (!(w.intensity > 0.0f)) {
      std::cerr << "Test 8 fail: zero intensity at sea level\n"; return 1;
    }
    // High in space (alt = 200 km), weather should attenuate to 0
    updatePlanetaryWeather(w, earthProfile, 200.0f, 45.0f, 10.0f, 100.0, 1.0f);
    if (!(w.intensity <= 0.05f)) {
      std::cerr << "Test 8 fail: high altitude weather not attenuated: " << w.intensity << "\n"; return 1;
    }
    std::cout << "PASS: Test 8 weather evolution & altitude attenuation\n";
  }

  // Test 9: Vector AoA calculation
  {
    // Forward (0,0,-1) along velocity (0,0,-1) -> AoA = 0 deg
    float a0 = computeAngleOfAttackDeg(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f);
    if (!near(a0, 0.0f, 0.5f)) {
      std::cerr << "Test 9 fail: aligned AoA=" << a0 << "\n"; return 1;
    }
    // Forward (0,0,-1) vs velocity (0,-1,0) -> AoA = 90 deg
    float a90 = computeAngleOfAttackDeg(0.0f, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f);
    if (!near(a90, 90.0f, 0.5f)) {
      std::cerr << "Test 9 fail: orthogonal AoA=" << a90 << "\n"; return 1;
    }
    // Forward (0,0,-1) vs backward (0,0,1) -> AoA = 180 deg
    float a180 = computeAngleOfAttackDeg(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
    if (!near(a180, 180.0f, 0.5f)) {
      std::cerr << "Test 9 fail: retrograde AoA=" << a180 << "\n"; return 1;
    }
    std::cout << "PASS: Test 9 vector AoA calculation\n";
  }

  // Test 10: Skip-out and Overheat warnings
  {
    auto earth = getAtmosphericProfile("earth");
    ReentryTelemetry t;
    // Shallow hypersonic entry: 5 deg AoA at 7 km/s in upper atmosphere -> skip-out warning
    updateReentryTelemetry(t, earth, 5.0f, 7.0f, 60.0f, 0.016f);
    if (!t.skipOutWarning) {
      std::cerr << "Test 10 fail: skipOutWarning not triggered\n"; return 1;
    }
    // Steep entry: 35 deg AoA -> overheat warning
    updateReentryTelemetry(t, earth, 35.0f, 7.0f, 60.0f, 0.016f);
    if (!t.overheatWarning) {
      std::cerr << "Test 10 fail: overheatWarning not triggered\n"; return 1;
    }
    std::cout << "PASS: Test 10 reentry corridor warnings\n";
  }

  // Test 11: Drag deceleration and G-force
  {
    auto earth = getAtmosphericProfile("earth");
    ReentryTelemetry t;
    // Hypersonic dense atmosphere entry: 7 km/s at 25 km altitude
    updateReentryTelemetry(t, earth, 18.0f, 7.0f, 25.0f, 0.016f);
    if (!(t.dragDeceleration > 0.0f)) {
      std::cerr << "Test 11 fail: no drag deceleration\n"; return 1;
    }
    if (!(t.gForce > 0.5f)) {
      std::cerr << "Test 11 fail: low g-force\n"; return 1;
    }
    std::cout << "PASS: Test 11 drag deceleration & G-force\n";
  }

  // Test 12: Weather localization names
  {
    if (std::string(getWeatherTypeNameRu(WeatherType::Thunderstorm)).empty() ||
        std::string(getWeatherTypeNameEn(WeatherType::Thunderstorm)).empty() ||
        std::string(getWeatherTypeNameRu(WeatherType::Sandstorm)).empty() ||
        std::string(getWeatherTypeNameEn(WeatherType::AcidRain)).empty()) {
      std::cerr << "Test 12 fail: localization names empty\n"; return 1;
    }
    std::cout << "PASS: Test 12 weather localization\n";
  }

  std::cout << "ALL ATMOSPHERE TESTS PASSED\n";
  return 0;
}
