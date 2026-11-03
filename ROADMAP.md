# Roadmap — path to AAA / Дорожная карта до AAA

Goal: grow astro-voyager from a strong tech-demo into a polished, deep,
cross-platform "AAA-feel" space explorer. AAA here is not a budget claim —
это **планка качества продакшна**: production values, глубина геймплея,
надёжность, охват платформ — и всё измеряется гейтами, а не обещаниями.

## Quality bar / Планка качества (действует каждый релиз)
- Visual regression: 17/17 харнесс-кадров байт-в-байт по умолчанию; каждая фича добавляет свои кадры
- Engineering: ctest зелёный; static checks только растут; save/load round-trip; покрытие локализации
- Performance: бюджет 60 fps @1080p Ultra на эталонной сцене, проверяется скриптованным пролётом в CI
- Platforms: Windows + Linux AppImage + macOS (GL) — все собираются в CI
- UX: каждый новый экран проходит проход onboarding/подсказок и локализован EN+RU

## Release Lifecycle & Milestones / Жизненный цикл релизов

| Стадия / Stage | Версии / Versions | Фокус / Primary Focus | Статус / Status |
|---|---|---|---|
| **Pre-Alpha** | `v1.0.0` – `v4.1.0` | Физика ОТО, релятивистские чёрные дыры, Солнечная система, сюжетная кампания 7 глав | Завершено (Shipped) |
| **Alpha Stage** | `v4.2.0` | Диегетический 3D-кокпит, коллиматорный ИЛС, динамика перегрузок, SpaceDrive 3D варп-туннель, авионика | Завершено (Shipped) |
| **Beta Stage** | `v4.3.0` – `v4.5.0` | Процедурные PBR-поверхности планет (LOD), Рэлей-Ми атмосфера, Photo Mode v2, LUT, авионика, провенанс ассетов, Егорка-6 | Завершено (Shipped v4.5.0) |
| **Release Candidate** | `v4.6.0` – `v4.8.0` | Научный спектрометр, радиоэфир экипажа, посадка на планеты, экономика хабов | **Текущий этап (In Progress)** |
| **Release 1.0** | `v5.0.0` | Полноценный AAA-релиз, Steam Deck верификация, Mod-API мастерская, 5+ языков | Финал карты (Target) |

## Stage 1 — Foundation / Фундамент (v3.4–v3.6)

### v3.4 — macOS GL + saves
- EN: Cocoa/OpenGL frameworks + GL loader (GLEW out), macos GL CI job (headless smoke), zip/dmg artefact; save system: JSON profile (settings/progress/customs/journey) with schema versioning and migration.
- RU: Портируем GL-приложение на macOS (framework-и + GL-loader вместо GLEW), macos GL-джоба в CI (headless smoke), zip/dmg; система сохранений: JSON-профиль (настройки/прогресс/объекты/путешествие) со схемой версий и миграциями.
- Gates: mac GL зелёный в CI, save round-trip тест, настройки переживают рестарт.

### v3.5 — Performance & image quality
- EN: Frame-time budget in CI (scripted fly-through scenes), resolution scaling, MSAA/FXAA options, ultrawide/HiDPI polish, LOD pass.
- RU: Бюджет времени кадра в CI (скриптованные пролёты), масштаб разрешения, MSAA/FXAA, ultrawide/HiDPI, LOD-проход.
- Gates: бюджет выполняется, байт-идентичность дефолтов сохранена, новый перф-пресет.

### v3.6 — Controls & accessibility
- EN: Gamepad (GLFW mapping + UI navigation), input remapping UI, text scaling, colorblind-safe HUD, onboarding tutorial (first flight) + hint system.
- RU: Геймпад (маппинг GLFW + навигация UI), перемаппинг клавиш, масштаб текста, безопасная для дальтоников палитра HUD, обучающий стартовый полёт + система подсказок.
- Gates: тесты маппинга, состояние туториала переживает рестарт.

## Stage 2 — World & gameplay / Мир и геймплей (v3.7–v4.0)

### v3.7 — Stations as hubs, economy
- EN: Docking, station vista/interior screen, credits, refuel/repair/ship upgrades, customs market.
- RU: Стыковка, вид станции изнутри, кредиты, заправка/ремонт/апгрейды корабля, рынок пользовательских объектов.
- Gates: тесты транзакций, интеграция HUD, кадры интерьеров в харнессе.

### v3.8 — Missions & contracts
- EN: Contract generator (deliver/scan/measure), mission log, HUD markers, rewards tied to economy.
- RU: Генератор контрактов (доставка/сканирование/замеры), лог заданий, маркеры в HUD, награды через экономику.
- Gates: детерминизм генератора, персистентность лога.

