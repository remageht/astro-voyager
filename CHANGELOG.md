# CHANGELOG

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
