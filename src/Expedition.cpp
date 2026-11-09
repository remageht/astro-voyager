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
    // v4.1: интерлюдия на орбите Земли. Завершается стыковкой с любым из
    // двух исторических кораблей - Dragon или «Союзом-МС» (цели через '|').
    c.push_back(ExpeditionChapter{"fleet", "dock-dragon|dock-soyuz",
                                  "Глава 2 · Последний караул",
                                  "Chapter 2 · The Last Watch",
                                  "Состыковаться с Crew Dragon или «Союзом-МС» "
                                  "на орбите Земли - попрощаться с первым "
                                  "орбитальным флотом.",
                                  "Dock with Crew Dragon or Soyuz-MS in Earth "
                                  "orbit - bid farewell to the first orbital "
                                  "fleet.",
                                  1});
    c.push_back(ExpeditionChapter{"sgra", "sgra",
                                  "Глава 3 · Сердце Галактики",
                                  "Chapter 3 · Heart of the Galaxy",
                                  "Совершить прыжок к Sgr A* - чёрной дыре "
                                  "в центре Млечного Пути.",
                                  "Jump to Sgr A*, the black hole at the "
                                  "center of the Milky Way.",
                                  2});
    c.push_back(ExpeditionChapter{"m87", "m87",
                                  "Глава 4 · Монстр с джетом",
                                  "Chapter 4 · The Jet Monster",
                                  "Наблюдать релятивистский джет M87* и "
                                  "доплер диска.",
                                  "Watch the relativistic jet of M87* and "
                                  "the Doppler disk.",
                                  3});
    c.push_back(ExpeditionChapter{"qso", "qso-3c273",
                                  "Глава 5 · Маяк ранней Вселенной",
                                  "Chapter 5 · Beacon of the Early Universe",
                                  "Оценить яркость квазара 3C 273.",
                                  "Gauge the brightness of quasar 3C 273.",
                                  4});
    c.push_back(ExpeditionChapter{"m13", "m13",
                                  "Глава 6 · Новые кандидаты",
                                  "Chapter 6 · New Candidates",
                                  "Обследовать скопление M13 и выбрать мир "
                                  "для колонии.",
                                  "Survey cluster M13 and choose the world "
                                  "for the colony.",
                                  5});
    c.push_back(ExpeditionChapter{"finale", "",
                                  "Глава 7 · Выбор",
                                  "Chapter 7 · The Choice",
                                  "Решить судьбу экспедиции: колония или "
                                  "дальний горизонт.",
                                  "Decide the fate of the expedition: colony "
                                  "or the far horizon.",
                                  6});
    return c;
  }();
  return chapters;
}

std::string finaleTitleRu(int choice) {
  if (choice == kFinaleHorizon) return "Финал · Дальний горизонт";
  if (choice == kFinaleColony) return "Финал · Новый Дом";
  if (choice == kFinaleSolarRebirth) return "Финал · Возрождение Солнца";
  if (choice == kFinaleLoopTranscendence) return "Финал · Петля Времени";
  if (choice == kFinaleArkCitadel) return "Финал · Ковчег Человечества";
  if (choice == kFinaleHyperBreakthrough) return "Финал · Гиперпрорыв Шестой Вселенной";
  if (choice == kFinaleFifthAlliance) return "Финал · Пакт Палача Ложных Надежд";
  if (choice == kFinaleDignity) return "Финал · Восхождение Достоинства";
  return "Глава 6 · Выбор";
}

std::string finaleTitleEn(int choice) {
  if (choice == kFinaleHorizon) return "Finale · Far Horizon";
  if (choice == kFinaleColony) return "Finale · New Home";
  if (choice == kFinaleSolarRebirth) return "Finale · Solar Rebirth";
  if (choice == kFinaleLoopTranscendence) return "Finale · Temporal Loop";
  if (choice == kFinaleArkCitadel) return "Finale · Ark Citadel";
  if (choice == kFinaleHyperBreakthrough) return "Finale · Sixth Universe Hyper-Breakthrough";
  if (choice == kFinaleFifthAlliance) return "Finale · Pact of the Executioner of False Hopes";
  if (choice == kFinaleDignity) return "Finale · Ascent of Dignity";
  return "Chapter 6 · The Choice";
}

