# BUILD — astro-voyager

## Core (CI, без GPU)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/astro-voyager --demo-geodesic
./build/astro-voyager --test-interaction
# Windows: .\build\Release\astro-voyager.exe --demo-geodesic
```

Требования: CMake >= 3.16, C++17, Threads.

## Full GL render (локально, 0.2.0+)

1. Зависимости (как в blackHole):
```
Dependencies/GLEW, Dependencies/GLFW
src/vendor/glm, src/vendor/imgui, src/vendor/stb_image
```
Все эти каталоги занесены в `.gitignore` и в git не попадают.

2. HDR-текстура звездного неба (опционально):
Положить equirectangular HDR файл по пути:
```
res/textures/starmap_2020_4k_gal.hdr
```
- Если файл отсутствует, `astro-voyager` автоматически генерирует процедурный cubemap звездного поля и рукавов Галактики (512×512 на грань), поэтому приложение работает сразу «из коробки» без внешних тяжелых ассетов.
- При наличии `res/textures/starmap_2020_4k_gal.hdr` он автоматически считывается через stb_image и проецируется на 6 граней cubemap.
- **ВАЖНО**: Файлы `*.hdr` и `*.exr` в `res/textures/` строго занесены в `.gitignore` и **никогда не должны попадать в git-репозиторий**.

3. Сборка:
```bash
cmake -S . -B build-gl -DASTROVOYAGER_ENABLE_GL=ON
cmake --build build-gl --config Release
```

4. Запуск и скриншоты:
```bash
# Интерактивное 3D-окно:
.\build-gl\Release\astro-voyager.exe
# Автоматический захват всех 6 станций в docs/screens/:
.\build-gl\Release\astro-voyager.exe --screenshot-all
```

## Тесты

```bash
# Запуск unit-тестов взаимодействия (камера, clamp, ImGui):
.\build\Release\test_interaction.exe
# Статические проверки репозитория:
powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
```

## Docker (только reproducible build)

```bash
docker build -t astro-voyager:0.3.0 .
docker run --rm astro-voyager:0.3.0 --demo-geodesic
```

Runtime-образа с окном нет: OpenGL требует GPU хоста.
