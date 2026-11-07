#pragma once

#include <cmath>
#include <string>
#include <vector>

namespace astro {

// State of deployable hydraulic landing gear
enum class GearState {
  Retracted = 0,
  Deploying = 1,
  Deployed = 2,
  Retracting = 3
};

// Touchdown rating evaluated on surface contact
enum class TouchdownRating {
  Airborne = 0,
  Safe = 1,   // |v_v| <= 2.0 m/s with gear locked
  Hard = 2,   // 2.0 < |v_v| <= 5.0 m/s with gear locked
  Crash = 3   // |v_v| > 5.0 m/s or landing gear retracted
};

// Dynamic telemetry for planetary approach and landing operations
struct LandingTelemetry {
  GearState gearState = GearState::Retracted;
  float gearProgress = 0.0f;        // 0.0 (retracted) .. 1.0 (fully locked)
  float radarAltM = -1.0f;          // Altitude above terrain in meters (< 0 in deep space)
  float verticalSpeedMs = 0.0f;     // Vertical velocity (m/s; negative is descent)
  TouchdownRating touchdown = TouchdownRating::Airborne;
  float suspensionCompression = 0.0f; // 0.0 .. 1.0 hydraulic strut compression
  float surfaceDustK = 0.0f;        // 0.0 .. 1.0 dust kickup opacity
  bool isLanded = false;
  std::string targetBodyId = "earth";
  std::string targetBodyName = "Земля";
  float surfaceGravity = 9.81f;     // m/s^2
  float surfacePressureBar = 1.013f;// bar
  float surfaceTempK = 288.15f;     // Kelvin
};

// Surface expedition science report
struct SurfaceReport {
  std::string bodyId;
  std::string nameRu;
  std::string nameEn;
  std::string biomeRu;
  std::string biomeEn;
  float gravityMps2 = 0.0f;
  float pressureBar = 0.0f;
  float tempK = 0.0f;
  std::string geologyRu;
  std::string geologyEn;
  std::string atmosphereRu;
  std::string atmosphereEn;
  int researchPointsReward = 150;
  bool samplesCollected = false;
};

// Update landing gear extension and retraction animation (1.5 seconds)
void updateLandingGear(LandingTelemetry& telem, float deltaTime, bool toggleRequested);

// Process physical ground contact, strut dampening, and damage response
void updateTouchdownPhysics(LandingTelemetry& telem,
                           float deltaTime,
                           float altitudeM,
                           float verticalVelMs,
                           float thrustK,
                           float& outHullDamage,
                           float& outShieldDamage);

// Generates canonical surface telemetry and geology report for catalog bodies
SurfaceReport generateSurfaceReport(const std::string& bodyId);

// === v5.3.0 Surface Science & Core Sampler ===
enum class CoreDrillState {
  Idle = 0,
  Drilling = 1,
  Retracting = 2,
  Extracted = 3,
  Overheated = 4
};

struct CoreStrataLayer {
  float depthMinM = 0.0f;
  float depthMaxM = 0.0f;
  std::string layerNameRu;
  std::string layerNameEn;
  std::string compositionRu;
  std::string compositionEn;
  float hardness = 0.5f;
  float densityGcm3 = 2.0f;
  float tempK = 300.0f;
};

struct CoreDrillTelemetry {
  CoreDrillState state = CoreDrillState::Idle;
  float depthM = 0.0f;
  float maxDepthM = 5.0f;
  float rpm = 0.0f;
  float bitTempC = 20.0f;
  float motorTorque = 0.0f;
  float penetrationRate = 0.0f;
  bool sampleExtracted = false;
  bool sampleAnalyzed = false;
  std::string primaryMineralRu;
  std::string primaryMineralEn;
  std::string rareElementsRu;
  std::string rareElementsEn;
  float helium3Ppm = 0.0f;
  float volatileWaterPercent = 0.0f;
  float volatileWaterEquivalentPercent = 0.0f;
  std::vector<CoreStrataLayer> strata;
  int scienceYield = 250;
};

void updateCoreDrill(CoreDrillTelemetry& drill, float deltaTime, bool drillInput, bool retractInput,
                     float surfaceTempK, float surfaceGravity);
CoreDrillTelemetry generateCoreDrillProfile(const std::string& bodyId, float surfaceTempK,
                                           float surfaceGravity);
std::string formatCoreSampleZettel(const std::string& bodyId, const std::string& bodyNameRu,
                                   const CoreDrillTelemetry& drill);

}  // namespace astro
