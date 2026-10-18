# CHANGELOG

## 3.8.0 - 2026-10-18

## What's new / Что нового
- EN: Difficulty affects hazards - the debris-strike damage scales with the difficulty preset (Easy softer, Hard harder, Normal unchanged); the tuning struct gains hazardK and the unit tests now cover 6 difficulty multipliers.
- EN: Ghost-jump fix - teleporting while a warp jump is mid-flight (loading a save via F10/menu, importing a route via F6, menu destinations, expedition jumps) now cleanly aborts the jump cinematic; the camera-spring and warpdrive overlays reset with it. The Burst's internal teleport explicitly keeps its own jump running.
- EN: Regression coverage - two new static checks (hazard scaling wired, load-external-teleport aborts the cine) on top of the difficulty unit tests; logic-only pass, harness refs untouched except the menu version footer.
- RU: Сложность влияет на угрозы - урон от столкновения с обломками масштабируется пресетом сложности (Easy мягче, Hard злее, Normal без изменений); в тюнинге появился hazardK, юнит-тесты покрывают 6 множителей сложности.
- RU: Фикс «призрачного прыжка» - телепорт посреди варп-прыжка (загрузка сейва по F10/из меню, импорт маршрута по F6, пункты меню, экспедиция) теперь корректно прерывает катсцену прыжка; камера-пружина и warpdrive-оверлеи сбрасываются вместе с ней. Внутренний телепорт Бурста явно оставляет собственный прыжок работать.
- RU: Регрессионное покрытие - два новых статик-чека поверх юнит-тестов сложности; чисто логический пасс, рефересы харнесса не тронуты, кроме строки версии в меню.
- Gates / Гейты: ctest 6/6, static checks all pass, GL build OK, harness refs: menu.png updated (planned - version footer), rest byte-identical.

## 3.7.0 - 2026-10-18

## What's new / Что нового
- EN: Three new achievements — colonist (first colony founded), architect (both colony sites: Mars and Hercules b), debris dancer (a successful dodge in a warp-jump debris field); colony presses and dodges now refresh trophies immediately; the panel header counts to 10.
- EN: Progress summary — the status panel gains a Progress section above achievements: colonies founded (N/2), debris dodged (lifetime counter), event horizon crossed (yes/no), stations visited.
- EN: Persistence — the lifetime dodge counter is saved in save-sandbox.json progress (asteroidDodges) and restored on load; older saves without the field start at 0.
- EN: Roadmap refresh — docs/ROADMAP.md now documents the shipped v0.3..v3.7 passes and the actual backlog (v4.0 adventure slot, offline science demos).
- RU: Три новые ачивки — «Первопроходец» (первая колония), «Архитектор сети» (оба колониальных сайта: Марс и Hercules b), «Танцор обломков» (успешный манёвр в поле обломков варп-прыжка); основание колонии и манёвры обновляют трофеи сразу; счётчик в панели доходит до 10.
- RU: Сводка прогресса — в панели над ачивками появилась секция Progress: основанные колонии (N/2), уклонения от обломков (жизненный счётчик), пересечение горизонта (да/нет), посещённые станции.
- RU: Сохранение — жизненный счётчик уклонений пишется в прогресс save-sandbox.json (asteroidDodges) и восстанавливается при загрузке; старые сейвы без поля стартуют с нуля.
- RU: Обновление роадмапа — docs/ROADMAP.md теперь отражает реализованные пассы v0.3..v3.7 и актуальный бэклог (слот адвенчура v4.0, офлайн научные демо).
- Gates / Гейты: ctest 6/6, static checks all pass, GL build OK, harness refs: gui.png + menu.png updated (planned — new Progress section and the version footer), rest byte-identical.

## 3.6.0 - 2026-10-18

