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
    n.textRu = "Сквозь узкие мансардные окна чердака в Латинском квартале доносился шум ливня. [[sol|Солнце]] тогда казалось вечным, неизменным светилом, согревавшим Париж миллионы лет. Мы верили, что [[z_casimir_metric|метрика Казимира]] вернёт нам тепло.";
    n.textEn = "Through the narrow attic dormers of the Latin Quarter, rain drummed on zinc roofs. Back then, [[sol|the Sun]] felt eternal, warming Paris across epochs. We believed [[z_casimir_metric|Casimir metrics]] would bring the warmth back.";
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
    n.textRu = "Эффект Казимира в субмикронных зазорах доказывает: локальная плотность энергии вакуума может быть отрицательной. Соединив его с [[z_solar_injection|каскадной инжекцией]], гравитационный коллапс обратится вспять, возвращая к жизни [[z_paris_rain|память о дожде]].";
    n.textEn = "The Casimir effect in sub-micron gaps proves vacuum energy density can be strictly negative. Combining it with [[z_solar_injection|cascade injection]], gravitational collapse inverts, keeping alive the [[z_paris_rain|memory of rain]].";
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
    n.textRu = "Нагнетание ультра-релятивистских протонов по геодезическим орбитам прямо в термоядерное ядро [[sol|Солнца]] сжигает шлаковый углерод. Вместе с [[z_casimir_metric|метрикой Казимира]] это открывает путь к [[z_master_solar_dawn|Рассвету Первого Огня]].";
    n.textEn = "Injecting ultra-relativistic protons along geodesic trajectories directly into [[sol|the Sun's]] core purges carbon slag. Together with [[z_casimir_metric|Casimir metrics]], it opens the path toward [[z_master_solar_dawn|Dawn of the First Flame]].";
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
    n.textRu = "Пятислойная регенерирующая губка из полимерного графена, используемая пилотами Пятой Вселенной во время [[z_universe5_collapse|энтропийного коллапса]]. Способна выдержать приливные силы черной дыры [[sgra|Sgr A*]].";
    n.textEn = "A five-layer regenerating graphene polymer sponge engineered during the [[z_universe5_collapse|collapse of Universe-05]]. Capable of withstanding [[sgra|Sgr A*]] black hole tidal forces.";
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
    n.textRu = "Записи бортового самописца Арсения. В их реальности [[sol|Солнце]] потухло на двести лет раньше. Выжили лишь те, кто использовал [[z_stasis_dampener|амортизаторы стазиса]] и изучил [[z_sgra_tidal|приливные щиты Sgr A*]].";
    n.textEn = "Flight recorder logs from Arseny's craft. In his reality, [[sol|the Sun]] froze two centuries earlier. Only those survived who deployed [[z_stasis_dampener|stasis dampeners]] and studied [[z_sgra_tidal|Sgr A* tidal shields]].";
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
    n.textRu = "Расчёт приливных сил сверхмассивной чёрной дыры [[sgra|Sgr A*]]. Гравитационные градиенты, пропущенные сквозь [[z_stasis_dampener|капсулы стазиса]], образуют монолитный [[z_master_stasis_bastion|Бастион Сингулярного Покоя]].";
    n.textEn = "Calculations of supermassive black hole [[sgra|Sgr A*]] tidal forces. Routing gradients through [[z_stasis_dampener|stasis hulls]] establishes the [[z_master_stasis_bastion|Bastion of Singular Stasis]].";
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
    n.textRu = "Мия сжимала в руках семена земной флоры. 'Мы не позволим человечеству остаться лишь эхом'. Мы отыщем для них пристанище в скоплении [[m13|M13]] и воздвигнем [[z_nomad_citadel|Цитадель Номад]].";
    n.textEn = "Mia held tightly a canister with heirloom Earth seeds. 'We will not let humanity remain just an echo'. We will build a sanctuary at [[m13|cluster M13]] and raise the [[z_nomad_citadel|Nomad Citadel]].";
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
    n.textRu = "В поясе Койпера мы настигли зонд [[voyager1|«Вояджер-1»]]. На золотой пластинке записаны голоса Земли 1977 года — маяк веры, ведущий нас сквозь холод пустоты к [[z_m13_biosphere|Новому Эдему]].";
    n.textEn = "In the Kuiper belt, we caught up with [[voyager1|Voyager 1]]. The golden record preserves the voices of 1977 Earth — a beacon of faith guiding us toward [[z_m13_biosphere|New Eden]].";
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
    n.textRu = "Спектральный анализ экзопланеты в шаровом скоплении [[m13|M13]]. Хлорофилл и кислород подтверждают: клятва Мии из [[z_mia_promise|стартового ангара]] будет исполнена, а [[z_nomad_citadel|Цитадель Номад]] обретет твердь.";
    n.textEn = "Spectral analysis of the exoplanet in globular cluster [[m13|M13]]. Chlorophyll and oxygen prove Mia's vow in [[z_mia_promise|the hangar]] will be fulfilled as [[z_nomad_citadel|Nomad Citadel]] finds bedrock.";
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
    n.textRu = "Модульный ковчег колонистов, пускающий корни в базальты планеты [[m13|M13-Prime]]. Совместно с биосферой из [[z_m13_biosphere|спектральных данных]] он образует [[z_master_eden_cradle|Колыбель Бесконечного Дома]].";
    n.textEn = "A modular ark sinking pylons into [[m13|M13-Prime]] basalts. Combined with biosphere telemetry from [[z_m13_biosphere|M13]], it manifests the [[z_master_eden_cradle|Cradle of the Infinite Home]].";
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
    n.textRu = "Сигнал тахионного передатчика из параллельной ветви времени. Егорка из шестой итерации оставил формулу, связывающую [[z_phase_shard|осколок темпоральной фазы]] и горизонт чёрной дыры [[sgra|Sgr A*]].";
    n.textEn = "A tachyon packet from a parallel temporal branch. Egorka from iteration six transmitted a formula uniting the [[z_phase_shard|temporal phase shard]] with the [[sgra|Sgr A*]] horizon.";
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
    n.textRu = "Векторы причинности вблизи сингулярности сворачиваются в петлю Мёбиуса. Опираясь на [[z_sixth_resonance|гармонику Шестого]], мы выстраиваем [[z_loop_transcend|сингулярный мост]].";
    n.textEn = "Causal vectors near the singularity curl into a Möbius strip. Anchored by the [[z_sixth_resonance|Sixth Egorka's Harmonic]], we assemble the [[z_loop_transcend|singular bridge]].";
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
    n.textRu = "Синтез данных шести итераций смыкает топологию пространства-времени. Мост Эйнштейна-Розена ведёт прямо в [[z_master_omni_nexus|Омни-Нексус Мультивселенной]].";
    n.textEn = "Synthesizing data from six iterations unifies spacetime topology. The Einstein-Rosen bridge ascends straight into the [[z_master_omni_nexus|Omni-Nexus of the Multiverse]].";
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

  // -------------------------------------------------------------
  // MASTER BREAKTHROUGH NODES (Эврика · Новые Открытия)
  // -------------------------------------------------------------
  {
    ZettelNote n;
    n.id = "z_master_solar_dawn";
    n.cluster = kClusterSolar;
    n.categoryRu = "Эврика";
    n.categoryEn = "Eureka";
    n.titleRu = "Рассвет Первого Огня";
    n.titleEn = "Dawn of the First Flame";
    n.textRu = "Объединение [[z_casimir_metric|метрики Казимира]] с [[z_solar_injection|протонной инжекцией]] обратило вспять коллапс [[sol|Солнца]]. [[z_paris_rain|Парижский дождь]] больше не эхо угасшего мира — это обещание нового рассвета.";
    n.textEn = "Synthesizing [[z_casimir_metric|Casimir metrics]] with [[z_solar_injection|proton cascades]] reverses [[sol|the Sun's]] collapse. [[z_paris_rain|Rain over Sorbonne]] is no longer an elegy — it is the dawn of a new noon.";
    n.quoteRu = "Мы зажгли огонь, который согреет миллиарды жизней. Спи спокойно, Земля: рассвет настал.";
    n.quoteEn = "We rekindled a hearth that will warm billions of lives. Sleep peacefully, Earth: the dawn is here.";
    n.speakerRu = "Д-р Елена Ростова";
    n.speakerEn = "Dr. Elena Rostova";
    n.posX = -135.0f;
    n.posY = -105.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    n.isMasterNode = true;
    n.parentIds = {"z_paris_rain", "z_casimir_metric", "z_solar_injection"};
    n.depthLevel = 0;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_master_stasis_bastion";
    n.cluster = kClusterStasis;
    n.categoryRu = "Эврика";
    n.categoryEn = "Eureka";
    n.titleRu = "Бастион Сингулярного Покоя";
    n.titleEn = "Bastion of Singular Stasis";
    n.textRu = "Синхронизация [[z_stasis_dampener|амортизаторов криосна]] с [[z_sgra_tidal|приливным щитом Sgr A*]] создала несокрушимый бастион. Уроки [[z_universe5_collapse|гибели Вселенной-05]] спасли нашу реальность от энтропии.";
    n.textEn = "Synchronizing [[z_stasis_dampener|cryostatis dampeners]] with [[z_sgra_tidal|Sgr A* tidal shields]] formed an impervious fortress. The tragic lessons of [[z_universe5_collapse|Universe-05]] saved our reality from entropy.";
    n.quoteRu = "Палач Ложных Надежд стал хранителем истинного рассвета. Теперь ни одна сингулярность нас не сломит.";
    n.quoteEn = "The Executioner of False Hopes has become guardian of the true dawn. No singularity shall break us now.";
    n.speakerRu = "Арсений";
    n.speakerEn = "Arseny";
    n.posX = -120.0f;
    n.posY = 120.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    n.isMasterNode = true;
    n.parentIds = {"z_stasis_dampener", "z_universe5_collapse", "z_sgra_tidal"};
    n.depthLevel = 0;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_master_eden_cradle";
    n.cluster = kClusterEden;
    n.categoryRu = "Эврика";
    n.categoryEn = "Eureka";
    n.titleRu = "Колыбель Бесконечного Дома";
    n.titleEn = "Cradle of the Infinite Home";
    n.textRu = "Верность [[z_mia_promise|клятве Мии]], биосфера скопления [[m13|M13]] и шпили [[z_nomad_citadel|Цитадели Номад]] слились в Колыбель Нового Эдема. Наше семя пустило корни среди звёзд.";
    n.textEn = "Faith to [[z_mia_promise|Mia's vow]], the [[m13|M13]] biosphere, and basalt spires of [[z_nomad_citadel|Nomad Citadel]] merged into New Eden's Cradle. Humanity has taken root among the stars.";
    n.quoteRu = "Посмотри на эти зелёные небеса, Егорка! Мы нашли наш вечный дом среди звёзд!";
    n.quoteEn = "Look at those emerald skies, Egorka! We found our eternal home among the stars!";
    n.speakerRu = "Мия Сато";
    n.speakerEn = "Mia Sato";
    n.posX = 140.0f;
    n.posY = -85.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    n.isMasterNode = true;
    n.parentIds = {"z_mia_promise", "z_m13_biosphere", "z_nomad_citadel"};
    n.depthLevel = 0;
    addSeed(n);
  }
  {
    ZettelNote n;
    n.id = "z_master_omni_nexus";
    n.cluster = kClusterMultiverse;
    n.categoryRu = "Эврика";
    n.categoryEn = "Eureka";
    n.titleRu = "Омни-Нексус Мультивселенной";
    n.titleEn = "Omni-Nexus of the Multiverse";
    n.textRu = "Объединение [[z_sixth_resonance|гармоники Шестого]], [[z_phase_shard|темпоральной фазы]] и [[z_loop_transcend|межпространственного моста]] разорвало петлю времени, открыв [[expedition|бесконечное странствие]].";
    n.textEn = "Synthesizing the [[z_sixth_resonance|Sixth's harmonic]], [[z_phase_shard|temporal phase]], and [[z_loop_transcend|singular bridge]] shattered the loop, opening an [[expedition|infinite voyage]].";
    n.quoteRu = "Шесть итераций искали выход. Седьмая сделала космос бесконечным.";
    n.quoteEn = "Six iterations searched for an exit. The seventh turned infinity into our highway.";
    n.speakerRu = "Егорка-6";
    n.speakerEn = "Egorka-6";
    n.posX = 125.0f;
    n.posY = 105.0f;
    n.seedDiscovered = false;
    n.synthesized = false;
    n.isMasterNode = true;
    n.parentIds = {"z_sixth_resonance", "z_phase_shard", "z_loop_transcend"};
    n.depthLevel = 0;
    addSeed(n);
  }

  // Recipes
  m_recipes.clear();
  {
    ZettelSynthesisRecipe r;
    r.id = "recipe_solar_dawn";
    r.cluster = kClusterSolar;
    r.masterNoteId = "z_master_solar_dawn";
    r.requiredNoteIds = {"z_paris_rain", "z_casimir_metric", "z_solar_injection"};
    r.titleRu = "Синтез: Рассвет Первого Огня";
    r.titleEn = "Synthesis: Dawn of the First Flame";
    r.descriptionRu = "Объединение физики Казимира, воспоминаний о Париже и релятивистской инжекции.";
    r.descriptionEn = "Uniting Casimir physics, Paris memories, and relativistic injection.";
    r.synthesized = false;
    m_recipes.push_back(r);
  }
  {
    ZettelSynthesisRecipe r;
    r.id = "recipe_stasis_bastion";
    r.cluster = kClusterStasis;
    r.masterNoteId = "z_master_stasis_bastion";
    r.requiredNoteIds = {"z_stasis_dampener", "z_universe5_collapse", "z_sgra_tidal"};
    r.titleRu = "Синтез: Бастион Сингулярного Покоя";
    r.titleEn = "Synthesis: Bastion of Singular Stasis";
    r.descriptionRu = "Объединение графенового криостазиса, хроник Вселенной-05 и приливных градиентов Sgr A*.";
    r.descriptionEn = "Uniting graphene cryostasis, Universe-05 chronicles, and Sgr A* tidal gradients.";
    r.synthesized = false;
    m_recipes.push_back(r);
  }
  {
    ZettelSynthesisRecipe r;
    r.id = "recipe_eden_cradle";
    r.cluster = kClusterEden;
    r.masterNoteId = "z_master_eden_cradle";
    r.requiredNoteIds = {"z_mia_promise", "z_m13_biosphere", "z_nomad_citadel"};
    r.titleRu = "Синтез: Колыбель Бесконечного Дома";
    r.titleEn = "Synthesis: Cradle of the Infinite Home";
    r.descriptionRu = "Объединение клятвы Мии, биосферного отклика M13 и Цитадели Номад.";
    r.descriptionEn = "Uniting Mia's vow, M13 biosphere resonance, and Nomad Citadel.";
    r.synthesized = false;
    m_recipes.push_back(r);
  }
  {
    ZettelSynthesisRecipe r;
    r.id = "recipe_omni_nexus";
    r.cluster = kClusterMultiverse;
    r.masterNoteId = "z_master_omni_nexus";
    r.requiredNoteIds = {"z_sixth_resonance", "z_phase_shard", "z_loop_transcend"};
    r.titleRu = "Синтез: Омни-Нексус Мультивселенной";
    r.titleEn = "Synthesis: Omni-Nexus of the Multiverse";
    r.descriptionRu = "Объединение гармоники Шестого Егорки, фазового осколка и моста Эйнштейна-Розена.";
    r.descriptionEn = "Uniting the Sixth Egorka's harmonic, phase shard, and Einstein-Rosen bridge.";
    r.synthesized = false;
    m_recipes.push_back(r);
  }
}

