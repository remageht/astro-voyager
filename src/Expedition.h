#pragma once

#include <string>
#include <vector>

namespace astro {

// Expedition (v3.5.0): the story arc of the journey - a person on Earth
// dreams of space, the Sun is dying, the crew departs to find a new planet
// (intro slides in StorySlides). Chapters chain the existing stations:
// Earth (sol) -> Sgr A* -> M87* -> quasar 3C 273 -> cluster M13 -> finale.
// State persists to its own small file (expedition.json next to the game
// profiles) so the SaveGame schema stays pinned at v1.
struct ExpeditionChapter {
  std::string id;        // chapter key (for tests/logs)
  std::string station;   // station id that completes the chapter
  std::string titleRu;
  std::string titleEn;
  std::string goalRu;
  std::string goalEn;
  int slideIdx = -1;     // index into storyChapterSlides() shown on completion
};

const std::vector<ExpeditionChapter>& expeditionChapters();

class ExpeditionState {
 public:
  // Index of the current chapter ([0..chapters().size()]); reaching
  // chapters().size() means the story is finished.
  int chapterIndex = 0;
  bool introSeen = false;

  void markVisited(const std::string& stationId);  // advances chapters
  bool finished() const;
  const ExpeditionChapter* current() const;
  std::string statusRu() const;  // "Глава 2/5" label of the current stage
  std::string statusEn() const;
};

// Flat text serialization: chapterIndex=<int>, introSeen=<0|1> - tolerant
// parser mirroring the SaveGame mini-reader style.
std::string serializeExpedition(const ExpeditionState& st);
bool deserializeExpedition(const std::string& text, ExpeditionState& out);

std::string expeditionFilePathFor(const std::string& dir);

}  // namespace astro
