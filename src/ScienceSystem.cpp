#include "ScienceSystem.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace astro {

namespace {

// Clamps beta to avoid relativistic singularity in sqrt((1+beta)/(1-beta))
double clampBeta(double b) {
  if (!std::isfinite(b)) return 0.0;
  if (b < -0.95) return -0.95;
  if (b > 0.95) return 0.95;
  return b;
}

double computeDopplerFactor(double beta) {
  const double b = clampBeta(beta);
  return std::sqrt((1.0 + b) / (1.0 - b));
}

void applyDopplerToLines(std::vector<SpectralLine>& lines, double dopplerFactor) {
  if (std::abs(dopplerFactor - 1.0) < 1e-4) return;
  for (auto& line : lines) {
    line.wavelengthNm = static_cast<float>(line.wavelengthNm * dopplerFactor);
  }
}

}  // namespace

SpectralAnalysis analyzeSpectrum(const std::string& stationId, double dopplerBeta) {
  SpectralAnalysis sp;
  sp.stationId = stationId;
  const double dopplerFactor = computeDopplerFactor(dopplerBeta);

  if (stationId == "sol") {
    sp.targetName = "Солнечная система / Земля";
    sp.spectralClass = "G2V Жёлтый карлик + Земная биосфера";
    sp.surfaceTempK = 5778.0f;
    sp.magneticFieldTesla = 1e-4f;
    sp.radiationLevel = "Норма (1.0 AU)";
    sp.hasBiosignature = true;
    sp.chemicalAbundance = {
        {"N2 (Азот)", 78.08f},
        {"O2 (Кислород)", 20.95f},
        {"Ar (Аргон)", 0.93f},
        {"H2O (Водяной пар)", 0.40f},
        {"CO2 (Диоксид углерода)", 0.04f},
        {"CH4 (Метан / Биосигнатура)", 0.002f},
    };
    sp.lines = {
        {656.3f, 0.95f, "H-alpha (Бальмер)", false},
        {486.1f, 0.85f, "H-beta (Бальмер)", false},
        {589.0f, 0.90f, "Na-D1 (Фраунгофер)", false},
        {589.6f, 0.90f, "Na-D2 (Фраунгофер)", false},
        {718.0f, 0.60f, "H2O (Поглощение пара)", false},
        {727.0f, 0.50f, "CH4 (Биомаркер метана)", false},
    };
    sp.summaryRu = "Земной спектр: выраженные линии воды, кислорода и метана. Жизнь подтверждена.";
    sp.summaryEn = "Terrestrial spectrum: prominent H2O, O2, and methane signatures. Biosphere confirmed.";
  } else if (stationId == "sgra") {
    sp.targetName = "Стрелец A* (Sgr A*)";
    sp.spectralClass = "Керровская сверхмассивная чёрная дыра (4.15e6 M☉)";
    sp.surfaceTempK = 1.2e7f;
    sp.magneticFieldTesla = 0.02f;
    sp.radiationLevel = "Экстремальный синхротронный рентген";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Ионизированный H+ (Плазма)", 74.0f},
        {"Гелий He++", 24.0f},
        {"Релятивистские электроны e-", 2.0f},
    };
    sp.lines = {
        {656.3f, 0.70f, "H-alpha (Грав. красное смещение)", false},
        {434.0f, 0.50f, "H-gamma (Широкий рентген-хвост)", false},
        {500.7f, 0.80f, "O-III (Горячий корональный газ)", true},
        {587.6f, 0.85f, "He-I (Ионизация эргосферы)", true},
    };
    sp.summaryRu = "Аккреционный диск ISCO: рентгеновское излучение, сильнейший гравитационный сдвиг метрики.";
    sp.summaryEn = "ISCO accretion disk: hard X-ray flux, immense gravitational spacetime metric shift.";
  } else if (stationId == "m87") {
    sp.targetName = "M87* (Virgo A)";
    sp.spectralClass = "Гигантская ЧД с релятивистским джетом (6.5e9 M☉)";
    sp.surfaceTempK = 3.5e7f;
    sp.magneticFieldTesla = 0.05f;
    sp.radiationLevel = "Ультрарелятивистский джет (гамма/ТэВ)";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Релятивистская e-/e+ пара-плазма", 85.0f},
        {"Ионизированный протонный газ", 15.0f},
    };
    sp.lines = {
        {486.1f, 0.90f, "H-beta (Синхротронный пик)", true},
        {500.7f, 0.95f, "O-III (Ударный фронт джета)", true},
        {656.3f, 0.85f, "H-alpha (Доплеровский пучок)", true},
    };
    sp.summaryRu = "Джет M87*: синхротронное излучение, доплеровское усиление наступающей струи.";
    sp.summaryEn = "M87* Jet: synchrotron emission, relativistic Doppler beaming along approach axis.";
  } else if (stationId == "qso-3c273") {
    sp.targetName = "Квазар 3C 273";
    sp.spectralClass = "Сверхяркий квазар ранней Вселенной (z = 0.158)";
    sp.surfaceTempK = 8.0e7f;
    sp.magneticFieldTesla = 0.10f;
    sp.radiationLevel = "Сверхэддингтоновская светимость 4e12 L☉";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Ионизированный водород H+", 70.0f},
        {"Гелий He++", 26.0f},
        {"Высокоионизированные металлы Fe, C, O", 4.0f},
    };
    sp.lines = {
        {656.3f, 0.98f, "H-alpha (Космологический сдвиг z=0.158)", true},
        {486.1f, 0.92f, "H-beta (Широкая эмиссионная область BLR)", true},
        {500.7f, 0.88f, "O-III (Узкие линии NLR)", true},
        {587.6f, 0.75f, "He-I (Квазарная корона)", true},
    };
    sp.summaryRu = "Ядро квазара: колоссальное космологическое излучение сквозь 2.4 млрд лет.";
    sp.summaryEn = "Quasar core: immense cosmological radiation bridging 2.4 billion light-years.";
  } else if (stationId == "m16") {
    sp.targetName = "Столпы Творения (M16)";
    sp.spectralClass = "Молекулярное облако и область звездообразования H II";
    sp.surfaceTempK = 75.0f;
    sp.magneticFieldTesla = 1e-8f;
    sp.radiationLevel = "Ультрафиолетовая фотоионизация NGC 6611";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Молекулярный водород H2", 89.0f},
        {"Гелий He", 9.5f},
        {"Полициклические углеводороды (PAH)", 1.0f},
        {"Силикатная и графитовая пыль", 0.5f},
    };
    sp.lines = {
        {656.3f, 0.99f, "H-alpha (Эмиссионная водородная вуаль)", true},
        {500.7f, 0.92f, "O-III (Ионизация O-звёздами)", true},
        {658.4f, 0.85f, "N-II (Границы газовых глобул)", true},
        {672.4f, 0.80f, "S-II (Фронты ударного сжатия)", true},
    };
    sp.summaryRu = "Колонны холодного газа: идеальный спектр зарождения звёзд и протопланетных дисков.";
    sp.summaryEn = "Cold hydrogen trunks: pristine star-forming and protoplanetary accretion spectra.";
  } else if (stationId == "gw170817") {
    sp.targetName = "Килоновая GW170817";
    sp.spectralClass = "Слияние нейтронных звёзд / r-процесс нуклеосинтеза";
    sp.surfaceTempK = 4.5e6f;
    sp.magneticFieldTesla = 1e11f;
    sp.radiationLevel = "Всплеск гамма-лучей (sGRB) + радиоактивный распад";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Золото (Au) и Платина (Pt)", 12.0f},
        {"Лантаноиды и актиноиды", 28.0f},
        {"Свободные нейтроны и осколки деления", 60.0f},
    };
    sp.lines = {
        {267.6f, 0.95f, "Au-I (Линии ультрафиолета золота)", true},
        {306.8f, 0.90f, "Pt-I (Эмиссия платины)", true},
        {750.0f, 0.85f, "Лантаноидный непрозрачный ИК-пик", true},
    };
    sp.summaryRu = "Килоновая: прямые спектральные маркеры синтеза золота и тяжелейших элементов Вселенной.";
    sp.summaryEn = "Kilonova: direct spectral fingerprints of gold, platinum, and r-process nucleosynthesis.";
  } else if (stationId == "oj287") {
    sp.targetName = "Двойная ЧД OJ 287";
    sp.spectralClass = "Прецессирующая бинарная ЧД (1.83e10 M☉ + 1.5e8 M☉)";
    sp.surfaceTempK = 4.2e7f;
    sp.magneticFieldTesla = 0.08f;
    sp.radiationLevel = "Периодические вспышки прокола диска";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Ионизированный аккреционный газ", 80.0f},
        {"Турбулентная плазма ударных волн", 20.0f},
    };
    sp.lines = {
        {656.3f, 0.90f, "H-alpha (Двугорбый профиль орбиты)", true},
        {486.1f, 0.86f, "H-beta (Орбитальный сдвиг компаньона)", true},
        {587.6f, 0.82f, "He-I (Вспышка пробоя диска)", true},
    };
    sp.summaryRu = "Двойная система: квантовые колебания метрики при проколах аккреционного диска.";
    sp.summaryEn = "Binary black hole: periodic disc punch flares and spacetime metric ripples.";
  } else if (stationId == "hyperdisk") {
    sp.targetName = "Гипердиск";
    sp.spectralClass = "Сверхмассивная ЧД с гигантским тонким диском (26 Rs)";
    sp.surfaceTempK = 6.0e6f;
    sp.magneticFieldTesla = 0.03f;
    sp.radiationLevel = "Равномерное ультрафиолетовое поле";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"Водородно-гелиевая тонкая плазма", 92.0f},
        {"Тяжёлые элементы диска", 8.0f},
    };
    sp.lines = {
        {656.3f, 0.88f, "H-alpha (Новиков-Торн релаксация)", false},
        {589.0f, 0.84f, "Na-D (Тонкая холодная периферия)", false},
        {486.1f, 0.80f, "H-beta (Кеплеровский сдвиг)", false},
    };
    sp.summaryRu = "Гигантский аккреционный диск: кеплеровский профиль скоростей, идеальный для разгона варпа.";
    sp.summaryEn = "Colossal accretion disc: Keplerian velocity profile suited for warp slingshots.";
  } else if (stationId == "voyager1") {
    sp.targetName = "Зонд «Вояджер-1» / Золотая пластинка";
    sp.spectralClass = "Межзвёздный аппарат / Реликвия человечества (1977)";
    sp.surfaceTempK = 38.0f;
    sp.magneticFieldTesla = 1e-6f;
    sp.radiationLevel = "Межзвёздные космические лучи (Гелиопауза)";
    sp.hasBiosignature = true;
    sp.chemicalAbundance = {
        {"Au (Золото / Фонографическая пластинка)", 68.0f},
        {"Al (Алюминий / Корпус)", 22.0f},
        {"Pu-238 (Плутоний / РИТЭГ)", 7.0f},
        {"Cu (Медь / Позолоченная матрица)", 3.0f},
    };
    sp.lines = {
        {1420.4f, 1.0f, "H-I 21cm (Радиочастота нейтрального водорода)", true},
        {267.6f, 0.90f, "Au-I (Позолоченное покрытие пластинки)", false},
        {396.1f, 0.75f, "Al-I (Несущая ферма антенны)", false},
        {589.0f, 0.60f, "Na-I (След межзвёздного газа)", false},
    };
    sp.summaryRu = "Зарегистрирован золотой диск «Sounds of Earth» и радиоизлучение на волне 21 см. Величайшая капсула времени человечества.";
    sp.summaryEn = "Detected 'Sounds of Earth' Golden Record and 21 cm hydrogen emission line. Humanity's greatest time capsule.";
  } else {
    // Default fallback celestial spectral profile
    sp.targetName = stationId;
    sp.spectralClass = "Астрономический объект";
    sp.surfaceTempK = 10000.0f;
    sp.magneticFieldTesla = 1e-4f;
    sp.radiationLevel = "Умеренная";
    sp.hasBiosignature = false;
    sp.chemicalAbundance = {
        {"H (Водород)", 75.0f},
        {"He (Гелий)", 23.0f},
        {"Металлы Z", 2.0f},
    };
    sp.lines = {
        {656.3f, 0.85f, "H-alpha", false},
        {486.1f, 0.75f, "H-beta", false},
        {500.7f, 0.70f, "O-III", true},
    };
    sp.summaryRu = "Стандартный спектр глубокого космоса с преобладанием водородно-гелиевых компонент.";
    sp.summaryEn = "Standard deep space spectrum with dominant hydrogen and helium components.";
  }

  // Apply relativistic Doppler frequency scaling to all spectral lines
  applyDopplerToLines(sp.lines, dopplerFactor);

  return sp;
}

