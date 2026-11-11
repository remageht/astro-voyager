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
| **Alpha Stage** | `v4.2.0` | Диегетический 3D-кокпит, коллиматорный ИЛС, динамика перегрузок, SpaceDrive 3D варп-туннель, авионика | Завершено (Shipped v4.2.0) |
| **Beta Stage** | `v4.3.0` – `v4.5.0` | Процедурные PBR-поверхности планет (LOD), Рэлей-Ми атмосфера, Photo Mode v2, LUT, авионика, провенанс ассетов, Егорка-6 | Завершено (Shipped v4.5.0) |
| **Release Candidate** | `v4.6.0` – `v4.8.0` | Научный спектрометр, радиоэфир экипажа, посадка на планеты, экономика хабов, центрифуга | Завершено (Shipped v4.8.0) |
| **Alpha Pre-Releases** | `v4.9.0` – `v4.9.4` | Astro-Zettelkasten («Второй Мозг»), синтез Эврики, кассетная дека, реликвия Voyager-1 | Завершено (Shipped v4.9.4) |
| **Release 1.0 (Grand Universe)** | `v5.0.0` | 22 станции, 8 концовок Мультивселенной, Zero-UI HUD, 3D Оррери, авионика посадки, геймпады & Steam Deck | Завершено (Shipped v5.0.0) |
| **Stability Patches** | `v5.0.1` – `v5.0.5` | Озвучка экипажа Whisper, тембр Мии/Елены, фикс VRAM-утечек ObjModel, CI Linux runner, синхронизация документации | **Текущий этап (Shipped / In Progress)** |
| **Stage 6: Exoplanetary Biomes** | `v5.1.0` | Процедурные континентальные биомы, динамическая гидродинамика океанов, автономный научный дрон | Запланировано (Planned) |
| **Stage 7: Next-Gen Engine** | `v5.2.0` | SPSC lock-free аудио-буфер, RHI слой абстракции (Vulkan/Metal), трассировка путей черных дыр Керра | Запланировано (Planned) |
| **Stage 8: Beacon Network** | `v5.3.0` | Асинхронный кооператив, квантовые ретрансляторы на планетах, P2P-обмен Zettelkasten, динамические SOS-события | Запланировано (Planned) |
| **Stage 9: Beyond the Singularity** | `v6.0.0` | Lua/WASM Mod-API, интеграция Steam Workshop, встроенный менеджер модификаций с Hot Reload | Перспектива (Future Target) |

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
- EN: Companion black hole event horizon capture & ray termination in `bh.frag` (no blowout white blob), 12.5s orbital revolution, vertical offset for Bridge Chatter toast below Expedition card, ship physics & gravity pause during story cutscenes, custom 16:9 cinematic illustrations for all 6 narrative endings.
- RU: Захват лучей горизонтом событий второй чёрной дыры в `bh.frag` (устранено белое пятно), вращение пары за 12.5с, вертикальное разделение плашек радиопереговоров и экспедиции, пауза корабля и физики при показе сюжета, кинематографичные 16:9 иллюстрации для всех 6 концовок сюжета.

### v4.7 — Station Hubs & RP Economy / Станции-хабы и экономика RP
- Status / Статус: **shipped in 4.7.0** / **выпущено в 4.7.0**.
- EN: Interior docking vista, Research Points (RP) spending on engineering upgrades (warp coil capacitors, shield matrix, RCS gimbal thrusters), interactive station hub dialog.
- RU: Вид станции изнутри при стыковке, расход очков исследований (RP) на инженерные модули (конденсаторы варп-катушек, матрица щита, маневровые RCS), интерактивный диалог хаба.

### v4.8 — Modular Centrifuge Station & Atmospheric Flight / Модульная центрифуга и атмосфера
- Status / Статус: **shipped in 4.8.0** / **выпущено в 4.8.0**.
- EN: Station artificial gravity centrifuge ring rotation ($\omega = 0.30$ rad/s), solar panels with SARJ sun-tracking, dynamic atmospheric weather, angle of attack entry corridor, aerodynamic drag, and re-entry plasma ionization.
- RU: Вращение жилой центрифуги искусственной гравитации станции ($\omega = 0.30$ рад/с), солнечные батареи с SARJ-слежением за солнцем, динамическая планетарная погода, угол атаки при спуске, аэродинамическое торможение и плазменный кокон входа.

