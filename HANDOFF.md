# HANDOFF — передача DeepSeek-V4-Pro / Antigravity

## Состояние: v2.1.1 RELEASED (2026-10-08)

- Репо `https://github.com/remageht/astro-voyager`, `main` = `6942189`, CI зелёный
  (в т.ч. валидация шейдеров glslang на Ubuntu).
- Теги: `v0.1.0, v0.2.0, v0.3.0, v1.0.0, v2.1.0, v2.1.1`. GitHub Releases: v1.0.0, v2.1.0, v2.1.1.
- Вехи DONE: core → gl-window → traveler (6 станций) → prod → физика (renormalizeTime + b_crit,
  Новиков-Торн + Барден + g⁴, redshift наблюдателя) → **fix знака доплеровского биминга**.
- Doppler-баг v2.1.0 закрыт в v2.1.1: `cosAlpha` считался вдоль направления трассировки
  (камера→сцена), фотон летит сцена→камера → приближающая сторона диска затемнялась.
  Фикс: `shaders/bh.frag` `g = √f / (γ(1 + β cosAlpha))` (знак `+`).
- Харнесс `tests/test_doppler_side.ps1`: меряет **вклад диска** (on − off, окно X 15–45/55–85%,
  Y 45–95%) — сырое окно не годилось, небо лево-яркое и гасило сигнал. Ожидаемый ratio 2.3–3.6,
  сейчас 2.445. Скриншоты с диском: `--screenshot-all --disk` → `docs/screens/disk/` (gitignored).
  Disk-off кадр `docs/screens/sgra.png` детерминирован (байт-в-байт) — на него опирается вычитание.
- Шейдеры валидируются glslang в CI (job ubuntu, шаг `Validate shaders`), локально:
  `glslangValidator shaders/*.vert shaders/*.frag`. В nebula.frag `noise2` переименован в
  `valueNoise` (конфликт с built-in `genType noise2`) — пункта "nebula не проходит" больше нет.
- Гейты (все зелёные): `tests/test_static_checks.ps1` (70+ проверок), `ctest`, `--demo-geodesic`,
  `--test-interaction`, `--screenshot-all` (7 PNG), `test_doppler_side.ps1` PASS, CI glslang.
- Локальный cmake: `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` в PATH.

## Открытые пункты

1. Идеи из `docs/ROADMAP.md` (v2.0 ideas): уравнение Бине u(φ), Керр, Linux AppImage.

## Промпт для следующего ИИ (копировать)

```
Репо: https://github.com/remageht/astro-voyager, ветка main, тег v2.1.1.
Читай README.md, HANDOFF.md, docs/ARCHITECTURE.md, docs/ROADMAP.md, docs/RELEASE.md.
Отвечай строго по-русски, код и команды — без перевода.
Задача: <сюда задачу>.
Ограничения: C++17; core без новых deps (GLM/GLFW/ImGui только при ASTROVOYAGER_ENABLE_GL=ON);
res/catalog.json и res/constellations.json синхронны с src/Catalog.cpp / src/Constellation.cpp;
физику shaders/bh.frag не ломать (geoAccel, renormalizeTime, capture r<=Rs*1.001, exit r>=shell,
smoothstep edge0<edge1, ISCO rIn=3*Rs, g^4, знак биминга (1.0 + beta*cosAlpha), ringMod запрещён);
HDR/Dependencies/src/vendor/dist/AGENTS.md/docs/screens/disk/ в git не коммить.
Проверка: cmake build (core+gl) + ctest + tests/test_static_checks.ps1 + --demo-geodesic
+ --screenshot-all (7 PNG) + tests/test_doppler_side.ps1 (PASS, ratio 2.3-3.6).
Коммиты, теги, релизы — делает владелец (см. ниже), ИИ только готовит и верифицирует.
Используй доступные skills / plugins / MCP (билд, git, docs): сначала проверь окружение ими, потом код.
Перед правками: git status + git log -5 — убедись, что работаешь от актуального HEAD.
```

## Правила пуша/релиза (делает владелец репо)

- Пуш: после проверки отчёта ИИ по факту кода (диф + прогоны гейтов), отчёт ≠ код — откат.
- Релиз: бамп `VERSION` + `src/Version.h` + `CMakeLists.txt:2` + `res/catalog.json`
  (проверки сверяют все 4 с VERSION), блок в `CHANGELOG.md`, зелёные 5 шагов `docs/RELEASE.md`,
  zip в `dist/` (12 файлов: exe + 7 shaders + res/*.json + README + LICENSE; не коммитить),
  `git tag vX.Y.Z`, `gh release create`.
