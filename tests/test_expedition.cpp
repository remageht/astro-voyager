// Expedition (v3.5.0): story chain + persistence, pure C++17.
#include "Expedition.h"

#include <iostream>
#include <string>

using astro::ExpeditionState;
using astro::expeditionChapters;
using astro::expeditionFilePathFor;
using astro::serializeExpedition;
using astro::deserializeExpedition;

int main() {
  // Test 1: fresh state - chapter 0 open, wrong targets ignored.
  {
    ExpeditionState st;
    if (st.finished()) {
      std::cerr << "Test 1 failed: fresh state claims finished\n";
      return 1;
    }
    st.markVisited("sgra");
    st.markVisited("m13");
    if (st.chapterIndex != 0) {
      std::cerr << "Test 1 failed: wrong-target visits advanced chapter ("
                << st.chapterIndex << ")\n";
      return 1;
    }
  }

  // Test 2: the full chain sol > sgra > m87 > qso-3c273 > m13 finishes.
  {
    ExpeditionState st;
    st.markVisited("sol");
    st.markVisited("sgra");
    st.markVisited("m87");
    st.markVisited("qso-3c273");
    if (st.finished()) {
      std::cerr << "Test 2 failed: finished before m13\n";
      return 1;
    }
    st.markVisited("m13");
    if (!st.finished()) {
      std::cerr << "Test 2 failed: chain did not finish at m13\n";
      return 1;
    }
    if (st.current() == nullptr) {
      std::cerr << "Test 2 failed: finale card must remain visible\n";
      return 1;
    }
    st.markVisited("sgra");  // visits after finale are harmless
    if (!st.finished()) {
      std::cerr << "Test 2 failed: post-finale visits un-finished the arc\n";
      return 1;
    }
  }

  // Test 3: out-of-order visits do not skip chapters.
  {
    ExpeditionState st;
    st.markVisited("m87");
    if (st.chapterIndex != 0) {
      std::cerr << "Test 3 failed: m87 first should not advance\n";
      return 1;
    }
    st.markVisited("sol");
    if (st.chapterIndex != 1) {
      std::cerr << "Test 3 failed: sol visit must open sgra chapter\n";
      return 1;
    }
    st.markVisited("sol");  // re-visit: no-op
    if (st.chapterIndex != 1) {
      std::cerr << "Test 3 failed: re-visit advanced chapter\n";
      return 1;
    }
  }

  // Test 4: serialization round trip and range clamping.
  {
    ExpeditionState st;
    st.chapterIndex = 3;
    st.introSeen = true;
    st.finaleChoice = astro::kFinaleHorizon;
    ExpeditionState out;
    if (!deserializeExpedition(serializeExpedition(st), out) ||
        out.chapterIndex != 3 || !out.introSeen ||
        out.finaleChoice != astro::kFinaleHorizon) {
      std::cerr << "Test 4 failed: round trip lost state\n";
      return 1;
    }
    const std::string upper =
        serializeExpedition(st) +
        "chapterIndex=999999\nintroSeen=7\nfinaleChoice=99\n";
    ExpeditionState clamped;
    if (!deserializeExpedition(upper, clamped)) {
      std::cerr << "Test 4 failed: request clamped values rejected\n";
      return 1;
    }
    if (clamped.chapterIndex != 0) {
      std::cerr << "Test 4 failed: out-of-range index must clamp to 0, got "
                << clamped.chapterIndex << '\n';
      return 1;
    }
    if (clamped.finaleChoice != astro::kFinaleUnset) {
      std::cerr << "Test 4 failed: out-of-range finale choice must clamp to 0, got "
                << clamped.finaleChoice << '\n';
      return 1;
    }
  }

  // Test 5: garbage / empty file rejected, state untouched.
  {
    ExpeditionState st;
    ExpeditionState out;
    out.chapterIndex = 2;
    if (deserializeExpedition("garbage\r\n---\r\n", out)) {
      std::cerr << "Test 5 failed: garbage accepted\n";
      return 1;
    }
    if (out.chapterIndex != 2 || out.introSeen) {
      std::cerr << "Test 5 failed: rejected parse mutated the state\n";
      return 1;
    }
    (void)st;
  }

  // Test 6: catalog shape - 6 chapters, unique targets and slides.
  {
    const auto& chapters = expeditionChapters();
    if (chapters.size() != 6) {
      std::cerr << "Test 6 failed: expected 6 chapters, got " << chapters.size()
                << '\n';
      return 1;
    }
    std::string seen;
    size_t goals = 0;
    for (const auto& c : chapters) {
      if (!c.station.empty()) {
        if (seen.find("[" + c.station + "]") != std::string::npos) {
          std::cerr << "Test 6 failed: duplicate target " << c.station << '\n';
          return 1;
        }
        seen += "[" + c.station + "]";
        ++goals;
      }
      if (c.slideIdx < 0) {
        std::cerr << "Test 6 failed: chapter " << c.id << " lacks a slide\n";
        return 1;
      }
    }
    if (goals != 5) {
      std::cerr << "Test 6 failed: expected 5 station chapters, got " << goals
                << '\n';
      return 1;
    }
  }

  // Test 7: status labels and file path.
  {
    ExpeditionState st;
    if (st.statusRu() != "Экспедиция: 1/6" || st.statusEn() != "Expedition: 1/6") {
      std::cerr << "Test 7 failed: unexpected status label\n";
      return 1;
    }
    st.markVisited("sol");
    if (st.statusRu() != "Экспедиция: 2/6") {
      std::cerr << "Test 7 failed: status did not advance: " << st.statusRu()
                << '\n';
      return 1;
    }
    st.markVisited("sgra");
    st.markVisited("m87");
    st.markVisited("qso-3c273");
    st.markVisited("m13");
    if (st.statusRu() != "Экспедиция: 6/6") {
      std::cerr << "Test 7 failed: finale label wrong: " << st.statusRu()
                << '\n';
      return 1;
    }
    if (expeditionFilePathFor("D/user").find("/expedition.json") ==
        std::string::npos) {
      std::cerr << "Test 7 failed: bad expedition file name\n";
      return 1;
    }
  }

  // Test 8: Adventure finale branch labels differ per ending.
  {
    const std::string colony = astro::finaleTitleRu(astro::kFinaleColony);
    const std::string horizon = astro::finaleTitleRu(astro::kFinaleHorizon);
    if (colony.empty() || horizon.empty() || colony == horizon) {
      std::cerr << "Test 8 failed: finale titles must differ\n";
      return 1;
    }
    if (astro::finaleGoalRu(astro::kFinaleColony) ==
        astro::finaleGoalRu(astro::kFinaleHorizon)) {
      std::cerr << "Test 8 failed: finale goals must differ\n";
      return 1;
    }
    if (astro::finaleTitleEn(astro::kFinaleColony) ==
        astro::finaleTitleEn(astro::kFinaleHorizon)) {
      std::cerr << "Test 8 failed: EN finale titles must differ\n";
      return 1;
    }
    // Legacy expedition.json (pre-4.1) has no finaleChoice: defaults to unset
    // and still reports the finished arc.
    ExpeditionState legacy;
    if (!deserializeExpedition("chapterIndex=5\nintroSeen=1\n", legacy) ||
        legacy.finaleChoice != astro::kFinaleUnset || !legacy.finished()) {
      std::cerr << "Test 8 failed: legacy finished save must load unset\n";
      return 1;
    }
    // The chosen ending survives a save/load cycle.
    legacy.finaleChoice = astro::kFinaleColony;
    ExpeditionState reloaded;
    if (!deserializeExpedition(serializeExpedition(legacy), reloaded) ||
        reloaded.finaleChoice != astro::kFinaleColony || !reloaded.finished()) {
      std::cerr << "Test 8 failed: chosen ending did not persist\n";
      return 1;
    }
  }

  std::cout << "test_expedition: ALL PASS\n";
  return 0;
}