## What's new / Что нового
- EN: Colonies — found a colony on Mars (Sol) and Hercules b (M13): the approach hint [C], a gold ring marker near the founded site, colony names saved in the profile and restored on load.
- EN: Ship inertia + orbit assist — inertial flight (thrust spools in ~3/s, decays ~0.9/s, the ship keeps its course after releasing the keys); near a massive center a tangential coast smoothly settles into a circular orbit (assist window 0.55..2.2 of circular speed); covered by a 7-case unit test.
- EN: Controls page — mouse sensitivity (0.3..3.0) and Y inversion, five rebindable hotkeys (colony, view, map, journal, tour) with on-screen key capture; everything persisted in settings.json.
- EN: Event-horizon interaction — the quasar core and the black-hole horizon no longer slide the ship past a wall: both run the 3.2 s capture sequence (tidal stretch and lens pop, plunge with a white flash, teleport to the entry point, shields -30 down to a floor of 5, ejection), each with its own messages; the state resets on save-load and respawn.
- EN: Asteroid barrage (Travel) — about a quarter of warp jumps fly into a debris field: a warning, dodge by holding D/A (at least 0.25 s of steady input) inside the danger window, otherwise a strike takes 18 shield points; a conveyor of streak-rocks sweeps along the course and shifts against the input so the maneuver reads on screen; deterministic roll, one-shot events, full reset between jumps, unit-tested.
- EN: Solar-size fix — star billboards are clamped in pixels (u_MaxPx), Hercules Prime is the bright warm core star of M13 rendered as a billboard, not a bloated planet sphere; Hercules b is a small exoplanet (radius 0.22, orbit 4.6) so nothing looks bigger than the cluster; the quasar/nebula imposters are anchored to the world (they no longer slide with the camera).
- EN: HUD fix — while the expedition slideshow is open, the station card, top bar, trophies and hints are hidden, so nothing overlaps the chapter art.
- RU: Колонии — основание колонии на Марсе (sol) и Hercules b (m13): подсказка [C] рядом с телом, золотое кольцо-маркер у основанного сайта, названия колоний сохраняются в профиль и восстанавливаются при загрузке.
- RU: Инерция корабля + орбитальный ассист — инерционный полёт (тяга набирается ~3/с, спадает ~0.9/с, корабль сохраняет ход после отпускания клавиш); у массивного центра тангенциальный дрейф плавно укладывается в круговую орбиту (окно 0.55..2.2 круговой скорости); покрытие юнит-тестом на 7 кейсов.
- RU: Страница управления — чувствительность мыши (0.3..3.0) и инверсия Y, пять переназначаемых клавиш (колония, вид, карта, журнал, тур) с захватом на экране; всё сохраняется в settings.json.
- RU: Горизонт событий — ядро квазара и горизонт ЧД больше не «скользят» мимо стены: оба запускают катсцену захвата на 3.2с (прилив и линза, провал со вспышкой, телепорт к точке входа, щит -30 до пола 5, выброс) со своими текстами; состояние сбрасывается при загрузке и респауне.
- RU: Астероидное поле в Travel — около четверти прыжков упирается в обломки: предупреждение, манёвр удержанием D/A (минимум 0.25с непрерывного ввода) в окне опасности, иначе удар на 18 единиц щита; конвейер стрик-камней движется вдоль курса и сдвигается против ввода, так что манёвр читается на экране; детерминированный ролл, одноразовые события, полный сброс между прыжками, юнит-тесты.
- RU: Фикс размеров — билборды звёзд получают потолок в пикселях (u_MaxPx), Hercules Prime рисуется яркой тёплой точкой ядра M13, а не раздутой сферой-планетой; Hercules b — маленькая экзопланета (радиус 0.22, орбита 4.6), ничего не выглядит крупнее скопления; импостеры квазара и туманности привязаны к миру (не «плывут» с камерой).
- RU: Фикс HUD — при открытых слайдах экспедиции карточка станции, верхняя панель, трофеи и подсказки скрываются и не накладываются на арт главы.
- Gates / Гейты: ctest 6/6, static checks all pass, GL build OK, harness refs: only m13.png updated (planned); windows-x86_64 zip and Linux AppImage attached by CI.

## 3.5.0 - 2026-10-17

## What's new / Что нового
- EN: Visual realism pass — reworked starfield (hash-based layers, banding gone), a sharper Milky Way band with a dust lane, physically brighter lensed accretion disks (black hole and quasar), refreshed README hero shots (sgra/m87).
- EN: Ship's journal — a bilingual story journal (key J): 10 entries unlocked by real progress (first jump, visited objects, horizon crossed, close approach, route archive, grand tour), an unread counter in the status bar, read-state preserved across sessions; 10 unit tests.
- EN: Live cosmos main menu — a fullscreen shader backdrop (nebula, Milky Way band, three parallax star layers, vignette) behind a glass panel with a gold title, main button and destination grid; menu.png hero reference in the harness.
- EN: Expedition mode — the story campaign you asked for: an intro cinematic told in 7 illustrated slides (Mia Sato dreams of space, the Sun starts dimming, humanity sets out to find a new home), then a 6-chapter chain across Sgr A*, M87*, 3C 273 and M13 with finish-of-chapter slides, an in-game chapter card, a chapter map (key N), expedition state saved to expedition.json; new Intro reference frame; the menu now offers "Begin Expedition" and "Free Visit" (free flight).
- RU: Пасс реализма — переработанное звёздное поле (hash-слои, полосы убраны), более чёткая полоса Млечного Пути с пылевой дорожкой, физически более яркие линзированные диски (ЧД и квазар), обновлённые hero-кадры README (sgra/m87).
- RU: Бортовой журнал корабля — двуязычный сюжетный журнал (клавиша J): 10 записей, открываются реальным прогрессом (первый прыжок, посещённые объекты, пересечение горизонта, близкий проход, архив маршрута, гранд-тур), счётчик непрочитанного в статус-баре, прочитанность сохраняется между сессиями; 10 юнит-тестов.
- RU: Живое космическое меню — фуллскрин-шейдерный фон (туманность, полоса Галактики, три параллакс-слоя звёзд, виньетка) за стеклянной панелью с золотым заголовком, главной кнопкой и сеткой направлений; hero-референс menu.png в харнессе.
- RU: Режим экспедиции — сюжетная кампания по вашему запросу: вводная катсцена из 7 иллюстрированных слайдов (Мия Сато мечтает о космосе, Солнце начинает гаснуть, человечество отправляется искать новый дом), затем цепочка из 6 глав по Sgr A*, M87*, 3C 273 и M13 со слайдами-итогами, карточка главы в игре, карта глав (клавиша N), состояние сохраняется в expedition.json; новый референс-кадр intro.png; в меню — «Начать экспедицию» и «Свободный визит» (свободный полёт).
- Gates / Гейты: ctest 6/6, static checks all pass, screenshots 17/17 byte-identical + menu.png/intro.png refs, doppler 3.111 PASS.

## 3.4.0 - 2026-10-16

