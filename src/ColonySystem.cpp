#include "ColonySystem.h"

#include <algorithm>
#include <cmath>

namespace astro {

const std::array<ColonyWorld, 8>& colonyWorlds() {
  static const std::array<ColonyWorld, 8> worlds = {{
      {"moon", "Луна", "Moon", "sol",
       "0.166 g", "Вакуум (< 10⁻¹⁰ Па)", "Vacuum (< 10⁻¹⁰ Pa)",
       "-130°C / +120°C", "-130°C / +120°C",
       {"Гелий-3 & Варп", "Helium-3 & Warp",
        "Добыча изотопов гелия-3 ускоряет перезарядку варп-прыжков на 25%",
        "Lunar He-3 mining accelerates warp jump recharge by 25%",
        0.25f, 0.0f, 0.0f, 0.0f, 0.0f}},
      {"mars", "Марс", "Mars", "sol",
       "0.38 g", "0.006 атм (95% CO₂)", "0.006 atm (95% CO₂)",
       "-63°C (-140°C / +20°C)", "-63°C (-140°C / +20°C)",
       {"Индустрия и Композиты", "Industry & Composites",
        "Красная планета богата рудами; стоимость построек снижена на 15%",
        "Red planet is rich in ores; construction costs reduced by 15%",
        0.0f, 0.0f, 0.15f, 0.0f, 0.0f}},
      {"mercury", "Меркурий", "Mercury", "sol",
       "0.38 g", "Экзосфера (Na, He, O)", "Exosphere (Na, He, O)",
       "-180°C / +430°C", "-180°C / +430°C",
       {"Солнечный Щит", "Solar Shield Matrix",
        "Близость к Солнцу питает дефлекторные матрицы; прочность щитов +20%",
        "Proximity to Sol powers deflector arrays; ship shield strength +20%",
        0.0f, 0.20f, 0.0f, 0.0f, 0.0f}},
      {"europa", "Европа", "Europa", "sol",
       "0.134 g", "Следовая O₂ (10⁻¹¹ атм)", "Trace O₂ (10⁻¹¹ atm)",
       "-160°C (лёд 20 км)", "-160°C (20 km ice crust)",
       {"Океаническая Ксенобиология", "Oceanic Xenobiology",
        "Подлёдный океан раскрывает тайны органики; +40% RP за биосканы",
        "Subsurface ocean unlocks origin of life; +40% RP from bioscans",
        0.0f, 0.0f, 0.0f, 0.40f, 0.0f}},
      {"ganymede", "Ганимед", "Ganymede", "sol",
       "0.146 g", "Следовая O₂", "Trace O₂",
       "-163°C (магнитосфера 1.43 мкТл)", "-163°C (magnetosphere 1.43 µT)",
       {"Магнитосферный Хаб", "Magnetospheric Hub",
        "Собственное магнитное поле защищает колонию; +15% к пассивному доходу науки",
        "Intrinsic magnetic field protects settlements; +15% passive science rate",
        0.0f, 0.05f, 0.0f, 0.10f, 0.15f}},
      {"callisto", "Каллисто", "Callisto", "sol",
       "0.126 g", "Следовая CO₂", "Trace CO₂",
       "-139°C (радиация < 0.01 мЗв/сут)", "-139°C (radiation < 0.01 mSv/day)",
       {"Дальняя База Снабжения", "Deep Logistics Base",
        "Низкий радиационный фон идеален для складов; ремонт и заправка улучшены",
        "Low radiation environment is ideal for depots; repairs and resupply improved",
        0.10f, 0.0f, 0.05f, 0.0f, 0.10f}},
      {"titan", "Титан", "Titan", "sol",
       "0.138 g", "1.45 атм (98% N₂, 2% CH₄)", "1.45 atm (98% N₂, 2% CH₄)",
       "-179°C (94 К, метан-этановые моря)", "-179°C (94 K, methane-ethane seas)",
       {"Криохимический Синтез", "Cryochemical Synthesis",
        "Метановые моря и плотная атмосфера обеспечивают синтетическим топливом",
        "Methane seas and dense atmosphere supply unlimited synthetic hydrocarbons",
        0.15f, 0.0f, 0.05f, 0.15f, 0.25f}},
      {"hercules-b", "Геркулес b", "Hercules b", "m13",
       "1.15 g", "Экзотическая плазменная мантия", "Exotic plasma mantle",
       "+14°C (тахионный резонанс)", "+14°C (tachyon resonance)",
       {"Экзоцивилизация & Резонанс", "Exocivilization & Resonance",
        "Мост между мирами; колоссальный бонус к науке и гипотезам Мультивселенной",
        "Bridge across worlds; colossal bonus to deep space science and Multiverse hypotheses",
        0.20f, 0.15f, 0.10f, 0.50f, 0.30f}}}};
  return worlds;
}

const ColonyWorld* colonyWorld(const std::string& id) {
  for (const auto& world : colonyWorlds()) {
    if (id == world.id) return &world;
  }
  return nullptr;
}

const std::array<ColonyStage, kColonyStageCount>& colonyStages() {
  static const std::array<ColonyStage, kColonyStageCount> stages = {{
      // =========================================================================
      // --- Эпоха I: Аванпост и Жизнеобеспечение (Stages 1-5) ---
      // =========================================================================
      {"Разведпост и Первичный Спуск", "Survey Outpost & Touchdown",
       "Командный посадочный модуль, автономная метеостанция и кевларовый шлюз",
       "Landing command module, autonomous weather mast and kevlar airlock",
       "Первая точка опоры на чужом мире. Криогенные посадочные опоры, многослойная вакуумно-экранная изоляция (MLI) и первичная радиационная защита из гидрида титана.",
       "First human foothold on an alien world. Cryogenic landing gear, multi-layer vacuum insulation (MLI) and titanium hydride cosmic ray shielding.",
       "Орбитальный навигационный маркер; телеметрия состава грунта в реальном времени",
       "Orbital landing navigation marker; real-time surface composition telemetry",
       0, 0, 4, 0, 0, false, ColonyEpoch::Outpost},

      {"Мачта Дальней Связи", "Deep-Space Relay Mast",
       "45-метровая углепластиковая решётчатая ферма, параболическая X/Ka-антенна и лазерный терминал",
       "45-meter composite truss mast, X/Ka-band dish and optical laser terminal",
       "Гигабитный оптический мост с Землей и орбитальными станциями. Преодолевает радиотень планетарного горизонта и задержки сигнала.",
       "Gigabit laser bridge to Earth and orbital stations. Overcomes radio horizon shadows and light lag.",
       "Пассивный доход 1 RP/мин; автоматическая передача поверхностных сканов в бортовой журнал",
       "1 RP/min passive generation; auto-relay of surface scans directly to ship log",
       25, 1, 8, 0, 1, false, ColonyEpoch::Outpost},

      {"Фотоэлектрическая Сеть", "Solar & Solid-State Power Grid",
       "Двусторонние перовскитные солнечные трекеры с виброочисткой от пыли и литий-серные батареи",
       "Bifacial perovskite PV trackers with electro-vibrational dust shedding and Li-S cells",
       "Преодолевает экстремальные перепады криогенных ночей. Обеспечивает базу круглосуточным стабильным питанием 500 кВт.",
       "Survives deep cryogenic nights. Supplies the colony base with continuous 500 kW bus power.",
       "Пассивный доход 2 RP/мин; ускоренная подзарядка бортовых конденсаторов варпа на 15%",
       "2 RP/min; 15% faster warp capacitor recharge on landing or low orbit",
       40, 1, 16, 0, 2, false, ColonyEpoch::Outpost},

      {"Замкнутый Гидроконтур & Сабатье", "ECLSS Closed-Cycle Water & Sabatier",
       "Криогенный шнековый бур льда, мембранная дистилляция и каталитический метанатор",
       "Cryogenic auger drill, membrane filtration and Sabatier methanation reactor",
       "Замкнутость рециркуляции воды 98.5%. Реакция Сабатье соединяет выдыхаемый CO₂ с водородом, получая воду и метан для двигателей.",
       "98.5% closed-loop water loop. Sabatier catalyst pairs exhaled CO₂ with hydrogen into pure water and methane propellant.",
       "Пассивный доход 3 RP/мин; пополнение бортовых запасов кислорода и воды при посадке",
       "3 RP/min; emergency replenishment of ship life support water and O₂ upon landing",
       50, 2, 30, 0, 3, false, ColonyEpoch::Outpost},

      {"Геодезические Биокупола I", "Geodesic Habitat Domes I",
       "Двухоболочечные купола с реголито-бетонным панцирем и прозрачным ETFE-сводом",
       "Dual-shell domes with sintered regolith radiation shell and transparent ETFE canopy",
       "Полноценная искусственная атмосфера 101.3 кПа (78% N₂, 21% O₂). Защита от микрометеоритов и сенсорной депривации.",
       "Full 101.3 kPa artificial atmosphere (78% N₂, 21% O₂). Total protection from micrometeorites and sensory deprivation.",
       "Пассивный доход 4 RP/мин; статус постоянного поселения; завершение Эпохи I",
       "4 RP/min; permanent human settlement charter; Epoch I completed",
       65, 2, 75, 0, 4, false, ColonyEpoch::Outpost},

      // =========================================================================
      // --- Эпоха II: Тяжёлая Индустрия и Ресурсы (Stages 6-10) ---
      // =========================================================================
      {"Агрокомплекс & Биореакторы", "Agro-Industrial Bioreactors",
       "Вертикальные аэропонные фермы, спектральные LED-ярусы и спирулиновые фотобиореакторы",
       "Vertical aeroponic towers, spectral LED racks and spirulina photobioreactors",
       "Свежая биомасса, белковый синтез и биологическая оксигенация. Полное избавление от консервированных пайков.",
       "Fresh biomass, protein synthesis and biological oxygenation. Total independence from Earth ration packs.",
       "Пассивный доход 6 RP/мин; высокая продовольственная безопасность поселения",
       "6 RP/min; self-sustaining colony food security and population stability",
       80, 3, 150, 0, 6, false, ColonyEpoch::Industry},

      {"Карьерная Буровая & Тяжёлые Роверы", "Surface Excavation & Heavy Rovers",
       "Роботизированные экскаваторы с индукционными ковшами и лидарной SLAM-навигацией",
       "Robotic excavators with induction buckets and lidar-based SLAM autonomy",
       "Добыча сотен тонн базальта, ильменита и металлосодержащего реголита в сутки в условиях жесткого вакуума.",
       "Extraction of hundreds of tons of basalt, ilmenite and metallic regolith daily under harsh vacuum.",
       "Пассивный доход 8 RP/мин; сырьевой запас металлов для тяжёлой индустрии",
       "8 RP/min; raw mineral feedstock stockpile for heavy metallurgy",
       100, 3, 250, 0, 8, false, ColonyEpoch::Industry},

      {"Вакуумно-Дуговой Плавильный Завод", "Vacuum Arc Smelter & Refining",
       "Солнечно-плазменные печи, графеновые радиаторы охлаждения и вакуумные дегазаторы",
       "Solar-plasma furnaces, graphene radiator panels and vacuum degassing units",
       "Прямое восстановление титана, алюминия и кремния без углеродного следа. Графеновые панели отводят мегаватты тепла.",
       "Zero-carbon direct reduction of titanium, aluminium and silicon. Graphene panels radiate megawatts into vacuum.",
       "Пассивный доход 10 RP/мин; полевой ремонт обшивки корабля конструкционными сплавами",
       "10 RP/min; field hull patching using indigenous structural alloys",
       120, 4, 400, 0, 10, false, ColonyEpoch::Industry},

      {"Завод 3D-Печати Модулей", "Additive Module Fabrication Plant",
       "Лазерные порошковые спекающие комплексы (SLM/SLS) крупногабаритных конструкций",
       "Selective Laser Melting (SLM) heavy gantries for monolithic module printing",
       "Печать гермосекций, силовых шпангоутов и стыковочных узлов на месте за считанные часы.",
       "Printing pressure hulls, bulkheads and airlock rings on-site in hours without Earth imports.",
       "Пассивный доход 12 RP/мин; СКИДКА 10% на все последующие постройки колонии",
       "12 RP/min; 10% construction discount on all subsequent colony stages",
       140, 4, 650, 0, 12, false, ColonyEpoch::Industry},

      {"Маглев-Транспортная Артерия", "Superconductor Maglev Transit Spine",
       "Вакуумная эстакадная трасса на ВТСП-магнитах, грузопассажирские капсулы 1200 км/ч",
       "Evacuated YBCO high-temp superconductor maglev tube with 1200 km/h capsules",
       "Связывает кратеры, карьеры и жилые комплексы без трения колес и без расхода топлива.",
       "Connects craters, mines and domes friction-free with zero fuel burn across treacherous terrain.",
       "Пассивный доход 15 RP/мин; единая промышленная сеть; завершение Эпохи II",
       "15 RP/min; unified industrial network; Epoch II completed",
       160, 5, 950, 0, 15, false, ColonyEpoch::Industry},

      // =========================================================================
      // --- Эпоха III: Социум и Исследования (Stages 11-15) ---
      // =========================================================================
      {"Клинический Центр & Генная Терапия", "Clinical Health & Gene Therapy Center",
       "Центрифуги короткого радиуса, мРНК-стабилизаторы костной ткани и биолаборатория BSL-4",
       "Short-radius centrifuges, bone-density mRNA therapeutics and BSL-4 biocontainment",
       "Предотвращение деградации скелета в низкой гравитации. Защита генома от радиационного мутагенеза.",
       "Prevents skeletal and cardiovascular deconditioning in low gravity. Shields genome against mutagenesis.",
       "Пассивный доход 18 RP/мин; полное исцеление экипажа при визите на планету",
       "18 RP/min; complete crew health and fatigue restoration upon visiting",
       180, 5, 1400, 0, 18, false, ColonyEpoch::Society},

      {"Институт Планетарных Наук", "Planetary Science & Geology Institute",
       "Комплекс чистых комнат, масс-спектрометры и квантовый вычислительный кластер",
       "Cleanroom suite, secondary-ion mass spectrometers and quantum compute cluster",
       "Глубокое компьютерное моделирование тектоники, магнитного динамо и квантовых эффектов планеты.",
       "Deep computational modeling of planetary mantle convection, magnetic dynamos and quantum anomalies.",
       "Пассивный доход 22 RP/мин; +20% RP за ВСЕ новые сканирования в системе!",
       "22 RP/min; +20% RP bonus for ALL new object scans in the system",
       210, 6, 2000, 20, 22, false, ColonyEpoch::Society},

      {"Гражданский Город & Биопарки", "Civic Township & Biosphere Parks",
       "250-метровые купола с террасными лесопарками, озерами, школами и общественным форумом",
       "250-meter macro-domes with terraced forest parks, lakes, schools and civic plaza",
       "Формирование постоянного общества с семьями, культурой и искусством. Биосферные самоочищающиеся озёра.",
       "Formation of multi-generational society with art, music and education. Self-purifying biosphere park lakes.",
       "Пассивный доход 26 RP/мин; устойчивый демографический рост и приток специалистов",
       "26 RP/min; self-sustaining demographic growth and immigration inflow",
       250, 7, 3200, 20, 26, false, ColonyEpoch::Society},

      {"Планетарный Космопорт & Верфи", "Planetary Spaceport & Ship Servicing",
       "Четыре восьмиугольные базальтовые площадки, криохранилища LOX/CH₄/LH₂ и ангар докования",
       "Quad octagonal basalt landing tarmac, cryogenic LOX/CH₄/LH₂ tanks and service hangar",
       "Главные ворота мира в космос. Приём челноков, грузовых транспортов и кораблей с варп-приводом.",
       "World's gateway to the cosmos. Servicing freighters, passenger shuttles and warp-drive exploration vessels.",
       "Пассивный доход 30 RP/мин; +25% RP; ПОЛНАЯ ДОЗАПРАВКА, РЕМОНТ КОРПУСА И ЩИТОВ КОРАБЛЯ!",
       "30 RP/min; +25% RP; FULL SHIP AUTO-REFUEL, HULL RESTORATION AND SHIELD RECHARGE",
       300, 8, 5000, 25, 30, true, ColonyEpoch::Society},

      {"Субмиллиметровая Обсерватория", "Submillimeter Interferometer Array",
       "16-элементная интерферометрическая решётка радиотелескопов с крио-болометрами 0.1 К",
       "16-element submillimeter radio interferometer array with 0.1 K cryo-bolometers",
       "Сверхдлинная база наблюдений в чистом вакууме. Прямое картирование экзопланет и поиск реликтовых варп-сигнатур.",
       "Ultra-long baseline array in clean vacuum. Direct imaging of exoplanet disks and relic warp signatures.",
       "Пассивный доход 35 RP/мин; +35% RP за новые сканирования; завершение Эпохи III",
       "35 RP/min; +35% RP for new scans; Epoch III completed",
       350, 9, 7500, 35, 35, true, ColonyEpoch::Society},

      // =========================================================================
      // --- Эпоха IV: Высокие Технологии и Экспансия (Stages 16-20) ---
      // =========================================================================
      {"Планетарный Мегаполис & Аркологии", "Metropolis Arcology Clusters",
       "Километровые гиперструктурные башни-аркологии с гравитационными лифтами и умной сетью",
       "Kilometer-tall hyperstructural arcologies with grav-assisted transit and smart city mesh",
       "Вертикальный урбанизм будущего. Миллионы квадратных метров жилья, лабораторий и производств в едином монолите.",
       "Vertical urbanism of the future. Millions of square meters of living, lab and fabrication space inside one hull.",
       "Пассивный доход 42 RP/мин; +50% RP за новые сканирования; доступ к элитным миссиям",
       "42 RP/min; +50% RP for new scans; unlocks elite deep-space mission contracts",
       420, 10, 15000, 50, 42, true, ColonyEpoch::HighTech},

      {"Синтез Квантовой Плазмы & Казимира", "Quantum Plasma & Casimir Exotic Matter",
       "D-³He стеллараторный реактор 150 млн °C и линейный генератор отрицательной плотности энергии",
       "D-³He stellarator at 150M °C and linear Casimir negative-energy density injector",
       "Производство экзотической материи для поддержания метрики варп-пузыря. Катализ квантового вакуума.",
       "Synthesizes exotic matter required to stabilize Alcubierre metric geometry and warp ring harmonics.",
       "Пассивный доход 50 RP/мин; +50% RP; +25% скорость зарядки варпа и снижение расхода энергии",
       "50 RP/min; +50% RP; +25% warp charging speed and lower jump energy drain",
       500, 11, 22000, 50, 50, true, ColonyEpoch::HighTech},

      {"Электромагнитный Орбитальный Лифт", "Planetary Space Elevator & Tether",
       "Нанотрубочный трос на геостационарную орбиту, лазерный привод и базовый анкер",
       "Carbon nanotube cable to geostationary orbit, beamed laser power and ground anchor terminal",
       "Перемещение десятков тысяч тонн грузов в космос без расхода ракетного топлива. Идеальная логистика.",
       "Lifts tens of thousands of cargo tons without chemical propellant. Flawless surface-to-orbit logistics.",
       "Пассивный доход 60 RP/мин; +55% RP за новые сканирования; бесплатная орбитальная логистика",
       "60 RP/min; +55% RP for scans; zero-cost orbital logistics for all system craft",
       600, 12, 35000, 55, 60, true, ColonyEpoch::HighTech},

      {"Глубинная Энергетическая Сеть", "Deep Core Geothermal / Cryothermal Tap",
       "Плазменные сверхглубокие скважины к границе мантии и планетарное сверхпроводящее кольцо",
       "Deep plasma-drilled shafts to core boundary and planetary superconductor bus ring",
       "Тераватты неисчерпаемой энергии из недр планеты. Полная независимость от удаленности от Солнца.",
       "Terawatts of inexhaustible energy tapped from planetary core thermodynamics. Immune to solar variance.",
       "Пассивный доход 72 RP/мин; +60% RP за сканирования; абсолютная энергоустойчивость мира",
       "72 RP/min; +60% RP for scans; total planetary energy grid invulnerability",
       720, 13, 50000, 60, 72, true, ColonyEpoch::HighTech},

      {"Роевой Роботизированный Комплекс", "Self-Replicating Machine Foundry",
       "Автоматы фон Неймана, молекулярная наносборка и распределенный квантовый управляющий ИИ",
       "Von Neumann autonomous replicators, molecular nanofoundries and distributed synthetic AI",
       "Заводы, строящие сами себя. Экспоненциальный рост созидательной мощи цивилизации без риска для жизней.",
       "Self-constructing robotic factories. Exponential industrial expansion with zero human casualty risk.",
       "Пассивный доход 85 RP/мин; +65% RP; ускорение строительства Эпохи V в 2 раза; завершение Эпохи IV",
       "85 RP/min; +65% RP; 2x construction speed for Epoch V megaprojects; Epoch IV completed",
       850, 14, 75000, 65, 85, true, ColonyEpoch::HighTech},

      // =========================================================================
      // --- Эпоха V: Планетарная Цивилизация и Терраформирование (Stages 21-24) ---
      // =========================================================================
      {"Планетарный Атмосферный Процессор", "Global Atmosphere Processing Array",
       "Двухкилометровые башни-процессоры, каталитические конвертеры и факельные реакторы",
       "2-km atmospheric processing spires, catalytic gas fractioners and catalytic burners",
       "Глобальное управление термодинамикой и составом атмосферы. Формирование пригодного для дыхания давления.",
       "Global thermodynamic steering of planetary atmosphere. Sublimating volatiles and generating breathable air.",
       "Пассивный доход 100 RP/мин; +70% RP; визуальная трансформация атмосферного слоя планеты",
       "100 RP/min; +70% RP; visible visual transformation of planetary atmospheric haze",
       1000, 15, 120000, 70, 100, true, ColonyEpoch::Civilization},

      {"Искусственный Магнитосферный Щит", "Planetary Magnetosphere Deflector",
       "Экваториальный сверхпроводящий пояс диполей 2 Тесла и спутники отклонения в точке L1",
       "Equatorial 2-Tesla superconducting ring and L1 solar wind magnetic deflector station",
       "Полная защита планеты от корональных выбросов массы и космических лучей. Сохранение биосферы на эпохи.",
       "Total shielding from coronal mass ejections and galactic cosmic rays. Preserves atmosphere for aeons.",
       "Пассивный доход 120 RP/мин; +75% RP; усиление щитов корабля игрока в системе на +25%",
       "120 RP/min; +75% RP; player ship shields strengthened by +25% across the system",
       1200, 16, 200000, 75, 120, true, ColonyEpoch::Civilization},

      {"Межзвёздный Институт Синтеза", "Interstellar Synthesis & Relic Complex",
       "Гравитационно-волновой интерферометр 100 км, архив квантовой запутанности и суперкомпьютер",
       "100-km gravitational wave array, quantum entanglement vault and hyper-dimensional AI",
       "Прорыв за горизонт Стандартной модели. Расшифровка реликтовых сигнатур Предтеч и гравитационных аномалий М13.",
       "Trans-Standard Model physics. Decodes Precursor tachyon signals and M13 anomalous spacetime metric.",
       "Пассивный доход 150 RP/мин; +90% RP за ВСЕ сканирования; раскрытие скрытых гипотез в Codex",
       "150 RP/min; +90% RP for all scans; reveals hidden relic hypotheses in Science Codex",
       1500, 18, 350000, 90, 150, true, ColonyEpoch::Civilization},

      {"Столица Мира & Маяк Субпространства", "Civilization Capital & Subspace Nexus",
       "Сияющий метрополис, Сенат Планеты и монолитный гиперпространственный маяк с плазменным фокусом",
       "Luminous planetary metropolis, Planetary Senate and tachyon subspace resonance nexus",
       "Триумф разума. Планета становится суверенной цивилизацией типа I+ по Кардашеву, связанной со звездами.",
       "Triumph of intelligence. World ascends to Kardashev Type I+ civilization, linked via tachyon resonance to deep stars.",
       "Пассивный доход 200 RP/мин; +120% RP; достижение 'Планетарный Триумф' и координаты ядра М13!",
       "200 RP/min; +120% RP; 'Planetary Triumph' achievement and M13 galactic core warp vector unlocked!",
       2000, 20, 600000, 120, 200, true, ColonyEpoch::Civilization}}};
  return stages;
}

int colonyStage(const ColonyStages& stages, const std::string& id) {
  const auto it = stages.find(id);
  return it == stages.end() ? 0 : std::clamp(it->second, 1, kColonyStageCount);
}

ColonyEpoch colonyEpoch(int stage) {
  if (stage <= 0) return ColonyEpoch::Outpost;
  const int idx = std::clamp(stage, 1, kColonyStageCount) - 1;
  return colonyStages()[idx].epoch;
}

const char* colonyEpochNameRu(ColonyEpoch epoch) {
  switch (epoch) {
    case ColonyEpoch::Outpost: return "I: Аванпост";
    case ColonyEpoch::Industry: return "II: Индустрия";
    case ColonyEpoch::Society: return "III: Социум";
    case ColonyEpoch::HighTech: return "IV: Высокие Технологии";
    case ColonyEpoch::Civilization: return "V: Планетарная Цивилизация";
  }
  return "";
}

const char* colonyEpochNameEn(ColonyEpoch epoch) {
  switch (epoch) {
    case ColonyEpoch::Outpost: return "I: Outpost";
    case ColonyEpoch::Industry: return "II: Industry";
    case ColonyEpoch::Society: return "III: Society";
    case ColonyEpoch::HighTech: return "IV: High-Tech";
    case ColonyEpoch::Civilization: return "V: Planetary Civilization";
  }
  return "";
}

void normalizeColonies(std::vector<std::string>& founded, ColonyStages& stages) {
  std::vector<std::string> valid;
  for (const auto& id : founded) {
    if (!colonyWorld(id) || std::find(valid.begin(), valid.end(), id) != valid.end()) continue;
    valid.push_back(id);
  }
  ColonyStages normalized;
  for (const auto& id : valid) normalized[id] = std::max(1, colonyStage(stages, id));
  founded = std::move(valid);
  stages = std::move(normalized);
}

const std::array<std::array<ColonyStructure, kColonyPlanSlots>, kColonyStageCount>&
colonyStagePlan() {
  // What actually appears on the surface when each stage completes. "power" is
  // resolved per world (fission / hydro / thermal), so the same stage reads
  // differently on the Moon and on Europa.
  static const ColonyStructure rows[kColonyStageCount][kColonyPlanSlots] = {
      {{"pad", 1}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}},                    // 1 touchdown
      {{"dome", 1}, {"", 0}, {"", 0}, {"", 0}, {"", 0}, {"", 0}},                   // 2 relay mast
      {{kColonyPowerAsset, 1}, {"diesel", 1}, {"", 0}, {"", 0}, {"", 0}, {"", 0}},  // 3 power grid
      {{"dome", 1}, {"pipeline", 1}, {"water", 1}, {"", 0}, {"", 0}, {"", 0}},      // 4 water loop
      {{"dome", 2}, {"houses", 1}, {"", 0}, {"", 0}, {"", 0}, {"", 0}},             // 5 bio-domes I
      {{"dome", 2}, {"houses", 1}, {"solar", 1}, {"greenhouse", 1}, {"", 0}, {"", 0}},  // 6 agro complex
      {{"factory", 1}, {"warehouse", 1}, {"", 0}, {"", 0}, {"", 0}, {"", 0}},       // 7 excavation yard
      {{"factory", 1}, {kColonyPowerAsset, 1}, {"powerline", 1}, {"", 0}, {"", 0}, {"", 0}},  // 8 arc smelter
      {{"factory", 1}, {"warehouse", 1}, {"houses", 1}, {"", 0}, {"", 0}, {"", 0}}, // 9 module fab plant
      {{"pad", 1}, {"factory", 1}, {"warehouse", 1}, {"pipeline", 1}, {"", 0}, {"", 0}},      // 10 maglev/logistics
      // Stage 11 is named medical centre, so it now builds one: a hospital for
      // inpatient care and a clinic that sits inside the housing blocks.
      {{"hospital", 1}, {"clinic", 1}, {"dome", 2}, {"", 0}, {"", 0}, {"", 0}},     // 11 medical centre
      {{"datacenter", 1}, {"police", 1}, {"fire", 1}, {"dome", 1}, {"", 0}, {"", 0}},// 12 laboratories
      // Township is where a settlement starts looking lived-in: civic hall,
      // retail and a public park instead of a fourth dome.
      {{"houses", 2}, {"apartments", 1}, {"park", 1}, {"market", 1}, {"townhall", 1}, {"", 0}},  // 13 township
      {{"pad", 2}, {"warehouse", 1}, {"powerline", 1}, {"", 0}, {"", 0}, {"", 0}},  // 14 spaceport
      {{"datacenter", 1}, {"school", 1}, {"apartments", 1}, {"solar", 1}, {"", 0}, {"", 0}},    // 15 research campus
      {{"dome", 3}, {"apartments", 2}, {"townhall", 1}, {"wind", 1}, {"", 0}, {"", 0}},         // 16 metropolis arcology
      {{"datacenter", 1}, {kColonyPowerAsset, 2}, {"inverter", 1}, {"", 0}, {"", 0}, {"", 0}},// 17 quantum plasma
      {{"pad", 1}, {"apartments", 1}, {"military", 1}, {"", 0}, {"", 0}, {"", 0}},  // 18 space elevator
      {{kColonyPowerAsset, 2}, {"wind", 1}, {"inverter", 1}, {"powerline", 1}, {"", 0}, {"", 0}},  // 19 deep energy grid
      {{"factory", 2}, {"warehouse", 1}, {"diesel", 1}, {"greenhouse", 1}, {"", 0}, {"", 0}}, // 20 replicator foundry
      {{"factory", 2}, {"dome", 2}, {"parliament", 1}, {"", 0}, {"", 0}, {"", 0}},  // 21 atmosphere processors
      {{kColonyPowerAsset, 2}, {"military", 1}, {"inverter", 1}, {"", 0}, {"", 0}, {"", 0}},    // 22 magnetosphere shield
      {{"datacenter", 2}, {"dome", 1}, {"parliament", 1}, {"apartments", 1}, {"", 0}, {"", 0}},    // 23 synthesis institute
      {{"dome", 2}, {"hospital", 1}, {"parliament", 1}, {"townhall", 1}, {"military", 1}, {"park", 1}},  // 24 civilization capital
  };
  // Nested std::array brace initializers are brittle on MSVC, so the table is
  // copied into the typed array once on first use.
  static const std::array<std::array<ColonyStructure, kColonyPlanSlots>, kColonyStageCount> plan =
      [] {
        std::array<std::array<ColonyStructure, kColonyPlanSlots>, kColonyStageCount> p{};
        for (int i = 0; i < kColonyStageCount; ++i)
          for (int s = 0; s < kColonyPlanSlots; ++s) p[i][s] = rows[i][s];
        return p;
      }();
  return plan;
}

