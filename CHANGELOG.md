# CHANGELOG

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