## What's new / Что нового
- EN: macOS support — the full OpenGL app now builds on Apple Silicon: system OpenGL 3.3 core via <OpenGL/gl3.h> (src/GLCompat.h makes GLEW calls no-ops, so Window.cpp stays shared), Cocoa/IOKit/CoreVideo/CoreAudio frameworks, macOS font paths; new CI job build-macos-gl verifies the linkage with otool (OpenGL.framework present, no GLEW), smokes the CLI and ships an astro-voyager-macos-arm64.tar.gz artifact, attached to releases on tags.
- EN: Save profiles — human-readable pretty JSON in the user directory (Windows %APPDATA%, Linux ~/.config, macOS Application Support): settings.json (perf preset, volume/mute, camera view) plus save-sandbox.json (progress, achievements, route history, editor objects with exact ids, session: station, camera, time pace, fuel/shields, ship style); adventure slot reserved for v4.0. F9/F10 quicksave/quickload, Save/Load menu buttons, silent autosave on exit; versioned schema, unknown fields ignored; own dependency-free mini JSON parser + serializer in pure C++17 (builds in the core target, covered by 12 round-trip tests).
- RU: Поддержка macOS — полное OpenGL-приложение собирается на Apple Silicon: системный OpenGL 3.3 core через <OpenGL/gl3.h> (src/GLCompat.h превращает вызовы GLEW в no-op, Window.cpp остаётся общим), фреймворки Cocoa/IOKit/CoreVideo/CoreAudio, macOS-пути шрифтов; новая CI-джоба build-macos-gl проверяет линковку через otool (OpenGL.framework есть, GLEW нет), гоняет CLI-смоук и собирает артефакт astro-voyager-macos-arm64.tar.gz — на тегах прикрепляется к релизу.
- RU: Профили сохранений — человекочитаемый pretty JSON в пользовательской директории (Windows %APPDATA%, Linux ~/.config, macOS Application Support): settings.json (пресет производительности, громкость/мьют, вид камеры) и save-sandbox.json (прогресс, ачивки, история маршрута, объекты редактора с точными id, сессия: станция, камера, темп времени, топливо/щиты, стиль корабля); слот сюжетного режима зарезервирован под v4.0. F9/F10 — быстрое сохранение/загрузка, кнопки Save/Load в меню, тихий автосейв при выходе; схема версионируется, неизвестные поля игнорируются; собственный мини-парсер JSON без зависимостей на чистом C++17 (собирается в core-таргете, 12 round-trip тестов).
- Gates / Гейты: ctest 4/4, static checks 234, screenshots 17/17, doppler 3.111 PASS.

## 3.3.0 - 2026-10-16

## What's new / Что нового
- EN: Ship survival mode — fuel (a jump costs 5, refuel near stars), shields (pulsar radiation 10/s), Newtonian gravity drift from stations and planets, emergency respawn at the Sun; HUD fuel/shield bars plus low-shield chromatic aberration and interference FX.
- EN: Speed effect and camera views — real speed and warp factor now drive the relativistic Doppler light shift, radial motion streaks and a micro-FOV boost (the warp tunnel never moves the camera, hence the boost); warp streak lines are stable at high speed; V cycles cockpit / behind-the-tail / far-overview views with the ship rendered in frame.
- EN: Flight feel — procedural engine hum tied to speed, thruster-scaled trail ribbon and engine glow, cold-blue shield impact flash on damage.
- RU: Режим выживания — топливо (прыжок −5, дозаправка у звёзд), щиты (радиация пульсаров 10/с), ньютоновский гравитационный дрейф у станций и планет, аварийный респавн у Солнца; HUD-полосы топлива/щитов + хроматика и помехи при низком щите.
- RU: Эффект скорости и виды камеры — реальная скорость и варп-фактор управляют доплер-сдвигом света, радиальными стриками и микро-FOV (варп-туннель не двигает камеру — отсюда буст); стрики варп-туннеля стабильны на высокой скорости; V переключает кабину / за кормой / дальний обзор, корабль виден в кадре.
- RU: Ощущения полёта — процедурный гул двигателя по скорости, шлейф и свечение сопла от тяги, холодно-синяя вспышка щита при уроне.
- Gates / Гейты: ctest 3/3, static checks 224, doppler 3.111 PASS.

## 3.2.0 - 2026-10-16