const char* colonyEnergyAsset(const std::string& worldId) {
  if (worldId == "moon" || worldId == "mars") return "nuclear";
  if (worldId == "mercury" || worldId == "titan") return "thermal";
  return "hydro";  // europa, ganymede, callisto, hercules-b: ice/ocean worlds
}

std::vector<std::pair<std::string, int>> colonyBuildPlan(const std::string& worldId,
                                                        int stage) {
  std::vector<std::pair<std::string, int>> out;
  const auto merge = [&](const std::string& asset, int count) {
    if (asset.empty() || count <= 0) return;
    for (auto& entry : out) {
      if (entry.first == asset) {
        entry.second += count;
        return;
      }
    }
    out.emplace_back(asset, count);
  };
  const auto& plan = colonyStagePlan();
  const int top = std::clamp(stage, 0, kColonyStageCount);
  for (int i = 0; i < top; ++i) {
    for (int s = 0; s < kColonyPlanSlots; ++s) {
      const ColonyStructure& item = plan[i][s];
      const std::string asset =
          (std::string(item.asset) == kColonyPowerAsset) ? colonyEnergyAsset(worldId)
                                                         : std::string(item.asset);
      merge(asset, item.count);
    }
  }
  return out;
}

const float* colonyAnchorNormal(const std::string& worldId) {
  static const float kDefault[3] = {0.0f, 0.7071f, 0.7071f};
  struct Anchor { const char* id; float n[3]; };
  static const Anchor anchors[] = {
      {"mars", {0.55f, 0.45f, 0.70f}},
      {"moon", {0.15f, 0.85f, 0.50f}},
      {"europa", {-0.60f, 0.35f, 0.72f}},
      {"titan", {0.40f, 0.20f, -0.89f}},
      {"mercury", {0.70f, 0.65f, 0.28f}},
      {"ganymede", {-0.50f, 0.70f, -0.51f}},
      {"callisto", {0.30f, -0.60f, 0.74f}},
      {"hercules-b", {0.0f, 0.7071f, 0.7071f}},
  };
  for (const auto& a : anchors) {
    if (worldId == a.id) {
      const float length = std::sqrt(a.n[0] * a.n[0] + a.n[1] * a.n[1] + a.n[2] * a.n[2]);
      static thread_local float out[3];
      // #ED: деление на length без защиты — дегенерированная запись таблицы
      // (все сейчас валидны, но улипли — NaN-у, если кто-то добавит нулевой
      // вектор). Fallback — вертикальный направление.
      if (length < 1e-12f) {
        out[0] = 0.0f; out[1] = 1.0f; out[2] = 0.0f;
        return out;
      }
      out[0] = a.n[0] / length;
      out[1] = a.n[1] / length;
      out[2] = a.n[2] / length;
      return out;
    }
  }
  return kDefault;
}

