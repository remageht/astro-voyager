# Spacecraft assets / Ассеты кораблей

Локальные исходники и пайплайн для пилотируемых кораблей на орбите Земли.
Тяжёлые GLTF/OBJ/PNG-файлы в git не попадают: в репозитории лежат только
скрипты экспорта, витрины и эта документация. Сами модели скачиваются
вручную с Sketchfab (см. ниже) либо берутся из уже подготовленного
`blender/out/`.

## SpaceX Crew Dragon

| Поле | Значение |
|---|---|
| Модель | SpaceX Dragon 2 exterior |
| Автор | Jon Ross ([zlsa](https://sketchfab.com/zlsa)) |
| Лицензия | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Источник | <https://sketchfab.com/3d-models/spacex-dragon-2-exterior-2575398db20c406cab2cf72b89a9be8b> |
| Локальный исходник | `blender/out/spacex_src/dragon/scene.gltf` |
| Проверка | `python tools/download_spacecraft_assets.py --check-dragon` |
| Экспорт | `blender/scripts/export_dragon_game_asset.py` |
| Игровые файлы | `res/models/dragon.obj` + 5 BMP-атласов 2048×2048 |
| Витрина | `docs/screens/spacex_dragon_hero.png` |
| В сцене | `dock-dragon`, масштаб `0.095` |

### Контрольные суммы Dragon (`blender/out/spacex_src/dragon/`)

| Файл | SHA256 |
|---|---|
| `scene.gltf` | `E63A1F9E2F6A6ADB95CA27C2204F381EED7B51279D5C3159F83EA31BFC9502E7` |
| `scene.bin` | `B1DC10A329B1ABC40C4944F402E098B16848916961CCCD2418FCA9B7782E35D1` |
| `textures/body_baseColor.png` | `93B4B4584CF9A75100E12909ED6BF18311E683B43D9802C2BF88F1FB6B7D4574` |
| `textures/body_metallicRoughness.png` | `832FB455916A2D864EDB7E7E9AD5FA27133A6DF82540CCE65EF0F2D8856E1185` |
| `textures/body_normal.png` | `8E36259A682F9129E7971461A4157948EF362EA561026A496893A61DC49B4552` |
| `textures/ground_baseColor.png` | `65EEC4468BED87FE995A68CB4FAA50CF5F295826B14113A99CE1E7C1DE500880` |
| `textures/ground_emissive.png` | `A31859560110589E5DBEBCB0548C543E83F3ADF76896182FA7DE3E1635DADB0E` |
| `textures/heatshield_baseColor.png` | `2B34A77D00FEC2801B667AEEC0BABCB93A19922FE934BD2C2BB733A23638E8F8` |
| `textures/heatshield_metallicRoughness.png` | `2A8727014836E7B95F9BF77BACD95BFA3730EAB08E330D8FCFD79CF18A13D323` |
| `textures/heatshield_normal.png` | `C7BF03F4C3284897D935F9FD3A3EE853742FCC7260334181E0181EEC65072E46` |
| `textures/trunk_baseColor.png` | `E23B8D84914D76FA88C35263AAD9631436AC52C4592E43DCD5ADB2F81CEDE30D` |
| `textures/trunk_metallicRoughness.png` | `D0A511AC5E5E45BF178EA9F9051F24025574E577886471D5C59079A79A8E7F89` |
| `textures/trunk_normal.png` | `C9172720A015B5174CC59CC2FB0E8C980CEBE09304D7E29260BBCA67F5A12ECA` |

Модель нормализуется до длины ~2.6 игровых единицы и получает PBR-атлас
(альбедо, нормали, roughness, metalness, emissive). В игре корабль
пристыкован к земной орбите, имеет отдельную вкладку телеметрии
`SpaceX Dragon` и медленно вращается вокруг вертикальной оси. Витрина
`spacex_dragon_hero.png` кадрируется по реальным габаритам импорта и
показывает капсулу и trunk целиком; этот же кадр служит иллюстрацией
главы «Последний караул» (`res/story/06_last_watch.png`).

## Soyuz-MS / Мир

| Поле | Значение |
|---|---|
| Модель | Mir Station with Soyuz and Progress docked |
| Автор | [tashtego](https://sketchfab.com/tashtego) |
| Лицензия | [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) |
| Источник | <https://sketchfab.com/3d-models/mir-station-with-soyuz-and-progress-docked-bd6ac084ae5645848a67597b17665579> |
| Локальный исходник | `blender/out/mir_src/scene.gltf` |
| Загрузка | `tools/download_mir_soyuz.ps1` или `python tools/download_spacecraft_assets.py` (зеркало; оригинал — Sketchfab) |
| Экспорт | `blender/scripts/export_soyuz_game_asset.py` |
| Игровые файлы | `res/models/soyuz.obj` + 5 BMP-атласов 1024×1024 |
| Витрины | `docs/screens/soyuz_hero.png`, `docs/screens/mir_hero.png` |
| В сцене | `dock-soyuz`, масштаб `0.085` |

### Контрольные суммы Mir & Soyuz (`blender/out/mir_src/`)

| Файл | SHA256 |
|---|---|
| `scene.gltf` | `B8D0F6A482FFA4A28DCEE2360D5AB11ADBD481451D3F39EEB10F65CEDCD49B7C` |
| `scene.bin` | `61F4B26AC7514204E1AD5ACE0CC1DB72F0E2515B611ED512FA5F0F6549F85C2C` |
| `textures/Mir_Antenna_baseColor.png` | `5D2BABDAF615C17055A3D8B2C783F19E89380A9254D09CA0C1D765681E4EE78A` |
| `textures/Mir_Antenna_metallicRoughness.png` | `7BAD39AE2DD939C116B5B6E19DACC8CE5B96CBF114C0B39CC9CCC2D482E0CC32` |
| `textures/Mir_White_Isolation_baseColor.png` | `C4CA5C5B0D6F42DA9B7424E37835933A8B80493EB541F98AF71081BA02F9EA88` |
| `textures/Progress_black_Isolation_metallicRoughness.png` | `AF5DE87A18935C46D9007AA71E0F3ED24E6A43E16C9D04E88BA5686733CEC41D` |
| `textures/Progress_black_Isolation_normal.png` | `8EF5409ABEB8EC6662F4474B45E1EBFFB85CE0D3771530B1EB8F37F67BD63D46` |
| `textures/Progress_color_baseColor.png` | `4E93A916C9C43069958C57EC27FED501CA58AF2F994360DFB2370C9141D94A8C` |
| `textures/Progress_color_metallicRoughness.png` | `C2888E8F5A1654BC665A8A82736F6687548DB6660A278A9D19C5274A5877F247` |
| `textures/Progress_color_normal.png` | `CC2C16C73560DD79EB2944B32537802AACAEFE5A8D1FE4B8937B26A705EFBB66` |
| `textures/Soyuz_black_Isolation_metallicRoughness.png` | `B63C0E262FB069EB0EBB21B6DF7311D74CD89B47159FA84CCD2B2B4FA741C1C2` |
| `textures/Soyuz_color_metallicRoughness.png` | `94E69F1BD38DFD67CE8604AD75F1CD69842940139AB6E39544ACACC3A1BC1D29` |
| `textures/mir_color_baseColor.png` | `388FFE8017C755957A060CF65A4276D31EA0B26D433C8A49F1774900C249BEC3` |
| `textures/mir_color_metallicRoughness.png` | `C16410BF1CE01B8E1FB6B8E0E09EC3CED284AEF9718ED281831D96FB9301BD84` |
| `textures/mir_color_normal.png` | `6004B46318D77B6F0CB6CFAD018838E56E5E1697F29DC32B68BB96B89455ABF4` |

Из модели станции «Мир» извлекается пристыкованный «Союз»; в игре он
показан отдельным кораблём на орбите Земли с вкладкой телеметрии
`Союз-МС`. Текстуры упакованы в атлас 1024×1024, чтобы не раздувать
локальный набор ассетов.

## Масштаб в сцене

Диаметр Земли в игровых единицах — `1.16` (радиус `0.58`). После ревью
масштабы приведены к общей иерархии: станция — самое крупное сооружение,
варп-ядро и обсерватория — следующие, капсулы — меньше, зонд — самый
маленький. Все значения остаются читаемыми в широком орбитальном кадре.

| Аппарат | Габарит модели | `stScale` | Длина в мире | Доля диаметра Земли |
|---|---:|---:|---:|---:|
| ISS (станция) | 1.000 | 0.380 | 0.380 | 32.8% |
| Warp core (Sgr A*/M13) | 1.000 | 0.350 | 0.350 | 30.2% |
| Hubble/JWST (обсерватория) | 2.200 | 0.135 | 0.297 | 25.6% |
| Crew Dragon | 2.600 | 0.095 | 0.247 | 21.3% |
| Soyuz-MS | 2.800 | 0.085 | 0.238 | 20.5% |
| Helios relay probe | 2.980 | 0.060 | 0.179 | 15.4% |
| Voyager 1 (реликвия) | 3.000 | 0.100 | 0.300 | 25.9% |
| O'Neill ring colony | 4.000 | 0.060 | 0.240 | 20.7% |
| Perseverance rover | 2.600 | 0.090 | 0.234 | 20.2% |
| Artemis lunar lander | 3.000 | 0.080 | 0.240 | 20.7% |
| Solar sail | 3.600 | 0.070 | 0.252 | 21.7% |

Старый масштаб `0.28` давал Dragon `0.728` (63% диаметра Земли) и Soyuz
`0.784` (68%) — оба были длиннее модели МКС, что и создавало ощущение
гигантских кораблей. Обсерватория (`0.616`) и зонд (`0.656`) тоже
перекрывали станцию; теперь они уменьшены до `0.297` и `0.179`.
Стыковочные ореолы следуют за размерами: станция `0.28`, варп-ядро
`0.24`, обсерватория `0.20`, Dragon `0.12`, Soyuz `0.11`, зонд `0.14`.

Это сжатая иерархия для читаемости в широком орбитальном кадре, а не
физический масштаб 1:1: настоящая МКС в 13 раз длиннее Crew Dragon, и при
точном масштабе капсулы были бы меньше пикселя на фоне Земли.

## Alcubierre Warp Bubble (процедурная)

| Поле | Значение |
|---|---|
| Модель | Alcubierre Warp Bubble (процедурная генерация) |
| Автор | Astro Voyager (процедурная) |
| Лицензия | MIT (код проекта) |
| Источник | `blender/scripts/generate_warp_bubble.py` |
| Игровые файлы | `res/models/warp_bubble.obj` + 5 BMP-атласов 1024×1024 |
| В сцене | `dock-warp-bubble` (Sgr A*), масштаб `0.30` |
| Витрина | `docs/screens/warp_bubble_hero.png` |

Процедурная модель: тор (варп-катушка) + центральная сфера (пузырь) + вторая наклонённая катушка. PBR-атласы: альбедо (тёмный корпус с циановыми акцентами), нормали, roughness, metalness, emissive (свечение катушки).

## Собственные модели флота (полностью процедурные)

Все пять ассетов сгенерированы в Blender 5.2 LTS собственными скриптами —
никаких сторонних моделей, атрибуция не требуется, лицензия MIT (код проекта).
Пайплайн тот же, что у станции Aurora: high-poly → PBR-атлас (albedo/normal/
rough/metal/emissive BMP) → `res/models/<name>.obj` → `ObjModel::load`.

| Аппарат | Генератор | Треугольников | Габарит | В сцене |
|---|---|---:|---:|---|
| «Вояджер-1», реликвия в поясе Койпера | `blender/scripts/generate_voyager_probe.py` | 30 302 | 3.000 | `dock-voyager`, масштаб `0.100` |
| Марсоход «Персеверанс» у Mars | `blender/scripts/generate_fleet_models.py` | 15 316 | 2.600 | `dock-rover`, масштаб `0.090` |
| Кольцевая колония О’Нила у Earth | `blender/scripts/generate_fleet_models.py` | 25 836 | 4.000 | `dock-colony`, масштаб `0.060` |
| Лунный лендер у Moon | `blender/scripts/generate_fleet_models.py` | 9 494 | 3.000 | `dock-lander`, масштаб `0.080` |
| Солнечный парусник у Earth | `blender/scripts/generate_fleet_models.py` | 5 690 | 3.600 | `dock-sail`, масштаб `0.070` |

Витрины: `blender/out/voyager/*.png`, `blender/out/fleet/*_showcase.png`
(1600×1066, Cycles, 64 сэмпла). Перезапуск генерации:

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b -P blender/scripts/generate_voyager_probe.py
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" -b -P blender/scripts/generate_fleet_models.py
```

Устройство PBR-атласа: сетка 4×4 квадратных тайлов по 256 px (по тайлу на
слот палитры), каждый тайл генерируется из height-поля, из которого
считается normal-карта. Поэтому швы панелей, заклёпки, складки золотого
MLI, жалюзи и переплёты окон дают реальный рельеф под ключевым светом
движка, а не плоское пятно цвета. Раньше слоты писались колонками
128×1024 и растягивались на всю грань — отсюда «полосатый» корпус.

Важная ловушка: `loadBmp` в `src/ObjModel.cpp` и импорт Blender читают один и
тот же файл с противоположным V. Поэтому каждый тайл строится на
зеркальной оси строк (`yy = min(yy, t-1-yy)`), а сам тайл пишется и в свою
строку, и в зеркальную. Без этого грани попадали на пустые строки атласа,
и в игре марсоход становился чёрным силуэтом.

Особые правила интеграции:

* `dock-voyager` привязан не к Земле, а к позиции тела `voyager1`
  (`src/SolarSystem.cpp`, большая полуось 55, наклон 0.62 рад) — модель
  движется вместе с реликвией. Пока меш загружен и доки видны, золотистая
  сфера-заглушка в `SceneManager::render` не рисуется.
* Для реликвии сознательно отключён `lodScale`: на дистанции ~55 единиц
  кривая сжала бы зонд в долю пикселя задолго до подлёта, поэтому парус
  и колония сжимаются, а «Вояджер» остаётся 0.300 игровой единицы.
* Позиции стыковочных портов для всех пяти доков заданы в
  `AppGL::getDockPortTransform` и повторяют углы вращения мешей в
  `SceneManager` — иначе автоматическая стыковка целилась бы в место,
  где аппарат был на момент телепорта.
* Телеметрия: вкладки «Вояджер-1», «Марсоход», «Колония», «Лендер»,
  «Солнечный парус» в терминале верфи открываются только при стыковке
  с соответствующим доком.

## Лицензии и авторские права

- Код проекта — MIT, но **3D-модели имеют собственные лицензии**.
- Dragon: **CC BY 4.0** (автор: Jon Ross / zlsa) — требуется указание автора и ссылки на источник;
  коммерческое использование разрешено.
- Soyuz/Мир: **CC BY-NC 4.0** (автор: tashtego) — требуется указание автора, а коммерческое
  использование **запрещено** без отдельного разрешения правообладателя. Если проект будет
  поставляться коммерчески, модель необходимо заменить на permissive-ассет.
- Атрибуция продублирована в `README.md`; при изменении витрин или
  экспортных скриптов её необходимо сохранять.
- В git не коммитятся `*.obj`, `res/models/*.bmp`, `blender/out/*`; витрины
  `docs/screens/*_hero.png` коммитятся как производные изображения и тоже
  требуют атрибуции.

## Скрипты загрузки и проверки провенанса

В репозитории предусмотрены инструменты для автоматической загрузки и криптографической верификации исходников:

### 1. PowerShell-скрипт `tools/download_mir_soyuz.ps1`
Загружает файлы модели станции «Мир» и корабля «Союз» с проверенного зеркала и выполняет сверку SHA256-хэшей. Пропускает повторную загрузку, если локальный файл уже присутствует и хэш совпадает.
```powershell
# Загрузка и проверка контрольных сумм:
powershell -ExecutionPolicy Bypass -File tools/download_mir_soyuz.ps1

# Принудительная перезагрузка:
powershell -ExecutionPolicy Bypass -File tools/download_mir_soyuz.ps1 -Force
```

### 2. Кроссплатформенный Python-хелпер `tools/download_spacecraft_assets.py`
Универсальный скрипт на стандартной библиотеке Python 3 (работает на Windows, Linux, macOS):
- Проверяет целостность локальных моделей `blender/out/mir_src/` и `blender/out/spacex_src/dragon/` по контрольным суммам SHA256.
- При необходимости докачивает недостающие файлы Mir/Soyuz с зеркала.
- **Грациозно обрабатывает оффлайн-режим**: при недоступности сети выводит понятное предупреждение и не прерывает пайплайн ошибкой (код возврата `0`).
- Поддерживает строгий режим `--strict` для CI-проверок.

```bash
# Проверка имеющихся и загрузка недостающих ассетов Mir/Soyuz:
python tools/download_spacecraft_assets.py

# Только проверка локальных файлов без попытки скачивания:
python tools/download_spacecraft_assets.py --verify-only

# Принудительная повторная загрузка с зеркала:
python tools/download_spacecraft_assets.py --force

# Проверка исходников SpaceX Dragon:
python tools/download_spacecraft_assets.py --check-dragon

# Строгий режим (exit code 1 при любых ошибках сети или хэша):
python tools/download_spacecraft_assets.py --strict
```

## Воспроизведение

1. Скачать или верифицировать исходники:
   - Для Soyuz/Мира: выполнить `python tools/download_spacecraft_assets.py` или `tools/download_mir_soyuz.ps1`.
   - Для Dragon: скачать исходник со Sketchfab в `blender/out/spacex_src/dragon/` (Sketchfab требует интерактивную сессию пользователя) и проверить хэши: `python tools/download_spacecraft_assets.py --check-dragon`.
2. Экспортировать игровые ассеты:
   ```powershell
   blender -b -P blender/scripts/export_dragon_game_asset.py
   blender -b -P blender/scripts/export_soyuz_game_asset.py
   ```
3. Перерендерить витрины:
   ```powershell
   blender -b -P blender/scripts/render_spacex_dragon.py
   blender -b -P blender/scripts/render_soyuz_hero.py
   blender -b -P blender/scripts/render_mir_hero.py
   ```
4. Собрать и проверить игру:
   ```powershell
   cmake --build build-gl --config Release
   powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
   ```

## SpaceDrive Warp Tunnel (tamminen, CC BY 4.0)

| Поле | Значение |
|---|---|
| Модель | SpaceDrive / Free Tunnel wormhole (трёхслойная кинематика) |
| Автор | [tamminen](https://sketchfab.com/tamminen) (Sketchfab) |
| Лицензия | [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) — коммерческое использование разрешено при указании автора |
| Источник | <https://sketchfab.com/3d-models/spacedrive-7b88110e3d2a4bb880824dba9aa81390> |
| Локальный исходник | `blender/out/warp_tunnel_src/scene.glb` |
| Экспорт | `blender/scripts/export_warp_tunnel.py` |
| Игровые файлы | `warp_tunnel_tube.obj` (внешняя труба), `warp_tunnel_scrap.obj` (кольца обломков), `warp_tunnel_cloud.obj` (плазменный тор) + 15 BMP PBR-атласов |
| В сцене | Анимированный полёт в варп-туннеле (сверхсветовой прыжок и высокий варп) с дифференциальным вращением слоёв ($\omega_1=+0.30$, $\omega_2=-0.60$, $\omega_3=+1.20$ рад/с); экспонат `dock-warp-tunnel` (Sgr A*) |
| Витрина | `docs/screens/warp_tunnel_hero.png` |


## Warp Drive Visualisation (процедурная)

| Поле | Значение |
|---|---|
| Модель | Warp Drive Visualisation (процедурная генерация) |
| Автор | Astro Voyager (процедурная) |
| Лицензия | MIT (код проекта) |
| Источник | blender/scripts/generate_warp_drive_vis.py |
| Игровые файлы | res/models/warp_drive_vis.obj + 5 BMP-атласов 1024x1024 |
| В сцене | dock-warp-drive-vis (Sgr A*), масштаб 0.30 |
| Витрина | docs/screens/warp_drive_vis_hero.png |


## Планетарная инфраструктура колоний (собственные процедурные ассеты)

Генератор: `blender/scripts/generate_colony_assets.py` (подмножество: `-- --only nuclear hydro`).
Атлас общий и flip-safe: `blender/scripts/asset_atlas.py` (сетка 4x4 тайлов по 256 px,
normal из height-поля, зеркальные строки). Витрины: `blender/out/colony/*_showcase.png`.

| Ассет | Треугольников | Габарит | Состав |
|---|---:|---:|---|
| colony_dome | 6 950 | 3.000 | геокупол, шлюзы, антенна, прожекторы |
| colony_factory | 24 212 | 3.400 | трубная эстакада, 4 силоса с галереей, пылевой циклон, 2 трубы, козловой кран, резервуары, конвейер с отвалом, ОРУ, забор |
| colony_pad | 9 160 | 3.200 | восьмиугольная площадка, разметка, 8 beacon-пилонов, диспетчерская вышка, криотанк |
| colony_nuclear | 31 216 | 4.200 | контеймент с бандажами, крановый отсек, турбинный зал, 2 гиперболические градирни, кассетный двор, ОРУ |
| colony_hydro | 21 074 | 4.600 | арочная дамба, галерея гребня, водосброс с козырьками, водозаборные башни, здание ГЭС, водобойный бассейн, подмкосты, ОРУ |
| colony_thermal | 41 852 | 4.600 | 4 котельных блока с экранами и площадками, деаэраторы, 2 бандажные трубы, ACC-рамы с веерами, топливный склад, электрофильтр, ОРУ |
| colony_datacenter | 29 224 | 4.200 | 2 машинных зала с жалюзи, чиллерная, dry coolers, подстанция, ряд ДГУ, волоконная мачта |

Размещение: `colonyStagePlan()` определяет, что строит каждая из 24 стадий; плейсхолдер
`power` разрешается `colonyEnergyAsset()` по миру (nuclear: moon, mars; thermal: mercury,
titan; hydro: europa, ganymede, callisto, hercules-b). Объекты раскладываются спиралью по
золотому углу вокруг `colonyAnchorNormal()`, а размер задаётся долей радиуса планеты
(footprint), а не длиной меши, поэтому застройка одинаково читается и на Марсе, и на Европе.

Вехи освоения (`colonyTerraformMilestones()`) привязаны к стадии и выдают разовый RP;
`colonyTerraformState()` возвращает доли атмосферы и биосферы, которыми `SceneManager`
смешивает `u_ColA/u_ColB` поверхности к зелёному суше и синей воде. Проверка кадров:
`astro-voyager --capture-colony <dir>` (четыре мира на четырёх стадиях, сравнение кадра
с застройкой и без неё).