std::string finaleGoalRu(int choice) {
  if (choice == kFinaleHorizon)
    return "Экипаж ушёл к дальнему горизонту. История завершена.";
  if (choice == kFinaleColony)
    return "Колония основана у старых звёзд M13. История завершена.";
  if (choice == kFinaleSolarRebirth)
    return "Солнце перезапущено через квантовый резонатор. Земля спасена!";
  if (choice == kFinaleLoopTranscendence)
    return "Егорка шагнул в сингулярность Sgr A*. Петля Парижа замкнута.";
  if (choice == kFinaleArkCitadel)
    return "Построена мобильная Цитадель человечества в глубоком космосе.";
  if (choice == kFinaleHyperBreakthrough)
    return "Варп разогнан до 135%. Петля времени разорвана, открыт путь в Мультивселенную!";
  if (choice == kFinaleFifthAlliance)
    return "Арсений и Егорка объединили варп-ядро и стазис-щит. Цивилизации спасены вопреки энтропии!";
  if (choice == kFinaleDignity)
    return "Егорка не мстил за кожу — за имя. Экипаж понял цену шутки.";
  return "Решить судьбу экспедиции: колония или дальний горизонт.";
}

std::string finaleGoalEn(int choice) {
  if (choice == kFinaleHorizon)
    return "The crew flew for the far horizon. Story complete.";
  if (choice == kFinaleColony)
    return "Colony founded by the old stars of M13. Story complete.";
  if (choice == kFinaleSolarRebirth)
    return "The Sun is reignited via quantum resonance. Earth is saved!";
  if (choice == kFinaleLoopTranscendence)
    return "Egorka entered the Sgr A* singularity. Paris loop is closed.";
  if (choice == kFinaleArkCitadel)
    return "Mobile Humanity Citadel constructed in deep space.";
  if (choice == kFinaleHyperBreakthrough)
    return "Warp overclocked to 135%. The temporal loop is shattered into the Multiverse!";
  if (choice == kFinaleFifthAlliance)
    return "Arseny and Egorka merged the warp core and stasis shield. Civilizations saved against entropy!";
  if (choice == kFinaleDignity)
    return "Egorka did not avenge skin — he avenged his name. The crew learned the cost of a joke.";
  return "Decide the fate of the expedition: colony or the far horizon.";
}

std::string finaleSpeakerRu(int choice) {
  if (choice == kFinaleColony || choice == kFinaleHorizon)
    return "Мия Сато (Главный пилот)";
  if (choice == kFinaleSolarRebirth)
    return "Д-р Елена Ростова (Главный астрофизик)";
  if (choice == kFinaleLoopTranscendence)
    return "Егорка (Теоретик варп-метрики)";
  if (choice == kFinaleArkCitadel)
    return "Ком. Марк Хейл (Ветеран флота)";
  if (choice == kFinaleHyperBreakthrough)
    return "Егорка-6 (Оверклокер варпа)";
  if (choice == kFinaleFifthAlliance)
    return "Арсений «Палач Ложных Надежд» («Матрас из 5»)";
  if (choice == kFinaleDignity)
    return "Егорка (Инженер челнока)";
  return "";
}

std::string finaleSpeakerEn(int choice) {
  if (choice == kFinaleColony || choice == kFinaleHorizon)
    return "Mia Sato (Lead Pilot)";
  if (choice == kFinaleSolarRebirth)
    return "Dr. Elena Rostova (Lead Astrophysicist)";
  if (choice == kFinaleLoopTranscendence)
    return "EgorKa (Warp Metric Theorist)";
  if (choice == kFinaleArkCitadel)
    return "Cmdr. Mark Hale (Fleet Veteran)";
  if (choice == kFinaleHyperBreakthrough)
    return "EgorKa-6 (Warp Overclocker)";
  if (choice == kFinaleFifthAlliance)
    return "Arseny «Executioner of False Hopes» («The Mattress»)";
  if (choice == kFinaleDignity)
    return "Egorka (Shuttle Engineer)";
  return "";
}