### v4.9 (v4.9.0 – v4.9.4) — Astro-Zettelkasten, Cassette Deck & Avionics / Второй Мозг и кассетная дека
- Status / Статус: **shipped in 4.9.4 (Alpha Stages)** / **выпущено в 4.9.4 (Alpha Stages)**.
- EN: Dynamic associative Zettelkasten thought graph with Coulomb-Hooke layout, 3 Eureka synthesis models, [[Wikilinks]] parser with UTF-8 Russian/English search, Obsidian vault export, analog Cosmic Ambient cassette deck with twin spinning reels, and Voyager-1 interstellar relic beacon.
- RU: Динамический ассоциативный граф мыслей Zettelkasten с физической раскладкой Кулона-Гука, 3 модели синтеза Эврики, парсер [[Вики-ссылок]] с поиском RU/EN, экспорт базы в Obsidian, аналоговая кассетная дека Cosmic Ambient со вращающимися катушками ленты и реликтовый маяк «Вояджер-1».

## Stage 5 — Grand Universe & Release 1.0 / Великая Вселенная и Release 1.0 (v5.0.0 – v5.0.5)

### v5.0.0 — Release 1.0: Grand Universe & Avionics
- Status / Статус: **shipped in 5.0.0 (Release 1.0)** / **выпущено в 5.0.0 (Release 1.0)**.
- EN: 22 celestial stations (including GW170817 kilonova, SN 1987A supernova, AT2019qiz TDE, M16 Pillars of Creation), 8 branching Multiverse endings with 16:9 cinematic illustrations and resonance badges, Zero-UI flight HUD with dynamic alpha fade, Unified Ship OS Terminal with 5 tabs, 3D Holographic Celestial Orrery (`M`) with Keplerian orbits, planetary landing avionics (vertical speed variometer ladder, 8x5 terrain wireframe grid, slope hazard warning > 15°), core drill sampling (+250 RP), full Gamepad & Steam Deck integration with radial stick deadzones and button glyphs, Cook-Torrance GGX microfacet ocean glints, Rayleigh-Mie atmospheric scattering, cloud shadows, and far-field geodesic acceleration.
- RU: 22 космические станции и объекта (включая килонову GW170817, сверхновую SN 1987A, приливной разрыв AT2019qiz, Столпы Творения M16), 8 концовок Мультивселенной с 16:9 иллюстрациями и бейджами резонанса, полетный Zero-UI HUD с плавным затуханием, Единый Терминал корабля на 5 вкладок, 3D-карта звездной системы Оррери (`M`) с кеплеровскими орбитами, посадочная авионика (вариометр спуска, радарная проволочная сетка рельефа 8×5, алерт опасного уклона > 15°), бур керна (+250 RP), нативная поддержка геймпадов и Steam Deck, микрофасеточные блики океанов Cook-Torrance GGX, рассеяние Рэлея-Ми, тени облаков и ускорение трассировки лучей.

### v5.0.1 – v5.0.5 — Stability, Voice Polish & EGM Remediations
- Status / Статус: **shipped (v5.0.1–v5.0.4) & in progress (v5.0.5)**.
- EN: 24 voiced crew cues and dialogues verified with OpenAI Whisper (0% hallucinations); timbre polish for Dr. Elena Rostova (deep mature astrophysicist, -1.5 semitones F0) and pilot Mia Sato (lively banter and humor); elimination of VRAM texture leaks in ObjModel via texture sharing; complete synchronization of ARCHITECTURE.md and README.md; Linux runner CI stabilization with Ubuntu 24.04 LTS and DNS fallback.
- RU: 24 озвученные реплики экипажа со 100% верификацией Whisper (0% галлюцинаций); тембральная полировка голоса д-ра Елены Ростовой (-1.5 полутона F0) и живые реплики пилота Мии Сато; устранение утечки VRAM в ObjModel через разделяемые текстуры; полная синхронизация ARCHITECTURE.md и README.md; стабилизация Linux CI раннеров на Ubuntu 24.04 LTS с DNS-фоллбэком.

