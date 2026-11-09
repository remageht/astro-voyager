#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace astro {

class ExpeditionState;
class ZettelGraph;

struct EpilogueStats {
  int visitedStationsCount = 0;
  int totalStationsCount = 0;
  int coreDrillSamplesCount = 0;
  int synthesizedZettelsCount = 0;
  int masterNodesCount = 0;
  int researchPoints = 0;
  int warpJumpsCount = 0;
  float flightHours = 0.0f;
  float hullIntegrityPercent = 100.0f;
  float discoveryPercent = 0.0f;
};

struct CrewFate {
  std::string nameRu;
  std::string nameEn;
  std::string roleRu;
  std::string roleEn;
  std::string fateRu;
  std::string fateEn;
  std::string finalQuoteRu;
  std::string finalQuoteEn;
};

struct EpilogueChronicle {
  int endingChoice = 0;
  bool isTrueEnding = false;
  std::string clusterKey;
  std::string titleRu;
  std::string titleEn;
  std::string subtitleRu;
  std::string subtitleEn;
  std::string anchorStationId;
  std::vector<std::string> narrativeParagraphsRu;
  std::vector<std::string> narrativeParagraphsEn;
  std::vector<CrewFate> crewFates;
  EpilogueStats stats;
  std::string closingQuoteRu;
  std::string closingQuoteEn;
};

// Generates the comprehensive epilogue chronicle for any finale choice,
// tailoring narrative arcs, crew fates, and anchor coordinates to whether
// the player synthesized the respective Grand Synthesis master node.
EpilogueChronicle generateEpilogueChronicle(
    int endingChoice,
    const ExpeditionState& expedition,
    const ZettelGraph& zettelkasten,
    int coreDrillCount,
    int researchPoints,
    int warpJumps,
    float hullIntegrity);

// Formats a structured ASCII / Unicode expedition logbook suitable for
// terminal output, export files, or scroll inspection.
std::string formatChronicleLogbook(const EpilogueChronicle& chronicle, bool russian);

// Resolves the primary celestial focal station for the procedural orbital camera flight.
std::string getAnchorStationForEnding(int endingChoice, bool isTrueEnding);

// === Solar Rebirth cinematic (procedural) ===
// Spiral approach to Sun: radius decays 30 -> 8 Rs, FOV kick on perihelion,
// solar glow pulse. Stats modulate depth (hull, warp jumps).
struct SolarRebirthCamera {
  float radius = 30.0f;
  float height = 6.0f;
  float angle = 0.0f;
  float fovKick = 0.0f;     // +deg on perihelion
  float solarGlow = 0.0f;   // 0..1 flare intensity
};
SolarRebirthCamera solarRebirthCamera(float t, const EpilogueStats& stats);

// === Blossoming Eden cinematic (procedural) ===
// Orbital descent over exoplanet Hercules-b: radius descends 24 -> 6.5,
// polar to equatorial terminator glide, banking roll sway, atmospheric dawn shimmer.
struct EdenDescentCamera {
  float radius = 24.0f;
  float height = 8.0f;
  float angle = 0.0f;
  float rollBank = 0.0f;    // rad banking angle along flight vector
  float fovKick = 0.0f;     // panoramic FOV expansion
  float atmoShimmer = 0.0f; // 0..1 atmospheric dawn glow
};
EdenDescentCamera edenDescentCamera(float t, const EpilogueStats& stats);

// === Architects of Infinity cinematic (procedural) ===
// Relativistic Kerr black hole plunge around Sagittarius A*:
// radius spirals 28 -> 5.2 into the ergosphere, relativistic frame-dragging
// angular acceleration, Cauchy horizon gravitational lensing & quantum aberration.
struct MultiverseSingularityCamera {
  float radius = 28.0f;
  float height = 4.0f;
  float angle = 0.0f;
  float horizonDistortion = 0.0f; // 0..1 tidal lensing strength
  float quantumAberration = 0.0f; // 0..1 chromatic aberration
  float fovKick = 0.0f;
};
MultiverseSingularityCamera multiverseSingularityCamera(float t, const EpilogueStats& stats);

// === Brotherhood of Five Realities cinematic (procedural) ===
// Sweeping 3D Lissajous & lemniscate weave through the dense stellar core of M13,
// tracking the ancient Precursor stasis relay beacon with rhythmic shield pulses.
struct AllianceRelicCamera {
  float radius = 18.0f;
  float height = 3.5f;
  float angle = 0.0f;
  float stasisShieldPulse = 0.0f; // 0..1 stasis field aura
  float fovKick = 0.0f;
};
AllianceRelicCamera allianceRelicCamera(float t, const EpilogueStats& stats);

// Unified frame output for epilogue cutscenes
struct EpilogueCinematicFrame {
  Vec3 cameraPosition{0.0, 6.0, 30.0};
  Vec3 lookTarget{0.0, 0.0, 0.0};
  float fovOffset = 0.0f;
  float cameraRoll = 0.0f;
  int colorGrade = 0;             // 0=Neutral, 1=Amber, 2=Neon, 3=Mono, 4=Vintage
  float filmicVignette = 0.0f;    // 0..1 smooth lens vignette
  float chromaK = 0.0f;           // 0..1 chromatic aberration
  float horizonK = 0.0f;          // 0..1 gravitational lens
  float shieldK = 0.0f;           // 0..1 edge aura glow
  float auraIntensity = 0.0f;     // 0..1 HUD badge pulse
  Vec3 auraColor{1.0, 1.0, 1.0};  // RGB aura tint
  std::string badgeRu;
  std::string badgeEn;
};

// Evaluates the comprehensive cinematic trajectory and post-fx frame for any ending
EpilogueCinematicFrame evaluateEpilogueCinematic(
    int endingChoice,
    const std::string& clusterKey,
    bool isTrueEnding,
    float t,
    const EpilogueStats& stats);

}  // namespace astro