ColonySurfaceFrame colonySurfaceFrame(const std::string& worldId) {
  ColonySurfaceFrame f{};
  const float* n = colonyAnchorNormal(worldId);
  const float ax = n[0], ay = n[1], az = n[2];

  // Same reference-up choice the renderer has always used: a settlement near a
  // pole has no east-west direction, so tilt the tangent frame onto X instead.
  const bool onEquator = std::fabs(ay) < 0.90f;
  const float rx = onEquator ? 0.0f : 1.0f, ry = onEquator ? 1.0f : 0.0f, rz = 0.0f;
  float ux = ay * rz - az * ry;
  float uy = az * rx - ax * rz;
  float uz = ax * ry - ay * rx;
  float ul = std::sqrt(ux * ux + uy * uy + uz * uz);
  // #ED: ul может быть ноль (таблица развита: все анкоры ненулевые, но
  // гуард — чтобы не делить на 0 при дегенерированном входе).
  if (ul < 1e-6f) { ux = 1.0f; uy = 0.0f; uz = 0.0f; } else {
    ux /= ul; uy /= ul; uz /= ul;
  }

  float vx = ay * uz - az * uy;
  float vy = az * ux - ax * uz;
  float vz = ax * uy - ay * ux;
  float vl = std::sqrt(vx * vx + vy * vy + vz * vz);
  // #ED: v — кросс с недеформированным r, для полюсных входов может быть 0;
  // гуард перед делением ниже и по тангенсам.
  if (vl < 1e-6f) {
    vx = 0.0f; vy = 0.0f; vz = -1.0f;
    vl = 1.0f;
  }

  f.anchor[0] = ax; f.anchor[1] = ay; f.anchor[2] = az;
  f.tangentU[0] = ux; f.tangentU[1] = uy; f.tangentU[2] = uz;
  f.tangentV[0] = vx / vl; f.tangentV[1] = vy / vl; f.tangentV[2] = vz / vl;
  return f;
}

