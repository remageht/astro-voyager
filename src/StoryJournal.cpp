#include "StoryJournal.h"

namespace astro {

bool storyEntryHasVisited(const std::vector<std::string>& visited,
                          const std::string& id) {
  for (const auto& v : visited) {
    if (v == id) return true;
  }
  return false;
}

namespace {

// Fixed catalog of story beats (order = panel order). Unlock conditions
// mirror the achievements' progress state, but the texts tell the outline
// of the v4.0 expedition story: the Sun is dying, the crew is looking for
// a world that will outlive it.
std::vector<StoryEntry> storyCatalog() {
  std::vector<StoryEntry> c;

  StoryEntry e;
  e.id = "departure";
  e.titleRu = "Отлёт";
  e.titleEn = "Departure";
  e.textRu =
      "Отчёт экспедиции. Свет Солнца слабеет на доли процента за "
      "десятилетие. Мы покидаем дом, чтобы найти мир, который его "
      "переживёт.";
  e.textEn =
      "Expedition log. The Sun dims a fraction of a percent each decade. "
      "We leave home to find a world that will outlive it.";
  e.unlocked = true;
  c.push_back(e);

  e.id = "first-steps";
  e.titleRu = "Первые прыжки";
  e.titleEn = "First Steps";
  e.textRu =
      "Первые варп-прыжки записаны в бортовой журнал. Маршрут строится из "
      "посещённых миров — открой карту и продолжай.";
  e.textEn =
      "First warp jumps are on the log. The route builds itself from the "
      "worlds you have seen - open the map and continue.";
  e.unlocked = false;
  c.push_back(e);

  e.id = "sgra";
  e.titleRu = "Сердце Галактики";
  e.titleEn = "Heart of the Galaxy";
  e.textRu =
      "Sgr A* — сверхмассивная чёрная дыра в центре Млечного Пути. "
      "Фотонное кольцо — это свет, сделавший оборот вокруг горизонта "
      "прежде, чем попасть в камеру.";
  e.textEn =
      "Sgr A*, the supermassive black hole at the center of the Milky "
      "Way. The photon ring is light that circled the horizon before "
      "reaching the camera.";
  c.push_back(e);

  e.id = "m87";
  e.titleRu = "Монстр с джетом";
  e.titleEn = "The Jet Monster";
  e.textRu =
      "M87* бросает релятивистский джет на тысячи световых лет. Одна "
      "сторона диска ослепительно ярче другой — эффект Доплера.";
  e.textEn =
      "M87* throws a relativistic jet thousands of light-years long. One "
      "side of the disk burns far brighter - Doppler beaming at work.";
  c.push_back(e);

  e.id = "qso";
  e.titleRu = "Маяк ранней Вселенной";
  e.titleEn = "Beacon of the Early Universe";
  e.textRu =
      "3C 273 — квазар: ядро ярче целой галактики. Его свет шёл к нам "
      "2.4 миллиарда лет — на пути домой загляни и в прошлое.";
  e.textEn =
      "3C 273, a quasar: the core outshines an entire galaxy. Its light "
      "traveled 2.4 billion years to reach us.";
  c.push_back(e);

  e.id = "m13";
  e.titleRu = "Старше Солнца";
  e.titleEn = "Older than the Sun";
  e.textRu =
      "В шаровом скоплении M13 звёзды почти вдвое старше Солнца. Если "
      "кто-то и научился жить без яркой звезды — это они.";
  e.textEn =
      "The stars of the globular cluster M13 are nearly twice the Sun's "
      "age. If anyone learned to live without a bright star, it is them.";
  c.push_back(e);

  e.id = "horizon";
  e.titleRu = "У горизонта";
  e.titleEn = "At the Horizon";
  e.textRu =
      "Камера подошла к тени горизонта событий. Часы на HUD идут "
      "медленнее, чем где-либо ещё — гравитационное замедление.";
  e.textEn =
      "The camera stands at the shadow of the event horizon. The HUD "
      "clock runs slower here - gravitational time dilation, live.";
  c.push_back(e);

  e.id = "close-approach";
  e.titleRu = "Никогда так близко";
  e.titleEn = "Never This Close";
  e.textRu =
      "Новый рекорд сближения: меньше пяти радиусов Шварцшильда. "
      "Доплеровская сторона диска в этот момент ослепляет.";
  e.textEn =
      "New approach record: under five Schwarzschild radii. The Doppler "
      "side of the disk blinds the sensor at this range.";
  c.push_back(e);

  e.id = "archive";
  e.titleRu = "Бортовой архив";
  e.titleEn = "The Ship's Archive";
  e.textRu =
      "Маршрут сохранён и восстановлен. Каждый прыжок теперь часть "
      "истории экспедиции — F9 и F10 ведут бортовой архив.";
  e.textEn =
      "Route saved and restored. Every jump is now part of the "
      "expedition history - F9 and F10 keep the ship's archive.";
  c.push_back(e);

  e.id = "grand-tour";
  e.titleRu = "Большое турне";
  e.titleEn = "Grand Tour";
  e.textRu =
      "Восемь миров за кормой. Экспедиция собрала данные, которых "
      "хватит на тысячу отчётов — и нашла, ради чего лететь дальше.";
  e.textEn =
      "Eight worlds behind us. The expedition gathered enough data for "
      "a thousand reports - and a reason to fly on.";
  c.push_back(e);

  return c;
}

}  // namespace

void StoryJournal::refresh(const Progress& progress) {
  if (m_entries.empty()) {
    m_entries = storyCatalog();
  }
  const size_t nVisited = progress.visited.size();
  for (auto& e : m_entries) {
    bool unlocked = false;
    if (e.id == "departure") {
      unlocked = true;
    } else if (e.id == "first-steps") {
      unlocked = nVisited >= 2;
    } else if (e.id == "sgra") {
      unlocked = storyEntryHasVisited(progress.visited, "sgra");
    } else if (e.id == "m87") {
      unlocked = storyEntryHasVisited(progress.visited, "m87");
    } else if (e.id == "qso") {
      unlocked = storyEntryHasVisited(progress.visited, "qso-3c273");
    } else if (e.id == "m13") {
      unlocked = storyEntryHasVisited(progress.visited, "m13");
    } else if (e.id == "horizon") {
      unlocked = progress.capturedHorizon;
    } else if (e.id == "close-approach") {
      unlocked = progress.minBhDistance < 5.0;
    } else if (e.id == "archive") {
      unlocked = progress.exported || progress.imported;
    } else if (e.id == "grand-tour") {
      unlocked = nVisited >= 8;
    }
    e.unlocked = unlocked;
  }
}

int StoryJournal::unlockedCount() const {
  int n = 0;
  for (const auto& e : m_entries) {
    if (e.unlocked) ++n;
  }
  return n;
}

int StoryJournal::unreadCount() const {
  int n = 0;
  for (const auto& e : m_entries) {
    if (e.unlocked && !e.read) ++n;
  }
  return n;
}

void StoryJournal::markAllRead() {
  for (auto& e : m_entries) {
    if (e.unlocked) e.read = true;
  }
}

}  // namespace astro
