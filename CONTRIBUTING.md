# Contributing to Astro-Voyager / Руководство для контрибьюторов

Спасибо за интерес к развитию проекта **Astro-Voyager**! Мы приветствуем вклад в физическое ядро, шейдеры, астрономические каталоги, документацию и сюжетные ветки.

---

## 🚀 Быстрый старт / Quick Start

### Требования
* Компилятор C++20 (MSVC 2022+, GCC 11+, Clang 14+ или Apple Clang)
* CMake 3.20+
* Git
* Видеокарта с поддержкой OpenGL 3.3 Core

### Сборка проекта
```bash
# Клонирование
git clone https://github.com/remageht/astro-voyager.git
cd astro-voyager

# Конфигурация и сборка
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## 🧪 Верификация и тесты / Verification & Testing

Перед отправкой любых изменений **обязательно** выполните локальные проверки:

```powershell
# 1. Запуск статических проверок и аудита кода
powershell -ExecutionPolicy Bypass -File ./tests/test_static_checks.ps1

# 2. Прогон эталонных численных тестов
./build/Release/astro-voyager.exe --demo-geodesic
./build/Release/astro-voyager.exe --demo-binet
./build/Release/astro-voyager.exe --demo-kerr
```

---

## 📐 Правила разработки и рабочий процесс (Workflow)

1. **GitHub Issues:**
   * Каждое изменение, баг или фича связываются с соответствующим Issue в репозитории.
   * **Правило:** Issue закрывается только после того, как исправление полностью верифицировано тестами.

2. **Ветки и Pull Requests:**
   * Разработка ведется в тематических ветках (`feat/your-feature` или `fix/issue-description`).
   * Для слияния изменений создается Pull Request в ветку `main` с заполненным шаблоном PR.

3. **Сообщения коммитов (Conventional Commits):**
   * `feat(...)`: новая функциональность (например, `feat(physics): add Carter equations for Kerr ergosphere`).
   * `fix(...)`: исправление ошибки (например, `fix(gl): resolve buffer overrun in texture loader`).
   * `docs(...)`: обновление документации, каталогов или Wiki.
   * `chore(...)`: служебные изменения, обновление зависимостей и CI.
