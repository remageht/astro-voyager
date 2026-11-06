#include "Zettelkasten.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace astro {

ZettelGraph::ZettelGraph() {
  init();
}

void ZettelGraph::addSeed(const ZettelNote& note) {
  m_notes.push_back(note);
}

void ZettelGraph::init() {
  m_notes.clear();

  // -------------------------------------------------------------
  // Cluster: SOLAR («Солнечный Синтез» · Д-р Елена Ростова)
  // -------------------------------------------------------------
  {
    ZettelNote n;
    n.id = "z_paris_rain";
    n.cluster = kClusterSolar;
    n.categoryRu = "Память";
    n.categoryEn = "Memory";
    n.titleRu = "Дождь над Сорбонной";
    n.titleEn = "Rain Over the Sorbonne";
    n.textRu = "Сквозь узкие мансардные окна чердака в Латинском квартале доносился шум ливня. Солнце тогда казалось вечным, неизменным светилом, согревавшим Париж миллионы лет. Мы думали, что у нас есть всё время мира.";
    n.textEn = "Through the narrow attic dormers of the Latin Quarter, rain drummed on zinc roofs. Back then, the Sun felt eternal and unchanging, keeping Paris warm across epochs. We believed we had all the time in the world.";
    n.quoteRu = "Ты помнишь запах озона на мостовых перед грозой? Мы вернём это тепло, Егорка.";
    n.quoteEn = "Do you remember the scent of ozone over the cobbles before a thunderstorm? We will bring that warmth back, Egorka.";
    n.speakerRu = "Д-р Елена Ростова";
    n.speakerEn = "Dr. Elena Rostova";
    n.posX = -220.0f;
    n.posY = -120.0f;
    n.seedDiscovered = true;  // Initial seed discovered at departure
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_casimir_metric";
    n.cluster = kClusterSolar;
    n.categoryRu = "Физика";
    n.categoryEn = "Physics";
    n.titleRu = "Метрика отрицательной плотности Казимира";
    n.titleEn = "Casimir Negative Density Metric";
    n.textRu = "Эффект Казимира в субмикронных зазорах доказывает: локальная плотность энергии вакуума может быть строго отрицательной. Если инвертировать градиент давления в вырожденном гелиевом ядре, гравитационный коллапс обратится вспять.";
    n.textEn = "The Casimir effect in sub-micron gaps proves that local vacuum energy density can be strictly negative. Inverting the pressure gradient inside a degenerate helium core turns gravitational collapse into expansion.";
    n.quoteRu = "Если вакуум давит изнутри, значит угасающее ядро звезды можно раскачать обратной волной.";
    n.quoteEn = "If the quantum vacuum pushes from within, a dying stellar core can be reignited by a reverse shockwave.";
    n.speakerRu = "Егорка";
    n.speakerEn = "Egorka";
    n.posX = -130.0f;
    n.posY = -50.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_solar_injection";
    n.cluster = kClusterSolar;
    n.categoryRu = "Синтез";
    n.categoryEn = "Synthesis";
    n.titleRu = "Каскадный инжектор релятивистского водорода";
    n.titleEn = "Relativistic Hydrogen Cascade Injector";
    n.textRu = "Нагнетание ультра-релятивистских протонов по геодезическим орбитам прямо в термоядерное ядро Солнца сжигает шлаковый углерод и перезапускает протон-протонный цикл на пять миллиардов лет вперёд.";
    n.textEn = "Injecting ultra-relativistic protons along geodesic trajectories directly into the Sun's thermonuclear core purges inert carbon slag and restarts the proton-proton chain for five billion years.";
    n.quoteRu = "Это не отсрочка агонии на одно поколение. Это полное омоложение Солнца для новой эры человечества.";
    n.quoteEn = "This is not stalling agony for a single generation. It is the full rejuvenation of the Sun for humanity's new era.";
    n.speakerRu = "Д-р Елена Ростова";
    n.speakerEn = "Dr. Elena Rostova";
    n.posX = -50.0f;
    n.posY = -140.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }

  // -------------------------------------------------------------
  // Cluster: STASIS («Пакт Вселенной-05» · Арсений)
  // -------------------------------------------------------------
  {
    ZettelNote n;
    n.id = "z_stasis_dampener";
    n.cluster = kClusterStasis;
    n.categoryRu = "Стазис";
    n.categoryEn = "Stasis";
    n.titleRu = "Амортизатор криостазиса «Матрас из 5»";
    n.titleEn = "Cryostatis Dampener «5-Layer Mattress»";
    n.textRu = "Пятислойная регенерирующая губка из полимерного графена, используемая пилотами Пятой Вселенной для гашения микрогравитационных конвульсий во время криосна при абсолютном нуле.";
    n.textEn = "A five-layer regenerating graphene polymer sponge engineered by Universe-05 pilots to absorb microgravitational convulsions during cryo-sleep at absolute zero.";
    n.quoteRu = "Пять слоёв полимерной пены не спасают от кошмаров об умирающей Земле. Но хотя бы рёбра остаются целы.";
    n.quoteEn = "Five layers of polymer foam won't cure nightmares of a dying Earth. But at least your ribs stay in one piece.";
    n.speakerRu = "Арсений «Палач Ложных Надежд»";
    n.speakerEn = "Arseny «Executioner of False Hopes»";
    n.posX = -210.0f;
    n.posY = 90.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_universe5_collapse";
    n.cluster = kClusterStasis;
    n.categoryRu = "Хроника";
    n.categoryEn = "Chronicle";
    n.titleRu = "Энтропийный коллапс Вселенной-05";
    n.titleEn = "Entropic Collapse of Universe-05";
    n.textRu = "Записи бортового самописца Арсения. В их реальности Солнце потухло на двести лет раньше. Выжили лишь те, кто отказался от иллюзий, научился выдирать плазму из сингулярностей и спать на бронепластинах.";
    n.textEn = "Flight recorder logs from Arseny's craft. In his reality, the Sun froze two centuries earlier. Only those survived who shed illusions, harvested accretion plasma from singularities, and slept on armor plates.";
    n.quoteRu = "Мы выжигали ложные надежды калёным железом. Надежда ослепляет, выживание требует холодной стали и верного товарища.";
    n.quoteEn = "We scorched false hope with red-hot iron. Hope blinds; survival demands cold steel and a steadfast brother.";
    n.speakerRu = "Арсений «Палач Ложных Надежд»";
    n.speakerEn = "Arseny «Executioner of False Hopes»";
    n.posX = -120.0f;
    n.posY = 170.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_sgra_tidal";
    n.cluster = kClusterStasis;
    n.categoryRu = "Гравитация";
    n.categoryEn = "Gravity";
    n.titleRu = "Приливный гравитационный щит Sgr A*";
    n.titleEn = "Sgr A* Tidal Gravity Shield";
    n.textRu = "Расчёт приливных сил сверхмассивной чёрной дыры показывает: гравитационные градиенты можно перенаправить через контуры стазис-капсул Арсения, создав абсолютно непробиваемый купол от гамма-всплесков.";
    n.textEn = "Calculations of supermassive black hole tidal shears show gradients can be routed across Arseny's stasis capsule hulls, establishing an impenetrable umbrella against lethal cosmic gamma bursts.";
    n.quoteRu = "Его цинизм — всего лишь панцирь обожжённого человека. С его закалкой и нашими технологиями мы удержим любую брешь.";
    n.quoteEn = "His cynicism is merely armor worn by a burned man. With his grit and our tech, we can plug any breach in reality.";
    n.speakerRu = "Егорка";
    n.speakerEn = "Egorka";
    n.posX = -30.0f;
    n.posY = 100.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }

  // -------------------------------------------------------------
  // Cluster: EDEN («Новый Эдем» · Мия Сато)
  // -------------------------------------------------------------
  {
    ZettelNote n;
    n.id = "z_mia_promise";
    n.cluster = kClusterEden;
    n.categoryRu = "Обещание";
    n.categoryEn = "Promise";
    n.titleRu = "Клятва в стартовом ангаре";
    n.titleEn = "The Hangar Vow";
    n.textRu = "Мия сжимала в руках семена японской вишни и земной пшеницы в вакуумном контейнере. 'Мы не позволим человечеству остаться лишь эхом в пустых радиодиапазонах'.";
    n.textEn = "Mia held tightly a sealed canister containing Japanese sakura and Earth wheat seeds. 'We will not let humanity vanish as an echo across dead radio frequencies'.";
    n.quoteRu = "Мы не просто ищем камень во тьме, Егорка. Мы ищем мир, где дети смогут смотреть в зелёное небо без скафандров.";
    n.quoteEn = "We aren't just searching for a rock in the void, Egorka. We are seeking a world where children gaze up into green skies without helmets.";
    n.speakerRu = "Мия Сато";
    n.speakerEn = "Mia Sato";
    n.posX = 110.0f;
    n.posY = -140.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_voyager_golden_record";
    n.cluster = kClusterEden;
    n.categoryRu = "Реликвия";
    n.categoryEn = "Relic";
    n.titleRu = "Золотая пластинка «Вояджера»: Послание сквозь эпохи";
    n.titleEn = "Voyager Golden Record: A Message Across Epochs";
    n.textRu = "В поясе Койпера мы настигли «Вояджер-1». На позолоченной медной пластинке выгравированы схема водорода, координаты 14 пульсаров и 115 изображений нашей цивилизации. Экипаж слушал шёпот Земли 1977 года.";
    n.textEn = "In the Kuiper belt, we caught up with Voyager 1. The gold-plated copper phonograph disc preserves the hydrogen diagram, 14-pulsar map, and 115 pictures of our home. The bridge crew listened to the whispered heartbeat of 1977 Earth.";
    n.quoteRu = "Они не знали, покорим ли мы варп. Они просто верили, что в бесконечной темноте кто-то протянет руку в ответ.";
    n.quoteEn = "They didn't know if we'd ever conquer warp. They simply believed that out in the infinite dark, someone would reach out a hand in return.";
    n.speakerRu = "Елена Ростова";
    n.speakerEn = "Elena Rostova";
    n.posX = 145.0f;
    n.posY = -195.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_m13_biosphere";
    n.cluster = kClusterEden;
    n.categoryRu = "Биосфера";
    n.categoryEn = "Biosphere";
    n.titleRu = "Биосферный отклик экзопланеты M13-Prime";
    n.titleEn = "M13-Prime Biosphere Resonance";
    n.textRu = "Спектральный анализ обнаружил мощную полосу поглощения хлорофилла-B и пары чистой воды. Плотная магнитосфера защищает материки от звёздного ветра шарового скопления.";
    n.textEn = "Spectrometric sweeps detected an unmistakable chlorophyll-B absorption signature alongside liquid water vapor. A dense magnetosphere shields lush continents from globular cluster radiation.";
    n.quoteRu = "Спектрометр подтверждает: жидкий океан и кислородная атмосфера! Настоящий Новый Эдем ждёт нас!";
    n.quoteEn = "The spectrometer confirms liquid oceans and an oxygen mantle! True New Eden is waiting for us!";
    n.speakerRu = "Мия Сато";
    n.speakerEn = "Mia Sato";
    n.posX = 200.0f;
    n.posY = -60.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_nomad_citadel";
    n.cluster = kClusterEden;
    n.categoryRu = "Ковчег";
    n.categoryEn = "Citadel";
    n.titleRu = "Шпили Цитадели «Номад»";
    n.titleEn = "Spires of Nomad Citadel";
    n.textRu = "Модульный ковчег колонистов спроектирован так, чтобы его опорные пилоны врастали в скальные базальты планеты, образуя вечные гидропонные террасы и купола жизни.";
    n.textEn = "The modular colony ark is engineered so its foundational pylons fuse directly into bedrock basalts, anchoring eternal hydroponic terraces and living bio-domes.";
    n.quoteRu = "Этот дом простоит тысячи лет. Земля может спать спокойно — наше семя пустило корни среди звёзд.";
    n.quoteEn = "This citadel will stand for millennia. Earth may sleep peacefully — our seeds have taken root among the stars.";
    n.speakerRu = "Егорка";
    n.speakerEn = "Egorka";
    n.posX = 90.0f;
    n.posY = -30.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }

  // -------------------------------------------------------------
  // Cluster: MULTIVERSE («Прорыв Мультивселенной» · Егорка-6)
  // -------------------------------------------------------------
  {
    ZettelNote n;
    n.id = "z_sixth_resonance";
    n.cluster = kClusterMultiverse;
    n.categoryRu = "Эхо";
    n.categoryEn = "Echo";
    n.titleRu = "Гармоника Шестого Егорки";
    n.titleEn = "The Sixth Egorka's Harmonic";
    n.textRu = "Сигнал тахионного передатчика из параллельной ветви времени. Егорка из шестой итерации оставил математический маркер в шуме квазара 3C 273.";
    n.textEn = "A tachyon transmitter packet received from a parallel temporal branch. Egorka from iteration six embedded a crystalline mathematical marker inside quasar 3C 273's radio noise.";
    n.quoteRu = "Я пробовал этот прыжок в тысяче итераций. Ты — единственный, кто услышал гармонику. Не бойся сингулярности.";
    n.quoteEn = "I attempted this leap across a thousand iterations. You are the only one who tuned into the harmonic. Fear no singularity.";
    n.speakerRu = "Егорка-6";
    n.speakerEn = "Egorka-6";
    n.posX = 60.0f;
    n.posY = 110.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_phase_shard";
    n.cluster = kClusterMultiverse;
    n.categoryRu = "Фаза";
    n.categoryEn = "Phase";
    n.titleRu = "Осколок темпоральной фазы";
    n.titleEn = "Temporal Phase Shard";
    n.textRu = "Векторы причинности вблизи горизонта событий не параллельны. Они сворачиваются в ленту Мёбиуса, позволяя информации течь сквозь барьеры квантовых реальностей.";
    n.textEn = "Causal vectors near the event horizon are not parallel. They curl into a Möbius strip, allowing information to tunnel across quantum realities.";
    n.quoteRu = "Время — это не река с одним истоком. Это океан, и мы научились ставить паруса.";
    n.quoteEn = "Time is not a river with a single source. It is an ocean, and we have learned to unfurl its sails.";
    n.speakerRu = "Егорка";
    n.speakerEn = "Egorka";
    n.posX = 150.0f;
    n.posY = 160.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_loop_transcend";
    n.cluster = kClusterMultiverse;
    n.categoryRu = "Трансцендентность";
    n.categoryEn = "Transcendence";
    n.titleRu = "Сингулярный межпространственный мост";
    n.titleEn = "Singular Cross-Dimensional Bridge";
    n.textRu = "Синтез данных шести итераций замыкает пространственно-временную топологию в устойчивый коридор Эйнштейна-Розена, открывая свободный переход между мультивселенными.";
    n.textEn = "Synthesizing data from six iterations collapses spacetime topology into a stable Einstein-Rosen corridor, opening uninhibited transit across the multiverse.";
    n.quoteRu = "Петля больше не клетка. Она стала лестницей к бесконечным мирам.";
    n.quoteEn = "The loop is no longer a cage. It has become a staircase to infinite worlds.";
    n.speakerRu = "Егорка-6";
    n.speakerEn = "Egorka-6";
    n.posX = 140.0f;
    n.posY = 50.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    addSeed(n);
  }
}