const std::vector<ColonyMilestone>& colonyTerraformMilestones() {
  // Ordered by stage. Rewards are one-off RP grants, paid when the player
  // crosses the milestone by building a stage.
  static const std::vector<ColonyMilestone> milestones = {
      {4, "Извлечение льда и воды", "Ice and water extraction",
       "Шнековые скважины дают воду и кислород для электролиза",
       "Auger wells supply water and oxygen for electrolysis", 0.05f, 0.0f, 60},
      {8, "Первичные элементы в грунте", "Primary elements released",
       "Детектируются O2, CO2, CH4 и летучие сульфиды",
       "O2, CO2, CH4 and volatile sulfides detected", 0.15f, 0.05f, 80},
      {11, "Химическая дифференциация", "Chemical differentiation",
       "Выветривание обогащает реголит Fe, Ni, Si, Mg и фосфатами",
       "Weathering enriches regolith with Fe, Ni, Si, Mg and phosphates", 0.25f, 0.10f, 100},
      {14, "Плотная атмосфера", "Dense atmosphere",
       "0.3 атм, устойчивый парниковый цикл и перенос влаги",
       "0.3 atm, stable greenhouse cycle and moisture transport", 0.45f, 0.15f, 140},
      {16, "Абиогенная органика", "Abiogenic organics",
       "Аминокислоты и липиды в тёплых мелководьях",
       "Amino acids and lipids in shallow warm seas", 0.55f, 0.30f, 170},
      {18, "Цианобактерии и фотосинтез", "Cyanobacteria and photosynthesis",
       "Строматолитовые маты начинают насыщать атмосферу кислородом",
       "Stromatolite mats begin oxygenating the air", 0.65f, 0.50f, 200},
      {20, "Лишайники и первичная почва", "Lichens and proto-soil",
       "Появляется гумус, круговорот азота и углерода",
       "Humus, nitrogen and carbon cycles appear", 0.75f, 0.65f, 240},
      {22, "Высшие растения", "Vascular plants",
       "Леса, испарение и полноценный гидрологический цикл",
       "Forests, transpiration and a full water cycle", 0.85f, 0.80f, 300},
      {23, "Животные и пищевые сети", "Animals and food webs",
       "Опылители, травоядные и хищники замыкают трофические цепи",
       "Pollinators, herbivores and predators close the trophic chains", 0.90f, 0.90f, 360},
      {24, "Самподдерживающаяся биосфера", "Self-sustaining biosphere",
       "Мир больше не требует terraforming-поддержки",
       "The world no longer needs terraforming support", 1.0f, 1.0f, 450}};
  return milestones;
}

