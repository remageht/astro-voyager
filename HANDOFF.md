# HANDOFF — передача DeepSeek-V4-Pro / Antigravity

## Состояние: v2.1.0 RELEASED (2026-10-08)

- Репо `https://github.com/remageht/astro-voyager`, `main` = `92c3148`, CI зелёный.
- Теги: `v0.1.0, v0.2.0, v0.3.0, v1.0.0, v2.1.0`. GitHub Release `v2.1.0` опубликован
  (`dist/astro-voyager-v2.1.0-windows-x64.zip`, 12 файлов, без HDR/vendor).
- Вехи DONE: core → gl-window → traveler (6 станций) → prod → физика
  (renormalizeTime + b_crit, Новиков-Торн + Барден + g⁴, redshift наблюдателя).
- Гейты (все зелёные): `tests/test_static_checks.ps1` (~60 проверок),
  `ctest`, `--demo-geodesic` (inward 298/1, tangent 673/0, b_crit, drift 2.4e-15),
  `--test-interaction`, `--screenshot-all` (7 PNG), `glslang shaders/bh.frag`.
- Локальный cmake: `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` в PATH.

## Открытые пункты

1. **Визуал знака доплера диска**: запустить `build-gl\Release\astro-voyager.exe`,
   станция `sgra`, чекбокс `Disk`. Если сторона яркости зеркальна — флип в
   `shaders/bh.frag` (~:184): `(1.0 - beta*cosAlpha)` → `(1.0 + beta*cosAlpha)`.
2. `shaders/nebula.frag` не проходит glslang (предсуществующий баг `noise2`,
   перегрузки с разным типом возврата) — не трогали.
3. Идеи из `docs/ROADMAP.md` (v2.0 ideas): уравнение Бине u(φ), Керр, Linux AppImage.

## Промпт для следующего ИИ (копировать)

```
Репо: https://github.com/remageht/astro-voyager, ветка main, тег v2.1.0.
Читай README.md, HANDOFF.md, docs/ARCHITECTURE.md, docs/ROADMAP.md, docs/RELEASE.md.
Отвечай строго по-русски, код и команды — без перевода.
Задача: <сюда задачу>.
Ограничения: C++17; core без новых deps (GLM/GLFW/ImGui только при ASTROVOYAGER_ENABLE_GL=ON);
res/catalog.json и res/constellations.json синхронны с src/Catalog.cpp / src/Constellation.cpp;
физику shaders/bh.frag не ломать (geoAccel, renormalizeTime, capture r<=Rs*1.001, exit r>=shell,
smoothstep edge0<edge1, ISCO rIn=3*Rs, g^4, ringMod запрещён);
HDR/Dependencies/src/vendor/dist/AGENTS.md в git не коммить.
Проверка: cmake build (core+gl) + ctest + tests/test_static_checks.ps1 + --demo-geodesic
+ --screenshot-all (7 PNG). Коммиты НЕ пушить — пуш проверяет владелец.
Используй доступные skills / plugins / MCP (билд, git, docs): сначала проверь окружение ими, потом код.
Перед правками: git status + git log -5 — убедись, что работаешь от актуального HEAD.
```

## Правила пуша/релиза (делает владелец репо)

- Пуш: только после проверки отчёта ИИ по факту кода (диф + гейты), отчёт ≠ код — откат.
- Релиз: бамп `VERSION` + `src/Version.h` + `CMakeLists.txt:2` + `res/catalog.json`
  (проверки сверяют все 4 с VERSION), CHANGELOG, зелёные 5 шагов `docs/RELEASE.md`,
  zip в `dist/` (не коммитить), тег, `gh release create`.
