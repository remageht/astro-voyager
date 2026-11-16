#pragma once

#include <string>

namespace astro {

// === Диагностика запуска =================================================
// Проблема, которую это закрывает: на части машин (старые драйверы AMD,
// Mesa без 3.3 core) игра показывала белый экран и молча закрывалась —
// исключение уходило в stderr, которого в оконном приложении никто не
// видит. Теперь:
//   * всё важное пишется в astro-voyager.log рядом с exe;
//   * фатальная ошибка показывается пользователю диалогом (Windows) и
//     печатается в консоль;
//   * `--diagnostics` печатает версии GL/драйвера и пишет их в лог.

// Открывает лог-файл (append). Без SafeMode — ничего не меняет.
void initLaunchLog(const char* argv0);
// Дописывает строку в лог (с меткой времени) и в stdout.
void logLaunch(const std::string& line);
// Путь к лог-файлу (пустая строка, если лог недоступен).
const std::string& launchLogPath();
// Возможности текущего GL-контекста (требует созданного контекста).
std::string glCapabilitiesString();
// Фатальная ошибка: лог + stderr + диалог пользователю (Windows).
void reportFatalError(const std::string& message);
// Безопасный режим (--safe): низкий пресет, без FXAA — для слабых GPU.
void setSafeModeRequested(bool value);
bool safeModeRequested();

}  // namespace astro