const ZettelSynthesisRecipe* ZettelGraph::getRecipe(const std::string& id) const {
  for (const auto& r : m_recipes) {
    if (r.id == id) return &r;
  }
  return nullptr;
}

bool ZettelGraph::canSynthesizeRecipe(const std::string& id) const {
  const ZettelSynthesisRecipe* r = getRecipe(id);
  if (!r || r->synthesized) return false;
  // All required notes must be discovered and synthesized
  for (const auto& nid : r->requiredNoteIds) {
    const ZettelNote* n = getNote(nid);
    if (!n || !n->seedDiscovered || !n->synthesized) return false;
  }
  // All required notes must be connected to each other
  int innerLinks = 0;
  for (size_t i = 0; i < r->requiredNoteIds.size(); ++i) {
    for (size_t j = i + 1; j < r->requiredNoteIds.size(); ++j) {
      if (areLinked(r->requiredNoteIds[i], r->requiredNoteIds[j])) {
        innerLinks++;
      }
    }
  }
  return (innerLinks >= static_cast<int>(r->requiredNoteIds.size()) - 1);
}

bool ZettelGraph::triggerSynthesisRecipe(const std::string& id) {
  ZettelSynthesisRecipe* r = nullptr;
  for (auto& rec : m_recipes) {
    if (rec.id == id) {
      r = &rec;
      break;
    }
  }
  if (!r || r->synthesized) return false;

  r->synthesized = true;
  ZettelNote* master = getNote(r->masterNoteId);
  if (master) {
    master->seedDiscovered = true;
    master->synthesized = true;
    master->depthLevel = 0; // foreground

    // Demote parent notes to background layer and establish radiant feeder links
    for (const auto& pId : r->requiredNoteIds) {
      ZettelNote* parent = getNote(pId);
      if (parent) {
        parent->depthLevel = 1; // background layer
        if (std::find(master->links.begin(), master->links.end(), pId) == master->links.end()) {
          master->links.push_back(pId);
        }
        if (std::find(parent->links.begin(), parent->links.end(), master->id) == parent->links.end()) {
          parent->links.push_back(master->id);
        }
      }
    }
  }
  return true;
}

