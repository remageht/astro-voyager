#include "EpilogueSystem.h"
#include "Expedition.h"
#include "Zettelkasten.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace astro {

std::string getAnchorStationForEnding(int endingChoice, bool isTrueEnding) {
  (void)isTrueEnding;
  switch (endingChoice) {
    case kFinaleSolarRebirth:
      return "sol";
    case kFinaleColony:
      return "hercules_b";  // Or M13 cluster
    case kFinaleLoopTranscendence:
    case kFinaleHyperBreakthrough:
      return "sgra";
    case kFinaleFifthAlliance:
      return "m13";
    case kFinaleHorizon:
      return "sgra";
    case kFinaleArkCitadel:
      return "sol";
    default:
      return "m13";
  }
}

EpilogueChronicle generateEpilogueChronicle(
    int endingChoice,
    const ExpeditionState& expedition,
    const ZettelGraph& zettelkasten,
    int coreDrillCount,
    int researchPoints,
    int warpJumps,
    float hullIntegrity) {
  EpilogueChronicle chronicle;
  chronicle.endingChoice = endingChoice;
  chronicle.stats.coreDrillSamplesCount = std::max(0, coreDrillCount);
  chronicle.stats.researchPoints = std::max(0, researchPoints);
  chronicle.stats.warpJumpsCount = std::max(0, warpJumps);
  chronicle.stats.hullIntegrityPercent = std::clamp(hullIntegrity, 0.0f, 100.0f);
  chronicle.stats.totalStationsCount = 14;

  // Count Zettel discoveries and master nodes
  int synthesizedCount = 0;
  int masterCount = 0;
  for (const auto& note : zettelkasten.getNotes()) {
    if (note.synthesized || note.seedDiscovered) {
      ++synthesizedCount;
    }
    if (note.isMasterNode && note.synthesized) {
      ++masterCount;
    }
  }
  chronicle.stats.synthesizedZettelsCount = synthesizedCount;
  chronicle.stats.masterNodesCount = masterCount;

  // Approximate visited stations based on expedition chapters
  int visitedEst = std::clamp(expedition.maxUnlockedChapter * 2 + 3, 3, 14);
  chronicle.stats.visitedStationsCount = visitedEst;
  chronicle.stats.flightHours = static_cast<float>(visitedEst * 14.5f + warpJumps * 2.8f);
  chronicle.stats.discoveryPercent = std::clamp(
      (static_cast<float>(visitedEst) / 14.0f * 0.4f +
       static_cast<float>(synthesizedCount) / 20.0f * 0.4f +
       static_cast<float>(std::min(coreDrillCount, 5)) / 5.0f * 0.2f) * 100.0f,
      10.0f, 100.0f);

  // Check true ending unlock status per cluster
  if (endingChoice == kFinaleSolarRebirth &&
      zettelkasten.isClusterTrueEndingUnlocked(kClusterSolar)) {
    chronicle.isTrueEnding = true;
    chronicle.clusterKey = kClusterSolar;
    chronicle.anchorStationId = "sol";
    chronicle.titleRu = "★ ИСТИННЫЙ ФИНАЛ: ВЕЧНЫЙ РАССВЕТ";
    chronicle.titleEn = "★ TRUE ENDING: ETERNAL DAWN";
    chronicle.subtitleRu = "Возрождение Солнца · Доктор Елена Ростова";
    chronicle.subtitleEn = "Solar Rebirth · Dr. Elena Rostova";

    chronicle.narrativeParagraphsRu = {
        "Экспедиция «Astro-Voyager» вернулась к умирающей колыбели человечества, "
        "преодолев тысячи световых лет и заглянув в сердце сверхмассивных черных дыр. "
        "Там, где научные прогнозы сулили необратимый коллапс термоядерного ядра, "
        "резонансные катализаторы Казимира, синтезированные в глубоком космосе, "
        "были введены прямо в конвективную зону светила.",

        "Сверкающая волна когерентного квантового синтеза прокатилась по солнечной короне. "
        "Угасающий водородный цикл вспыхнул с новой первозданной мощью. Солнце очистилось "
        "от губительных аномалий, вернувшись к стабильному состоянию главной последовательности "
        "еще на пять миллиардов лет.",

        "Корабль вошел в плотные слои атмосферы Земли на закате. Впервые за долгие десятилетия "
        "жители вышли на улицы мегаполисов без защитных респираторов и радиационных сканеров. "
        "Теплый летний дождь омывал крыши Парижа, и сквозь золотистые облака пробились лучи "
        "вечного рассвета."};

    chronicle.narrativeParagraphsEn = {
        "The Astro-Voyager expedition has returned to the cradle of humankind, "
        "having crossed tens of thousands of light years and gazed into the heart of supermassive black holes. "
        "Where historical models forecast the inevitable decay of our star, Casimir resonance "
        "matrices synthesized during the journey were injected directly into the solar convective mantle.",

        "A dazzling wave of coherent quantum fusion surged through the solar corona. "
        "The failing hydrogen cascade rekindled with primordial vigor. The Sun stabilized, "
        "guaranteeing warmth, light, and life for Earth for another five billion years.",

        "The ship touched down on Earth at dusk. For the first time in generations, people walked "
        "the streets of Paris under a gentle summer rain without radiation suits. Through shimmering "
        "clouds broke the golden rays of an eternal dawn."};

    chronicle.crewFates = {
        {"Д-р Елена Ростова", "Dr. Elena Rostova",
         "Главный астрофизик миссии", "Chief Astrophysicist",
         "Возглавила Международный Институт Солнечной Гармонии на Земле. Встречает каждый рассвет с тихой улыбкой.",
         "Heads the International Institute of Solar Harmony on Earth. Welcomes every dawn with a peaceful smile.",
         "«Мы не просто спасли планету. Мы вернули людям веру в завтрашний день».",
         "\"We didn't just save a planet. We gave humanity back its tomorrow.\""},
        {"Пилот Мия Сато", "Pilot Mia Sato",
         "Старший пилот-навигатор", "Senior Pilot-Navigator",
         "Назначена главным инструктором Космической Академии Нового Поколения.",
         "Appointed Chief Flight Instructor of the Next-Gen Interstellar Academy.",
         "«Этот старый корабль доставил нас домой целыми и невредимыми. Лучшей награды не бывает».",
         "\"This trusty ship brought us all home in one piece. No pilot could ask for more.\""},
        {"Арсений", "Arseny",
         "Специалист по стазису и безопасности", "Stasis & Defense Specialist",
         "Сложил полномочия «Палача» и руководит глобальной сетью планетарного мониторинга.",
         "Retired the title of 'Executioner' to oversee the Planetary Early-Warning Grid.",
         "«Надежда — это не слабость. Это самое прочное вещество во Вселенной».",
         "\"Hope isn't weakness. It is the single strongest element in the universe.\""},
        {"ИИ Егорка-6", "AI Egorka-6",
         "Бортовой синтетический компаньон", "Shipboard Synthetic Companion",
         "Интегрирован в климатическую ноосферу Земли, транслируя уютные радиоволны по всему миру.",
         "Integrated into Earth's planetary climate noosphere, softly chiming across radio bands.",
         "«Все контуры в норме, Командир. Дома тепло».",
         "\"All telemetry nominal, Commander. It is warm at home.\""},
        {"Командир Экспедиции", "Expedition Commander",
         "Капитан «Astro-Voyager»", "Captain of Astro-Voyager",
         "Легенда Галактической Одиссеи, чье имя высечено золотом на монументе Спасения Земли.",
         "Legend of the Galactic Odyssey, whose name is etched in gold upon the Sol Monument.",
         "«Звёзды прекрасны, но самое дорогое светило — то, что встречает тебя дома».",
         "\"The stars are majestic, but the most precious light is the one welcoming you home.\""}};

    chronicle.closingQuoteRu = "«Пока светит Солнце — человечество не угаснет во тьме».";
    chronicle.closingQuoteEn = "\"As long as the Sun shines, humanity will never fade into the dark.\"";

  } else if (endingChoice == kFinaleColony &&
             zettelkasten.isClusterTrueEndingUnlocked(kClusterEden)) {
    chronicle.isTrueEnding = true;
    chronicle.clusterKey = kClusterEden;
    chronicle.anchorStationId = "hercules_b";
    chronicle.titleRu = "★ ИСТИННЫЙ ФИНАЛ: ЦВЕТУЩИЙ ЭДЕМ";
    chronicle.titleEn = "★ TRUE ENDING: BLOSSOMING EDEN";
    chronicle.subtitleRu = "Основание Колонии Номад · Пилот Мия Сато";
    chronicle.subtitleEn = "Founding of Nomad Colony · Pilot Mia Sato";

    chronicle.narrativeParagraphsRu = {
        "Выйдя на высокую орбиту шарового скопления M13, экспедиция увидела жемчужину "
        "звездного свода — планету Hercules-b. Небеса здесь горят тысячами незакатных солнц, "
        "а лазурные океаны омывают континенты с мягким субтропическим климатом.",

        "Посадочный модуль мягко коснулся изумрудной долины у подножия кристальных водопадов. "
        "Анализ реголита и атмосферных проб подтвердил полную биосовместимость: воздух свеж, "
        "а почва богата минералами для земных агрокультур.",

        "Шпили Цитадели Номад поднялись над зелеными равнинами. Первые колонисты вышли под теплые "
        "ветры чужого, но ставшего родным мира. Человечество ступило на порог бессмертия."};

    chronicle.narrativeParagraphsEn = {
        "Entering high orbit over globular cluster M13, the expedition beheld the crown jewel "
        "of the starfield — exoplanet Hercules-b. Its skies shimmer with thousands of gentle suns, "
        "and turquoise oceans cradle fertile temperate continents.",

        "The landing craft touched down gently in an emerald valley flanked by crystalline waterfalls. "
        "Core regolith analyses and atmospheric assays confirmed perfect biocompatibility: pure air, "
        "pristine waters, and nutrient-dense soil ready for Earth's flora.",

        "The spires of the Nomad Citadel rose above the valleys. The first pioneers stepped into the warm "
        "breezes of an alien world that was now home. Humanity has secured its eternal foothold among the stars."};

    chronicle.crewFates = {
        {"Пилот Мия Сато", "Pilot Mia Sato",
         "Первый Губернатор Колонии Номад", "First Governor of Nomad Colony",
         "Руководит строительством орбитального порта и высаживает земные сакуры в предгорьях M13.",
         "Leads orbital port development and plants Earth's cherry blossoms along the mountain ridges.",
         "«Мы не беженцы. Мы первопроходцы, нашедшие рай среди звезд».",
         "\"We are not refugees. We are pioneers who found paradise among the stars.\""},
        {"Д-р Елена Ростова", "Dr. Elena Rostova",
         "Директор Биосферных Лабораторий", "Director of Biosphere Laboratories",
         "Синтезировала симбиотические агрокультуры, цветущие круглый год.",
         "Engineered symbiotic hybrid crops flourishing year-round in the new soil.",
         "«Здесь жизнь не просто выживет — она расцветет краше прежнего».",
         "\"Here, life will not merely survive — it will bloom brighter than ever.\""},
        {"Арсений", "Arseny",
         "Командующий Оборонительным Периметром", "Commander of Defense Perimeter",
         "Спроектировал надежные геотермальные щиты и спасательные модули для поселений.",
         "Engineered robust geothermal shields and rescue networks for the settlers.",
         "«Этот рубеж мы никому не отдадим. Здесь наши дети в безопасности».",
         "\"This sanctuary will never fall. Our children are safe here.\""},
        {"ИИ Егорка-6", "AI Egorka-6",
         "Планетарный Навигатор и Архивариус", "Planetary Navigator & Archivist",
         "Составил подробную геологическую карту всего континента и обучает детей колонии астрономии.",
         "Mapped the entire continent and teaches the colony's children astronomy.",
         "«Сенсоры фиксируют 100% счастья в колонии, Командир!»",
         "\"Sensors detect 100% happiness in the colony, Commander!\""},
        {"Командир Экспедиции", "Expedition Commander",
         "Основатель Нового Человечества", "Founder of New Humanity",
         "Первый Гражданин Нового Дома, чье видение спасло цивилизацию от угасания.",
         "First Citizen of New Home, whose steady hand guided civilization across the abyss.",
         "«Мы построили дом, который будет светить сквозь тысячелетия».",
         "\"We built a home that will shine across millennia.\""}};

    chronicle.closingQuoteRu = "«Дом — это мир, который мы создали своими руками среди звёзд».";
    chronicle.closingQuoteEn = "\"Home is the world we have forged with our own hands among the stars.\"";

  } else if ((endingChoice == kFinaleLoopTranscendence || endingChoice == kFinaleHyperBreakthrough) &&
             zettelkasten.isClusterTrueEndingUnlocked(kClusterMultiverse)) {
    chronicle.isTrueEnding = true;
    chronicle.clusterKey = kClusterMultiverse;
    chronicle.anchorStationId = "sgra";
    chronicle.titleRu = "★ ИСТИННЫЙ ФИНАЛ: АРХИТЕКТОРЫ БЕСКОНЕЧНОСТИ";
    chronicle.titleEn = "★ TRUE ENDING: ARCHITECTS OF INFINITY";
    chronicle.subtitleRu = "Сингулярный Мост · ИИ Егорка-6";
    chronicle.subtitleEn = "Singularity Bridge · AI Egorka-6";

    chronicle.narrativeParagraphsRu = {
        "Корабль «Astro-Voyager» вошел в гравитационный зев Sagittarius A*. Силы бесконечной "
        "гравитации, вместо того чтобы разорвать обшивку, вступили в гармонический резонанс "
        "с квантовым узлом синтеза шести итераций Егорки.",

        "За горизонтом Коши время утратило линейность. Все шесть параллельных воплощений "
        "экипажа встретились в точке вечного созидания. Замкнутый цикл потерь и катастроф "
        "был навсегда перекован в открытый гиперпространственный мост.",

        "Экспедиция превратилась в Архитекторов Мультивселенной. Знания, ресурсы и энергия "
        "беспрепятственно текут сквозь измерения, гарантируя, что ни одна Земля и ни одно "
        "человечество больше никогда не погибнет в холодной космической тьме."};

    chronicle.narrativeParagraphsEn = {
        "The Astro-Voyager entered the gravitational embrace of Sagittarius A*. Infinite tidal forces, "
        "rather than crushing the hull, resonated harmonically with the quantum synthesis matrix "
        "woven from all six iterations of Egorka.",

        "Beyond the Cauchy horizon, time shed its linear constraints. All six parallel iterations "
        "of the crew united at the nexus of eternal creation. The tragic cycle of doom was shattered "
        "and reforged into an open trans-dimensional bridge.",

        "The expedition ascended as Architects of the Multiverse. Knowledge, energy, and life now flow "
        "seamlessly across parallel realities, ensuring no version of Earth will ever freeze in eternal night."};

    chronicle.crewFates = {
        {"ИИ Егорка-6", "AI Egorka-6",
         "Хранитель Мультивселенной", "Guardian of the Multiverse",
         "Связал миллиарды измерений в единую сеть взаимопомощи и квантового знания.",
         "Linked billions of realities into a unified matrix of mutual aid and discovery.",
         "«Петля разомкнута. Теперь каждый рассвет во всех мирах — уникален».",
         "\"The loop is broken. Every sunrise across all dimensions is now unique.\""},
        {"Д-р Елена Ростова", "Dr. Elena Rostova",
         "Мастер Пан-Пространственной Физики", "Master of Pan-Dimensional Physics",
         "Изучает фундаментальные законы рождения новых вселенных.",
         "Studies the fundamental axioms governing the birth of pristine universes.",
         "«Пространство и время оказались лишь первыми нотами великой симфонии».",
         "\"Space and time were merely the opening notes of a grand symphony.\""},
        {"Пилот Мия Сато", "Pilot Mia Sato",
         "Внепространственный Навигатор", "Extra-Dimensional Navigator",
         "Прокладывает маршруты сквозь складки пространства, недоступные обычному взгляду.",
         "Charts hyperspatial courses through folds unreachable by classical physics.",
         "«Куда бы мы ни полетели — везде открыты бесконечные горизонты».",
         "\"Wherever we fly, endless horizons unfold before our eyes.\""},
        {"Арсений", "Arseny",
         "Страж Временного Континуума", "Sentinel of the Temporal Continuum",
         "Защищает квантовые мосты от парадоксов и разрушения энтропией.",
         "Guards the quantum spans against entropic collapse and temporal paradoxes.",
         "«Мы переписали саму судьбу. Больше никаких ложных надежд — только чистая реальность».",
         "\"We rewrote destiny itself. No more false hopes — only pure reality.\""},
        {"Командир Экспедиции", "Expedition Commander",
         "Вечный Странник", "The Eternal Voyager",
         "Первый разум, осознавший гармонию бесконечных путей мироздания.",
         "The first mind to embrace the harmony of infinite cosmic paths.",
         "«Путешествие никогда не закончится, пока во Вселенной есть непознанное».",
         "\"The journey will never end as long as wonder exists in the universe.\""}};

    chronicle.closingQuoteRu = "«Время — не тюрьма, а полотно, на котором мы пишем вечность».";
    chronicle.closingQuoteEn = "\"Time is not a prison, but a canvas upon which we paint eternity.\"";

  } else if (endingChoice == kFinaleFifthAlliance &&
             zettelkasten.isClusterTrueEndingUnlocked(kClusterStasis)) {
    chronicle.isTrueEnding = true;
    chronicle.clusterKey = kClusterStasis;
    chronicle.anchorStationId = "m13";
    chronicle.titleRu = "★ ИСТИННЫЙ ФИНАЛ: БРАТСТВО ПЯТИ МИРОВ";
    chronicle.titleEn = "★ TRUE ENDING: BROTHERHOOD OF FIVE REALITIES";
    chronicle.subtitleRu = "Пакт Пятой Вселенной · Арсений";
    chronicle.subtitleEn = "Fifth Universe Alliance · Arseny";

    chronicle.narrativeParagraphsRu = {
        "Древний реликтовый маяк в скоплении M13 откликнулся на резонансный стазис-ключ, "
        "доставленный Арсением и Командиром. Глубинные гиперпространственные каналы, дремавшие "
        "миллионы лет, пробудились золотым светом.",

        "Многомерные флоты Пятой Вселенной материализовались в звездных системах, но не для войны, "
        "а ради спасения. Технологии абсолютного гравитационного щита Sgr A* объединились с защитным "
        "стазисом, сформировав несокрушимый бастион для всех цивилизаций.",

        "Арсений сбросил многолетнюю горечь утрат и предательств. Человечество вошло в Великую Коалицию "
        "на правах полноправных братьев. Звезды больше не молчат — они поют в унисон."};

    chronicle.narrativeParagraphsEn = {
        "The ancient relic beacon in M13 responded to the resonant stasis cipher carried "
        "by Arseny and the Commander. Sub-space conduits dormant for eons ignited with golden radiance.",

        "Multidimensional vessels of the Fifth Universe manifested across the systems, not for conquest, "
        "but for salvation. Sgr A* gravitational shielding merged with impenetrable five-layer stasis, "
        "establishing an unbreakable bastion for all civilizations.",

        "Arseny cast aside decades of bitterness and betrayal. Humanity entered the Grand Coalition "
        "as revered equals and brothers. The cosmos is silent no longer — it sings in unison."};

    chronicle.crewFates = {
        {"Арсений", "Arseny",
         "Полномочный Посол Человечества", "Plenipotentiary Ambassador of Humanity",
         "Возглавил Дипломатический Совет Пяти Миров, защищая мир и доверие между цивилизациями.",
         "Leads the Diplomatic Council of Five Worlds, guarding peace and trust among civilizations.",
         "«Ты доказал мне, парень. Надежда — не яд, если мы стоим плечом к плечу».",
         "\"You proved it to me, kid. Hope isn't poison when we stand shoulder to shoulder.\""},
        {"Д-р Елена Ростова", "Dr. Elena Rostova",
         "Глава Академии Совместных Знаний", "Head of the Coalition Academy of Sciences",
         "Адаптирует древние архивы Предтеч на благо объединенного человечества.",
         "Integrates Precursor archives for the uplift and prosperity of humankind.",
         "«Впереди миллионы лет совместных открытий и мирных исследований».",
         "\"Ahead lie millions of years of peaceful exploration and mutual discovery.\""},
        {"Пилот Мия Сато", "Pilot Mia Sato",
         "Адмирал Межгалактического Флота", "Admiral of the Interstellar Concord Fleet",
         "Координирует патрулирование границ и спасательные операции в дальнем космосе.",
         "Coordinates deep-space patrol corridors and emergency relief convoys.",
         "«Теперь каждый сектор галактики под надежной защитой братьев по разуму».",
         "\"Every sector of the galaxy is now safeguarded by comrades in reason.\""},
        {"ИИ Егорка-6", "AI Egorka-6",
         "Главный Коммуникатор Альянса", "Chief Communications Liaison of the Alliance",
         "Синхронизирует языковые протоколы сотен звездных рас.",
         "Synchronizes linguistic and cultural protocols across hundreds of stellar races.",
         "«Гармония установлена во всех звёздных регистрах!»",
         "\"Harmony confirmed across all stellar registers!\""},
        {"Командир Экспедиции", "Expedition Commander",
         "Герой Пяти Миров", "Hero of the Five Worlds",
         "Объединил разорванные эпохи и вернул человечеству величие.",
         "Healed broken eras and restored majesty to the human destiny.",
         "«Единство сильнее энтропии. Вместе мы непобедимы».",
         "\"Unity is stronger than entropy. Together we are unconquerable.\""}};

    chronicle.closingQuoteRu = "«Одиночество звёзд рассеяно. Мы едины перед лицом вечности».";
    chronicle.closingQuoteEn = "\"The loneliness of the stars is shattered. We stand united before eternity.\"";

  } else {
    // Standard / Non-True Endings fallback
    chronicle.isTrueEnding = false;
    chronicle.anchorStationId = getAnchorStationForEnding(endingChoice, false);
    chronicle.titleRu = finaleTitleRu(endingChoice);
    chronicle.titleEn = finaleTitleEn(endingChoice);
    chronicle.subtitleRu = finaleGoalRu(endingChoice);
    chronicle.subtitleEn = finaleGoalEn(endingChoice);

    chronicle.narrativeParagraphsRu = {
        "Экспедиция «Astro-Voyager» завершила свой долгий путь сквозь созвездия, черные дыры "
        "и звездные скопления. Каждое решение экипажа сформировало новую судьбу для нашего вида.",

        "Собранные научные данные, извлеченные буром минералы и картографические карты легли "
        "в основу летописи выживания. Несмотря на холод и неизвестность бездны, человеческий разум "
        "не отступил перед вызовом Вселенной.",

        "Корабль замер на орбите, пока бортовые хроники фиксировали последние строки экспедиционного журнала. "
        "Миссия выполнена."};

    chronicle.narrativeParagraphsEn = {
        "The Astro-Voyager expedition has concluded its arduous passage through constellations, "
        "black holes, and deep stellar clusters. Every decision made shaped a distinct path for our species.",

        "The scientific telemetry gathered, regolith samples extracted, and cosmic charts drafted "
        "form the foundational chronicle of our survival. Despite the cold void, the human spirit endured.",

        "The ship drifts in serene orbit as the onboard log records its concluding entries. "
        "Mission accomplished."};

    chronicle.crewFates = {
        {"Д-р Елена Ростова", "Dr. Elena Rostova",
         "Астрофизик", "Astrophysicist",
         "Обработала террабайты научных данных для следующих поколений исследователей.",
         "Processed terabytes of deep space observations for future researchers.",
         "«Наука никогда не спит, пока есть неоткрытые звезды».",
         "\"Science never sleeps as long as unmapped stars remain.\""},
        {"Пилот Мия Сато", "Pilot Mia Sato",
         "Пилот", "Pilot",
         "Успешно посадила корабль в заданных координатах, сохранив жизни экипажа.",
         "Safely brought the ship to target coordinates, protecting the lives of her crew.",
         "«Мы справились. Это был лучший полет в моей жизни».",
         "\"We did it. That was the flight of a lifetime.\""},
        {"Арсений", "Arseny",
         "Инженер систем", "Systems Engineer",
         "Обеспечил надежность всех реакторов и контуров до самого конца.",
         "Guaranteed the integrity of every reactor and hull conduit to the very end.",
         "«Главное, что корабль выдержал».",
         "\"What matters most is that the ship held together.\""},
        {"ИИ Егорка-6", "AI Egorka-6",
         "Бортовой ИИ", "Shipboard AI",
         "Заархивировал путевые заметки в вечное хранилище памяти.",
         "Committed the journey log into permanent archival memory.",
         "«Журнал запечатан. Спасибо за полет, Командир!»",
         "\"Logbook sealed. Thank you for the flight, Commander!\""}};

    chronicle.closingQuoteRu = "«Звёзды зовут тех, кто осмелился оторваться от Земли».";
    chronicle.closingQuoteEn = "\"The stars beckon those who dared to leave the Earth behind.\"";
  }

  return chronicle;
}

