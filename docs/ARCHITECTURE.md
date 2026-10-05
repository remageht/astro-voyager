# ARCHITECTURE — astro-voyager v0.1.0

## Идея

Развитие `RuCatGH/blackHole`: полноэкранный raytrace-шейдер с RK4-геодезикой
Шварцшильда + путешественник по станциям вместо одной сцены.

Бесшовной вселенной нет осознанно: `Rs=1.0` против парсек ломает float-precision.
Паттерн — Station: телепорт меняет skybox-параметры и шейдер, внутри станции
свободный полет WASD + мышь.

## Модули

- `Catalog` — builtin-каталог 6 станций, зеркало `res/catalog.json`.
  Контракт: `docs/CATALOG.md`.
- `Camera` — yaw/pitch + forward/right/up, `clampNearHorizon(Rs*1.05)`.
  Своя `Vec3` чтобы core собирался без GLM в CI.
- `Geodesic` — CPU-эталон формул из `shaders/bh.frag`:
  `geodesicAcceleration`, `rk4Step`, `traceRay`.
  Используется в `--demo-geodesic` и будущих unit-тестах.
- `main` — CLI: `--list`, `--info <id>`, `--demo-geodesic`, `--version`.
  Окно/GL — только при `ASTROVOYAGER_ENABLE_GL=ON` (этап 0.2.0).

## Рендер (0.2.0)

- `shaders/quad.vert + bh.frag` — порт blackHole 1:1.
- `shaders/quasar.frag` — core+disk+jets, без ОТО.
- `shaders/lines.vert` — созвездия `GL_LINES`.
- Галактики/туманности — impostor-спрайты на cubemap.

## Физика

Метрика Шварцшильда, `c=1`, `Rs=2GM/c^2` как единица длины.
Начальная `dt/dl` из `ds^2=0`. Останов: `r<=Rs*1.001` capture,
`r>=shell` escape, иначе dim. Ограничения: нет Керра, диска, redshift.