## What's new / Что нового
- EN: Black-hole/quasar polish — filament strand texture on the accretion disk, a collimated M87-style jet (cone + spine, 4 knots, asymmetric 0.45 counter-jet); quasar lensed secondary arcs, wider knots, brighter host core; Sgr A* orbital hot-spot flare (close-range only, harness-safe gate).
- EN: Solar System beautification — Saturn ring shadow on the planet and planet eclipse on the rings, Galilean moons + Titan with moon-transit shadows (u_MoonOcc), Earth ocean glint and night city lights, asteroid belt densified to 5200, faint vertical Uranus rings, Pluto on an inclined eccentric Kuiper orbit.
- EN: Close-range surface detail — rocky, moon and gas-giant surfaces gain two extra noise octaves that fade in with camera proximity (nearK gate; far screenshots unchanged); dedicated sol/earth close-up harness frames.
- EN: Flexible object editor v2 — palette of 5 types (rock, rocky planet, moon, gas giant, star) with size, two-colour palette, seed and an optional ring; click-select, live edit of the selected object, delete, clear, collisions (Solar System station).
- RU: Полировка ЧД и квазара — филаментная текстура диска, коллимированный джет M87-типа (конус + spine, 4 узла, контр-джет 0.45); линзированные дуги квазара, крупнее узлы, ярче ядро; орбитальная вспышка-хотспот Sgr A* (только вблизи, гейт для харнесса).
- RU: Красоты Солнечной системы — тень колец Сатурна на планете и затмение колец планетой, галилеевы луны + Титан с транзитными тенями (u_MoonOcc), блик океана и ночные огни городов Земли, пояс уплотнён до 5200, слабые вертикальные кольца Урана, Плутон по наклонной эксцентрической орбите.
- RU: Детализация поверхностей вблизи — скалы, луны и газовые гиганты получают два дополнительных шумовых слоя, проявляющихся при приближении камеры (гейт nearK; дальние кадры не меняются); добавлены хелпер-кадры sol_close/earth_close.
- RU: Гибкий редактор объектов v2 — палитра из 5 типов (камень, скалистая планета, луна, газовый гигант, звезда) с размером, двумя цветами палитры, seed и опциональным кольцом; выбор кликом, живое редактирование выбранного, удаление, очистка, коллизии (станция Solar System).
- Gates / Гейты: ctest 3/3, static checks 209, doppler 3.111 PASS.

## 3.1.0 - 2026-10-14

## What's new / Что нового
- EN: Black hole max-out — tertiary photon sub-ring (n=3) and a compact NT-weighted ISCO corona (axisymmetric, beaming math untouched); quasar maxi-out — ejected shock blob every 9 s plus a slow core flicker mode, both exactly zero at the harness epoch (qso-3c273.png byte-identical).
- EN: Deep-field zoom — volumetric approach LOD (M31 pattern) for m51/m104/mw with named hero stars and double-click fly-to in every galaxy; Milky Way station with the Sun hero and auto-jump home to sol within 3 units; Hercules Cluster M13 (new StarCluster type, Plummer ball, fly between stars).
- RU: Максимум ЧД — третичное фотонное суб-кольцо (n=3) и компактная NT-взвешенная корона ISCO (осесимметрична, бимінг не тронут); максимум квазара — вылетающий blob каждые 9 с и медленный flicker ядра, оба строго нулевые в эпохе харнесса (qso-3c273.png байт-в-байт).
- RU: Дальний зум — объёмный LOD сближения (образец M31) для m51/m104/mw с именными hero-звёздами и долётом по double-click в каждой галактике; станция Млечный Путь с героем-Солнцем и автопрыжком домой в sol ближе 3 единиц; скопление Геркулеса M13 (новый тип StarCluster, сфера Пламмера, полёт между звёздами).
- Gates / Гейты: ctest 3/3, static checks 187, doppler 2.962 PASS.

## 3.0.0 - 2026-10-14

## What's new / Что нового
- EN: True HDR pipeline — scene FBO and 5-level bloom chain upgraded RGBA8 to RGBA16F (resize recreation kept); ACES tonemap + gamma + grain moved from 9 station shaders into the single composite (post_comp.frag); display-referred sprite/clear colors decoded to linear (pow 2.2); scene target cleared to deterministic black every frame.
- EN: Pulsar family — Crab Pulsar (33 ms rotator, pairs with station m1), magnetar SGR 1806-20 (amber flare halo, giant magnetosphere loops), Hulse-Taylor PSR B1913+16 (orbital decay proved gravitational waves); per-station presets (u_SpinRate/MagTilt/BeamTint/HaloGain/ArcScale); pulsar tick SFX with the true Vela period of 89 ms (Crab x2, magnetar /8 for audibility; visual spin stays artistic).
- RU: Настоящий HDR-конвейер — scene FBO и 5-уровневая bloom-цепочка переведены с RGBA8 на RGBA16F (пересоздание при resize сохранено); тонмаппинг ACES + гамма + зерно вынесены из 9 станционных шейдеров в единый композит (post_comp.frag); display-referred цвета спрайтов и clear декодированы в linear (pow 2.2); таргет чистится в детерминированный чёрный каждый кадр.
- RU: Пульсарная семья — Crab Pulsar (ротатор 33 мс, пара к станции m1), магнетар SGR 1806-20 (янтарное гало вспышек, гигантские дуги), Халс-Тейлор PSR B1913+16 (распад орбиты доказал гравитационные волны); per-station пресеты (u_SpinRate/MagTilt/BeamTint/HaloGain/ArcScale); тик пульсара с настоящим периодом Vela 89 мс (Crab x2, магнетар /8 для слышимости; визуальный спин художественный).
- Gates / Гейты: ctest 3/3, static checks 180, doppler 3.059 PASS.

## 2.8.0 - 2026-10-14

## What's new / Что нового
- EN: New station — Vela Pulsar (neutron star): tilted dipole beams, lighthouse pulse, magnetosphere arcs, hot polar caps; full-screen impostor billboard with ACES tonemapping and grain.
- EN: Catalog entry `psr-vela` (type `Pulsar`), UI short label «Пульс.», cyan chip, cinema mode name «Пульсар»; station list and teleport search updated.
- RU: Новая станция — Vela Pulsar (нейтронная звезда): наклонённые дипольные лучи, маятниковый пульс, дуги магнитосферы, горячие полюса; полноэкранный импостор-спрайт с ACES-тонмаппингом и зерном.
- RU: Запись каталога `psr-vela` (тип `Pulsar`), короткое имя «Пульс.», бирюзовый чип, «Пульсар» в катсцене; список станций и поиск обновлены.
- Gates / Гейты: ctest 3/3, static checks 173, doppler 1.448 PASS.