const ZettelNote* ZettelGraph::getNote(const std::string& id) const {
  for (const auto& n : m_notes) {
    if (n.id == id) return &n;
  }
  return nullptr;
}

ZettelNote* ZettelGraph::getNote(const std::string& id) {
  for (auto& n : m_notes) {
    if (n.id == id) return &n;
  }
  return nullptr;
}

bool ZettelGraph::linkNotes(const std::string& idA, const std::string& idB) {
  if (idA == idB) return false;
  ZettelNote* a = getNote(idA);
  ZettelNote* b = getNote(idB);
  if (!a || !b) return false;
  if (!a->seedDiscovered || !b->seedDiscovered) return false;

  if (std::find(a->links.begin(), a->links.end(), idB) == a->links.end()) {
    a->links.push_back(idB);
  }
  if (std::find(b->links.begin(), b->links.end(), idA) == b->links.end()) {
    b->links.push_back(idA);
  }

  // Synthesizing a valid link automatically promotes both notes to synthesized!
  a->synthesized = true;
  b->synthesized = true;
  return true;
}

bool ZettelGraph::unlinkNotes(const std::string& idA, const std::string& idB) {
  ZettelNote* a = getNote(idA);
  ZettelNote* b = getNote(idB);
  if (!a || !b) return false;

  auto itA = std::find(a->links.begin(), a->links.end(), idB);
  if (itA != a->links.end()) a->links.erase(itA);

  auto itB = std::find(b->links.begin(), b->links.end(), idA);
  if (itB != b->links.end()) b->links.erase(itB);

  return true;
}

