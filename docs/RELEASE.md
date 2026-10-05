# RELEASE — astro-voyager v1.0.0

Документ описывает состав официального релизного дистрибутива, правила исключения служебных файлов и шаги предрелизной верификации.

## 1. Состав релизного архива (`astro-voyager-v1.0.0-windows-x64.zip`)

Релизный архив собирается в каталоге `dist/` и содержит минимально необходимый набор файлов для автономного запуска на системе пользователя:

```
astro-voyager-v1.0.0-windows-x64.zip
├── astro-voyager.exe       # Скомпилированный бинарный файл (OpenGL 3.3 Core GUI + headless CLI)
├── shaders/                # Набор GLSL 330 шейдеров для рендеринга всех станций
│   ├── bh.frag             # Численный RK4 raytracer черной дыры (Schwarzschild metric)
│   ├── quasar.frag         # Raymarching квазара 3C 273 (ядро, аккреционный диск, джеты)
│   ├── lines.vert          # Вертексный шейдер для созвездий Orion и Ursa Major
│   ├── lines.frag          # Фрагментный шейдер линий и звезд созвездий
│   ├── galaxy.frag         # Процедурный спрайт спиральной галактики M31
│   ├── nebula.frag         # Процедурный спрайт Крабовидной туманности M1
│   └── quad.vert           # Полноэкранный квад для raytracing/impostor шейдеров
├── res/
│   └── catalog.json        # Контракт каталога 6 астрономических станций
├── README.md               # Документация, управление и системные требования
└── LICENSE                 # Лицензия проекта (MIT)
```

## 2. Политика исключений (Что строго НЕ входит в архив и git)

1. **Тяжелые HDR-панорамы** (`res/textures/*.hdr`, `res/textures/*.exr`):
   - Не упаковываются в архив и не хранятся в git (`.gitignore`).
   - Приложение функционирует автономно благодаря процедурному cubemap звездного поля и Галактики. Локальный HDR загружается только при ручном добавлении пользователем в `res/textures/starmap_2020_4k_gal.hdr`.
2. **Исходные библиотеки и зависимости** (`Dependencies/`, `src/vendor/`):
   - Исходники GLFW, GLEW, ImGui, GLM и stb_image используются только при локальной сборке и не включаются в релизный пакет.
3. **Промежуточные артефакты сборки** (`build/`, `build-gl/`, `*.obj`, `*.pdb`, `imgui.ini`, `/dist/`).

## 3. Регламент проверки перед релизом

Перед выпуском релиза выполняются следующие обязательные шаги валидации:

### Шаг 1. Проверка численной физики геодезических (CPU эталон)
```powershell
.\build\Release\astro-voyager.exe --demo-geodesic
```
*Критерий:* Inward geodesic захватывается горизонтом (`captured=1`), tangent geodesic уходит на бесконечность (`captured=0`).

### Шаг 2. Unit-тесты взаимодействия камеры и интерфейса
```powershell
.\build\Release\test_interaction.exe
# или через CLI:
.\build\Release\astro-voyager.exe --test-interaction
```
*Критерий:* 100% тестов проходят (`All BlackHoleInteraction tests passed!`, проверка захвата мыши, блокировки WASD при ImGui вводе и clamp $R \ge 1.05 R_s$).

### Шаг 3. Автоматический рендер всех 6 станций и UI
```powershell
.\build-gl\Release\astro-voyager.exe --screenshot-all
```
*Критерий:* В каталоге `docs/screens/` сформированы 7 PNG-изображений:
`sgra.png`, `qso-3c273.png`, `ori.png`, `uma.png`, `m31.png`, `m1.png`, `gui.png`.

### Шаг 4. Набор статических тестов репозитория
```powershell
powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
```
*Критерий:* `All checks passed` (проверка соответствия стандарту C++17, синхронизации каталога JSON и C++, наличия документации, шейдеров и отсутствия секретов).

### Шаг 5. CTest проверка
```powershell
ctest --test-dir build -C Release --output-on-failure
```
*Критерий:* 100% тестов успешно пройдены.