### v3.9 — Science & discovery
- EN: Spectrometer / body analysis, Codex encyclopedia, discovery rewards, journal integration.
- RU: Спектрометр и анализ тел, энциклопедия-кодекс, награды за открытия, связка с журналом путешествия.
- Gates: тесты модели данных, полнота кодекса в static checks.

### v4.0 — Two modes: Adventure & Sandbox / Два режима: приключение и песочница
- Status / Статус: **shipped in 4.0.0 (foundation) & 4.1.0 (campaign)** / **выпущено в 4.0.0 (фундамент) и 4.1.0 (кампания)**.
- EN: Shipped: mode entry "New Game · Adventure", Egorka's illustrated prologue, dedicated save profiles, 7-chapter campaign with Mia Sato, "The Last Watch" fleet interlude, dual endings (Colony vs Horizon) and shared "Legacy" epilogue. Still ahead: in-chapter branching and deadline countdown meta-game.
- RU: Выпущено: пункт «Новая игра · Adventure», иллюстрированный пролог Егорки, раздельные профили сохранений, кампания из 7 глав с репликами Мии Сато, интерлюдия «Последний караул», развилка двух концовок и эпилог «Наследие». Впереди: ветвления внутри глав и обратный отсчёт срока.
- Gates: раздельные профили сохранений, тесты экспедиции и развилок (test_expedition), скриптованный smoke-прогон.

## Stage 3 — Production values & Flight Realism / Продакшн-ценности и реализм полёта (v4.1–v4.5)

### v4.1 — Cosmic events
- Supernova, tidal disruption flares, gravitational-wave ripples (visual + gameplay hooks).

### v4.2 — 3D Cockpit, SpaceDrive Warp Tunnel & Avionics / 3D-кокпит, варп-туннель SpaceDrive и авионика
- Status / Статус: **shipped in 4.2.0 (Alpha Stage)** / **выпущено в 4.2.0 (Alpha Stage)**.
- EN: Shipped: diegetic 3D cockpit with canopy struts and photochromic tinting, collimated holographic HUD at infinity (pitch ladder, boresight, horizon, velocity tapes), pilot head spring-damper G-force inertia, animated multi-layer 3D SpaceDrive warp tunnel (tamminen, CC BY 4.0) with differential rotation during warp cruise and jump flight, modernized multi-tab Settings menu.
- RU: Выпущено: диегетический 3D-кокпит с лонжеронами и фотохромным автотинтом, коллиматорный голографический ИЛС (авиагоризонт, шкала тангажа, ленты скорости), пружинно-демпферная инерция головы пилота при перегрузках, анимированный трёхслойный 3D варп-туннель SpaceDrive (tamminen, CC BY 4.0) с дифференциальным вращением в прыжках и крейсерском варпе, модернизированное многовкладочное меню настроек авионики.

### v4.3 — Audio pass & Planetary Relief PBR / Звуковой проход и PBR-рельеф планет
- Layered dynamic music on events, positional station audio, engine Doppler pitch by speed, procedural FBM crater and mountain normal relief, Rayleigh-Mie atmospheric scattering.

### v4.4 — Cinematography & Gravitational Wave Science / Кинематография и гравитационные волны
- Director camera (F8), photo mode v2 (rule of thirds, golden ratio, custom FOV), filmic LUT color grading, scientific dual-polarization metric strain oscilloscope.

### v4.5 — Flight Avionics, Spacecraft Provenance & The Sixth Universe / Авионика полёта, провенанс ассетов и Шестая Вселенная
- Status / Статус: **shipped in 4.5.0 (Beta Milestone)** / **выпущено в 4.5.0 (Beta Milestone)**.
- EN: Shipped: dynamic collimated Prograde ($\hat{v}$) and Retrograde ($-\hat{v}$) HUD flight path vectors, continuous G-force avionics with pilot physiological strain shaders (blackout/redout), radar altimeter (ALT AGL) with TERRAIN / PULL UP alert, hypersonic atmospheric entry drag and canopy plasma glow, CC BY / CC BY-NC spacecraft asset audit with SHA-256 pinning, Egorka-6 character lore, canonical portrait, and 6th Multiverse ending (`kFinaleHyperBreakthrough = 6`).
- RU: Выпущено: динамические коллиматорные маркеры Prograde ($\hat{v}$) и Retrograde ($-\hat{v}$) на ИЛС, непрерывный расчёт перегрузок $G$ с шейдерами блэкаута/редаута, радиовысотомер (ALT AGL) с алертом опасного сближения с рельефом, гиперзвуковой вход в атмосферу с аэродинамическим торможением и плазмой на остеклении фонаря, аудит лицензий космических кораблей с SHA-256 контрольными суммами, образ Егорки-6, каноничный портрет и 6-й финал Мультивселенной (`kFinaleHyperBreakthrough = 6`).

