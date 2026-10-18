#include "../src/SaveGame.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool close(double a, double b, double eps = 1e-6) {
  return std::fabs(a - b) <= eps;
}

bool sameVec(const astro::Vec3& a, const astro::Vec3& b) {
  return close(a.x, b.x) && close(a.y, b.y) && close(a.z, b.z);
}

}  // namespace

int main() {
  // Test 1: полный round-trip SaveGame (прогресс, объекты, сессия)
  {
    astro::SaveGame g;
    g.mode = astro::SaveMode::Sandbox;
    g.savedUtc = "2026-10-16T12:00:00Z";
    g.progress.visited = {"sol", "sgra", "m31"};
    g.progress.unlocked = {"first_flight", "bh_hunter"};
    g.progress.route = {"sol", "sgra"};
    g.progress.colonies = {"mars", "hercules-b"};  // v3.6.0
    g.progress.minBhDistance = 2.345;
    g.progress.exported = true;
    g.progress.imported = false;
    g.progress.capturedHorizon = true;
    g.progress.asteroidDodges = 3;  // v3.7.0

    astro::SaveCustom c1;
    c1.id = "planet-3";
    c1.name = "Планета 3";
    c1.pos = {1.5, -2.0, 3.25};
    c1.vel = {0.1, 0.0, -0.2};
    c1.radius = 0.85f;
    c1.color = {0.5, 0.25, 0.125};
    c1.colorB = {0.1, 0.2, 0.3};
    c1.type = 1;
    c1.detail = 7;
    c1.ringOuter = 1.75f;
    c1.seed = 42.0f;
    g.customs.push_back(c1);

    astro::SaveCustom c2;
    c2.id = "star-7";
    c2.name = "Star 7";
    g.customs.push_back(c2);

    g.session.station = "m31";
    g.session.camPos = {-4.5, 0.5, 9.75};
    g.session.yaw = -1.23456789;
    g.session.pitch = 0.4567;
    g.session.timeScale = 300.0f;
    g.session.fuel = 73.5f;
    g.session.shield = 42.25f;
    g.session.shipStyle = 2;

    const std::string text = astro::saveSerialize(g);
    astro::SaveGame out;
    std::string err;
    if (!astro::saveDeserialize(text, out, err)) {
      std::cerr << "Test 1 failed: " << err << '\n';
      return 1;
    }
    if (out.schema != astro::kSaveSchema || out.mode != g.mode ||
        out.savedUtc != g.savedUtc) {
      std::cerr << "Test 1 failed: header mismatch\n";
      return 1;
    }
    if (out.progress.visited != g.progress.visited ||
        out.progress.unlocked != g.progress.unlocked ||
        out.progress.route != g.progress.route ||
        out.progress.colonies != g.progress.colonies ||
        !close(out.progress.minBhDistance, g.progress.minBhDistance) ||
        out.progress.exported != g.progress.exported ||
        out.progress.imported != g.progress.imported ||
        out.progress.capturedHorizon != g.progress.capturedHorizon ||
        out.progress.asteroidDodges != 3) {
      std::cerr << "Test 1 failed: progress mismatch\n";
      return 1;
    }
    if (out.customs.size() != 2) {
      std::cerr << "Test 1 failed: customs size " << out.customs.size() << '\n';
      return 1;
    }
    const astro::SaveCustom& oc1 = out.customs[0];
    if (oc1.id != c1.id || oc1.name != c1.name || !sameVec(oc1.pos, c1.pos) ||
        !sameVec(oc1.vel, c1.vel) || !close(oc1.radius, c1.radius) ||
        !sameVec(oc1.color, c1.color) || !sameVec(oc1.colorB, c1.colorB) ||
        oc1.type != c1.type || oc1.detail != c1.detail ||
        !close(oc1.ringOuter, c1.ringOuter) || !close(oc1.seed, c1.seed)) {
      std::cerr << "Test 1 failed: custom 1 mismatch\n";
      return 1;
    }
    if (out.session.station != g.session.station ||
        !sameVec(out.session.camPos, g.session.camPos) ||
        !close(out.session.yaw, g.session.yaw) ||
        !close(out.session.pitch, g.session.pitch) ||
        !close(out.session.timeScale, g.session.timeScale) ||
        !close(out.session.fuel, g.session.fuel) ||
        !close(out.session.shield, g.session.shield) ||
        out.session.shipStyle != g.session.shipStyle) {
      std::cerr << "Test 1 failed: session mismatch\n";
      return 1;
    }
  }

  // Test 2: настройки round-trip
  {
    astro::SaveSettings s;
    s.perfPreset = 2;
    s.audioVolume = 41;
    s.muted = true;
    s.viewMode = 1;
    // v3.6.0: расширенные настройки (сложность/AA/палитра/кастомный рендер).
    s.difficulty = 2;
    s.aaMode = 0;
    s.shipReflections = false;
    s.enginePalette = 3;
    s.renderScale = 0.85f;
    s.shellRadius = 42.0f;
    s.stepSize = 0.033f;
    s.maxSteps = 900;
    // v3.6.0: управление (мышь + переназначение клавиш).
    s.mouseSensitivity = 1.35f;
    s.invertY = 1;
    s.keyColony = 88;   // X
    s.keyView = 67;     // C
    s.keyMap = 87;      // W
    s.keyJournal = 81;  // Q
    s.keyTour = 90;     // Z
    const std::string text = astro::saveSerializeSettings(s);
    astro::SaveSettings out;
    std::string err;
    if (!astro::saveDeserializeSettings(text, out, err)) {
      std::cerr << "Test 2 failed: " << err << '\n';
      return 1;
    }
    if (out.perfPreset != s.perfPreset || out.audioVolume != s.audioVolume ||
        out.muted != s.muted || out.viewMode != s.viewMode ||
        out.difficulty != s.difficulty || out.aaMode != s.aaMode ||
        out.shipReflections != s.shipReflections ||
        out.enginePalette != s.enginePalette ||
        !close(out.renderScale, s.renderScale, 1e-5) ||
        !close(out.shellRadius, s.shellRadius, 1e-4) ||
        !close(out.stepSize, s.stepSize, 1e-5) ||
        out.maxSteps != s.maxSteps ||
        !close(out.mouseSensitivity, s.mouseSensitivity, 1e-5) ||
        out.invertY != s.invertY || out.keyColony != s.keyColony ||
        out.keyView != s.keyView || out.keyMap != s.keyMap ||
        out.keyJournal != s.keyJournal || out.keyTour != s.keyTour) {
      std::cerr << "Test 2 failed: settings mismatch\n";
      return 1;
    }
  }

  // Test 3: битый JSON -> false + непустая ошибка
  {
    astro::SaveGame out;
    std::string err;
    if (astro::saveDeserialize("{\"schema\":1,\"session\":{", out, err)) {
      std::cerr << "Test 3 failed: expected failure on truncated JSON\n";
      return 1;
    }
    if (err.empty()) {
      std::cerr << "Test 3 failed: expected non-empty error\n";
      return 1;
    }
  }

  // Test 4: пустой текст -> false
  {
    astro::SaveGame out;
    std::string err;
    if (astro::saveDeserialize("", out, err)) {
      std::cerr << "Test 4 failed: expected failure on empty text\n";
      return 1;
    }
  }

  // Test 5: схема из будущего -> false, ошибка упоминает schema
  {
    astro::SaveGame out;
    std::string err;
    if (astro::saveDeserialize("{\"schema\":99}", out, err)) {
      std::cerr << "Test 5 failed: expected failure on newer schema\n";
      return 1;
    }
    if (err.find("schema") == std::string::npos) {
      std::cerr << "Test 5 failed: error does not mention schema: " << err << '\n';
      return 1;
    }
  }

  // Test 6: без schema -> false
  {
    astro::SaveGame out;
    std::string err;
    if (astro::saveDeserialize("{\"mode\":\"sandbox\"}", out, err)) {
      std::cerr << "Test 6 failed: expected failure without schema\n";
      return 1;
    }
  }

  // Test 7: неизвестные поля игнорируются, отсутствующие — дефолты
  {
    const std::string text =
        "{\"schema\":1,\"unknown\":{\"x\":[1,2]},\"session\":{\"station\":\"sgra\"}}";
    astro::SaveGame out;
    std::string err;
    if (!astro::saveDeserialize(text, out, err)) {
      std::cerr << "Test 7 failed: " << err << '\n';
      return 1;
    }
    if (out.session.station != "sgra" || !close(out.session.fuel, 100.0) ||
        !close(out.session.shield, 100.0) || !out.customs.empty() ||
        !out.progress.visited.empty() || !out.progress.colonies.empty()) {
      std::cerr << "Test 7 failed: defaults/missing-field handling\n";
      return 1;
    }
  }

  // Test 8: запись/чтение файла (с автосозданием директории) + отсутствие файла
  {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "astro_save_test" / "nested";
    const std::string path = (dir / "save-sandbox.json").string();
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "astro_save_test");

    astro::SaveGame g;
    g.session.station = "ori";
    const std::string text = astro::saveSerialize(g);
    std::string err;
    if (!astro::saveWriteFile(path, text, err)) {
      std::cerr << "Test 8 failed: write: " << err << '\n';
      return 1;
    }
    if (!astro::saveFileExists(path)) {
      std::cerr << "Test 8 failed: file missing after write\n";
      return 1;
    }
    std::string readBack;
    if (!astro::saveReadFile(path, readBack, err)) {
      std::cerr << "Test 8 failed: read: " << err << '\n';
      return 1;
    }
    if (readBack != text) {
      std::cerr << "Test 8 failed: byte mismatch after round-trip\n";
      return 1;
    }
    astro::SaveGame out;
    if (!astro::saveDeserialize(readBack, out, err) ||
        out.session.station != "ori") {
      std::cerr << "Test 8 failed: parsed back state mismatch\n";
      return 1;
    }
    std::string missing;
    if (astro::saveReadFile((dir / "nope.json").string(), missing, err)) {
      std::cerr << "Test 8 failed: expected failure on missing file\n";
      return 1;
    }
    if (astro::saveFileExists((dir / "nope.json").string())) {
      std::cerr << "Test 8 failed: exists() true for missing file\n";
      return 1;
    }
    std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                "astro_save_test");
  }

  // Test 9: пути слотов
  {
    const std::string dir = "/tmp/x";
    if (astro::saveFilePathFor(dir, astro::SaveMode::Sandbox) !=
        "/tmp/x/save-sandbox.json") {
      std::cerr << "Test 9 failed: sandbox path\n";
      return 1;
    }
    if (astro::saveFilePathFor(dir, astro::SaveMode::Adventure) !=
        "/tmp/x/save-adventure.json") {
      std::cerr << "Test 9 failed: adventure path\n";
      return 1;
    }
    if (astro::saveSettingsPathFor(dir) != "/tmp/x/settings.json") {
      std::cerr << "Test 9 failed: settings path\n";
      return 1;
    }
    if (astro::saveProfileDir().empty()) {
      std::cerr << "Test 9 failed: profile dir empty\n";
      return 1;
    }
  }

  // Test 10: экранирование (кавычки, слэши, переводы строк, юникод) выживает
  {
    astro::SaveGame g;
    astro::SaveCustom c;
    c.id = "planet-9";
    c.name = "«Планета» \\ \"line\"\nbreak\tend";
    g.customs.push_back(c);
    const std::string text = astro::saveSerialize(g);
    astro::SaveGame out;
    std::string err;
    if (!astro::saveDeserialize(text, out, err)) {
      std::cerr << "Test 10 failed: " << err << '\n';
      return 1;
    }
    if (out.customs.size() != 1 || out.customs[0].name != c.name) {
      std::cerr << "Test 10 failed: escaped name mismatch\n";
      return 1;
    }
  }

  // Test 11: режим adventure пишется строкой и парсится обратно
  {
    astro::SaveGame g;
    g.mode = astro::SaveMode::Adventure;
    const std::string text = astro::saveSerialize(g);
    if (text.find("\"adventure\"") == std::string::npos) {
      std::cerr << "Test 11 failed: mode string missing\n";
      return 1;
    }
    astro::SaveGame out;
    std::string err;
    if (!astro::saveDeserialize(text, out, err) ||
        out.mode != astro::SaveMode::Adventure) {
      std::cerr << "Test 11 failed: adventure mode round-trip\n";
      return 1;
    }
  }

  // Test 12: отрицательные/дробные числа и float-поля схемы не портятся
  {
    astro::SaveGame g;
    g.session.camPos = {-123.456789, 0.000123456, -0.5};
    g.session.yaw = -3.14159265;
    g.session.fuel = 0.125f;
    const std::string text = astro::saveSerialize(g);
    astro::SaveGame out;
    std::string err;
    if (!astro::saveDeserialize(text, out, err)) {
      std::cerr << "Test 12 failed: " << err << '\n';
      return 1;
    }
    if (!sameVec(out.session.camPos, g.session.camPos) ||
        !close(out.session.yaw, g.session.yaw) ||
        !close(out.session.fuel, g.session.fuel, 1e-5)) {
      std::cerr << "Test 12 failed: numeric round-trip\n";
      return 1;
    }
  }

  // Test 13 (v3.6.0): старый файл настроек без новых полей - дефолты.
  {
    const std::string text =
        "{\"schema\":1,\"perfPreset\":2,\"audioVolume\":50,\"muted\":false,"
        "\"viewMode\":1}";
    astro::SaveSettings out;
    std::string err;
    if (!astro::saveDeserializeSettings(text, out, err)) {
      std::cerr << "Test 13 failed: " << err << '\n';
      return 1;
    }
    if (out.perfPreset != 2 || out.difficulty != 1 || out.aaMode != 1 ||
        !out.shipReflections || out.enginePalette != 0 ||
        out.renderScale != 0.0f || out.shellRadius != 0.0f ||
        out.stepSize != 0.0f || out.maxSteps != 0) {
      std::cerr << "Test 13 failed: defaults for missing fields\n";
      return 1;
    }
  }

  std::cout << "save tests: ALL PASS\n";
  return 0;
}
