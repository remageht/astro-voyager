#include "Expedition.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace astro {

const std::vector<ExpeditionChapter>& expeditionChapters() {
  static const std::vector<ExpeditionChapter> chapters = [] {
    std::vector<ExpeditionChapter> c;
    c.push_back(ExpeditionChapter{"earth", "sol",
                                  "Глава 1 · Дом",
                                  "Chapter 1 · Home",
                                  "Прибыть в Солнечную систему и осмотреть "
                                  "Землю (станция Solar System).",
                                  "Reach the Solar System and see the Earth "
                                  "(Solar System station).",
                                  0});
    c.push_back(ExpeditionChapter{"sgra", "sgra",
                                  "Глава 2 · Сердце Галактики",
                                  "Chapter 2 · Heart of the Galaxy",
                                  "Совершить прыжок к Sgr A* - чёрной дыре "
                                  "в центре Млечного Пути.",
                                  "Jump to Sgr A*, the black hole at the "
                                  "center of the Milky Way.",
                                  1});
    c.push_back(ExpeditionChapter{"m87", "m87",
                                  "Глава 3 · Монстр с джетом",
                                  "Chapter 3 · The Jet Monster",
                                  "Наблюдать релятивистский джет M87* и "
                                  "доплер диска.",
                                  "Watch the relativistic jet of M87* and "
                                  "the Doppler disk.",
                                  2});
    c.push_back(ExpeditionChapter{"qso", "qso-3c273",
                                  "Глава 4 · Маяк ранней Вселенной",
                                  "Chapter 4 · Beacon of the Early Universe",
                                  "Оценить яркость квазара 3C 273.",
                                  "Gauge the brightness of quasar 3C 273.",
                                  3});
    c.push_back(ExpeditionChapter{"m13", "m13",
                                  "Глава 5 · Новые кандидаты",
                                  "Chapter 5 · New Candidates",
                                  "Обследовать скопление M13 и выбрать мир "
                                  "для колонии.",
                                  "Survey cluster M13 and choose the world "
                                  "for the colony.",
                                  4});
    c.push_back(ExpeditionChapter{"finale", "",
                                  "Глава 6 · Выбор",
                                  "Chapter 6 · The Choice",
                                  "Решить судьбу экспедиции: колония или "
                                  "дальний горизонт.",
                                  "Decide the fate of the expedition: colony "
                                  "or the far horizon.",
                                  5});
    return c;
  }();
  return chapters;
}

std::string finaleTitleRu(int choice) {
  if (choice == kFinaleHorizon) return "Финал · Дальний горизонт";
  if (choice == kFinaleColony) return "Финал · Новый Дом";
  return "Глава 6 · Выбор";
}

std::string finaleTitleEn(int choice) {
  if (choice == kFinaleHorizon) return "Finale · Far Horizon";
  if (choice == kFinaleColony) return "Finale · New Home";
  return "Chapter 6 · The Choice";
}

std::string finaleGoalRu(int choice) {
  if (choice == kFinaleHorizon)
    return "Экипаж ушёл к дальнему горизонту. История завершена.";
  if (choice == kFinaleColony)
    return "Колония основана у старых звёзд M13. История завершена.";
  return "Решить судьбу экспедиции: колония или дальний горизонт.";
}

std::string finaleGoalEn(int choice) {
  if (choice == kFinaleHorizon)
    return "The crew flew for the far horizon. Story complete.";
  if (choice == kFinaleColony)
    return "Colony founded by the old stars of M13. Story complete.";
  return "Decide the fate of the expedition: colony or the far horizon.";
}

void ExpeditionState::markVisited(const std::string& stationId) {
  const auto& chapters = expeditionChapters();
  if (chapterIndex >= static_cast<int>(chapters.size())) return;
  if (chapters[chapterIndex].station != stationId) return;
  // Финальная глава (N) - без цели: визиты её не двигают; достигнув её,
  // сюжет завершён (finished чует индекс последней главы).
  ++chapterIndex;
}

bool ExpeditionState::finished() const {
  // Сюжет завершён при достижении последней (финальной) главы.
  return chapterIndex >= static_cast<int>(expeditionChapters().size()) - 1;
}

const ExpeditionChapter* ExpeditionState::current() const {
  const auto& chapters = expeditionChapters();
  if (chapterIndex >= static_cast<int>(chapters.size())) return nullptr;
  return &chapters[chapterIndex];
}

std::string ExpeditionState::statusRu() const {
  const auto& chapters = expeditionChapters();
  const int shown =
      std::min(chapterIndex + 1, static_cast<int>(chapters.size()));
  std::ostringstream os;
  os << "Экспедиция: " << shown << "/" << chapters.size();
  return os.str();
}

std::string ExpeditionState::statusEn() const {
  const auto& chapters = expeditionChapters();
  const int shown =
      std::min(chapterIndex + 1, static_cast<int>(chapters.size()));
  std::ostringstream os;
  os << "Expedition: " << shown << "/" << chapters.size();
  return os.str();
}

std::string serializeExpedition(const ExpeditionState& st) {
  std::ostringstream os;
  os << "# expedition state (v3.5.0)\n";
  os << "chapterIndex=" << st.chapterIndex << "\n";
  os << "introSeen=" << (st.introSeen ? 1 : 0) << "\n";
  os << "finaleChoice=" << st.finaleChoice << "\n";
  return os.str();
}

bool deserializeExpedition(const std::string& text, ExpeditionState& out) {
  ExpeditionState probe;
  probe.chapterIndex = 0;
  probe.introSeen = false;
  std::istringstream is(text);
  std::string line;
  bool any = false;
  while (std::getline(is, line)) {
    const size_t eq = line.find('=');
    if (eq == std::string::npos || line.empty() || line[0] == '#') continue;
    const std::string key = line.substr(0, eq);
    const std::string val = line.substr(eq + 1);
    if (key == "chapterIndex") {
      probe.chapterIndex = std::atoi(val.c_str());
      if (probe.chapterIndex < 0 ||
          probe.chapterIndex > static_cast<int>(expeditionChapters().size())) {
        probe.chapterIndex = 0;
      }
      any = true;
    } else if (key == "introSeen") {
      probe.introSeen = (std::atoi(val.c_str()) != 0);
      any = true;
    } else if (key == "finaleChoice") {
      probe.finaleChoice = std::atoi(val.c_str());
      if (probe.finaleChoice < kFinaleUnset ||
          probe.finaleChoice > kFinaleHorizon) {
        probe.finaleChoice = kFinaleUnset;
      }
      any = true;
    }
  }
  if (!any) return false;
  out = probe;
  return true;
}

std::string expeditionFilePathFor(const std::string& dir) {
  return dir + "/expedition.json";
}

}  // namespace astro