void ExpeditionState::markVisited(const std::string& stationId) {
  maxUnlockedChapter = std::max(maxUnlockedChapter, chapterIndex);
  const auto& chapters = expeditionChapters();
  if (chapterIndex >= static_cast<int>(chapters.size())) return;
  // v4.1: a chapter may list several valid targets separated by '|'
  // (the fleet interlude completes with either Crew Dragon or Soyuz).
  const std::string& targets = chapters[chapterIndex].station;
  bool match = false;
  size_t start = 0;
  while (start <= targets.size()) {
    const size_t sep = targets.find('|', start);
    const std::string target =
        targets.substr(start, sep == std::string::npos ? std::string::npos
                                                       : sep - start);
    if (!target.empty() && target == stationId) {
      match = true;
      break;
    }
    if (sep == std::string::npos) break;
    start = sep + 1;
  }
  if (!match) return;
  // Финальная глава (N) - без цели: визиты её не двигают; достигнув её,
  // сюжет завершён (finished чует индекс последней главы).
  ++chapterIndex;
  maxUnlockedChapter = std::max(maxUnlockedChapter, chapterIndex);
}

void ExpeditionState::setChapter(int index) {
  const int maxUnl = getMaxUnlocked();
  if (index >= 0 && index <= maxUnl &&
      index < static_cast<int>(expeditionChapters().size())) {
    chapterIndex = index;
  }
}

int ExpeditionState::getMaxUnlocked() const {
  return std::max(maxUnlockedChapter, chapterIndex);
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
  os << "# expedition state (version=2)\n";
  os << "version=2\n";
  os << "chapterIndex=" << st.chapterIndex << "\n";
  os << "maxUnlockedChapter=" << std::max(st.maxUnlockedChapter, st.chapterIndex) << "\n";
  os << "introSeen=" << (st.introSeen ? 1 : 0) << "\n";
  os << "finaleChoice=" << st.finaleChoice << "\n";
  os << "egorkaStance=" << st.egorkaStance << "\n";
  return os.str();
}

bool deserializeExpedition(const std::string& text, ExpeditionState& out) {
  ExpeditionState probe;
  probe.chapterIndex = 0;
  probe.maxUnlockedChapter = 0;
  probe.introSeen = false;
  int fileVersion = 0;
  std::istringstream is(text);
  std::string line;
  bool any = false;
  while (std::getline(is, line)) {
    const size_t eq = line.find('=');
    if (eq == std::string::npos || line.empty() || line[0] == '#') continue;
    const std::string key = line.substr(0, eq);
    const std::string val = line.substr(eq + 1);
    if (key == "version") {
      fileVersion = std::atoi(val.c_str());
      any = true;
    } else if (key == "chapterIndex") {
      probe.chapterIndex = std::atoi(val.c_str());
      if (probe.chapterIndex < 0 ||
          probe.chapterIndex > static_cast<int>(expeditionChapters().size())) {
        probe.chapterIndex = 0;
      }
      any = true;
    } else if (key == "maxUnlockedChapter") {
      probe.maxUnlockedChapter = std::atoi(val.c_str());
      if (probe.maxUnlockedChapter < 0 ||
          probe.maxUnlockedChapter > static_cast<int>(expeditionChapters().size())) {
        probe.maxUnlockedChapter = 0;
      }
      any = true;
    } else if (key == "introSeen") {
      probe.introSeen = (std::atoi(val.c_str()) != 0);
      any = true;
    } else if (key == "finaleChoice") {
      probe.finaleChoice = std::atoi(val.c_str());
      if (probe.finaleChoice < kFinaleUnset ||
          probe.finaleChoice > kFinaleDignity) {
        probe.finaleChoice = kFinaleUnset;
      }
      any = true;
    } else if (key == "egorkaStance") {
      probe.egorkaStance = std::atoi(val.c_str());
      if (probe.egorkaStance < 0 || probe.egorkaStance > 2) probe.egorkaStance = 1;
      any = true;
    }
  }
  if (!any) return false;
  // v4.1 inserted the fleet interlude after "Home". Legacy files (v1, no
  // version key) keep their old chapter numbering, so indices 1..N shift by
  // one; index 0 (Home) stays. The last index is clamped to the finale.
  if (fileVersion < 2 && probe.chapterIndex > 0) {
    const int last = static_cast<int>(expeditionChapters().size()) - 1;
    probe.chapterIndex = std::min(probe.chapterIndex + 1, last);
  }
  probe.maxUnlockedChapter = std::max(probe.maxUnlockedChapter, probe.chapterIndex);
  out = probe;
  return true;
}

std::string expeditionFilePathFor(const std::string& dir) {
  return dir + "/expedition.json";
}

}  // namespace astro
