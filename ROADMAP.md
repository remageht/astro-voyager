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
- EN: The game splits into two modes. **Sandbox** = today's free exploration (unrestricted, separate save profile). **Adventure** = story campaign (prototype premise: the Sun is dying — telescopes confirm the star will not survive; humanity has one window left, and we are sent as the expedition to find a new habitable planet before the deadline. The countdown is the meta-mechanic: every detour costs time, the galaxy map hides candidates that must be scouted, analyzed and chosen). 6–8 chapters with cine scenes (reuse Tour/WarpJump), finale and credits. Mode select on start; saves are per mode.
- RU: Игра делится на два режима. **Песочница** = текущий свободный полёт (без ограничений, отдельный профиль сохранения). **Приключение** = сюжетная кампания (прототипный замысел: Солнце умирает — наблюдения подтверждают, что звезда не выживет; у человечества остаётся одно окно, и нас отправляют в экспедицию — найти новую пригодную планету до срока. Обратный отсчёт — мета-механика: любой крюк стоит времени, карта галактики скрывает кандидатов, которых нужно разведать, проанализировать и выбрать). 6–8 глав с катсценами (движки Tour/WarpJump), финал и титры. Выбор режима на старте; сохранения раздельные по режимам.
- Gates: тесты стейт-машины глав и таймера кампании, раздельные профили сохранений, скриптованный полный прогон (smoke).

## Stage 3 — Production values / Продакшн-ценности (v4.1–v4.5)

### v4.1 — Cosmic events
- Supernova, tidal disruption flares, gravitational-wave ripples (visual + gameplay hooks).

### v4.2 — Cockpit & damage v2
- 3D cockpit interior, module damage states, sparks/smoke, repair animations.

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