bool ZettelGraph::areLinked(const std::string& idA, const std::string& idB) const {
  const ZettelNote* a = getNote(idA);
  if (!a) return false;
  return std::find(a->links.begin(), a->links.end(), idB) != a->links.end();
}

bool ZettelGraph::discoverSeed(const std::string& id) {
  ZettelNote* n = getNote(id);
  if (!n) return false;
  if (n->seedDiscovered) return false;
  n->seedDiscovered = true;
  return true;
}

bool ZettelGraph::synthesizeNote(const std::string& id) {
  ZettelNote* n = getNote(id);
  if (!n) return false;
  n->synthesized = true;
  return true;
}

void ZettelGraph::refresh(const std::vector<std::string>& visitedStations,
                          double minBhDistance,
                          bool journeyExported) {
  auto hasStation = [&](const std::string& stId) {
    return std::find(visitedStations.begin(), visitedStations.end(), stId) != visitedStations.end();
  };

  // Base departure
  discoverSeed("z_paris_rain");

  // Earth / Fleet stations
  if (hasStation("sol") || hasStation("dock-dragon") || hasStation("dock-soyuz") || hasStation("voyager1")) {
    discoverSeed("z_casimir_metric");
    discoverSeed("z_mia_promise");
    discoverSeed("z_voyager_golden_record");
  }

  // Stasis / Fleet contact
  if (hasStation("dock-dragon") || hasStation("dock-soyuz") || journeyExported || visitedStations.size() >= 2) {
    discoverSeed("z_stasis_dampener");
  }

  // Black hole proximity / Sgr A*
  if (hasStation("sgra") || minBhDistance < 25.0) {
    discoverSeed("z_universe5_collapse");
    discoverSeed("z_sgra_tidal");
    discoverSeed("z_loop_transcend");
  }

  // Distant relativistic giants (M87 / Quasar 3C 273)
  if (hasStation("m87") || hasStation("qso-3c273")) {
    discoverSeed("z_solar_injection");
    discoverSeed("z_sixth_resonance");
    discoverSeed("z_phase_shard");
  }

  // M13 Globular Cluster
  if (hasStation("m13")) {
    discoverSeed("z_m13_biosphere");
    discoverSeed("z_nomad_citadel");
  }
}

