#pragma once

#include <string>
#include <vector>

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

}  // namespace astro