ScienceCodex::ScienceCodex() {
  init();
}

void ScienceCodex::init() {
  m_entries.clear();
  m_totalPoints = 0;

  auto add = [this](const std::string& id, const std::string& ru,
                    const std::string& en, const std::string& catRu,
                    const std::string& catEn, const std::string& descRu,
                    const std::string& descEn) {
    CodexEntry e;
    e.stationId = id;
    e.nameRu = ru;
    e.nameEn = en;
    e.categoryRu = catRu;
    e.categoryEn = catEn;
    e.descriptionRu = descRu;
    e.descriptionEn = descEn;
    e.scanned = false;
    e.scannedTimestamp = 0.0;
    e.researchPoints = 100;
    m_entries.push_back(e);
  };

  add("sol", "Солнечная система", "Solar System", "Планетная система",
      "Planetary System",
      "Колыбель человечества. Жёлтый карлик G2V, 8 планет, пояс астероидов и "
      "Земля с уникальной биосферой.",
      "Cradle of humanity. G2V yellow dwarf, 8 planets, asteroid belt and "
      "Earth with a thriving biosphere.");

  add("sgra", "Стрелец A*", "Sagittarius A*", "Чёрная дыра", "Black Hole",
      "Сверхмассивная чёрная дыра в центре Млечного Пути. 4.15 млн масс "
      "Солнца. Гравитационный резонатор петли времени.",
      "Supermassive black hole at the center of the Milky Way. 4.15 million "
      "solar masses. Time loop gravitational resonator.");

  add("m87", "M87*", "M87*", "Чёрная дыра", "Black Hole",
      "Сверхмассивная чёрная дыра с релятивистским джетом длиной 5000 световых "
      "лет в созвездии Девы.",
      "Supermassive black hole hosting a 5,000 light-year relativistic jet in "
      "the constellation Virgo.");

  add("oj287", "OJ 287", "OJ 287", "Чёрная дыра", "Black Hole",
      "Уникальная бинарная чёрная дыра (18.3 млрд M☉), где меньший компаньон "
      "периодически пробивает аккреционный диск.",
      "Binary black hole system (18.3 billion M☉) where a companion smashes "
      "through the giant accretion disc.");

  add("hyperdisk", "Гипердиск", "Hyperdisk", "Чёрная дыра", "Black Hole",
      "Экстремальный тонкий аккреционный диск радиусом 26 Rs с кеплеровским "
      "вращением и ламинарной плазмой.",
      "Colossal 26-Rs thin accretion disc with laminar Keplerian plasma flow.");

  add("qso-3c273", "Квазар 3C 273", "Quasar 3C 273", "Квазар", "Quasar",
      "Первый открытый квазар. Ядро ярче 4 триллионов солнц, свет шёл к нам "
      "2.4 миллиарда лет.",
      "The first discovered quasar. Outshines 4 trillion suns, light traveling "
      "2.4 billion years.");

  add("m16", "Столпы Творения (M16)", "Pillars of Creation", "Туманность",
      "Nebula",
      "Колонны холодного молекулярного водорода и пыли длиной в 4 световых "
      "года в туманности Орёл.",
      "Trunks of interstellar molecular hydrogen and dust four light-years "
      "tall in the Eagle Nebula.");

  add("gw170817", "Килоновая GW170817", "Kilonova GW170817", "Килоновая",
      "Kilonova",
      "Столкновение двух нейтронных звёзд. Кузница тяжёлых элементов: "
      "нуклеосинтез золота и платины.",
      "Neutron star merger generating gravitational ripples and forging oceans "
      "of gold and platinum via the r-process.");

  add("sn1987a", "Сверхновая SN 1987A", "Supernova SN 1987A", "Сверхновая",
      "Supernova",
      "Остаток вспышки сверхновой в БМО: ожерелье из 32 светящихся жемчужин "
      "горячего ударного фронта.",
      "Supernova remnant in the LMC featuring a luminous triple ring with 32 "
      "incandescent hot pearls.");

  add("at2019qiz", "TDE AT2019qiz", "TDE AT2019qiz", "Приливной разрыв",
      "Tidal Disruption",
      "Звезда, разорванная приливными силами чёрной дыры в тончайшие нити "
      "релятивистского спагетти.",
      "A star shredded into glowing gravitational filaments by supermassive "
      "black hole tidal forces.");

  add("m1", "Крабовидная туманность (M1)", "Crab Nebula", "Туманность",
      "Nebula",
      "Плеромион (PWN): релятивистский пульсарный ветер, экваториальный "
      "синхротронный тор и динамические виспы.",
      "Pulsar wind nebula remnant of 1054 CE supernova with synchrotron torus "
      "and relativistic wisps.");

  add("psr-crab", "Крабовый пульсар", "Crab Pulsar", "Пульсар", "Pulsar",
      "Быстро вращающаяся нейтронная звезда (33 мс), космический метроном в "
      "центре туманности M1.",
      "Rapidly spinning 33 ms neutron star, an ultra-precise cosmic metronome.");

  add("psr-vela", "Пульсар в Парусах", "Vela Pulsar", "Пульсар", "Pulsar",
      "Молодой пульсар с наклонённым магнитным диполем и радио-маяком.",
      "Young pulsar with tilted magnetic dipole beams and radio lighthouse.");

  add("sgr-1806", "Магнетар SGR 1806-20", "Magnetar SGR 1806-20", "Магнетар",
      "Magnetar",
      "Нейтронная звезда со сверхсильным магнитным полем 10^15 Гаусс и "
      "гигантскими звездотрясениями.",
      "Neutron star with extreme 10^15 Gauss magnetic field and giant gamma "
      "starquakes.");

  add("psr-b1913", "Пульсар Халса-Тейлора", "Hulse-Taylor Binary", "Пульсар",
      "Pulsar",
      "Двойной пульсар PSR B1913+16, чьё орбитальное сближение впервые доказало "
      "гравитационные волны.",
      "Binary pulsar whose orbital decay first proved the existence of "
      "gravitational waves.");

  add("m13", "Скопление Геркулеса (M13)", "Hercules Cluster (M13)", "Скопление",
      "Star Cluster",
      "Шаровое скопление из 300 000 звёзд возрастом более 11 миллиардов лет.",
      "Globular cluster of 300,000 ancient stars over 11 billion years old.");

  add("m31", "Галактика Андромеды", "Andromeda Galaxy (M31)", "Галактика",
      "Galaxy",
      "Ближайшая к нам гигантская спиральная галактика, летящая навстречу "
      "Млечному Пути.",
      "Nearest major spiral galaxy, currently on a collision course with the "
      "Milky Way.");

  add("m51", "Галактика Водоворот (M51)", "Whirlpool Galaxy (M51)", "Галактика",
      "Galaxy",
      "Классическая грандиозная спираль с двумя раскрытыми рукавами и "
      "карликовым спутником NGC 5195.",
      "Grand-design spiral galaxy with two magnificent open arms and companion "
      "NGC 5195.");

  add("m104", "Галактика Сомбреро (M104)", "Sombrero Galaxy (M104)",
      "Галактика", "Galaxy",
      "Спиральная галактика с гигантским ярким балджем и чёткой полосой тёмной "
      "пыли по экватору.",
      "Spiral galaxy with an enormous brilliant bulge and sharp equatorial dust "
      "lane.");

  add("mw", "Млечный Путь", "Milky Way", "Галактика", "Galaxy",
      "Наш космический дом. Спиральная галактика с перемычкой диаметром 100 000 "
      "световых лет.",
      "Our cosmic home. Barred spiral galaxy spanning 100,000 light-years.");

  add("ori", "Созвездие Орион", "Orion", "Созвездие", "Constellation",
      "Ярчайшее экваториальное созвездие с красным сверхгигантом Бетельгейзе и "
      "поясом трёх звёзд.",
      "Equatorial constellation featuring red supergiant Betelgeuse and three "
      "belt stars.");

  add("uma", "Большая Медведица", "Ursa Major", "Созвездие", "Constellation",
      "Знаменитый Ковш северного неба, тысячелетиями служивший главным компасом "
      "мореплавателей.",
      "Famous northern constellation whose Pointer stars guide navigation "
      "toward Polaris.");

  add("voyager1", "Зонд «Вояджер-1»", "Voyager 1 Probe", "Реликвия",
      "Cosmic Relic",
      "Запущенный в 1977 году посланник Земли. Несёт Золотую пластинку с "
      "приветствиями на 55 языках, музыкой Баха и схемой расположения Солнца "
      "относительно 14 пульсаров.",
      "Launched in 1977, Earth's first interstellar ambassador. Carries the "
      "Golden Record with greetings in 55 languages and a 14-pulsar map.");
}

