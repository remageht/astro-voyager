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

  e.id = "sn1987a";
  e.titleRu = "Нить жемчуга";
  e.titleEn = "String of Pearls";
  e.textRu =
      "Мия считает тридцать две вспышки на кольце SN 1987A. Егорка смотрит не на кольцо, "
      "а на отражение в её визоре. «Помнишь гирлянды на мосту Александра III под дождём? "
      "Ты сказала, что они светят слишком ярко». Мия не отводит глаз от датчиков: "
      "«Тогда вокруг не было вакуума, Егорка. И мы не летели сквозь пепел сверхновой. Но... да. Я помню».";
  e.textEn =
      "Mia counts thirty-two bright knots on the ring of SN 1987A. Yegorka looks not at the ring, "
      "but at the reflection in her visor. \"Remember the streetlights on Pont Alexandre III in the rain? "
      "You said they were far too bright.\" Mia keeps her eyes fixed on telemetry: "
      "\"There was no vacuum around us back then, Yegorka. Nor were we flying through supernova ashes. But... yes. I remember.\"";
  c.push_back(e);

  e.id = "gw170817";
  e.titleRu = "Золото для Мии";
  e.titleEn = "Gold for Mia";
  e.textRu =
      "Килоновая GW170817: при столкновении нейтронных звёзд родились горы золота и платины. "
      "Егорка вертит в пальцах звёздный кулон: «Миллиарды людей на Земле воевали за обручальные "
      "кольца, не зная, что это просто осколки космических аварий. А я стою здесь и понимаю: "
      "всё золото Вселенной не стоит того, чтобы вернуться на мостик одному». "
      "Мия тихо сжимает его ладонь: «Мы не вернёмся одни, Егорка».";
  e.textEn =
      "Kilonova GW170817: the merger of two neutron stars forged oceans of gold and platinum. "
      "Yegorka turns the star pendant between his fingers: \"Billions back on Earth fought wars over "
      "wedding rings, never realizing they were merely wearing debris from cosmic collisions. "
      "And standing here, I realize: all the gold in the Universe isn't worth returning to this bridge alone.\" "
      "Mia quietly squeezes his hand: \"We won't return alone, Yegorka.\"";
  c.push_back(e);

  e.id = "m31";
  e.titleRu = "Танец двух миров";
  e.titleEn = "Dance of Two Worlds";
  e.textRu =
      "Андромеда несётся навстречу Млечному Пути. Четыре миллиарда лет до слияния. "
      "Звёзды пройдут сквозь звёзды, ни одна не разобьётся — они просто сплетутся в общий танец. "
      "«Мы как эти две галактики, Мия. Разные орбиты, но гравитация всё равно свела нас вместе». "
      "Мия улыбается: «Твоя тройка по физике не мешает тебе быть поэтом, Егорка. Держи курс на ядро».";
  e.textEn =
      "Andromeda hurtles toward the Milky Way. Four billion years until the collision. "
      "Stars will drift right past stars without crashing — they will merely intertwine in a mutual dance. "
      "\"We're just like those two galaxies, Mia. Different orbits, yet gravity pulled us together anyway.\" "
      "Mia smiles: \"Your C-grade in physics doesn't stop you from being a poet, Yegorka. Steady course to the core.\"";
  c.push_back(e);

  e.id = "crab-pulse";
  e.titleRu = "Сердцебиение";
  e.titleEn = "Heartbeat of the Void";
  e.textRu =
      "Пульсар отсчитывает импульсы с точностью атомных часов. Ритмичный стук сквозь радиоэфир корабля. "
      "«Тридцать ударов в секунду, — замечает Егорка. — Моё бьётся втрое чаще, каждый раз когда ты глушишь "
      "маршевые и бросаешь нас в свободный дрейф». Мия не спорит: «Потому что я знаю, что ты поймаешь наш "
      "варп-пузырь, когда я скажу пора».";
  e.textEn =
      "The pulsar clicks with the precision of an atomic clock. A rhythmic pulse echoing through the ship's comms. "
      "\"Thirty beats per second,\" Yegorka murmurs. \"Mine beats thrice as fast whenever you cut the "
      "thrusters and throw us into free drift.\" Mia doesn't dispute it: \"Because I trust you to catch our "
      "warp bubble the moment I say 'now'.\"";
  c.push_back(e);

  e.id = "letter-to-earth";
  e.titleRu = "Письмо на остывающую Землю";
  e.titleEn = "Letter to a Cooling Earth";
  e.textRu =
      "Пакетный квантовый сигнал направлен к Солнечной системе. Дойдёт нескоро. "
      "«Земля, это мостик Astro-Voyager. Здесь, на краю изгиба пространства, очень темно и холодно. "
      "Но если вы слышите нас сквозь годы — знайте: мы нашли не просто координаты нового дома. "
      "Мы нашли то, ради чего стоит пережить любую зиму». Подпись в конце: Мия и Егорка.";
  e.textEn =
      "A quantum packet signal directed toward the Solar System. It won't arrive soon. "
      "\"Earth, this is the bridge of Astro-Voyager. Out here, on the edge of warped space, it is terribly dark and cold. "
      "But if you hear us across the years — know this: we found more than coordinates for a new home. "
      "We found what makes any winter worth surviving.\" Signed at the bottom: Mia & Yegorka.";
  c.push_back(e);

  e.id = "at2019qiz";
  e.titleRu = "Приливное спагетти";
  e.titleEn = "Spaghettification";
  e.textRu =
      "Сверхмассивная чёрная дыра разорвала приблизившуюся звезду на тончайшие светящиеся нити. "
      "«Даже звёзды гибнут, когда подходят слишком близко, — шепчет Мия у визора. — Но посмотри, "
      "какой свет она отдала напоследок. Он виден сквозь двести миллионов световых лет». "
      "Егорка берёт её за руку: «Гравитация может согнуть луч света, Мия, но не то, что держит нас».";
  e.textEn =
      "A supermassive black hole shredded a nearby star into ultra-thin, incandescent filaments. "
      "\"Even stars break apart when they drift too close,\" Mia whispers at the visor. \"Yet look at "
      "the brilliant light she gave back in return. Visible across two hundred million light-years.\" "
      "Yegorka gently takes her hand: \"Gravity can bend a ray of light, Mia, but never what binds us.\"";
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
    } else if (e.id == "sn1987a") {
      unlocked = storyEntryHasVisited(progress.visited, "sn1987a");
    } else if (e.id == "gw170817") {
      unlocked = storyEntryHasVisited(progress.visited, "gw170817");
    } else if (e.id == "m31") {
      unlocked = storyEntryHasVisited(progress.visited, "m31");
    } else if (e.id == "crab-pulse") {
      unlocked = storyEntryHasVisited(progress.visited, "m1") ||
                 storyEntryHasVisited(progress.visited, "psr-crab") ||
                 storyEntryHasVisited(progress.visited, "psr-vela");
    } else if (e.id == "letter-to-earth") {
      unlocked = (nVisited >= 5) && (progress.minBhDistance < 10.0 || progress.capturedHorizon);
    } else if (e.id == "at2019qiz") {
      unlocked = storyEntryHasVisited(progress.visited, "at2019qiz");
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
