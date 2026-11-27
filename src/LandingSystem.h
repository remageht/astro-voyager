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
  float surfaceSlopeDeg = 0.0f;     // v5.5.0: Terrain surface inclination angle (degrees)
  bool slopeHazard = false;         // v5.5.0: true if surfaceSlopeDeg > 15.0f
  float variometerAlpha = 0.0f;     // v5.5.0: HUD variometer & wireframe dynamic alpha (0..1)
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

// Evaluates procedural terrain slope angle in degrees
float computePlanetarySurfaceSlope(const std::string& bodyId, float x, float z);

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

// === v5.1.0 Stage 6 Autonomous Surface Reconnaissance Drone (10 km Radius) ===
enum class ReconDroneState {
  Docked = 0,
  Deploying = 1,
  Scouting = 2,
  Returning = 3,
  DockedComplete = 4
};

struct SurfaceAnomaly {
  std::string id;
  std::string nameRu;
  std::string nameEn;
  std::string typeRu;
  std::string typeEn;
  float posXKm = 0.0f;
  float posZKm = 0.0f;
  float distanceKm = 0.0f;
  float signalStrength = 1.0f;
  int rpReward = 80;
  bool scanned = false;
};

struct ReconDroneTelemetry {
  std::string bodyId = "mars";
  ReconDroneState state = ReconDroneState::Docked;
  float posXKm = 0.0f;
  float posZKm = 0.0f;
  float rangeRadiusKm = 10.0f;
  float batteryPct = 100.0f;
  float altitudeM = 0.0f;
  float speedKmh = 0.0f;
  float distanceTraveledKm = 0.0f;
  float headingDeg = 0.0f;
  int targetIndex = -1;
  int scannedCount = 0;
  int totalRpEarned = 0;
  float scanDwellSec = 0.0f;
  bool reportLogged = false;
  std::vector<SurfaceAnomaly> anomalies;
};

ReconDroneTelemetry createReconDroneMission(const std::string& bodyId);
bool deployReconDrone(ReconDroneTelemetry& drone);
void recallReconDrone(ReconDroneTelemetry& drone);
void updateReconDrone(ReconDroneTelemetry& drone, float deltaTime, bool isLanded, int& outRpEarned);
std::string formatReconDroneZettel(const std::string& bodyId, const std::string& bodyNameRu,
                                   const ReconDroneTelemetry& drone);

}  // namespace astro