## Stage 4 — Release Candidates & Systems Depth / Кандидаты в релиз и глубина систем (v4.6–v4.8)

### v4.6 — Science Spectrometer, Ship Codex & Live Radio Chatter / Научный спектрометр, Кодекс и радиоэфир
- Status / Статус: **shipped in 4.6.0 (RC1)** / **выпущено в 4.6.0 (RC1)**.
- EN: Shipped: physical spectroscopy for 22 celestial stations with rainbow dispersion canvas, Fraunhofer dark absorption notches and glowing emission spikes, relativistic Doppler wavelength scaling, Ship Codex encyclopedia with 12 cosmological categories and +100 RP progression, live bridge radio chatter cards with dialogues for Egorka-6, Mia Sato, Dr. Elena Rostova.
- RU: Выпущено: физическая спектроскопия 22 небесных тел с радужной полосой дисперсии, фраунгоферовыми линиями поглощения и светящимися пиками эмиссии, релятивистским доплеровским смещением длин волн, Кодекс открытий с 12 категориями и прогрессией +100 RP, живые радиопереговоры на мостике с репликами Егорки-6, Мии Сато и д-ра Елены Ростовой.

### v4.6.1 — Planetary Landing & Surface Operations / Посадка на планеты и операции на поверхности
- Status / Статус: **shipped in 4.6.1** / **выпущено в 4.6.1**.
- EN: Deployable landing gear (key G) with hydraulic suspension compression, surface touchdown physics with vertical speed tolerances, landing dust kickup and surface expedition telemetry reports (+150 RP in Codex).
- RU: Выдвижные посадочные опоры/шасси (клавиша G) с гидравлической амортизацией, физика касания поверхности с допуском по вертикальной скорости, пылевой шлейф при посадке и отчёты об экспедиции на грунт (+150 RP в Кодексе).

### v4.6.2 — OJ 287 Binary Orbit & Horizon Capture, UI Polish & Cutscene Pause / Вращение OJ 287, полировка UI и пауза
- Status / Статус: **shipped in 4.6.2** / **выпущено в 4.6.2**.
- EN: Companion black hole event horizon capture & ray termination in `bh.frag` (no blowout white blob), 12.5s orbital revolution, vertical offset for Bridge Chatter toast below Expedition card, ship physics & gravity pause during story cutscenes.
- RU: Захват лучей горизонтом событий второй чёрной дыры в `bh.frag` (устранено белое пятно), вращение пары за 12.5с, вертикальное разделение плашек радиопереговоров и экспедиции, пауза корабля и физики при показе сюжета.

### v4.7 — Station Hubs & Modular Upgrades / Станции-хабы и модульные апгрейды
- Status / Статус: **planned (RC2)** / **запланировано (RC2)**.
- EN: Interior docking vista, credits & research economy, ship subsystem tuning (warp coil capacitors, sensor arrays, thruster gimbal).
- RU: Вид станции изнутри при стыковке, экономика кредитов и исследований, тюнинг подсистем корабля (конденсаторы варп-катушек, сенсорные решётки, подвес маневровых).

## Stage 5 — Ecosystem & Release 1.0 / Экосистема и Release 1.0 (v5.0+)
- EN: Mod API (data-driven stations/ships/shaders) + workshop, Steam achievements/cloud saves, Windows installer + auto-update, Steam Deck verification (1280x800, full gamepad UX), 5+ language localization, demo/trailer/press kit.
- RU: Мод-API (станции/корабли/шейдеры как данные) + мастерская, достижения/облачные сейвы платформы, инсталлятор для Windows + автообновление, верификация Steam Deck (1280x800, навигация геймпадом), локализация на 5+ языков, демо/трейлер/пресс-кит.

## Process rules / Правила процесса
- Цикл: один фича-пасс → гейты → один коммит → релиз каждые 2–3 пасса (текущий ритм).
- Harness byte-identity и static checks — ядро регрессий; каждая стадия добавляет свои гейты.
- Semver + двуязычный CHANGELOG; тег `v*` запускает CI-релиз (AppImage + windows zip).
- Ничего не попадает в релиз без кадра в харнессе (для визуала) и без теста (для логики).
