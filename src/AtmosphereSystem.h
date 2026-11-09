#pragma once

#include <string>

namespace astro {

enum class WeatherType : int {
  Clear = 0,
  Rain = 1,
  Thunderstorm = 2,
  Sandstorm = 3,
  AcidRain = 4,
  CryoSnow = 5,
  Cyclone = 6
};

struct WeatherCondition {
  WeatherType type = WeatherType::Clear;
  float intensity = 0.0f;        // 0.0 .. 1.0
  float windSpeedKps = 0.0f;     // horizontal wind in km/s (e.g. 0.025 = 25 m/s)
  float windAngleRad = 0.0f;     // wind bearing in radians
  float turbulence = 0.0f;       // 0.0 .. 1.0 buffeting
  float visibilityNorm = 1.0f;   // 1.0 clear .. 0.15 zero-visibility storm
  float lightningFlash = 0.0f;   // 0.0 .. 1.0 strobe flash
  float rainStreaks = 0.0f;      // 0.0 .. 1.0 water droplet flow
  float dustParticles = 0.0f;    // 0.0 .. 1.0 grit abrasion
  float acidCorrosion = 0.0f;    // 0.0 .. 1.0 sulfur mist
  float frostIce = 0.0f;         // 0.0 .. 1.0 ice dendrites
  std::string nameRu = "Ясно";
  std::string nameEn = "Clear";
};

// === Atmospheric Entry & Thermal Shield === (Stage 5): clean C++17, no GL.
struct AtmosphericBodyProfile {
  std::string bodyId;
  float scaleHeightKm = 8.5f;
  float surfaceDensityKgM3 = 1.225f;
  float surfacePressureKPa = 101.3f;
  float avgWindSpeedKps = 0.015f;
  float entryCorridorMinAoADeg = 12.0f;
  float entryCorridorMaxAoADeg = 26.0f;
  float maxShieldTempK = 2600.0f;
  WeatherType dominantWeather = WeatherType::Clear;
  WeatherType stormWeather = WeatherType::Thunderstorm;
};

struct ReentryTelemetry {
  float altitudeKm = 0.0f;
  float speedKps = 0.0f;
  float density = 0.0f;
  float heatShieldTempK = 300.0f;
  float shieldIntegrityPercent = 100.0f;
  float currentAoADeg = 18.0f;
  bool corridorSafe = true;
  bool isBlackout = false;
  float gForce = 0.0f;
  float entryPlasma = 0.0f;
  float dragDeceleration = 0.0f; // km/s^2
  float liftForce = 0.0f;
  bool skipOutWarning = false;
  bool overheatWarning = false;
};

AtmosphericBodyProfile getAtmosphericProfile(const std::string& bodyId);
void updateReentryTelemetry(ReentryTelemetry& telem,
                            const AtmosphericBodyProfile& profile,
                            float pitchDeg, float speedKps, float altitudeKm,
                            float dt);
bool isAtmosphericDescentActive(const ReentryTelemetry& telem);

// Weather & Aerodynamics (v5.4.0)
void updatePlanetaryWeather(WeatherCondition& weather,
                            const AtmosphericBodyProfile& profile,
                            float altitudeKm, float latDeg, float lonDeg,
                            double simTime, float dt);
float computeAngleOfAttackDeg(float fwdX, float fwdY, float fwdZ,
                             float velX, float velY, float velZ);
const char* getWeatherTypeNameRu(WeatherType type);
const char* getWeatherTypeNameEn(WeatherType type);
WeatherCondition generateInitialWeather(const std::string& bodyId, double seed = 0.0);

}  // namespace astro