void colonyTerraformState(int stage, float& atmosphere, float& biosphere) {
  atmosphere = 0.0f;
  biosphere = 0.0f;
  for (const auto& m : colonyTerraformMilestones()) {
    if (stage < m.stage) break;
    atmosphere = std::max(atmosphere, m.atmosphere);
    biosphere = std::max(biosphere, m.biosphere);
  }
}

int colonyMilestonesReached(int stage) {
  int reached = 0;
  for (const auto& m : colonyTerraformMilestones()) {
    if (stage >= m.stage) ++reached;
  }
  return reached;
}

int colonyMilestoneReward(int previousStage, int newStage) {
  int reward = 0;
  for (const auto& m : colonyTerraformMilestones()) {
    if (m.stage > previousStage && m.stage <= newStage) reward += m.rewardRP;
  }
  return reward;
}

int colonyStructureCount(const std::string& worldId, int stage) {
  int total = 0;
  for (const auto& entry : colonyBuildPlan(worldId, stage)) total += entry.second;
  return total;
}

int colonyResearchBonus(const ColonyStages& stages) {
  int bonus = 0;
  for (const auto& entry : stages) {
    const auto* w = colonyWorld(entry.first);
    if (!w) continue;
    const int st = colonyStage(stages, entry.first);
    if (st <= 0) continue;
    int baseBonus = colonyStages()[st - 1].researchBonusPercent;
    if (w->perk.scienceMultiplier > 0.0f) {
      baseBonus = static_cast<int>(baseBonus * (1.0f + w->perk.scienceMultiplier));
    }
    bonus = std::max(bonus, baseBonus);
  }
  return bonus;
}