float ZettelGraph::getClusterProgress(const std::string& cluster) const {
  std::vector<std::string> clusterNodes;
  std::vector<std::pair<std::string, std::string>> requiredLinks;

  if (cluster == kClusterSolar) {
    clusterNodes = {"z_paris_rain", "z_casimir_metric", "z_solar_injection"};
    requiredLinks = {{"z_paris_rain", "z_casimir_metric"}, {"z_casimir_metric", "z_solar_injection"}};
  } else if (cluster == kClusterStasis) {
    clusterNodes = {"z_stasis_dampener", "z_universe5_collapse", "z_sgra_tidal"};
    requiredLinks = {{"z_stasis_dampener", "z_universe5_collapse"}, {"z_universe5_collapse", "z_sgra_tidal"}};
  } else if (cluster == kClusterEden) {
    clusterNodes = {"z_mia_promise", "z_m13_biosphere", "z_nomad_citadel"};
    requiredLinks = {{"z_mia_promise", "z_m13_biosphere"}, {"z_m13_biosphere", "z_nomad_citadel"}};
  } else if (cluster == kClusterMultiverse) {
    clusterNodes = {"z_sixth_resonance", "z_phase_shard", "z_loop_transcend"};
    requiredLinks = {{"z_sixth_resonance", "z_phase_shard"}, {"z_phase_shard", "z_loop_transcend"}};
  } else {
    return 0.0f;
  }

  int disc = 0;
  int synth = 0;
  for (const auto& nid : clusterNodes) {
    const ZettelNote* n = getNote(nid);
    if (n) {
      if (n->seedDiscovered) disc++;
      if (n->synthesized) synth++;
    }
  }

  int linksMade = 0;
  for (const auto& p : requiredLinks) {
    if (areLinked(p.first, p.second)) linksMade++;
  }

  // Weights: Discovery 30%, Synthesis 40%, Canonical Resonance Links 30%
  const float fDisc = static_cast<float>(disc) / static_cast<float>(clusterNodes.size());
  const float fSynth = static_cast<float>(synth) / static_cast<float>(clusterNodes.size());
  const float fLinks = static_cast<float>(linksMade) / static_cast<float>(requiredLinks.size());

  float total = fDisc * 0.30f + fSynth * 0.40f + fLinks * 0.30f;
  if (total > 1.0f) total = 1.0f;
  return total;
}

