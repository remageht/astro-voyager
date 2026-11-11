# CATALOG — спецификация и контракт каталога станций v5.0.0

Каталог астрофизических объектов `astro-voyager` синхронизируется между двумя источниками:
1. `res/catalog.json` — основной источник данных для UI, импосторов, 3D Оррери карты и экспедиций.
2. `src/Catalog.cpp:builtinCatalog` — встроенное C++ зеркало для автономного запуска Core, безфайловых CLI тестов и headless CI.

> Любое расхождение по ID, именам или типам между `res/catalog.json` и `src/Catalog.cpp` считается ошибкой сборки и блокируется тестом `tests/test_static_checks.ps1`.

---

## 1. Схема полей объекта каталога

```json
{
  "id": "kebab-case-unique-identifier",
  "name": "Человекочитаемое название (RU / EN)",
  "type": "black_hole|quasar|constellation|galaxy|nebula|solar_system|pulsar|star_cluster|kilonova|supernova|tidal_disruption|pillars",
  "description": "Астрофизическое и сюжетное описание",
  "spawnDistanceRs": 15.0,
  "params": {
    "rs": 1.0,
    "shellRadius": 30.0,
    "step": 0.05,
    "maxSteps": 600
  }
}
```

- `id`: уникальный ключ в формате kebab-case (например, `sgra`, `m87`, `gw170817`, `at2019qiz`).
- `type`: один из 12 поддерживаемых типов объектов:
  - `black_hole` — одиночные и двойные черные дыры (`sgra`, `m87`, `cygnus-x1`, `oj287`, `gargantua`).
  - `quasar` — активные релятивистские ядра галактик (`3c273`).
  - `galaxy` — спиральные галактики (`m31`, `m51`, `m104`).
  - `pulsar` — быстровращающиеся нейтронные звезды и магнетары (`vela`, `crab`, `sgr1806`).
  - `nebula` — диффузные и планетарные туманности (`orion`, `ring-nebula`).
  - `solar_system` — Солнечная система с Землей, Луной, Марсом и Сатурном (`solar-system`).
  - `star_cluster` — шаровые звездные скопления (`m13`).
  - `constellation` — опорные навигационные созвездия (`orion-const`).
  - `kilonova` — слияние нейтронных звезд с r-процессом (`gw170817`).
  - `supernova` — остаток сверхновой с кольцом ударной волны (`sn1987a`).
  - `tidal_disruption` — приливное разрушение звезды черной дырой (`at2019qiz`).
  - `pillars` — Столпы Творения в туманности Орел (`m16`).
- `spawnDistanceRs`: расстояние безопасной точки выхода из варп-прыжка (строго `> 1.05 Rs`).
- `params` (опционально для `black_hole`): параметры численного трассирования лучей (радиус Шварцшильда `rs`, радиус внешней сферы `shellRadius`, базовый шаг $h_0$ и лимит шагов `maxSteps`).

---

## 2. Полный реестр 22 станций (v5.0.0)

| ID | Название | Тип | Шейдер / Рендер |
|---|---|---|---|
| `sgra` | Стрелец A* (Sgr A*) | `black_hole` | `shaders/bh.frag` |
| `m87` | Релятивистский джет M87* | `black_hole` | `shaders/bh.frag` |
| `cygnus-x1` | Лебедь X-1 (Cygnus X-1) | `black_hole` | `shaders/bh.frag` |
| `oj287` | Двойная сверхмассивная дыра OJ 287 | `black_hole` | `shaders/bh.frag` |
| `gargantua` | Релятивистский гигант «Гаргантюа» | `black_hole` | `shaders/bh.frag` |
| `3c273` | Релятивистский квазар 3C 273 | `quasar` | `shaders/quasar.frag` |
| `solar-system` | Солнечная система (Sol) | `solar_system` | `shaders/planet.*`, `shaders/ring.*` |
| `m31` | Галактика Андромеды (M31) | `galaxy` | `shaders/galaxy.frag` |
| `m51` | Галактика Водоворот (M51) | `galaxy` | `shaders/galaxy.frag` |
| `m104` | Галактика Сомбреро (M104) | `galaxy` | `shaders/galaxy.frag` |
| `vela` | Пульсар в Парусах (Vela Pulsar) | `pulsar` | `shaders/pulsar.frag` |
| `crab` | Пульсар Крабовидной туманности (Crab) | `pulsar` | `shaders/pulsar.frag` |
| `sgr1806` | Магнетар SGR 1806-20 | `pulsar` | `shaders/pulsar.frag` |
| `orion` | Большая туманность Ориона (M42) | `nebula` | `shaders/nebula.frag` |
| `ring-nebula` | Кольцевая туманность (M57) | `nebula` | `shaders/nebula.frag` |
| `m13` | Шаровое скопление Геркулеса (M13) | `star_cluster` | `shaders/galaxy.frag` (LOD) |
| `orion-const` | Навигационное созвездие Орион | `constellation` | `src/Constellation.cpp` |
| `halley` | Комета Галлея (1P/Halley) | `solar_system` | `shaders/comet.*` |
| `gw170817` | Килонова GW170817 | `kilonova` | `shaders/kilonova.frag` |
| `sn1987a` | Сверхновая SN 1987A | `supernova` | `shaders/supernova.frag` |
| `at2019qiz` | Приливной разрыв звезды AT2019qiz | `tidal_disruption` | `shaders/tde.frag` |
| `m16` | Столпы Творения (Туманность Орел M16) | `pillars` | `shaders/pillars.frag` |

---

## 3. Регламент добавления нового объекта
1. Добавить запись в `res/catalog.json`.
2. Добавить зеркальную запись в `src/Catalog.cpp` в массив `builtinCatalog`.
3. Добавить спектральные линии фраунгоферова поглощения и эмиссии в `src/ScienceSystem.cpp`.
4. Зарегистрировать реплики радиоэфира и озвучки в `src/audio/AudioEngine.cpp` и `src/AppGL.cpp`.
5. Обновить таблицы в `README.md` и запустить `powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1`.

