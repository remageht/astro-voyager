#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <set>

namespace astro {

// Clusters of knowledge corresponding to key finale paths
constexpr const char* kClusterSolar = "solar";            // Солнечный Синтез (Elena Rostova / Reignite the Sun)
constexpr const char* kClusterStasis = "stasis";          // Пакт Вселенной-05 (Arseny / 5th Universe Alliance)
constexpr const char* kClusterEden = "eden";              // Новый Эдем (Mia Sato / Colony at M13)
constexpr const char* kClusterMultiverse = "multiverse";  // Прорыв Мультивселенной (Egorka-6 / Loop Transcendence)

// Atomic thought note in Egorka's Second Brain
struct ZettelNote {
  std::string id;             // Unique key, e.g. "z_paris_rain"
  std::string cluster;        // "solar", "stasis", "eden", "multiverse"
  std::string categoryRu;     // "Память", "Физика", "Стазис", "Биосфера", "Эхо"
  std::string categoryEn;     // "Memory", "Physics", "Stasis", "Biosphere", "Echo"
  std::string titleRu;
  std::string titleEn;
  std::string textRu;         // Insight / reflection
  std::string textEn;
  std::string quoteRu;        // Character dialogue / memory quote
  std::string quoteEn;
  std::string speakerRu;      // Speaker name
  std::string speakerEn;

  float posX = 0.0f;          // 2D position in canvas space
  float posY = 0.0f;

  bool seedDiscovered = false;// Found through exploration / story
  bool synthesized = false;   // Synthesized via Mind Resonance mini-game

  std::vector<std::string> links; // IDs of connected notes
};

class ZettelGraph {
 public:
  ZettelGraph();
  void init();

  // Accessors
  const std::vector<ZettelNote>& getNotes() const { return m_notes; }
  std::vector<ZettelNote>& getNotes() { return m_notes; }
  const ZettelNote* getNote(const std::string& id) const;
  ZettelNote* getNote(const std::string& id);

  // Link connections
  bool linkNotes(const std::string& idA, const std::string& idB);
  bool unlinkNotes(const std::string& idA, const std::string& idB);
  bool areLinked(const std::string& idA, const std::string& idB) const;

  // State modifiers
  bool discoverSeed(const std::string& id);
  bool synthesizeNote(const std::string& id);

  // Discovery progression based on expedition stations & telemetry
  void refresh(const std::vector<std::string>& visitedStations,
               double minBhDistance = 1e9,
               bool journeyExported = false);

  // Cluster calculations
  float getClusterProgress(const std::string& cluster) const; // 0.0f .. 1.0f
  bool isClusterTrueEndingUnlocked(const std::string& cluster) const;

  int getDiscoveredCount() const;
  int getSynthesizedCount() const;
  int getTotalCount() const { return static_cast<int>(m_notes.size()); }
  int getTotalLinksCount() const;

  // Serialization / Deserialization
  std::string serialize() const;
  bool deserialize(const std::string& data);

  // Narrative descriptions
  static std::string getClusterTitleRu(const std::string& cluster);
  static std::string getClusterTitleEn(const std::string& cluster);
  static std::string getClusterDescriptionRu(const std::string& cluster);
  static std::string getClusterDescriptionEn(const std::string& cluster);
  static std::string getTrueEndingEpilogueRu(const std::string& cluster);
  static std::string getTrueEndingEpilogueEn(const std::string& cluster);

 private:
  std::vector<ZettelNote> m_notes;
  void addSeed(const ZettelNote& note);
};

std::string zettelkastenFilePathFor(const std::string& dir);

}  // namespace astro