## Stage 6 — Exoplanetary Biomes & Surface Hydrology / Экзопланетарные биомы и гидродинамика (v5.1.0)
- Status / Статус: **planned (Stage 6)** / **запланировано (Stage 6)**.
- EN: Procedural continental biomes for terraformed planets (cryovolcanic plains, basalt canyons, hydrocarbon lakes, turquoise archipelagos of Hercules-b); dynamic ocean shoreline froth and water wake upon low-altitude flight; autonomous deployable exploration science drone/rover for scouting anomalies within a 10 km radius of landing touchdown.
- RU: Процедурные континентальные биомы для терраформированных планет (криовулканические равнины, базальтовые каньоны, метановые озера, бирюзовые архипелаги Hercules-b); динамическая пена прибоя и кильватерный след при бреющем полете над водой; автономный исследовательский дрон/ровер для разведки аномалий в радиусе 10 км от места посадки.
- Gates: шейдер биомов без просадки фреймрейта (< 0.5 мс на кадр), физика плавучести и приводнения, тест отстыковки и автономного возвращения дрона.

## Stage 7 — Multi-Threaded Rendering & SPSC Lock-Free Audio / Многопоточный рендеринг и бессерверный звук (v5.2.0)
- Status / Статус: **planned (Stage 7)** / **запланировано (Stage 7)**.
- EN: AudioEngine migration to a Single-Producer Single-Consumer (SPSC) lock-free ring buffer, eliminating WASAPI/ALSA mutex contention with the render thread at 144+ Hz; Render Hardware Interface (RHI) modular abstraction layer preparing native Vulkan 1.3 and Metal backends beyond OpenGL 3.3; compute shader path-tracing for rotating Kerr black hole ergospheres with extreme spin $a \to 0.998$.
- RU: Перевод AudioEngine на безблокировочный кольцевой буфер SPSC (устранение mutex contention между WASAPI/ALSA и рендер-потоком OpenGL на частотах 144+ Гц); модульный слой абстракции RHI (Render Hardware Interface) с подготовкой бэкендов Vulkan 1.3 и Metal в дополнение к OpenGL 3.3; численная трассировка путей для эргосферы вращающейся черной дыры Керра с экстремальным спином $a \to 0.998$.
- Gates: нулевой оверхед локов в профилировщике на 240 Гц, кроссплатформенная сборка RHI, верификация гравитационного фрейм-дрэггинга Лензе-Тирринга.

## Stage 8 — Galactic Beacon Network & Async Co-op / Сеть маяков и асинхронный кооператив (v5.3.0)
- Status / Статус: **planned (Stage 8)** / **запланировано (Stage 8)**.
- EN: Asynchronous multiplayer: deployable Quantum Eureka Relays and beacons on planetary landing sites; cryptographic P2P hash sharing of Zettelkasten knowledge graphs, custom player notes, and star charts; dynamic deep space emergent events (automated distress beacons, drifting relic probes, rogue asteroid deflections).
- RU: Асинхронный мультиплеер: установка квантовых ретрансляторов и маяков Эврики в точках высадки; криптографический P2P-обмен графами мыслей Zettelkasten, заметками пилота и звездными картами; динамические события глубокого космоса (сигналы бедствия SOS, дрейфующие реликтовые зонды, перехват опасных астероидов).
- Gates: детерминизм сериализации маяков, устойчивость P2P-пакетов без центрального сервера, тесты генератора событий.

## Stage 9 — Beyond the Singularity: Mod-API & Steam Workshop / За Сингулярностью: Мод-API и Мастерская (v6.0.0)
- Status / Статус: **future target (v6.0.0)** / **перспективная цель (v6.0.0)**.
- EN: Embedded Lua/WASM scripting runtime for community-driven celestial bodies, custom spacecraft hulls, custom GLSL shaders, and procedural narrative quests; Steam Workshop integration with one-click subscription, automated asset sandboxing, and runtime hot-reloading.
- RU: Встроенная среда выполнения Lua/WASM для пользовательских небесных тел, корпусов кораблей, GLSL-шейдеров и сюжетных квестов; интеграция Steam Workshop с установкой модов в один клик, песочницей безопасности ассетов и горячей перезагрузкой модификаций на лету.
- Gates: изоляция песочницы Lua/WASM (отсутствие доступа к ФС за пределами мода), обратная совместимость сохранений с модами, валидатор мод-паков в CI.

## Process rules / Правила процесса
- Цикл: один фича-пасс → гейты → один коммит → релиз каждые 2–3 пасса (текущий ритм).
- Harness byte-identity и static checks — ядро регрессий; каждая стадия добавляет свои гейты.
- Semver + двуязычный CHANGELOG; тег `v*` запускает CI-релиз (AppImage + windows zip).
- Ничего не попадает в релиз без кадра в харнессе (для визуала) и без теста (для логики).
