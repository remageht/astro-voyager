#pragma once

#include <string>
#include <vector>

namespace astro {

// One narrative entry of the ship's log ("Story Journal", v3.5.0).
struct StoryEntry {
  std::string id;
  std::string titleRu;
  std::string titleEn;
  std::string textRu;
  std::string textEn;
  bool unlocked = false;
  bool read = false;
};

// StoryJournal derives unlocked story beats from the SAME progress flags the
// save profile already stores (visited stations, BH proximity record,
// editor export/import, horizon capture). No new save state and no schema
// bump: entries are recomputed deterministically on every refresh, read
// marks stay session-local. Bilingual (ru/en): the HUD picks a language per
// call (uiRu), texts are UTF-8 literals.
class StoryJournal {
 public:
  // Progress snapshot the host (AppGL) fills from its journey state.
  struct Progress {
    std::vector<std::string> visited;
    double minBhDistance = 1e9;
    bool exported = false;
    bool imported = false;
    bool capturedHorizon = false;
  };

  void refresh(const Progress& progress);

  const std::vector<StoryEntry>& entries() const { return m_entries; }
  int unlockedCount() const;
  int unreadCount() const;
  void markAllRead();

 private:
  std::vector<StoryEntry> m_entries;
};

// True when the visited list contains id (shared helper for tests).
bool storyEntryHasVisited(const std::vector<std::string>& visited,
                          const std::string& id);

}  // namespace astro