const ZettelSynthesisRecipe* ZettelGraph::checkPendingSynthesis() const {
  for (const auto& r : m_recipes) {
    if (!r.synthesized && canSynthesizeRecipe(r.id)) {
      return &r;
    }
  }
  return nullptr;
}

const ZettelSynthesisRecipe* ZettelGraph::findMatchingRecipe(const std::string& noteA, const std::string& noteB) const {
  for (const auto& r : m_recipes) {
    if (r.synthesized) continue;
    bool hasA = (std::find(r.requiredNoteIds.begin(), r.requiredNoteIds.end(), noteA) != r.requiredNoteIds.end());
    bool hasB = (std::find(r.requiredNoteIds.begin(), r.requiredNoteIds.end(), noteB) != r.requiredNoteIds.end());
    if (hasA && hasB) {
      return &r;
    }
  }
  return nullptr;
}

int ZettelGraph::getMasterNodesCount() const {
  int count = 0;
  for (const auto& n : m_notes) {
    if (n.isMasterNode && n.seedDiscovered) count++;
  }
  return count;
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
  // If master node for this cluster is synthesized, 100% resonance is achieved!
  for (const auto& n : m_notes) {
    if (n.cluster == cluster && n.isMasterNode && n.synthesized) {
      return 1.0f;
    }
  }

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
  os << "# astro-voyager zettelkasten second brain state (version=2)\n";
  os << "version=2\n";
  for (const auto& n : m_notes) {
    os << "note=" << n.id
       << "|disc=" << (n.seedDiscovered ? 1 : 0)
       << "|synth=" << (n.synthesized ? 1 : 0)
       << "|pos=" << n.posX << "," << n.posY
       << "|depth=" << n.depthLevel
       << "|master=" << (n.isMasterNode ? 1 : 0)
       << "|links=";
    for (size_t i = 0; i < n.links.size(); ++i) {
      if (i > 0) os << ",";
      os << n.links[i];
    }
    os << "\n";
  }
  for (const auto& r : m_recipes) {
    os << "recipe=" << r.id << "|synth=" << (r.synthesized ? 1 : 0) << "\n";
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

    // Recipe line
    if (line.rfind("recipe=", 0) == 0) {
      std::string payload = line.substr(7);
      size_t pipe = payload.find('|');
      std::string recId = (pipe == std::string::npos) ? payload : payload.substr(0, pipe);
      bool synth = false;
      if (pipe != std::string::npos) {
        std::string rest = payload.substr(pipe + 1);
        if (rest.rfind("synth=", 0) == 0) {
          synth = (std::atoi(rest.substr(6).c_str()) != 0);
        }
      }
      for (auto& r : m_recipes) {
        if (r.id == recId) {
          r.synthesized = synth;
          any = true;
          break;
        }
      }
      continue;
    }

    if (line.rfind("note=", 0) != 0) continue;

    // Format: note=ID|disc=0|synth=0|pos=x,y|depth=0|master=0|links=a,b
    std::string payload = line.substr(5);
    std::istringstream ss(payload);
    std::string token;

    std::string noteId;
    bool disc = false;
    bool synth = false;
    float px = 0.0f, py = 0.0f;
    int depth = 0;
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
        } else if (field.rfind("depth=", 0) == 0) {
          depth = std::atoi(field.substr(6).c_str());
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
      n->depthLevel = depth;
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