const CodexEntry* ScienceCodex::getEntry(const std::string& stationId) const {
  for (const auto& e : m_entries) {
    if (e.stationId == stationId) return &e;
  }
  return nullptr;
}

bool ScienceCodex::isScanned(const std::string& stationId) const {
  for (const auto& e : m_entries) {
    if (e.stationId == stationId) return e.scanned;
  }
  return false;
}

bool ScienceCodex::scanObject(const std::string& stationId, double timestamp) {
  for (auto& e : m_entries) {
    if (e.stationId == stationId) {
      if (!e.scanned) {
        e.scanned = true;
        e.scannedTimestamp = timestamp;
        m_totalPoints += e.researchPoints;
        return true;  // Newly scanned!
      }
      return false;  // Already scanned before
    }
  }
  return false;
}

int ScienceCodex::getScannedCount() const {
  int count = 0;
  for (const auto& e : m_entries) {
    if (e.scanned) ++count;
  }
  return count;
}

int ScienceCodex::getTotalCount() const {
  return static_cast<int>(m_entries.size());
}

std::string ScienceCodex::serialize() const {
  std::ostringstream ss;
  ss << "points=" << m_totalPoints << "\n";
  for (const auto& e : m_entries) {
    if (e.scanned) {
      ss << "scanned=" << e.stationId << ":" << e.scannedTimestamp << "\n";
    }
  }
  return ss.str();
}

