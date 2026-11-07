#pragma once

#include <string>

namespace astro {

// === Atmospheric Entry & Thermal Shield === (Stage 5): clean C++17, no GL.
struct AtmosphericBodyProfile {
  std::string bodyId;
  float scaleHeightKm = 8.5f;
  float surfaceDensityKgM3 = 1.225f;
  float entryCorridorMinAoADeg = 12.0f;
  float entryCorridorMaxAoADeg = 26.0f;
  float maxShieldTempK = 2600.0f;
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
};

AtmosphericBodyProfile getAtmosphericProfile(const std::string& bodyId);
void updateReentryTelemetry(ReentryTelemetry& telem,
                            const AtmosphericBodyProfile& profile,
                            float pitchDeg, float speedKps, float altitudeKm,
                            float dt);
bool isAtmosphericDescentActive(const ReentryTelemetry& telem);

}  // namespace astro