bool ZettelGraph::isClusterTrueEndingUnlocked(const std::string& cluster) const {
  return getClusterProgress(cluster) >= 0.999f;
}

int ZettelGraph::getDiscoveredCount() const {
  int count = 0;
  for (const auto& n : m_notes) {
    if (n.seedDiscovered) count++;
  }
  return count;
}

int ZettelGraph::getSynthesizedCount() const {
  int count = 0;
  for (const auto& n : m_notes) {
    if (n.synthesized) count++;
  }
  return count;
}

int ZettelGraph::getTotalLinksCount() const {
  int count = 0;
  for (const auto& n : m_notes) {
    count += static_cast<int>(n.links.size());
  }
  return count / 2; // bidirectional
}

std::string ZettelGraph::serialize() const {
  std::ostringstream os;
  os << "# astro-voyager zettelkasten second brain state (version=1)\n";
  os << "version=1\n";
  for (const auto& n : m_notes) {
    os << "note=" << n.id
       << "|disc=" << (n.seedDiscovered ? 1 : 0)
       << "|synth=" << (n.synthesized ? 1 : 0)
       << "|pos=" << n.posX << "," << n.posY
       << "|links=";
    for (size_t i = 0; i < n.links.size(); ++i) {
      if (i > 0) os << ",";
      os << n.links[i];
    }
    os << "\n";
  }
  return os.str();
}

