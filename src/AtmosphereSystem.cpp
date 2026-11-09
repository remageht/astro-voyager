#include "AtmosphereSystem.h"

#include <algorithm>
#include <cmath>

namespace astro {

static inline std::string toLowerCopy(const std::string& s) {
  std::string r = s;
  for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return r;
}

AtmosphericBodyProfile getAtmosphericProfile(const std::string& bodyId) {
  std::string id = toLowerCopy(bodyId);
  AtmosphericBodyProfile p;
  p.bodyId = id;
  if (id == "earth") {
    p.scaleHeightKm = 8.5f;
    p.surfaceDensityKgM3 = 1.225f;
    p.surfacePressureKPa = 101.3f;
    p.avgWindSpeedKps = 0.012f; // ~12 m/s
    p.dominantWeather = WeatherType::Rain;
    p.stormWeather = WeatherType::Thunderstorm;
  } else if (id == "mars") {
    p.scaleHeightKm = 11.1f;
    p.surfaceDensityKgM3 = 0.0158f;
    p.surfacePressureKPa = 0.636f;
    p.avgWindSpeedKps = 0.024f; // ~24 m/s
    p.dominantWeather = WeatherType::Clear;
    p.stormWeather = WeatherType::Sandstorm;
  } else if (id == "venus") {
    p.scaleHeightKm = 15.9f;
    p.surfaceDensityKgM3 = 65.0f;
    p.surfacePressureKPa = 9200.0f; // 92 bar
    p.avgWindSpeedKps = 0.075f; // ~75 m/s
    p.dominantWeather = WeatherType::AcidRain;
    p.stormWeather = WeatherType::Cyclone;
  } else if (id == "titan") {
    p.scaleHeightKm = 9.0f;
    p.surfaceDensityKgM3 = 5.3f;
    p.surfacePressureKPa = 146.7f;
    p.avgWindSpeedKps = 0.010f; // ~10 m/s
    p.dominantWeather = WeatherType::CryoSnow;
    p.stormWeather = WeatherType::CryoSnow;
  } else if (id == "hercules-b" || id == "hercules_b") {
    p.scaleHeightKm = 7.5f;
    p.surfaceDensityKgM3 = 2.1f;
    p.surfacePressureKPa = 185.0f;
    p.avgWindSpeedKps = 0.035f;
    p.dominantWeather = WeatherType::Cyclone;
    p.stormWeather = WeatherType::Thunderstorm;
  } else if (id == "jupiter" || id == "saturn") {
    p.scaleHeightKm = 27.0f;
    p.surfaceDensityKgM3 = 0.85f;
    p.surfacePressureKPa = 200.0f;
    p.avgWindSpeedKps = 0.120f; // 120 m/s jet streams
    p.dominantWeather = WeatherType::Cyclone;
    p.stormWeather = WeatherType::Thunderstorm;
  } else {
    p.scaleHeightKm = 8.5f;
    p.surfaceDensityKgM3 = 1.225f;
    p.surfacePressureKPa = 101.3f;
    p.avgWindSpeedKps = 0.015f;
    p.dominantWeather = WeatherType::Clear;
    p.stormWeather = WeatherType::Thunderstorm;
  }
  p.entryCorridorMinAoADeg = 12.0f;
  p.entryCorridorMaxAoADeg = 26.0f;
  p.maxShieldTempK = 2600.0f;
  return p;
}

void updateReentryTelemetry(ReentryTelemetry& telem,
                            const AtmosphericBodyProfile& profile,
                            float pitchDeg, float speedKps, float altitudeKm,
                            float dt) {
  constexpr float kSigma = 5.67e-8f;
  constexpr float kEmissivity = 0.85f;
  if (!(dt > 0.0f)) dt = 0.016f;
  if (altitudeKm < 0.0f) altitudeKm = 0.0f;
  if (speedKps < 0.0f) speedKps = 0.0f;

  telem.altitudeKm = altitudeKm;
  telem.speedKps = speedKps;
  telem.currentAoADeg = pitchDeg;

  // Barometric density rho(h) = rho0 * exp(-h / H)
  telem.density = profile.surfaceDensityKgM3 *
                  std::exp(-altitudeKm / profile.scaleHeightKm);

  // Corridor safe check
  telem.corridorSafe =
      (pitchDeg >= profile.entryCorridorMinAoADeg &&
       pitchDeg <= profile.entryCorridorMaxAoADeg);

  // Skip-out warning: shallow entry (< minAoA) at hypersonic velocity in upper atmosphere
  telem.skipOutWarning =
      (speedKps > 3.0f && altitudeKm < profile.scaleHeightKm * 10.0f &&
       pitchDeg < profile.entryCorridorMinAoADeg);

  // Overheat warning: steep entry (> maxAoA) or extreme temperature
  telem.overheatWarning =
      (pitchDeg > profile.entryCorridorMaxAoADeg || telem.heatShieldTempK > 2400.0f);

  // Heat flux q ~ sqrt(rho) * v^3 (normalized) and Stefan-Boltzmann cooling
  float v = speedKps * 1000.0f;  // m/s
  float qHeat = 0.0f;
  if (v > 1.0f) {
    float rho = std::max(telem.density, 0.0f);
    // AoA heat multiplier: steeper angle increases shock stagnation heat
    float aoaFactor = std::clamp(1.0f + (pitchDeg - 18.0f) * 0.04f, 0.6f, 2.5f);
    qHeat = 180.0f * std::sqrt(rho) * (v / 7000.0f) * (v / 7000.0f) * (v / 7000.0f) * 40.0f * aoaFactor;
  }
  float T = telem.heatShieldTempK;
  float rad = kEmissivity * kSigma * T * T * T * T * 0.001f;  // scaled to K/s
  float dT = (qHeat - rad) * dt;
  telem.heatShieldTempK = std::clamp(telem.heatShieldTempK + dT, 300.0f, 3000.0f);
  T = telem.heatShieldTempK;

  // Integrity degradation when T > 2300 K
  if (T > 2300.0f) {
    float excess = T - 2300.0f;
    float rate = excess * 0.02f;  // % per sec
    telem.shieldIntegrityPercent = std::clamp(
        telem.shieldIntegrityPercent - rate * dt, 0.0f, 100.0f);
  }

  // Blackout when plasma intense T > 1800 K
  telem.isBlackout = (T > 1800.0f);

  // Plasma 0..1 normalized
  float p = (T - 800.0f) / 1600.0f;
  telem.entryPlasma = std::clamp(p, 0.0f, 1.0f);

  // Aerodynamic deceleration: a_drag = 0.5 * rho * v^2 * Cd / m (normalized in km/s^2)
  if (v > 1.0f && telem.density > 1e-6f) {
    float rhoNorm = telem.density / 1.225f;
    float dragAccMs2 = 0.5f * rhoNorm * (v * v) / 18000.0f; // m/s^2
    telem.dragDeceleration = dragAccMs2 / 1000.0f; // km/s^2
    telem.gForce = std::clamp(dragAccMs2 / 9.81f, 0.0f, 16.0f);

    // Aerodynamic lift as function of AoA: L ~ sin(2 * aoa)
    float radAoA = pitchDeg * 0.01745329f;
    telem.liftForce = std::clamp(std::sin(2.0f * radAoA) * dragAccMs2 * 0.28f, -50.0f, 50.0f);
  } else {
    telem.dragDeceleration = 0.0f;
    telem.liftForce = 0.0f;
    telem.gForce = 0.0f;
  }
}

bool isAtmosphericDescentActive(const ReentryTelemetry& telem) {
  return (telem.altitudeKm < 120.0f && telem.speedKps > 2.2f);
}

const char* getWeatherTypeNameRu(WeatherType type) {
  switch (type) {
    case WeatherType::Clear: return "Ясно";
    case WeatherType::Rain: return "Ливень";
    case WeatherType::Thunderstorm: return "Грозовой фронт";
    case WeatherType::Sandstorm: return "Песчаная буря";
    case WeatherType::AcidRain: return "Кислотный ливень";
    case WeatherType::CryoSnow: return "Метановая метель";
    case WeatherType::Cyclone: return "Гипер-циклон";
    default: return "Штиль";
  }
}

const char* getWeatherTypeNameEn(WeatherType type) {
  switch (type) {
    case WeatherType::Clear: return "Clear";
    case WeatherType::Rain: return "Heavy Rain";
    case WeatherType::Thunderstorm: return "Thunderstorm";
    case WeatherType::Sandstorm: return "Dust Storm";
    case WeatherType::AcidRain: return "Acid Rain";
    case WeatherType::CryoSnow: return "Cryo Blizzard";
    case WeatherType::Cyclone: return "Super Cyclone";
    default: return "Calm";
  }
}

float computeAngleOfAttackDeg(float fwdX, float fwdY, float fwdZ,
                             float velX, float velY, float velZ) {
  float lenFwd = std::sqrt(fwdX * fwdX + fwdY * fwdY + fwdZ * fwdZ);
  float lenVel = std::sqrt(velX * velX + velY * velY + velZ * velZ);
  if (lenFwd < 1e-4f || lenVel < 1e-4f) return 18.0f; // default safe baseline

  // Cosine of angle between forward pointing axis and velocity vector
  float dot = (fwdX * velX + fwdY * velY + fwdZ * velZ) / (lenFwd * lenVel);
  dot = std::clamp(dot, -1.0f, 1.0f);
  float angleRad = std::acos(dot);
  return angleRad * 57.2957795f;
}

WeatherCondition generateInitialWeather(const std::string& bodyId, double seed) {
  AtmosphericBodyProfile profile = getAtmosphericProfile(bodyId);
  WeatherCondition w;
  w.type = profile.dominantWeather;
  w.intensity = 0.35f + static_cast<float>(std::fmod(std::abs(seed) * 0.13, 0.45));
  w.windSpeedKps = profile.avgWindSpeedKps * (0.8f + 0.4f * static_cast<float>(std::sin(seed * 0.05)));
  w.windAngleRad = static_cast<float>(std::fmod(std::abs(seed), 6.283185));
  w.turbulence = w.intensity * 0.4f;
  w.visibilityNorm = 1.0f - w.intensity * 0.3f;
  w.nameRu = getWeatherTypeNameRu(w.type);
  w.nameEn = getWeatherTypeNameEn(w.type);
  return w;
}

void updatePlanetaryWeather(WeatherCondition& weather,
                            const AtmosphericBodyProfile& profile,
                            float altitudeKm, float latDeg, float lonDeg,
                            double simTime, float dt) {
  if (dt <= 0.0f) dt = 0.016f;

  // 1. Altitude attenuation: above 4x scale height, weather thins into space
  float maxWeatherAlt = profile.scaleHeightKm * 4.0f;
  float altFactor = 1.0f;
  if (altitudeKm > maxWeatherAlt) {
    altFactor = 0.0f;
  } else if (altitudeKm > 0.0f) {
    altFactor = std::clamp(1.0f - (altitudeKm / maxWeatherAlt), 0.0f, 1.0f);
  }

  // 2. Procedural atmospheric wave dynamics based on coordinates and time
  double t = simTime * 0.02;
  double wave1 = std::sin(static_cast<double>(latDeg) * 0.06 + t);
  double wave2 = std::cos(static_cast<double>(lonDeg) * 0.08 + t * 1.3);
  double stormWave = (wave1 * 0.55 + wave2 * 0.45);

  // Transition between dominant weather and storm weather
  bool inStorm = (stormWave > 0.28);
  WeatherType targetType = inStorm ? profile.stormWeather : profile.dominantWeather;
  weather.type = targetType;
  weather.nameRu = getWeatherTypeNameRu(targetType);
  weather.nameEn = getWeatherTypeNameEn(targetType);

  // Intensity modulated by storm wave and altitude
  float targetIntensity = inStorm
      ? std::clamp(static_cast<float>(0.50 + 0.50 * (stormWave - 0.28) / 0.72), 0.0f, 1.0f)
      : std::clamp(static_cast<float>(0.20 + 0.25 * (stormWave + 1.0) * 0.5), 0.0f, 1.0f);
  targetIntensity *= altFactor;

  // Smooth lerp intensity; quick decay when breaking out of atmosphere
  if (altFactor <= 0.001f) {
    weather.intensity *= std::exp(-dt * 3.5f);
  } else {
    weather.intensity += (targetIntensity - weather.intensity) * (1.0f - std::exp(-dt * 0.8f));
  }

  // Wind speed & bearing
  float targetWind = profile.avgWindSpeedKps * (0.6f + 1.2f * weather.intensity);
  weather.windSpeedKps += (targetWind - weather.windSpeedKps) * (1.0f - std::exp(-dt * 1.2f));
  weather.windAngleRad = static_cast<float>(std::fmod(simTime * 0.04 + latDeg * 0.02, 6.283185));

  // Turbulence from wind and storm intensity
  weather.turbulence = std::clamp(weather.intensity * 0.75f + (weather.windSpeedKps / 0.08f) * 0.25f, 0.0f, 1.0f) * altFactor;

  // Visibility reduction: sandstorms, acid rain, and cyclones reduce visibility heavily
  float visReduction = weather.intensity * 0.45f;
  if (weather.type == WeatherType::Sandstorm || weather.type == WeatherType::Cyclone) {
    visReduction = weather.intensity * 0.75f;
  } else if (weather.type == WeatherType::AcidRain) {
    visReduction = weather.intensity * 0.60f;
  }
  weather.visibilityNorm = std::clamp(1.0f - visReduction * altFactor, 0.15f, 1.0f);

  // Particulate types
  weather.rainStreaks = (weather.type == WeatherType::Rain || weather.type == WeatherType::Thunderstorm)
                            ? weather.intensity * altFactor : 0.0f;
  weather.dustParticles = (weather.type == WeatherType::Sandstorm)
                            ? weather.intensity * altFactor : 0.0f;
  weather.acidCorrosion = (weather.type == WeatherType::AcidRain)
                            ? weather.intensity * altFactor : 0.0f;
  weather.frostIce = (weather.type == WeatherType::CryoSnow)
                            ? weather.intensity * altFactor : 0.0f;

  // Lightning strobe generation during electrical storms
  if ((weather.type == WeatherType::Thunderstorm || weather.type == WeatherType::Cyclone) &&
      weather.intensity > 0.35f && altFactor > 0.1f) {
    // Stochastic lightning strike trigger (~1 strike every few seconds)
    double strikeChance = std::fmod(simTime * 2.7 + latDeg * 3.1, 1.0);
    if (strikeChance > 0.985 && weather.lightningFlash < 0.1f) {
      weather.lightningFlash = 1.0f; // instant blinding discharge
    } else {
      // Exponential flash decay
      weather.lightningFlash = std::max(0.0f, weather.lightningFlash - dt * 6.5f);
    }
  } else {
    weather.lightningFlash = std::max(0.0f, weather.lightningFlash - dt * 8.0f);
  }
}

}  // namespace astro