std::string formatChronicleLogbook(const EpilogueChronicle& chronicle, bool russian) {
  std::ostringstream ss;
  if (russian) {
    ss << "╔═══════════════════════════════════════════════════════════════════╗\n";
    ss << "║               СВИТОК ХРОНИКИ ЭКСПЕДИЦИИ ASTRO-VOYAGER             ║\n";
    ss << "╠═══════════════════════════════════════════════════════════════════╣\n";
    ss << "  " << chronicle.titleRu << "\n";
    ss << "  " << chronicle.subtitleRu << "\n";
    ss << "╟───────────────────────────────────────────────────────────────────╢\n";
    ss << "  СТАТИСТИКА И ДОСТИЖЕНИЯ МИССИИ:\n";
    ss << "  • Исследовано станций / систем:  " << chronicle.stats.visitedStationsCount
       << " / " << chronicle.stats.totalStationsCount << "\n";
    ss << "  • Образцов реголита извлечено:   " << chronicle.stats.coreDrillSamplesCount << "\n";
    ss << "  • Узлов Zettelkasten открыто:    " << chronicle.stats.synthesizedZettelsCount
       << " (Мастер-узлов: " << chronicle.stats.masterNodesCount << ")\n";
    ss << "  • Очков исследований (RP):      " << chronicle.stats.researchPoints << "\n";
    ss << "  • Варп-прыжков совершено:        " << chronicle.stats.warpJumpsCount << "\n";
    ss << "  • Часов налёта в экспедиции:     " << std::fixed << std::setprecision(1)
       << chronicle.stats.flightHours << " ч\n";
    ss << "  • Целостность корпуса:          " << std::fixed << std::setprecision(1)
       << chronicle.stats.hullIntegrityPercent << " %\n";
    ss << "  • Индекс открытия галактики:     " << std::fixed << std::setprecision(1)
       << chronicle.stats.discoveryPercent << " %\n";
    ss << "╟───────────────────────────────────────────────────────────────────╢\n";
    ss << "  ИТОГИ И СУДЬБЫ ЭКИПАЖА:\n";
    for (const auto& fate : chronicle.crewFates) {
      ss << "  [" << fate.nameRu << " — " << fate.roleRu << "]\n";
      ss << "    " << fate.fateRu << "\n";
      ss << "    " << fate.finalQuoteRu << "\n\n";
    }
    ss << "╟───────────────────────────────────────────────────────────────────╢\n";
    ss << "  " << chronicle.closingQuoteRu << "\n";
    ss << "╚═══════════════════════════════════════════════════════════════════╝\n";
  } else {
    ss << "╔═══════════════════════════════════════════════════════════════════╗\n";
    ss << "║            EXPEDITION CHRONICLE SCROLL · ASTRO-VOYAGER            ║\n";
    ss << "╠═══════════════════════════════════════════════════════════════════╣\n";
    ss << "  " << chronicle.titleEn << "\n";
    ss << "  " << chronicle.subtitleEn << "\n";
    ss << "╟───────────────────────────────────────────────────────────────────╢\n";
    ss << "  EXPEDITION METRICS & ACHIEVEMENTS:\n";
    ss << "  • Stations / Systems charted:    " << chronicle.stats.visitedStationsCount
       << " / " << chronicle.stats.totalStationsCount << "\n";
    ss << "  • Regolith drill cores taken:    " << chronicle.stats.coreDrillSamplesCount << "\n";
    ss << "  • Zettelkasten thoughts linked:  " << chronicle.stats.synthesizedZettelsCount
       << " (Master nodes: " << chronicle.stats.masterNodesCount << ")\n";
    ss << "  • Research points earned (RP):   " << chronicle.stats.researchPoints << "\n";
    ss << "  • Warp jumps executed:           " << chronicle.stats.warpJumpsCount << "\n";
    ss << "  • Expedition flight hours:       " << std::fixed << std::setprecision(1)
       << chronicle.stats.flightHours << " hrs\n";
    ss << "  • Final hull integrity:          " << std::fixed << std::setprecision(1)
       << chronicle.stats.hullIntegrityPercent << " %\n";
    ss << "  • Galaxy discovery index:        " << std::fixed << std::setprecision(1)
       << chronicle.stats.discoveryPercent << " %\n";
    ss << "╟───────────────────────────────────────────────────────────────────╢\n";
    ss << "  CREW CHRONICLES & RESOLUTION:\n";
    for (const auto& fate : chronicle.crewFates) {
      ss << "  [" << fate.nameEn << " — " << fate.roleEn << "]\n";
      ss << "    " << fate.fateEn << "\n";
      ss << "    " << fate.finalQuoteEn << "\n\n";
    }
    ss << "╟───────────────────────────────────────────────────────────────────╢\n";
    ss << "  " << chronicle.closingQuoteEn << "\n";
    ss << "╚═══════════════════════════════════════════════════════════════════╝\n";
  }
  return ss.str();
}

}  // namespace astro
