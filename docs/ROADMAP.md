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

## 1.0.0 prod

- Версионирование `VERSION` + `CHANGELOG`, тег `v1.0.0`.
- Подпись артефактов, `docs/RELEASE.md`.
- Perf-профили: step/maxSteps пресеты Low/Med/Ultra.
- Известные лимиты вынесены в README (нет Керра/диска/redshift).
