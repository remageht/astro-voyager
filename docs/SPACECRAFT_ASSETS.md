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
| Экспорт | `blender/scripts/export_dragon_game_asset.py` |
| Игровые файлы | `res/models/dragon.obj` + 5 BMP-атласов 2048×2048 |
| Витрина | `docs/screens/spacex_dragon_hero.png` |
| В сцене | `dock-dragon`, масштаб `0.095` |

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
| Загрузка | `tools/download_mir_soyuz.ps1` (зеркало; оригинал — Sketchfab) |
| Экспорт | `blender/scripts/export_soyuz_game_asset.py` |
| Игровые файлы | `res/models/soyuz.obj` + 5 BMP-атласов 1024×1024 |
| Витрины | `docs/screens/soyuz_hero.png`, `docs/screens/mir_hero.png` |
| В сцене | `dock-soyuz`, масштаб `0.085` |

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

- Код проекта — MIT, но **3D-модели имеют собственные лицензии**.
- Dragon: **CC BY 4.0** — требуется указание автора и ссылки на источник;
  коммерческое использование разрешено.
- Soyuz/Мир: **CC BY-NC 4.0** — требуется указание автора, а коммерческое
  использование **запрещено** без отдельного разрешения. Если проект будет
  поставляться коммерчески, модель нужно заменить на permissive-ассет.
- Атрибуция продублирована в `README.md`; при изменении витрин или
  экспортных скриптов её нужно сохранять.
- В git не коммитятся `*.obj`, `res/models/*.bmp`, `blender/out/*`; витрины
  `docs/screens/*_hero.png` коммитятся как производные изображения и тоже
  требуют атрибуции.

## Воспроизведение

1. Скачать исходники с Sketchfab (для Dragon — вручную, для Soyuz/Мира можно
   использовать `tools/download_mir_soyuz.ps1`, но он указывает на зеркало).
2. Положить их в `blender/out/spacex_src/dragon/` и `blender/out/mir_src/`.
3. Экспортировать игровые ассеты:
   ```powershell
   blender -b -P blender/scripts/export_dragon_game_asset.py
   blender -b -P blender/scripts/export_soyuz_game_asset.py
   ```
4. Перерендерить витрины:
   ```powershell
   blender -b -P blender/scripts/render_spacex_dragon.py
   blender -b -P blender/scripts/render_soyuz_hero.py
   blender -b -P blender/scripts/render_mir_hero.py
   ```
5. Собрать и проверить игру:
   ```powershell
   cmake --build build-gl --config Release
   powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
   ```

Автоматического загрузчика для Dragon пока нет: Sketchfab требует
авторизованную сессию. Это известный пробел; при появлении стабильной
публичной ссылки его стоит закрыть отдельным скриптом с проверкой SHA256.

## Warp Tunnel (tamminen, CC BY 4.0) - скачиваемые, не интегрированы

| Поле | Значение |
|---|---|
| Модель | SpaceDrive / Free Tunnel wormhole (одна геометрия) |
| Автор | tamminen (Sketchfab) |
| Лицензия | CC BY 4.0 - коммерческое использование разрешено при указании автора |
| Источник | https://sketchfab.com/3d-models/spacedrive-7b88110e3d2a4bb880824dba9aa81390 |
| Формат | GLB/glTF, 44.5k полигонов, 23.5k вершин, анимирован (loop) |
| Статус | Не скачано - требуется ручная загрузка с Sketchfab (нужен аккаунт) |

**Инструкция по загрузке:**
1. Войдите на sketchfab.com, скачайте модель (кнопка Download - GLB).
2. Поместите файл в blender/out/warp_tunnel_src/scene.gltf (+ scene.bin).
3. Запустите экспорт по шаблону export_dragon_game_asset.py (переименовав пути).


## Warp Tunnel (tamminen, CC BY 4.0)

| Поле | Значение |
|---|---|
| Модель | SpaceDrive / Free Tunnel wormhole (одна геометрия) |
| Автор | tamminen (Sketchfab) |
| Лицензия | CC BY 4.0 - коммерческое использование разрешено при указании автора |
| Источник | https://sketchfab.com/3d-models/spacedrive-7b88110e3d2a4bb880824dba9aa81390 |
| Игровые файлы | res/models/warp_tunnel.obj + 5 BMP-атласов 1024x1024 |
| В сцене | dock-warp-tunnel (Sgr A*), масштаб 0.25 |
| Витрина | docs/screens/warp_tunnel_hero.png |


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