bool ScienceCodex::deserialize(const std::string& data) {
  std::istringstream ss(data);
  std::string line;
  while (std::getline(ss, line)) {
    if (line.rfind("points=", 0) == 0) {
      m_totalPoints = std::atoi(line.substr(7).c_str());
    } else if (line.rfind("scanned=", 0) == 0) {
      const std::string payload = line.substr(8);
      const size_t col = payload.find(':');
      if (col != std::string::npos) {
        const std::string id = payload.substr(0, col);
        const double ts = std::atof(payload.substr(col + 1).c_str());
        for (auto& e : m_entries) {
          if (e.stationId == id) {
            e.scanned = true;
            e.scannedTimestamp = ts;
            break;
          }
        }
      }
    }
  }
  return true;
}

void BridgeComms::queueMessage(const RadioMessage& msg, double currentTime) {
  // If active message exists with higher priority and not yet expired, ignore lower priority
  if (m_activeMessage.has_value()) {
    if (m_activeMessage->priority > msg.priority &&
        (currentTime - m_activeMessage->timestamp < m_activeMessage->durationSec)) {
      return;
    }
  }
  RadioMessage m = msg;
  m.timestamp = currentTime;
  m_activeMessage = m;
  markTriggered(msg.id);
}

void BridgeComms::update(double currentTime) {
  if (m_activeMessage.has_value()) {
    if (currentTime - m_activeMessage->timestamp >= m_activeMessage->durationSec) {
      m_activeMessage.reset();
    }
  }
}

float BridgeComms::getMessageProgress(double currentTime) const {
  if (!m_activeMessage.has_value()) return 0.0f;
  const double elapsed = currentTime - m_activeMessage->timestamp;
  if (m_activeMessage->durationSec <= 0.0) return 1.0f;
  return static_cast<float>(std::clamp(elapsed / m_activeMessage->durationSec, 0.0, 1.0));
}

void BridgeComms::dismiss() {
  m_activeMessage.reset();
}

bool BridgeComms::hasTriggered(const std::string& id) const {
  return m_triggeredIds.find(id) != m_triggeredIds.end();
}

void BridgeComms::markTriggered(const std::string& id) {
  if (!id.empty()) {
    m_triggeredIds.insert(id);
  }
}

void BridgeComms::resetTriggers() {
  m_triggeredIds.clear();
  m_activeMessage.reset();
}

}  // namespace astro