bool ZettelGraph::deserialize(const std::string& data) {
  if (data.empty()) return false;
  std::istringstream is(data);
  std::string line;
  bool any = false;

  while (std::getline(is, line)) {
    if (line.empty() || line[0] == '#') continue;
    if (line.rfind("note=", 0) != 0) continue;

    // Format: note=ID|disc=0|synth=0|pos=x,y|links=a,b
    std::string payload = line.substr(5);
    std::istringstream ss(payload);
    std::string token;

    std::string noteId;
    bool disc = false;
    bool synth = false;
    float px = 0.0f, py = 0.0f;
    std::vector<std::string> loadedLinks;

    size_t pipe1 = payload.find('|');
    if (pipe1 == std::string::npos) {
      noteId = payload;
    } else {
      noteId = payload.substr(0, pipe1);
      std::string rest = payload.substr(pipe1 + 1);
      std::istringstream restStream(rest);
      std::string field;
      while (std::getline(restStream, field, '|')) {
        if (field.rfind("disc=", 0) == 0) {
          disc = (std::atoi(field.substr(5).c_str()) != 0);
        } else if (field.rfind("synth=", 0) == 0) {
          synth = (std::atoi(field.substr(6).c_str()) != 0);
        } else if (field.rfind("pos=", 0) == 0) {
          std::string coords = field.substr(4);
          size_t c = coords.find(',');
          if (c != std::string::npos) {
            px = static_cast<float>(std::atof(coords.substr(0, c).c_str()));
            py = static_cast<float>(std::atof(coords.substr(c + 1).c_str()));
          }
        } else if (field.rfind("links=", 0) == 0) {
          std::string lks = field.substr(6);
          std::istringstream lkStream(lks);
          std::string linkId;
          while (std::getline(lkStream, linkId, ',')) {
            if (!linkId.empty()) loadedLinks.push_back(linkId);
          }
        }
      }
    }

    ZettelNote* n = getNote(noteId);
    if (n) {
      n->seedDiscovered = disc;
      n->synthesized = synth;
      n->posX = px;
      n->posY = py;
      n->links = loadedLinks;
      any = true;
    }
  }

  return any;
}

