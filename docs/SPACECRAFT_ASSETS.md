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
`SpaceX Dragon` и медленно вращается вокруг вертикальной оси.

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
масштабы приведены к общей иерархии: капсулы заметно меньше станции,
но всё ещё читаются в широком орбитальном кадре.

| Аппарат | Габарит модели | `stScale` | Длина в мире | Доля диаметра Земли |
|---|---:|---:|---:|---:|
| Crew Dragon | 2.600 | 0.095 | 0.247 | 21.3% |
| Soyuz-MS | 2.800 | 0.085 | 0.238 | 20.5% |
| ISS (станция) | 1.000 | 0.380 | 0.380 | 32.8% |

Старый масштаб `0.28` давал Dragon `0.728` (63% диаметра Земли) и Soyuz
`0.784` (68%) — оба были длиннее модели МКС, что и создавало ощущение
гигантских кораблей. Стыковочные ореолы уменьшены вместе с кораблями:
`0.12` для Dragon и `0.11` для Soyuz.

## Лицензии и ограничения

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
