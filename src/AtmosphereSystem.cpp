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
  } else if (id == "mars") {
    p.scaleHeightKm = 11.1f;
    p.surfaceDensityKgM3 = 0.0158f;
  } else if (id == "venus") {
    p.scaleHeightKm = 15.9f;
    p.surfaceDensityKgM3 = 65.0f;
  } else if (id == "titan") {
    p.scaleHeightKm = 9.0f;
    p.surfaceDensityKgM3 = 5.3f;
  } else if (id == "hercules-b" || id == "hercules_b") {
    p.scaleHeightKm = 7.5f;
    p.surfaceDensityKgM3 = 2.1f;
  } else {
    p.scaleHeightKm = 8.5f;
    p.surfaceDensityKgM3 = 1.225f;
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

  // Heat flux q ~ sqrt(rho) * v^3 (normalized) and Stefan-Boltzmann cooling
  float v = speedKps * 1000.0f;  // m/s
  float qHeat = 0.0f;
  if (v > 1.0f) {
    float rho = std::max(telem.density, 0.0f);
    // normalized: at rho=1.225, v=7000 -> q ~ 1e6 scale; we map to K/s
    qHeat = 2.5e-3f * std::sqrt(rho) * v * v * v / 1.0e6f;  // arbitrary scale -> K/s
    qHeat = 180.0f * std::sqrt(rho) * (v / 7000.0f) * (v / 7000.0f) * (v / 7000.0f) * 40.0f;
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

  // gForce from deceleration ~ rho * v^2
  float g = 0.0f;
  if (v > 1.0f) {
    g = telem.density * v * v / 500000.0f;
  }
  telem.gForce = std::clamp(g, 0.0f, 12.0f);
}

bool isAtmosphericDescentActive(const ReentryTelemetry& telem) {
  return (telem.altitudeKm < 120.0f && telem.speedKps > 2.2f);
}

}  // namespace astro