std::string ZettelGraph::getClusterTitleRu(const std::string& cluster) {
  if (cluster == kClusterSolar) return "Солнечный Синтез";
  if (cluster == kClusterStasis) return "Пакт Вселенной-05";
  if (cluster == kClusterEden) return "Новый Эдем";
  if (cluster == kClusterMultiverse) return "Прорыв Мультивселенной";
  return "Нейросеть Сознания";
}

std::string ZettelGraph::getClusterTitleEn(const std::string& cluster) {
  if (cluster == kClusterSolar) return "Solar Synthesis";
  if (cluster == kClusterStasis) return "Universe-05 Pact";
  if (cluster == kClusterEden) return "New Eden";
  if (cluster == kClusterMultiverse) return "Multiverse Breakthrough";
  return "Consciousness Matrix";
}

std::string ZettelGraph::getClusterDescriptionRu(const std::string& cluster) {
  if (cluster == kClusterSolar)
    return "Синтез парижских воспоминаний, эффекта Казимира и водородного каскада для спасения колыбели человечества.";
  if (cluster == kClusterStasis)
    return "Сплав гравитационных приливов Sgr A* и стазис-брони Арсения для создания несокрушимого братства.";
  if (cluster == kClusterEden)
    return "Связывание обета Мии, биосферы M13 и цитадели колонистов в вечный цветущий дом среди звёзд.";
  if (cluster == kClusterMultiverse)
    return "Слияние математических гармоник Шестого Егорки и темпоральных фаз для покорения сингулярности.";
  return "Единое полотно познания.";
}

std::string ZettelGraph::getClusterDescriptionEn(const std::string& cluster) {
  if (cluster == kClusterSolar)
    return "Synthesizing Paris memories, Casimir vacuum physics, and hydrogen cascades to revive humanity's cradle.";
  if (cluster == kClusterStasis)
    return "Forging Sgr A* gravitational tides and Arseny's stasis armor into an unbreakable trans-dimensional brotherhood.";
  if (cluster == kClusterEden)
    return "Linking Mia's vow, M13 biosphere resonances, and the Nomad citadel into a blooming stellar haven.";
  if (cluster == kClusterMultiverse)
    return "Merging the Sixth Egorka's harmonics and temporal phase shards to master singularity bridges.";
  return "Unified tapestry of thought.";
}

