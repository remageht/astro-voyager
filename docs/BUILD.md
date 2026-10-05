# BUILD — astro-voyager

## Core (CI, без GPU)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/astro-voyager --demo-geodesic
```

Требования: CMake >= 3.16, C++17, Threads.

## Full GL render (локально, 0.2.0+)

1. Положить зависимости как в blackHole:
```
Dependencies/GLEW, Dependencies/GLFW
astro-voyager/src/vendor/glm, imgui, stb_image
```
2. Положить HDR:
```
res/textures/starmap_2020_4k_gal.hdr
```
3. Собрать:
```bash
cmake -S . -B build-gl -DASTROVOYAGER_ENABLE_GL=ON
cmake --build build-gl --config Release
```

## Docker (только reproducible build)

```bash
docker build -t astro-voyager:0.1.0 .
docker run --rm astro-voyager:0.1.0 --demo-geodesic
```

Runtime-образа с окном нет: OpenGL требует GPU хоста.
