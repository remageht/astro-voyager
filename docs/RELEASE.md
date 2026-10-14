# RELEASE — astro-voyager

Документ описывает состав официального релизного дистрибутива, правила исключения служебных файлов и шаги предрелизной верификации. Пример ниже — для версии 2.1.0; при выпуске новой версии подставляется актуальный номер.

## 1. Состав релизного архива (`astro-voyager-v2.1.0-windows-x64.zip`)

Релизный архив собирается в каталоге `dist/` и содержит минимально необходимый набор файлов для автономного запуска на системе пользователя:

```
astro-voyager-v2.1.0-windows-x64.zip
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
│   ├── catalog.json        # Контракт каталога 6 астрономических станций
│   └── constellations.json # Контракт данных созвездий (Hipparcos, миррор в C++)
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

### Шаг 3. Автоматический рендер всех станций и UI
```powershell
.\build-gl\Release\astro-voyager.exe --screenshot-all
```
*Критерий:* В каталоге `docs/screens/` сформированы 15 PNG-изображений:
`sgra.png`, `qso-3c273.png`, `ori.png`, `uma.png`, `m31.png`, `m1.png`, `sol.png`,
`m87.png`, `m51.png`, `m104.png`, `gui.png`, `psr-vela.png`, `psr-crab.png`,
`sgr-1806.png`, `psr-b1913.png` (плюс gitignored-хелперы `*_close.png`
для проверки ближних планов, в коммит не входят).

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

## 4. Двуязычные описания коммитов и релизов (EN + RU)

Описания — короткие и читаемые: буллеты вместо стен текста, без формул
(формулы живут в коде/docs, в описании — ссылка). Каждое описание —
на двух языках: сначала English, затем Russian.

### 4.1. Формат коммита
```
<type>(<scope>): short English subject (conventional commits)

EN: 2-4 bullet lines, plain words, what changed and why.
- bullet per user-visible change
- key numbers (gates, versions) as a last line if any

RU: то же самое по-русски, 2-4 строки буллетами.
- буллет на каждое видимое изменение
```
Пример:
```
feat(sol): Solar System station with detailed planets

EN: New "sol" station: Sun + 8 planets + Moon, Saturn rings,
asteroid belt, procedural surfaces, fly-to labels.
- Gates: ctest 3/3, static checks 100, doppler 3.29 PASS.

RU: Новая станция "sol": Солнце + 8 планет + Луна, кольца Сатурна,
пояс астероидов, процедурные поверхности, подписи при подлёте.
- Гейты: ctest 3/3, static checks 100, доплер 3.29 PASS.
```

### 4.2. Формат релиза (GitHub release notes + блок CHANGELOG.md)
```markdown
## What's new / Что нового
- EN bullet (one line per change)
- ...
- RU буллет (по строке на изменение)
- ...
Gates / Гейты: ctest 3/3, checks 100, doppler 3.29 PASS.
```
Заголовок релиза двуязычный: `vX.Y.Z — English title / Русский заголовок`.
Старые релизы (до v2.4.0 включительно) остаются как есть — формат
применяется начиная со следующего релиза.
