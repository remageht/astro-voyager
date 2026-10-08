# ROADMAP — до прода

## 0.1.0 core (этот скелет) — DONE

CLI, каталог 6 станций, CPU-эталон геодезики, шейдеры портированы,
CI Windows+Linux, Docker build, static checks.

## 0.2.0 gl-window — DONE

- `Window` (GLFW) + `Shader` лоадер + fullscreen quad.
- `SceneManager`: телепорт `sgra/ori/qso-3c273`.
- ImGui: список станций, слайдеры `shell/step/maxSteps/FOV`, FPS.
- Ручной прогон WASD + скриншоты в `docs/screens/`.

## 0.3.0 traveler — DONE

- Все 6 станций, cubemap из HDR, `lines` для ori/uma.
- `BlackHoleInteraction`-тесты как в blackHole.
- Релиз `v0.3.0` с zip: exe + shaders + catalog.json (без HDR).

## 1.0.0 prod — DONE

- Версионирование `VERSION` (`1.0.0`) + `CHANGELOG.md`.
- Состав релизного zip, правила исключений и шаги верификации в `docs/RELEASE.md`.
- Perf-профили: комбобокс и пресеты Low / Med / Ultra / Custom в `SceneManager` и ImGui.
- Фиксация осознанной модели Sgr A*: чистый Schwarzschild-RK4 raytracing без диска/доплера в `bh.frag` (аккреционный диск и релятивистские джеты вынесены в станцию квазара `3C 273`).
- Документирование физических лимитов в `README.md` и `docs/ARCHITECTURE.md` (нет метрики Керра / спина).
- 100% зеленые проверки: `cmake build`, `ctest`, `test_static_checks.ps1`, скриншоты всех станций.

## Будущее развитие (v2.0 ideas)

- Уравнение Бине $u(\varphi)$: независимая валидация CPU-интегратора геодезических `src/Geodesic.cpp` аналитическим уравнением Бине $d^2u/d\varphi^2 + u = 3M u^2$ (`src/Binet.cpp`, CLI флаг `--demo-binet`) — DONE.
- Вращающаяся черная дыра в метрике Керра (спин $a \ne 0$, эргосфера, расщепление фотонных орбит).
- Адаптивный шаг интегрирования Рунге-Кутты (RKF45 / Dormand-Prince) для ускорения лучей вдали от дыры.
- Эффект гравитационного красного смещения на текстуре скайбокса.
- Bloom / HDR tone mapping пост-процессинг.

