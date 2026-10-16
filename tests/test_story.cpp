// StoryJournal (v3.5.0): unlock/read logic of the ship's log, pure C++17.
#include "StoryJournal.h"

#include <iostream>
#include <string>
#include <vector>

using astro::StoryJournal;

int main() {
  // Test 1: with empty progress exactly one entry (departure) is unlocked.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    j.refresh(p);
    if (j.unlockedCount() != 1) {
      std::cerr << "Test 1 failed: expected 1 unlocked on empty progress, got "
                << j.unlockedCount() << '\n';
      return 1;
    }
    bool depOk = false;
    for (const auto& e : j.entries()) {
      if (e.id == "departure") depOk = e.unlocked;
    }
    if (!depOk) {
      std::cerr << "Test 1 failed: departure entry missing/locked\n";
      return 1;
    }
  }

  // Test 2: first-steps unlocks at 2 visited stations, not at 1.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.visited = {"sgra"};
    j.refresh(p);
    if (j.unlockedCount() != 2) {  // departure + sgra
      std::cerr << "Test 2 failed: sgra visit should unlock sgra only, got "
                << j.unlockedCount() << '\n';
      return 1;
    }
    p.visited.push_back("m13");
    j.refresh(p);
    if (j.unlockedCount() != 4) {  // + first-steps + m13
      std::cerr << "Test 2 failed: two visits should unlock first-steps, got "
                << j.unlockedCount() << '\n';
      return 1;
    }
  }

  // Test 3: qso unlocks via the station id "qso-3c273".
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.visited = {"qso-3c273"};
    j.refresh(p);
    bool qsoOk = false;
    for (const auto& e : j.entries()) {
      if (e.id == "qso") qsoOk = e.unlocked;
    }
    if (!qsoOk) {
      std::cerr << "Test 3 failed: qso-3c273 visit did not unlock qso\n";
      return 1;
    }
  }

  // Test 4: close-approach boundary at 5 Schwarzschild radii.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.minBhDistance = 5.0;
    j.refresh(p);
    for (const auto& e : j.entries()) {
      if (e.id == "close-approach" && e.unlocked) {
        std::cerr << "Test 4 failed: 5.0 must stay locked\n";
        return 1;
      }
    }
    p.minBhDistance = 4.9;
    j.refresh(p);
    bool ok = false;
    for (const auto& e : j.entries()) {
      if (e.id == "close-approach") ok = e.unlocked;
    }
    if (!ok) {
      std::cerr << "Test 4 failed: 4.9 should unlock close-approach\n";
      return 1;
    }
  }

  // Test 5: archive unlocks on export OR import.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    j.refresh(p);
    p.imported = true;
    j.refresh(p);
    if (j.unlockedCount() != 2) {
      std::cerr << "Test 5 failed: import should unlock archive, got "
                << j.unlockedCount() << '\n';
      return 1;
    }
  }

  // Test 6: grand-tour unlocks at 8 visited stations.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.visited = {"sgra", "m87", "qso-3c273", "m13", "m31", "m51", "m104", "ori"};
    j.refresh(p);
    int expect = 0;  // departure, first-steps, sgra, m87, qso, m13, grand-tour
    for (const auto& e : j.entries()) {
      if (e.unlocked) ++expect;
    }
    if (expect != 7) {
      std::cerr << "Test 6 failed: grand tour should give 7 unlocked, got "
                << expect << '\n';
      return 1;
    }
  }

  // Test 7: horizon capture unlocks the horizon entry.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.capturedHorizon = true;
    j.refresh(p);
    if (j.unlockedCount() != 2) {
      std::cerr << "Test 7 failed: horizon capture did not unlock horizon\n";
      return 1;
    }
  }

  // Test 8: unread counting and markAllRead; read marks survive refresh.
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.visited = {"sgra", "m87"};
    j.refresh(p);
    if (j.unreadCount() != 4) {
      std::cerr << "Test 8 failed: expected 4 unread, got " << j.unreadCount()
                << '\n';
      return 1;
    }
    j.markAllRead();
    if (j.unreadCount() != 0) {
      std::cerr << "Test 8 failed: markAllRead left unread entries\n";
      return 1;
    }
    p.visited.push_back("m13");  // progress grows; read marks must persist
    j.refresh(p);
    if (j.unreadCount() != 1) {
      std::cerr << "Test 8 failed: refresh should not mark old entries unread, got "
                << j.unreadCount() << '\n';
      return 1;
    }
  }

  // Test 9: unlock state and read marks survive many refreshes (idempotent).
  {
    StoryJournal j;
    StoryJournal::Progress p;
    p.visited = {"sgra"};
    j.refresh(p);
    const int before = j.unlockedCount();
    j.refresh(p);
    j.refresh(p);
    if (j.unlockedCount() != before) {
      std::cerr << "Test 9 failed: repeated refresh changed unlock state\n";
      return 1;
    }
    if (j.entries().size() != 10) {
      std::cerr << "Test 9 failed: catalog size drifted ("
                << j.entries().size() << ")\n";
      return 1;
    }
  }

  if (astro::storyEntryHasVisited({"a", "sgra"}, "sgra") &&
      !astro::storyEntryHasVisited({"a", "b"}, "sgra")) {
    std::cout << "test_story: ALL PASS\n";
    return 0;
  }
  std::cerr << "Test 10 failed: storyEntryHasVisited broken\n";
  return 1;
}