float colonyWarpRechargeBoost(const ColonyStages& stages) {
  float best = 0.0f;
  for (const auto& entry : stages) {
    if (colonyStage(stages, entry.first) <= 0) continue;
    const auto* w = colonyWorld(entry.first);
    if (w) best = std::max(best, w->perk.warpRechargeBonus);
  }
  return best;
}

float colonyShieldBoost(const ColonyStages& stages) {
  float best = 0.0f;
  for (const auto& entry : stages) {
    if (colonyStage(stages, entry.first) <= 0) continue;
    const auto* w = colonyWorld(entry.first);
    if (w) best = std::max(best, w->perk.shieldBonus);
  }
  return best;
}

int colonyMaxStage(const ColonyStages& stages) {
  int best = 0;
  for (const auto& entry : stages) {
    best = std::max(best, colonyStage(stages, entry.first));
  }
  return best;
}

bool colonyUnlocksEliteContracts(const ColonyStages& stages) {
  return colonyMaxStage(stages) >= kColonyEliteContractStage;
}

bool colonyRevealsHiddenHypotheses(const ColonyStages& stages) {
  return colonyMaxStage(stages) >= kColonyHiddenHypothesisStage;
}

bool colonyUnlocksGalacticVector(const ColonyStages& stages) {
  return colonyMaxStage(stages) >= kColonyGalacticVectorStage;
}

