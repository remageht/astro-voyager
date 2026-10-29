# NASA Science assets / Ассеты NASA Science

Витрины трёх научных аппаратов NASA, отрендеренные из официальных
public-domain GLB-моделей NASA 3D Resources. Это локальные тяжёлые исходники:
в git попадают только итоговые PNG-витрины, скрипт загрузки и этот манифест.

## Что добавлено

| Аппарат | Модель | Витрина | Миссия |
|---|---|---|---|
| Voyager | `Voyager Probe (B).glb` | `docs/screens/voyager_showcase.png` | Межзвёздный зонд, «Золотая пластинка», рекорд дальности |
| Hubble | `Hubble Space Telescope (A).glb` | `docs/screens/hubble_showcase.png` | Орбитальная обсерватория, глубокое поле |
| Parker Solar Probe | `Parker Solar Probe.glb` | `docs/screens/parker_showcase.png` | Солнечный зонд, теплозащитный экран |

Фон витрин — реальная карта всего неба **Deep Star Maps 2020**
(NASA/Goddard SVS, Gaia DR2: ESA/Gaia/DPAC), а не процедурное звёздное поле.

## Загрузка

```powershell
powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1
# только модели, без карты неба:
powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1 -SkipStarMap
# перекачать и проверить SHA256:
powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1 -Force
```

Куда кладутся файлы (оба пути в `.gitignore`):

```
blender/out/nasa_src/voyager_b.glb
blender/out/nasa_src/hubble_a.glb
blender/out/nasa_src/parker.glb
res/textures/starmap_2020_4k_gal.exr
```

## Рендер

```powershell
blender -b -P blender/scripts/render_nasa_showcase.py -- --all
blender -b -P blender/scripts/render_nasa_showcase.py -- --model hubble --quick
```

`--all` рендерит три финальных кадра 1280×720 (Cycles CPU, 64 spp + denoise)
в `blender/out/final_showcase/` и копирует их в `docs/screens/`.
`--quick` делает 640×360 превью в `blender/out/nasa_preview/` и не трогает
`docs/screens/`.  `--no-starmap` отключает карту неба, если EXR не скачан.

## Источники и лицензия

- NASA 3D Resources: <https://science.nasa.gov/3d-resources/>,
  зеркало <https://github.com/nasa/NASA-3D-Resources>.
  Ассеты «free and without copyright»; условия использования:
  <https://www.nasa.gov/nasa-brand-center/images-and-media/>.
- Deep Star Maps 2020: <https://svs.gsfc.nasa.gov/4851/>.
  Credit: NASA/Goddard Space Flight Center Scientific Visualization Studio.
  Gaia DR2: ESA/Gaia/DPAC.
- Модели: Voyager Probe (B), Hubble Space Telescope (A), Parker Solar Probe.
- SHA256 закреплены в `tools/fetch_nasa_assets.ps1`; скрипт откажется
  использовать файл с другим хешем.

NASA не спонсирует и не одобряет astro-voyager; кредит указан как
требуется условиями использования материалов NASA.