## 2.7.0 - 2026-10-14

## What's new / Что нового
- EN: Solid warp-ship hulls — wireframe silhouettes replaced with shaded ellipsoid/torus geometry (Dart, Ring Ship IXS, Shuttle); accretion disk now on by default at black hole stations.
- EN: M87* jet upgrade — longer reach, brighter core, dual helical filaments winding along the axis, denser HST-1 knots, wider radio lobe.
- EN: Einstein-ring lensing — lensed background sky magnified near the photon sphere, making the ring of light around the shadow clearly brighter.
- EN: Photo mode (F8) — hides all UI, exposure slider, Save PNG to docs/photos/; auto-orbit (O) circles the camera around the current station, WASD exits.
- EN: Gravitational time-dilation clock in the status bar at black hole stations — ship-clock rate dτ/dt = sqrt(1−Rs/r) with live distance in Rs.
- RU: Сплошные корпуса варп-корабля — каркасные силуэты заменены на отрисованные эллипсоиды/торы (Dart, Ring Ship IXS, Shuttle); аккреционный диск ЧД включён по умолчанию.
- RU: Апгрейд джета M87* — большая длина, ядро ярче, две геликоидальные нити вдоль оси, плотнее узлы HST-1, шире радио-лоб.
- RU: Линзирование кольца Эйнштейна — линзованный фон усилен у фотонной сферы, кольцо света вокруг тени заметно ярче.
- RU: Фото-режим (F8) — скрывает UI, слайдер exposure, Save PNG в docs/photos/; автоорбита (O) кружит камеру вокруг станции, WASD выходит.
- RU: Часы гравитационного замедления в статус-баре на ЧД — ход корабельных часов dτ/dt = sqrt(1−Rs/r) с живой дистанцией в Rs.
- Gates / Гейты: ctest 3/3, static checks 167, doppler 1.448 PASS.

## 2.6.0 - 2026-10-14

## What's new / Что нового
- EN: Two new galaxy stations — Whirlpool (M51, wide arms, bright bulge, companion NGC 5195) and Sombrero (M104, edge-on dust lane, giant bulge); galaxy shader parameterized per-station.
- EN: PostFX pipeline — scene FBO, 5-level bloom chain, HDR composite with texelFetch and ACES; black hole ring now glows.
- EN: Relativistic light shift at high speed — doppler tint (blue ahead, red behind) and starlight aberration toward the direction of motion; active during warp jumps and fast flight.
- EN: Switchable warp-ship hulls — Dart, Ring Ship (IXS Enterprise style with Alcubierre rings), Shuttle — selectable in the warp panel; engine glow is now red.
- RU: Две новые галактики — Whirlpool (M51, широкие рукава, яркий балдж, компаньон NGC 5195) и Sombrero (M104, пылевая полоса по краю, крупный балдж); шейдер галактик параметризован per-station.
- RU: PostFX-конвейер — scene FBO, 5-уровневая цепочка bloom, HDR-композит с texelFetch и ACES; кольцо ЧД теперь светится.
- RU: Релятивистский сдвиг света на больших скоростях — доплер-тинт (вперёд синий, назад красный) и аберрация звёзд к направлению движения; активен в варп-прыжках и быстром полёте.
- RU: Сменяемые корпуса варп-корабля — Dart, Ring Ship (стиль IXS Enterprise с кольцами Алькубьерра) и Shuttle — выбор в варп-панели; свечение движка красное.
- Gates / Гейты: ctest 3/3, static checks 160, glslang OK, doppler 1.419 PASS.

## 2.5.0 - 2026-10-13

- **Кинематографичный варп-прыжок**: state machine Charge/Pullback/Burst/Travel/Arrival/Pushin/Cooldown, пружинная third-person камера, корабль-силуэт (контур + движок + шлейф, ноль ассетов), ударная волна u_Burst в warp.frag, смена станции в пик вспышки.
- **Звук**: процедурный движок на miniaudio (генеративный эмбиент Am-F-C-G + синтез SFX: клики, whoosh, рокот варпа с дакингом, бум прыжка, перезвон ачивок, сирена горизонта), M — мьют.
- **Современный UI**: стеклянная тема, карточки станций, транспортная панель, статус-бар; GL-приложение без консольного окна; FPS в харнессе запинен (gui.png стабилен).
- **Вращение и физика**: дифференциальный свирл дисков ЧД/квазара, ускоренные S-звёзды, осевое вращение планет, far 1e7 с нестираемыми маркерами, скользящие столкновения, катсцена захвата горизонтом + ачивка.
- **Quick wins визуала**: горячий обод ISCO, беление пика фотонного кольца, HDR-ядро M31 с экстинкцией пыли, асимметричный хост квазара.
- **Гейты**: ctest 3/3, 152 static checks, glslang чист, доплер 2.899 (порог 1.3).

## 2.4.0 - 2026-10-08

