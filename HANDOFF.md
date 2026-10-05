# HANDOFF — передача DeepSeek / Antigravity

## Состояние v0.1.0 (готово локально, в GitHub пока пусто)
Скелет собран в `Default Project/astro-voyager/`, remote `https://github.com/remageht/astro-voyager` пуст.
Проверено:
- `tests/test_static_checks.ps1` → `All checks passed` (39 checks).
- Python-эталон геодезики: `inward steps=458 captured=True`, `tangent steps=1687 captured=False`.
- Локально нет `cmake/g++` — сборка ляжет на CI (`windows+ubuntu`) после пуша.

## Как запушить (выполнить вручную)
```bash
Set-Location "C:\Users\Эмир\Documents\Default Project\astro-voyager"
git init -b main
git add .
git commit -m "astro-voyager v0.1.0 core skeleton"
git remote add origin https://github.com/remageht/astro-voyager.git
git push -u origin main
```

## Промпт для следующего ИИ (скопировать, AGENTS.md не в репо)
```
Репо: https://github.com/remageht/astro-voyager, ветка main, v0.1.0.
Читай README.md, docs/ARCHITECTURE.md, docs/ROADMAP.md, docs/CATALOG.md.
Задача 0.2.0: Window (GLFW) + Shader-лоадер + fullscreen quad + SceneManager
телепорт sgra/ori/qso-3c273 + ImGui (список станций, слайдеры shell/step/maxSteps/FOV, FPS).
Ограничения: C++17, core без новых deps (GLM/GLFW только при ASTROVOYAGER_ENABLE_GL=ON),
res/catalog.json и src/Catalog.cpp всегда синхронны, HDR/Dependencies/src/vendor никогда в git,
физику shaders/bh.frag не упрощать (geodesicAcceleration + rk4Step + r<=Rs*1.001 capture + r>=shell escape).
Проверка: cmake build + tests/test_static_checks.ps1 + скриншоты в docs/screens/.
Используй доступные skills / plugins / MCP (билд, git, docs): сначала проверь окружение ими, потом код.
```

## Следующие задачи
1. `0.2.0 gl-window` (см. `docs/ROADMAP.md`).
2. Положить HDR вручную в `res/textures/starmap_2020_4k_gal.hdr` (не в git).
3. Тег `v0.1.0` после первого зеленого CI.