std::string ZettelGraph::getTrueEndingEpilogueRu(const std::string& cluster) {
  if (cluster == kClusterSolar) {
    return "★ ИСТИННЫЙ ФИНАЛ: ВЕЧНЫЙ РАССВЕТ.\n"
           "Благодаря синтезу эффекта Казимира и водородного каскада ядро Солнца полностью "
           "стабилизировано. Земля спасена не на десятилетия, а на 5 миллиардов лет вперед. "
           "С крыш Парижа под теплым летним дождем Елена и Егорка наблюдают за золотым рассветом "
           "возрожденного светила.";
  }
  if (cluster == kClusterStasis) {
    return "★ ИСТИННЫЙ ФИНАЛ: БРАТСТВО ПЯТИ МИРОВ.\n"
           "Арсений сбрасывает тяжесть пережитого предательства Пятой Вселенной. Объединив "
           "технологии гравитационных щитов Sgr A* и пятислойного стазиса, флот создает "
           "несокрушимый форпост человечества. Арсений впервые за долгие годы улыбается: "
           "«Ты доказал, парень. Надежда — не яд, если мы стоим плечом к плечу».";
  }
  if (cluster == kClusterEden) {
    return "★ ИСТИННЫЙ ФИНАЛ: ЦВЕТУЩИЙ ЭДЕМ.\n"
           "Экзопланета скопления M13 встречает экспедицию лазурными океанами и цветущими горными "
           "долинами. Цитадель «Номад» разворачивает шпили среди водопадов. Мия высаживает земную "
           "вишню на склоне изумрудного холма, сжимая руку Егорки: «Наш новый дом прекраснее любых снов».";
  }
  if (cluster == kClusterMultiverse) {
    return "★ ИСТИННЫЙ ФИНАЛ: АРХИТЕКТОРЫ БЕСКОНЕЧНОСТИ.\n"
           "Шесть воплощений Егорки соединяются в гармонический тандем. Временная петля Sgr A* "
           "перекована в вечный мост между параллельными реальностями. Ни одна Земля больше не "
           "погибнет во тьме — знания и ресурсы свободно текут сквозь измерения.";
  }
  return "";
}

std::string ZettelGraph::getTrueEndingEpilogueEn(const std::string& cluster) {
  if (cluster == kClusterSolar) {
    return "★ TRUE ENDING: ETERNAL DAWN.\n"
           "Through the synthesis of the Casimir metric and hydrogen cascade, the Sun's core is "
           "permanently rejuvenated for 5 billion years. Watching from the rooftops of Paris under "
           "a gentle warm rain, Elena and Egorka witness the radiant golden dawn of a reborn home.";
  }
  if (cluster == kClusterStasis) {
    return "★ TRUE ENDING: BROTHERHOOD OF FIVE REALITIES.\n"
           "Arseny overcomes the cynicism and trauma of Universe-05's tragic collapse. Combining "
           "Sgr A* gravity shielding with 5-layer stasis, the fleet builds an impenetrable fortress "
           "for humanity. Arseny smiles: 'You proved it, kid. Hope isn't poison when we stand shoulder to shoulder'.";
  }
  if (cluster == kClusterEden) {
    return "★ TRUE ENDING: BLOSSOMING EDEN.\n"
           "The M13 exoplanet welcomes the expedition with turquoise oceans and blooming mountain "
           "valleys. The Nomad Citadel raises shining spires among waterfalls. Mia plants the seeds "
           "of Earth's cherry blossom on an emerald hill: 'Our new home is more beautiful than anything we ever dreamed'.";
  }
  if (cluster == kClusterMultiverse) {
    return "★ TRUE ENDING: ARCHITECTS OF INFINITY.\n"
           "Six iterations of Egorka unite in harmonic synchrony. The temporal loop of Sgr A* is "
           "reforged into an eternal bridge across parallel timelines. No Earth will ever freeze in "
           "darkness again — knowledge and life flow freely through the multiverse.";
  }
  return "";
}

std::string zettelkastenFilePathFor(const std::string& dir) {
  return dir + "/zettelkasten.json";
}

}  // namespace astro