bool colonyGalacticVectorRoute(const ColonyStages& stages, const std::string& stationId) {
  return colonyUnlocksGalacticVector(stages) && stationId == kColonyGalacticVectorStation;
}

int colonyTotalPopulation(const ColonyStages& stages) {
  int total = 0;
  for (const auto& entry : stages) {
    const int st = colonyStage(stages, entry.first);
    if (st > 0) {
      total += colonyStages()[st - 1].population;
    }
  }
  return total;
}

int colonyPassiveRatePerMin(const ColonyStages& stages) {
  int totalRate = 0;
  for (const auto& entry : stages) {
    const auto* w = colonyWorld(entry.first);
    if (!w) continue;
    const int st = colonyStage(stages, entry.first);
    if (st <= 0) continue;
    int rate = colonyStages()[st - 1].passiveRpPerMin;
    if (w->perk.passiveRpRate > 0.0f) {
      rate = static_cast<int>(rate * (1.0f + w->perk.passiveRpRate));
    }
    totalRate += rate;
  }
  return totalRate;
}

int colonyUpgradeCost(int currentStage) {
  if (currentStage < 1 || currentStage >= kColonyStageCount) return 0;
  const int discount = currentStage >= 9 ? 10 : 0;
  return colonyStages()[currentStage].costRP * (100 - discount) / 100;
}