- **Метрика Керра — CPU-модуль нулевых геодезических** (последний пункт roadmap):
  - Новый модуль `src/Kerr.h` / `src/Kerr.cpp`: экваториальные нулевые геодезические Керра в координатах Бойера-Линдквиста через радиальный потенциал `R(r) = [r² + a(a−b)]² − Δ(b−a)² = r⁴ + (a²−b²)r² + 2Mr(b−a)²` и уравнение 2-го порядка `d²r/dλ² = R'/(2r⁴) − 2R/r⁵` (свободно от сингулярности горизонта Δ=0); захват по горизонту `r₊ = M + √(M²−a²)`.
  - Аналитика критического параметра: `kerrPhotonSphereRadius` (формула Бардена 1972 `r_ph = 2M[1 + cos(⅔ arccos(∓a/M))]`) и `kerrBCrit` (Барден 1973 / Чандрасекар 1983), лимиты: a=0 → 3√3 M, a→M проград → 2M, a→M ретроград → 7M.
  - Новый CLI-тест `--demo-kerr` с тремя разделами: (A) лимит a→0 против Schwarzschild-модуля (погрешность ≤ 1.3e-5 при пороге 1e-4 + совпадение сценариев), (B) строгая асимметрия `b_crit(prog) < 3√3/2 Rs < b_crit(retro)` при a=0.9M с динамическими порогами захвата, (C) сверка с литературой: бисекция численного порога против аналитики (diff ≤ 1e-6) и лимиты a→M.
  - Рендерер остаётся Шварцшильдским — Kerr только в CPU-валидации; ограничение задокументировано в README и docs/ARCHITECTURE.md.
  - Статические проверки +2 (`cmake lists Kerr.cpp`, `cli demo-kerr`, итого 92).
- **Документация**: ROADMAP закрыт полностью (все вехи DONE), ARCHITECTURE дополнен уравнениями Керра.

## 2.3.0 - 2026-10-08

- **Linux AppImage (x86_64)**:
  - Полноценный Linux-дистрибутив: GL-сборка (ASTROVOYAGER_ENABLE_GL=ON) + AppDir + type-2 AppImage с бандлингом GLEW/X11-библиотек через linuxdeploy; системные libGL/libGLX и libc/libstdc++ сознательно не бандлятся (excludelist — не ломают GL-стек хоста).
  - Новый CI-job `build-appimage` в `.github/workflows/build.yml`: сборка, упаковка, headless-смоук внутри AppImage (--version / --demo-geodesic / --demo-binet + сверка версии с VERSION и контракт payload), upload-artifact, а на тегах `v*` — автоматический ассет `astro-voyager-vX.Y.Z-x86_64.AppImage` в GitHub Release.
  - `packaging/`: `build-appimage.sh` (AppDir + appimagetool), `fetch-gl-deps.sh` (pinned-выкачка GLFW 3.4 / GLM 1.0.1 / ImGui 1.91.8 / stb, т.к. Dependencies/ и src/vendor/ в gitignored), `astro-voyager.desktop`, `icon.png` 256×256.
  - CMake (Linux-ветка GL): GLFW 3.4 собирается X11-only (`GLFW_BUILD_WAYLAND=OFF`, `GLFW_BUILD_X11=ON`) — без wayland-scanner/xkbcommon/libdecor в рантайме, Wayland покрывается XWayland. Windows/macOS не затронуты.
  - `.gitattributes`: `*.sh text eol=lf` — bash-скрипты не ломаются на CRLF.
  - Static checks +20 проверок упаковки (итого 90): файлы packaging, ключи .desktop, шаги CI-job.

## 2.2.0 - 2026-10-08

- **Уравнение Бине u(φ) — аналитическая кросс-валидация интегратора**:
  - Новый модуль `src/Binet.h` / `src/Binet.cpp`: вывод уравнения Бине для нулевых геодезических Шварцшильда `d²u/dφ² + u = 3Mu² = (3/2)Rs·u²` (в комментарии хеддера — пошаговый вывод с размерностями), аналитический предикат `binetIsCaptured` (b_crit = 3√3/2 Rs) и RK4-интегратор орбиты u(φ) с остановами по горизонту и эскейпу.
  - Новый CLI-тест `--demo-binet`: построчное сравнение траекторий Binet vs существующего 4D-RK4 (`src/Geodesic.cpp`) для b ∈ {1.5, 2, 3, 5} Rs (относительная ошибка ≤ 1.3e-5 при пороге 1e-4), совпадение сценариев захват/уход для 6 значений b и чувствительность границы b_crit ± 0.005 Rs.
  - Это независимая проверка физики шейдерной геодезики: оба интегратора выводятся из одной метрики, но решают задачи по-разному.
- **Валидация шейдеров glslang в CI**:
  - Job ubuntu в `.github/workflows/build.yml` теперь устанавливает `glslang-tools` и прогоняет `glslangValidator` по всем `shaders/*.vert` и `shaders/*.frag`.
  - `shaders/nebula.frag`: функция `noise2` переименована в `valueNoise` — имя конфликтовало с built-in GLSL `genType noise2(genType)` и роняло строгие компиляторы (glslang: "overloaded functions must have the same return type"); на потребительских драйверах конфликт маскировался.
- **Документация**: `docs/ARCHITECTURE.md` и `docs/ROADMAP.md` описывают модуль Binet и его статус DONE.

## 2.1.1 - 2026-10-08
- **Fix: знак доплеровского биминга диска** — cosAlpha считался вдоль направления трассировки (камера→сцена), тогда как фотон летит сцена→камера; приближающаяся сторона диска Sgr A* затемнялась вместо свечения (зеркальность подтверждена численно: ratio 1.386 в неверную сторону). Фактор Бардена исправлен на g = √f / (γ(1 + β cosα)).
- **Численный харнесс проверки стороны** — tests/test_doppler_side.ps1: меряется ВКЛАД ДИСКА (кадр с диском минус кадр без диска, L/R окна X 15-45% и 55-85%, Y 45-95%), так как в сыром окне лево-яркое процедурное небо гасит сигнал; ожидаемая сторона выведена аналитически из teleportTo и vDir (deltaR >= 1.3 * deltaL). Добавлен флаг --screenshot-all --disk со скриншотами в docs/screens/disk/ (в .gitignore).

