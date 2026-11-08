#pragma once

#include <cmath>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace astro {

// A physical spectral line (Fraunhofer dark absorption or emission peak)
struct SpectralLine {
  float wavelengthNm = 0.0f;  // e.g. 656.3 nm for H-alpha
  float intensity = 1.0f;     // 0.0 to 1.0
  std::string element;        // "H-alpha", "O-III", "CH4", "Na-D", etc.
  bool isEmission = false;    // true = bright emission line, false = dark absorption line
};

// Physical astrophysical spectroscopy breakdown for an astronomical station
struct SpectralAnalysis {
  std::string stationId;
  std::string targetName;
  std::string spectralClass;    // e.g. "G2V Main Sequence", "Kerr SMBH Ergosphere"
  float surfaceTempK = 0.0f;    // Kelvin
  float magneticFieldTesla = 0.0f;
  std::string radiationLevel;   // "Low / Baseline", "Extreme Synchrotron", etc.
  bool hasBiosignature = false;
  std::vector<std::pair<std::string, float>> chemicalAbundance;  // element name -> percentage
  std::vector<SpectralLine> lines;
  std::string summaryRu;
  std::string summaryEn;
};

// Computes the physical spectral analysis for any catalog station.
// Applies relativistic Doppler factor: lambda_obs = lambda_0 * sqrt((1+beta)/(1-beta))
SpectralAnalysis analyzeSpectrum(const std::string& stationId, double dopplerBeta = 0.0);

// Codex encyclopedia card for an astronomical body
struct CodexEntry {
  std::string stationId;
  std::string nameRu;
  std::string nameEn;
  std::string categoryRu;
  std::string categoryEn;
  std::string descriptionRu;
  std::string descriptionEn;
  bool scanned = false;
  double scannedTimestamp = 0.0;
  int researchPoints = 100;     // Awarded on first scan
};

// Manages discovery progress and research points
class ScienceCodex {
 public:
  ScienceCodex();
  void init();

  const std::vector<CodexEntry>& getEntries() const { return m_entries; }
  const CodexEntry* getEntry(const std::string& stationId) const;

  bool isScanned(const std::string& stationId) const;
  bool scanObject(const std::string& stationId, double timestamp = 0.0);

  int getScannedCount() const;
  int getTotalCount() const;
  int getTotalResearchPoints() const { return m_totalPoints; }
  void addResearchPoints(int points) { m_totalPoints += points; }
  bool spendResearchPoints(int cost) {
    if (cost <= 0 || m_totalPoints < cost) return false;
    m_totalPoints -= cost;
    return true;
  }

  std::string serialize() const;
  bool deserialize(const std::string& data);

 private:
  std::vector<CodexEntry> m_entries;
  int m_totalPoints = 0;
};

// Live radio transmission on the bridge comms channel
struct RadioMessage {
  std::string id;
  std::string callsignRu;
  std::string callsignEn;
  std::string textRu;
  std::string textEn;
  std::string portraitPath;
  float priority = 1.0f;
  double durationSec = 7.5;
  double timestamp = 0.0;
};

// Bridge communications receiver with deduplication and active timer
class BridgeComms {
 public:
  BridgeComms() = default;

  void queueMessage(const RadioMessage& msg, double currentTime);
  void update(double currentTime);

  bool hasActiveMessage() const { return m_activeMessage.has_value(); }
  const RadioMessage* getActiveMessage() const {
    return m_activeMessage.has_value() ? &m_activeMessage.value() : nullptr;
  }
  float getMessageProgress(double currentTime) const;
  void dismiss();

  bool hasTriggered(const std::string& id) const;
  void markTriggered(const std::string& id);
  void resetTriggers();

 private:
  std::optional<RadioMessage> m_activeMessage;
  std::set<std::string> m_triggeredIds;
};

}  // namespace astro
