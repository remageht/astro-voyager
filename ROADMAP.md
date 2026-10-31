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
| **Alpha Stage** | `v4.2.0` | Диегетический 3D-кокпит, коллиматорный ИЛС, динамика перегрузок, SpaceDrive 3D варп-туннель, авионика | **Текущий релиз (Current)** |
| **Beta Stage** | `v4.3.0` – `v4.5.0` | Процедурные PBR-поверхности планет (LOD), Рэлей-Ми атмосфера, объёмные туманности, баланс экономики | В разработке (Planned) |
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

## Stage 3 — Production values / Продакшн-ценности (v4.1–v4.5)

### v4.1 — Cosmic events
- Supernova, tidal disruption flares, gravitational-wave ripples (visual + gameplay hooks).

### v4.2 — 3D Cockpit, SpaceDrive Warp Tunnel & Avionics / 3D-кокпит, варп-туннель SpaceDrive и авионика
- Status / Статус: **shipped in 4.2.0 (Alpha Stage)** / **выпущено в 4.2.0 (Alpha Stage)**.
- EN: Shipped: diegetic 3D cockpit with canopy struts and photochromic tinting, collimated holographic HUD at infinity (pitch ladder, boresight, horizon, velocity tapes), pilot head spring-damper G-force inertia, animated multi-layer 3D SpaceDrive warp tunnel (tamminen, CC BY 4.0) with differential rotation during warp cruise and jump flight, modernized multi-tab Settings menu.
- RU: Выпущено: диегетический 3D-кокпит с лонжеронами и фотохромным автотинтом, коллиматорный голографический ИЛС (авиагоризонт, шкала тангажа, ленты скорости), пружинно-демпферная инерция головы пилота при перегрузках, анимированный трёхслойный 3D варп-туннель SpaceDrive (tamminen, CC BY 4.0) с дифференциальным вращением в прыжках и крейсерском варпе, модернизированное многовкладочное меню настроек авионики.

### v4.3 — Audio pass
- Layered dynamic music on events, positional station audio, engine Doppler pitch by speed.

### v4.4 — Cinematography
- Director camera, photo mode v2 (LUT/filters, pose the ship), clip recorder.

### v4.5 — Image quality
- SMAA/TAA, HDR output, filmic LUT grading, lens effects polish.

## Stage 4 — Ecosystem / Экосистема (v5.0+)
- EN: Mod API (data-driven stations/ships/shaders) + workshop, Steam achievements/cloud saves, Windows installer + auto-update, Steam Deck verification, 5+ language localization, demo/trailer/press kit.
- RU: Мод-API (станции/корабли/шейдеры как данные) + мастерская, достижения/облачные сейвы платформы, инсталлятор для Windows + автообновление, верификация Steam Deck, локализация на 5+ языков, демо/трейлер/пресс-кит.

## Process rules / Правила процесса
- Цикл: один фича-пасс → гейты → один коммит → релиз каждые 2–3 пасса (текущий ритм).
- Harness byte-identity и static checks — ядро регрессий; каждая стадия добавляет свои гейты.
- Semver + двуязычный CHANGELOG; тег `v*` запускает CI-релиз (AppImage + windows zip).
- Ничего не попадает в релиз без кадра в харнессе (для визуала) и без теста (для логики).
