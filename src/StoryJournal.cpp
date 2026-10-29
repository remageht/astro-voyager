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
      "Солнце красное. Земля — маленькая, тихая, слишком знакомая. Мы увозим "
      "не только людей: мы увозим их несбывшиеся планы, первые влюблённости и "
      "чьи-то несданные домашки. Планета умирает. Но мы ещё не закончили.";
  e.textEn =
      "The Sun is red. Earth is small, silent, too familiar. We carry not "
      "just people: we carry unfulfilled plans, first loves, and someone's "
      "overdue homework. The planet is dying. But we are not finished.";
  e.unlocked = true;
  c.push_back(e);

  e.id = "first-steps";
  e.titleRu = "Варп-фактор 9";
  e.titleEn = "Warp Factor 9";
  e.textRu =
      "Варп-фактор 9. Корабль не «летит» — он договаривается с пространством, "
      "чтобы расстояние перестало быть наглым. Внутри пузыря тишина. "
      "Примечание: керосиновые ракеты были великолепны. Как велосипед у лайнера.";
  e.textEn =
      "Warp factor 9. The ship doesn't 'fly' — it negotiates with space so "
      "distance stops being insolent. Inside the bubble is silence. "
      "Note: kerosene rockets were great. Like a bicycle next to an ocean liner.";
  e.unlocked = false;
  c.push_back(e);

  e.id = "sgra";
  e.titleRu = "Sgr A* — Сердце Галактики";
  e.titleEn = "Heart of the Galaxy (Sgr A*)";
  e.textRu =
      "Подхожу к Sgr A*. Мои часы стали ленивыми, а Вселенная снаружи — "
      "нетерпеливой. Горизонт событий не выглядит страшно: он как чёрная дверь "
      "без ручки. Если не вернусь, передайте Земле: я всё-таки сдал физику.";
  e.textEn =
      "Approaching Sgr A*. My clocks grew lazy, and the Universe outside grew "
      "impatient. The event horizon isn't terrifying: it's like a black door "
      "without a handle. If I don't return, tell Earth: I passed physics after all.";
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
  e.titleRu = "У горизонта событий";
  e.titleEn = "At the Event Horizon";
  e.textRu =
      "Я снова думаю о том вечере на Сене. О себе в мокрой куртке, с тройкой "
      "по физике и верой, что космос — для избранных. Звёзды покоряют не "
      "полубоги, а упрямые мечтатели с несданной домашкой.";
  e.textEn =
      "I think back to that evening by the Seine. Myself in a soaked jacket, "
      "C-grades in physics and a belief that space is for the chosen. "
      "Stars are conquered not by demigods, but by stubborn dreamers with overdue homework.";
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

  e.id = "constellation-sync";
  e.titleRu = "Секрет созвездий";
  e.titleEn = "Secret of Constellations";
  e.textRu =
      "Созвездие собрано: звёздный кулон Егорки-5 срезонировал с метрикой Млечного Пути. "
      "Опорные координаты Пятой Вселенной подтверждены. Расстояние снова уступило место упрямству.";
  e.textEn =
      "Constellation aligned: the star pendant of Yegorka-5 resonated with the Milky Way metric. "
      "Reference coordinates of the Fifth Universe are locked. Distance has yielded to stubbornness once more.";
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
    } else if (e.id == "constellation-sync") {
      unlocked = progress.constellationAligned;
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