int colonyUpgradeCost(const ColonyStages& stages, const std::string& worldId, int currentStage) {
  (void)stages;
  if (currentStage < 1 || currentStage >= kColonyStageCount) return 0;
  int baseCost = colonyStages()[currentStage].costRP;
  int discountPercent = (currentStage >= 9) ? 10 : 0;
  const auto* w = colonyWorld(worldId);
  if (w && w->perk.costDiscount > 0.0f) {
    discountPercent += static_cast<int>(w->perk.costDiscount * 100.0f);
  }
  discountPercent = std::min(discountPercent, 50); // cap discount at 50%
  return baseCost * (100 - discountPercent) / 100;
}

bool colonyCanResupply(const ColonyStages& stages, const std::string& worldId) {
  const int st = colonyStage(stages, worldId);
  if (st <= 0) return false;
  return colonyStages()[st - 1].allowsResupply;
}

ColonyUpgradeResult advanceColony(ColonyStages& stages, const std::string& id,
                                 int scannedCount, int& researchPoints) {
  if (!colonyWorld(id)) return ColonyUpgradeResult::UnknownWorld;
  const int stage = colonyStage(stages, id);
  if (stage == 0) return ColonyUpgradeResult::NotFounded;
  if (stage == kColonyStageCount) return ColonyUpgradeResult::Complete;
  if (scannedCount < colonyStages()[stage].requiredScans) return ColonyUpgradeResult::NeedScans;
  const int cost = colonyUpgradeCost(stages, id, stage);
  if (researchPoints < cost) return ColonyUpgradeResult::NeedPoints;
  researchPoints -= cost;
  stages[id] = stage + 1;
  return ColonyUpgradeResult::Success;
}

}  // namespace astro