## 2.1.0 - 2026-10-08

- **Физически честный тонкий диск (Новиков-Торн + фактор Бардена)**:
  - Внутренний край диска в `shaders/bh.frag` перенесён на ISCO Шварцшильда `rIn = 3 Rs` (`6M`); прежний `rIn = 2.6 Rs` лежал внутри запретной зоны устойчивых круговых орбит. Внешний край `rOut = 10 Rs` без изменений.
  - Скорость круговой орбиты в локальном статическом кадре `beta = sqrt(M/r) / sqrt(1 - Rs/r)`, `gamma = 1 / sqrt(1 - beta^2)`; направление фотона нормируется метрикой (`dr/sqrt(f)`, `r dTheta`, `r sin(Theta) dPhi`), угол `alpha` считается через `cosAlpha = dot(dHat, vDir)`.
  - Фактор Бардена `g = sqrt(1 - Rs/r) / (gamma (1 - beta cos alpha))` заменяет прежнюю приближённую формулу; поток Новикова-Торна `F ~ r^-3 (1 - sqrt(rIn/r))`, температура `T ~ F^0.25`, болометрическая яркость `F g^4`.
  - Цвет — планковская аппроксимация чёрного тела (Tanner Helland) от `T g` с нормировкой на яркость (`T_DISK = 10000 K`); ад-хок-модуляция `ringMod` и `pow(g, 3.5)` удалены, яркость клампится сверху (`4.0`) без пересвета на пресете `Med`.
  - Интерфейс не изменился: по-прежнему только `u_DiskOn`, новых uniform и чекбоксов нет; геодезические уравнения (`geoAccel`), перенормировка `renormalizeTime` и пороги `r <= 1.001 Rs` / `r >= shell` не тронуты.
- **Гравитационное красное смещение наблюдателя**:
  - Статический наблюдатель получает свет с фактором `fCam = 1 - Rs/r_cam`, `r_cam = max(|u_CameraPosition|, 1.001 Rs)`.
  - Множитель яркости `sqrt(fCam)` применён ко всем финальным выходам шейдера (ветка захвата, выход на `u_ShellRadius` и fallback после лимита шагов) — как к диску, так и к небу.
  - Спектральный сдвиг `pow(color, vec3(1/max(sqrt(fCam), 0.2)))` усиливает красное и глушит синее по мере приближения к горизонту, не превращая кадр в чёрный на клампе `1.001 Rs`.

## v2.0 — 2026-10-07

- **Аккреционный диск и релятивистский redshift для Sgr A***:
  - В `shaders/bh.frag` добавлен uniform `u_DiskOn` (0/1, дефолт 0).
  - При `u_DiskOn=0` гарантировано строгое совпадение с чистой гравитационной линзой Шварцшильда (захват $r \le 1.001 R_s$, свободный выход $r \ge R_{\text{shell}}$).
  - При `u_DiskOn=1` визуализируется тонкий аккреционный диск в экваториальной плоскости ($3 R_s \dots 9 R_s$) с учетом гравитационного красного смещения и релятивистского доплеровского биминга.
  - В интерфейс Dear ImGui добавлен чекбокс `Disk`. Физика RK4-геодезических сохранена в неприкосновенности.
- **Адаптивный шаг Рунге-Кутты (Adaptive RK4 Step)**:
  - В шейдере `shaders/bh.frag` и CPU-модуле `src/Geodesic.cpp` реализован адаптивный шаг $h(r) = h_{\text{Far}} + (h_{\text{Near}} - h_{\text{Far}}) \times (1.0 - \text{smoothstep}(3 R_s, 8 R_s, r))$.
  - Базовый шаг ($1.0 \times h$) вблизи горизонта ($r \le 3 R_s$) и укрупненный ($2.5 \times h$) на удалении ($r \ge 8 R_s$).
  - Пресеты `Low`/`Med`/`Ultra` в `SceneManager` задают базовый шаг $h$.
  - Тест `--demo-geodesic` успешно подтверждает захват (`inward captured=1`) и уход на бесконечность (`tangent captured=0`).
- **Настоящие созвездия каталога Hipparcos**:
  - Захардкоженные заглушки созвездий Ориона (`ori`) и Большой Медведицы (`uma`) заменены на реальные астрометрические данные каталога Hipparcos (прямое восхождение, склонение, звездная величина, HIP ID).
  - Данные оформлены в `res/constellations.json` со строгим зеркалом в C++ (`src/Constellation.h` и `src/Constellation.cpp`).
  - Отрезки связей соответствуют каноническим созвездиям: 9 сегментов для Ориона, 7 сегментов для Большого Ковша.
  - Рендеринг через `shaders/lines.vert` и `lines.frag`, подробный список звезд с величинами и номерами HIP в ImGui.
