# CATALOG — контракт станций v0.1.0

`res/catalog.json` — единственный источник для UI-списка.
`src/Catalog.cpp:builtinCatalog` — зеркало для headless/тестов.
Расхождение — баг CI.

Поля: `id (kebab-case, unique)`, `type` из
`black_hole|quasar|constellation|galaxy|nebula`,
`name`, `description`, `spawnDistanceRs > 1.05`.

Для `black_hole` опционально `params {rs, shellRadius, step, maxSteps}`.
Дефолт Sgr A*: `rs=1.0, shell=30.0, step=0.05, maxSteps=600`.

Добавление станции: дописать оба файла + `README` таблицу + static check.
