# ARCHITECTURE — astro-voyager v0.2.0

## Идея

Развитие `RuCatGH/blackHole`: полноэкранный raytrace-шейдер с RK4-геодезикой
Шварцшильда + путешественник по станциям вместо одной сцены.

Бесшовной вселенной нет осознанно: `Rs=1.0` против парсек ломает float-precision.
Паттерн — Station: телепорт меняет skybox-параметры и шейдер, внутри станции
свободный полет WASD + мышь.

## Модули

### Core (headless, без GPU / GLM / GLFW)
- `Catalog` — builtin-каталог 6 станций, зеркало `res/catalog.json`.
  Контракт: `docs/CATALOG.md`.
- `Camera` — yaw/pitch + forward/right/up, `clampNearHorizon(Rs*1.05)`.
  Своя `Vec3` чтобы core собирался без GLM в CI.
- `Geodesic` — CPU-эталон формул из `shaders/bh.frag`:
  `geodesicAcceleration`, `rk4Step`, `traceRay`.
  Используется в `--demo-geodesic` и unit-тестах.
- `main` — CLI: `--list`, `--info <id>`, `--demo-geodesic`, `--version`.

### OpenGL Renderer (`ASTROVOYAGER_ENABLE_GL=ON`)
- `Window` — обертка GLFW: создание окна, контекст OpenGL 3.3 Core, VSync.
- `Shader` — загрузка GLSL из файлов, компиляция vertex/fragment, кеширование и установка uniform.
- `Mesh` (`ScreenQuad`, `LineMesh`) — геометрия fullscreen quad (VAO/VBO/EBO) и полилинии созвездий (GL_LINES, GL_POINTS).
- `Cubemap` — 6 граней скайбокса. Процедурный генератор звездного поля и рукавов Галактики с автозагрузкой локального HDR при наличии.
- `SceneManager` — управление сценами и телепортация (`sgra`, `ori`, `qso-3c273`, `uma`, `m31`, `m1`), выбор шейдеров и установка физических параметров.
- `AppGL` — главный цикл, WASD-перемещение, обзор мышью, интеграция Dear ImGui (HUD со списком станций, слайдерами и FPS), захват скриншотов.

## Рендер

- `shaders/quad.vert + bh.frag` — порт blackHole: Schwarzschild raytracing с численным RK4.
- `shaders/quasar.frag` — core+disk+jets, procedural billboard квазара.
- `shaders/lines.vert + lines.frag` — созвездия `GL_LINES` и звезды `GL_POINTS`.
- Скриншоты сохраняются в `docs/screens/`.

## Физика

Метрика Шварцшильда, `c=1`, `Rs=2GM/c^2` как единица длины.
Начальная `dt/dl` из `ds^2=0`. Останов: `r<=Rs*1.001` capture,
`r>=shell` escape, иначе dim. Ограничения: нет Керра, диска, redshift.
