#pragma once

// === SaveGame === (v3.4.0): профиль пользователя в JSON — чистый C++17 без
// GL/платформенных зависимостей (собирается в core-таргет, тестируется без GPU).
// Файлы (<профиль-директория>, см. saveProfileDir()):
//   settings.json         — глобальные настройки (пресет, звук, вид)
//   save-sandbox.json     — песочница (свободный полёт; текущий режим)
//   save-adventure.json   — сюжетный режим (слот зарезервирован под v4.0)
// Схема версионируется (kSaveSchema): файл с более новой схемой не грузим,
// неизвестные поля игнорируем, отсутствующие берут дефолты. JSON пишется
// pretty-напечатанным и человекочитаемым — продвинутые игроки могут править
// его руками; парсер терпим к порядку полей и пробелам.

#include <string>
#include <vector>

#include "Types.h"

namespace astro {

constexpr int kSaveSchema = 1;

enum class SaveMode { Sandbox = 0, Adventure = 1 };

struct SaveSettings {
  int perfPreset = 1;    // PerfPreset: 0 Low, 1 Med, 2 Ultra, 3 Custom
  int audioVolume = 70;  // 0..100
  bool muted = false;
  int viewMode = 0;      // view::ViewMode: 0 Cockpit, 1 Behind, 2 Overview
  // === v3.6.0: расширенные настройки (сложность/AA/трассировка/корабль) ===
  int difficulty = 1;           // 0 Easy, 1 Normal, 2 Hard
  int aaMode = 1;               // 0 Off, 1 FXAA
  bool shipReflections = true;  // зеркальные IBL-отражения на корпусе
  int enginePalette = 0;        // палитра выхлопа: 0 Red, 1 Ion, 2 Emerald, 3 Plasma
  float renderScale = 0.0f;     // 0 = по пресету (кастомный r_s)
  float shellRadius = 0.0f;     // 0 = по пресету
  float stepSize = 0.0f;        // 0 = по пресету
  int maxSteps = 0;             // 0 = по пресету
  // === v3.6.0: управление (мышь + переназначение клавиш) ===
  float mouseSensitivity = 1.0f;  // 0.3..3.0, 1 = стандарт
  int invertY = 0;                // 1 = инвертировать вертикаль мыши
  int keyColony = 67;             // GLFW: C — основать колонию
  int keyView = 86;               // GLFW: V — цикл видов
  int keyMap = 78;                // GLFW: N — карта/навигация
  int keyJournal = 74;            // GLFW: J — журнал
  int keyTour = 84;               // GLFW: T — кинематографический тур
};

struct SaveCustom {
  std::string id;
  std::string name;
  Vec3 pos;
  Vec3 vel;
  float radius = 0.3f;
  Vec3 color{0.62, 0.58, 0.52};
  Vec3 colorB{0.37, 0.35, 0.31};
  int type = 2;  // u_Type: 0 star, 1 gas, 2 rocky, 3 ice
  int detail = 0;
  float ringOuter = 0.0f;
  float seed = 1.0f;
};

struct SaveProgress {
  std::vector<std::string> visited;   // посещённые станции (ачивки)
  std::vector<std::string> unlocked;  // открытые ачивки (id)
  std::vector<std::string> route;     // история маршрута
  // === v3.6.0: колонии ===
  std::vector<std::string> colonies;  // id колонизированных тел (mars, hercules-b)
  double minBhDistance = 1e9;
  bool exported = false;
  bool imported = false;
  bool capturedHorizon = false;
};

struct SaveSession {
  std::string station = "sol";
  Vec3 camPos{0.0, 0.0, 10.0};
  double yaw = 0.0;
  double pitch = 0.0;
  float timeScale = 1.0f;  // sim-секунд на wall-секунду (пресеты 1..1000)
  float fuel = 100.0f;
  float shield = 100.0f;
  int shipStyle = 0;  // 0 Dart, 1 Ring, 2 Shuttle
};

struct SaveGame {
  int schema = kSaveSchema;
  SaveMode mode = SaveMode::Sandbox;
  std::string savedUtc;  // ISO-8601 UTC, информационное поле
  SaveProgress progress;
  std::vector<SaveCustom> customs;  // объекты редактора
  SaveSession session;
};

// --- Сериализация (pretty JSON, round-trip через свой же парсер) ---
std::string saveSerialize(const SaveGame& g);
bool saveDeserialize(const std::string& text, SaveGame& out, std::string& err);

std::string saveSerializeSettings(const SaveSettings& s);
bool saveDeserializeSettings(const std::string& text, SaveSettings& out,
                             std::string& err);

// --- Пути (чистые варианты — для тестов; обёртки берут env-директорию) ---
std::string saveProfileDir();  // %APPDATA% / ~/.config / ~/Library/Application Support
std::string saveFilePathFor(const std::string& dir, SaveMode mode);
std::string saveSettingsPathFor(const std::string& dir);
std::string saveFilePath(SaveMode mode);
std::string saveSettingsFilePath();
const char* saveModeName(SaveMode mode);  // "sandbox" / "adventure"

// --- Файловый ввод-вывод (директория создаётся при записи) ---
bool saveWriteFile(const std::string& path, const std::string& text,
                   std::string& err);
bool saveReadFile(const std::string& path, std::string& out, std::string& err);
bool saveFileExists(const std::string& path);

std::string saveUtcNowISO8601();  // для поля savedUtc

}  // namespace astro
