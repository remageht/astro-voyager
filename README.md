# astro-voyager — путешественник по Вселенной

### 3D-приложение: созвездия, черные дыры, квазары и дип-скай

<p align="right">Выполнил: Ваджипов Эмир Аленович</p>
<p align="right">Группа: ПИ-б-о-241(1)</p>

Идея наследована от `RuCatGH/blackHole`: не картинка, а численная физика
линзирования в метрике Шварцшильда (RK4-геодезические в шейдере).
`astro-voyager` расширяет ее до путешественника: свободный полет WASD + телепорт
по каталогу объектов.

Название: `astro-voyager` — коротко, GitHub/Docker-friendly, покрывает все типы
объектов (не только дыру). Отвергнуты: `event-horizon` (только ЧД),
`cosmo-traveler` (длинно), `starhopper` (ассоциация с игрой).

## Станции MVP (предложены автором)

| id | Тип | Что рендерит |
|----|-----|--------------|
| `sgra` | black_hole | Порт `BlackHoleRaytrace.shader`: Schwarzschild + RK4, `Rs=1.0` |
| `qso-3c273` | quasar | Billboard + диск + джеты, additive glow (без ОТО) |
| `ori` | constellation | Орион, `GL_LINES` по Hipparcos-упрощению |
| `uma` | constellation | Большая Медведица, `GL_LINES` |
| `m31` | galaxy | Андромеда, impostor-спрайт |
| `m1` | nebula | Крабовидная, impostor-спрайт |

Фон всех станций: cubemap из HDR (`starmap_2020_4k_gal.hdr`, в git не входит).

## Быстрый старт (headless core, без GPU)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/astro-voyager --list
./build/astro-voyager --info sgra
./build/astro-voyager --demo-geodesic
# Windows: .\build\Release\astro-voyager.exe --list
```

Полный 3D-рендер (OpenGL) — опция `ASTROVOYAGER_ENABLE_GL=ON`, требует
GLFW/GLEW/GLM/ImGui локально, см. `docs/BUILD.md`. В CI собирается только core.

## Структура

```
astro-voyager/
  CMakeLists.txt
  src/            # Catalog, Camera, Geodesic (CPU-эталон), main CLI
  shaders/        # quad.vert, bh.frag, quasar.frag, lines.vert (GLSL 330)
  res/catalog.json# каталог станций
  docs/           # ARCHITECTURE, BUILD, CATALOG, ROADMAP
  tests/test_static_checks.ps1
  .github/workflows/build.yml
  Dockerfile      # reproducible build (не runtime: GPU нужен хост)
  .env.example    # ASTRO_* переменные
```

HDR-ассеты положить вручную:
```
res/textures/starmap_2020_4k_gal.hdr  # любой equirect HDR, в git не входит
```

## Прод-гейты

```powershell
powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
```

CI: сборка Windows + Linux, static checks, сборка Docker-образа.
Контракт каталога: `docs/CATALOG.md`. Архитектура: `docs/ARCHITECTURE.md`.

## Версии

См. `CHANGELOG.md`, текущая — `VERSION` (`0.3.0`). Скриншоты станций — в `docs/screens/`.