- **Автоперформанс (2-секундный бенчмарк)**:
  - Автоматический замер FPS в течение первых 2 секунд работы 3D-сцены (`AppGL`).
  - Автоматический выбор оптимального пресета по порогам: $\ge 55$ FPS $\to$ `Ultra`, $30 \dots 55$ FPS $\to$ `Med`, $< 30$ FPS $\to$ `Low`.
  - Пороги задокументированы в `docs/ARCHITECTURE.md`.
  - Комбобокс пресетов в `src/AppGL.cpp` сохранен как ручной оверрайд (при ручной правке слайдеров фиксируется режим `Custom`).

## 1.0.0 prod — 2026-10-05

- **Пресеты качества и производительности (Quality / Perf Presets)**:
  - Добавлены пресеты `Low` (шаг 0.08, 300 шагов), `Med` (шаг 0.05, 600 шагов, дефолт), `Ultra` (шаг 0.025, 1200 шагов) и `Custom`.
  - Интеграция комбобокса пресетов в Dear ImGui интерфейс с автопереключением в `Custom` при ручной корректировке слайдеров.
  - Таблица параметров пресетов задокументирована в `docs/ARCHITECTURE.md`.
- **Регламент релизов и дистрибуции**:
  - Создан `docs/RELEASE.md` с описанием состава официального архива (`exe` + `shaders/` + `res/catalog.json` + `README` + `LICENSE`), правил изоляции HDR/Dependencies/vendor и 5-шаговой проверки качества.
- **Осознанная физическая модель Sgr A***:
  - Подтвержден чистый Schwarzschild-RK4 raytracing (`shaders/bh.frag`) без искусственного диска/доплера.
  - Аккреционный диск, релятивистские полярные джеты и свечение выделены в отдельную станцию квазара `3C 273` (`shaders/quasar.frag`).
  - Лимиты модели (отсутствие метрики Керра / спина $a=0$) задокументированы в `README.md` и `docs/ARCHITECTURE.md`.
- **Синхронизация и стабильность**:
  - `VERSION`, `src/Version.h`, `CMakeLists.txt` и `res/catalog.json` обновлены до версии `1.0.0`.
  - Успешный прогон unit-тестов взаимодействия, CTest, static checks и захвата скриншотов всех 6 станций.

## 0.3.0 — 2026-10-05

- Доведены до ума все 6 станций каталога:
  - `sgra`: Schwarzschild raytracing с численным решением RK4 в метрике Шварцшильда.
  - `qso-3c273`: квазар с аккреционным диском, релятивистскими джетами и динамическим свечением.
  - `ori`: созвездие Ориона (линии связей и звезды Hipparcos).
  - `uma`: созвездие Большой Медведицы (ковш и ручка с центрированной проекцией).
  - `m31`: спиральная галактика Андромеда (`shaders/galaxy.frag`) с наклоном диска, спиральными рукавами и пылевыми полосами.
  - `m1`: Крабовидная туманность (`shaders/nebula.frag`) с пульсаром, синхротронным ядром и турбулентными волокнами.
- Портированы тесты `BlackHoleInteraction` (`src/Interaction.h`, `src/Interaction.cpp`, `tests/test_interaction.cpp`, CLI `--test-interaction`).
- Обновлена документация `docs/BUILD.md`: руководство по процедурному Cubemap и опциональной загрузке `res/textures/starmap_2020_4k_gal.hdr` (HDR строго исключен из git).
- Расширены статические проверки в `tests/test_static_checks.ps1` (47 проверок).
- Полный набор скриншотов в `docs/screens/` (`sgra.png`, `qso-3c273.png`, `ori.png`, `uma.png`, `m31.png`, `m1.png`, `gui.png`).
- Сформирован релизный архив `dist/astro-voyager-v0.3.0-windows-x64.zip` (exe + shaders + catalog.json без HDR и vendor).

## 0.2.0 — 2026-10-05

- OpenGL 3.3 Core рендерер под опцией `ASTROVOYAGER_ENABLE_GL=ON` (core остается без внешних зависимостей при `OFF`).
- Модуль `Window`: GLFW окно, управление контекстом OpenGL 3.3, VSync.
- Модуль `Shader`: загрузка и компиляция вертексных/фрагментных GLSL шейдеров, uniform setters.
- Модуль `Mesh`: полноэкранный квад `ScreenQuad` + полилинии и звезды `LineMesh` для созвездий.
- Модуль `Cubemap`: процедурная звездная карта / Млечный Путь с автозагрузкой локального HDR при наличии.
- Модуль `SceneManager`: телепорт между станциями `sgra`, `ori`, `qso-3c273`, `uma`, `m31`, `m1`.
- Интеграция Dear ImGui: список станций с подсветкой активной, слайдеры `shellRadius`, `stepSize`, `maxSteps`, `FOV`, мониторинг FPS и времени кадра.
- Автоматический захват скриншотов станций и UI в `docs/screens/` (`sgra.png`, `ori.png`, `qso-3c273.png`, `gui.png`).

## 0.1.0 — 2026-10-05

- Repo `astro-voyager` создан (идея: путешественник SgrA*/3C273/Orion/UMA/M31/M1).
- Core CLI: `--list`, `--info`, `--demo-geodesic`, `--version`.
- CPU-эталон Schwarzschild+RK4 (порт `BlackHoleRaytrace.shader`).
- Шейдеры `bh.frag`, `quasar.frag`, `lines.vert`, `quad.vert`.
- `res/catalog.json` контракт + `docs/` (ARCH/BUILD/CATALOG/ROADMAP).
- CI Windows+Linux + static checks + Docker build-check.
