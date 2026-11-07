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
  std::string categoryRu;     // "Память", "Физика", "Стазис", "Биосфера", "Эхо", "Эврика"
  std::string categoryEn;     // "Memory", "Physics", "Stasis", "Biosphere", "Echo", "Eureka"
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
  bool synthesized = false;   // Synthesized via Mind Resonance / Eureka

  bool isMasterNode = false;  // Synthesized Master Node / Breakthrough Insight
  bool isCustom = false;      // Player-authored thought note
  std::vector<std::string> parentIds; // Component notes that yielded this master note
  int depthLevel = 0;         // 0: foreground, 1: background ancestor layer

  std::vector<std::string> links; // IDs of connected notes
};

enum class EurekaModel : int {
  QuantumWave = 0,          // Волновой коллапс / схождение
  OrbitalConstellation = 1, // Орбитальное вращение спутников
  SupernovaBurst = 2        // Взрыв сверхновой / расширение кольца
};

// Recipe defining the synthesis of a new breakthrough Master Node
struct ZettelSynthesisRecipe {
  std::string id;
  std::string cluster;
  std::string masterNoteId;
  std::vector<std::string> requiredNoteIds;
  std::string titleRu;
  std::string titleEn;
  std::string descriptionRu;
  std::string descriptionEn;
  bool synthesized = false;
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

  // Recipes Accessors
  const std::vector<ZettelSynthesisRecipe>& getRecipes() const { return m_recipes; }
  std::vector<ZettelSynthesisRecipe>& getRecipes() { return m_recipes; }
  const ZettelSynthesisRecipe* getRecipe(const std::string& id) const;
  bool canSynthesizeRecipe(const std::string& id) const;
  bool triggerSynthesisRecipe(const std::string& id);
  const ZettelSynthesisRecipe* findMatchingRecipe(const std::string& noteA, const std::string& noteB) const;
  const ZettelSynthesisRecipe* checkPendingSynthesis() const;

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
  int getMasterNodesCount() const;
  bool isAllMasterNodesSynthesized() const;
  int getTotalCount() const { return static_cast<int>(m_notes.size()); }
  int getTotalLinksCount() const;

  // Player-authored custom notes
  bool addCustomNote(const std::string& titleRu,
                     const std::string& textRu,
                     const std::string& categoryRu,
                     float posX = 0.0f,
                     float posY = 0.0f);
  bool deleteCustomNote(const std::string& id);

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
  std::vector<ZettelSynthesisRecipe> m_recipes;
  void addSeed(const ZettelNote& note);
};

std::string zettelkastenFilePathFor(const std::string& dir);

}  // namespace astro
