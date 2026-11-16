#include "LaunchDiagnostics.h"

#include "GLCompat.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

#if defined(__linux__)
#include <unistd.h>
#endif

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace astro {

namespace {

std::ofstream g_log;
std::string g_logPath;
bool g_safeMode = false;

// Каталог с исполняемым файлом (пусто, если определить не удалось).
std::string executableDir(const char* argv0) {
  std::error_code ec;
#ifdef _WIN32
  char buf[MAX_PATH] = {0};
  const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
  if (n > 0 && n < MAX_PATH) {
    const std::filesystem::path p(buf);
    return p.parent_path().string();
  }
#elif defined(__APPLE__)
  char buf[4096] = {0};
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0) {
    const std::filesystem::path p(buf);
    return p.parent_path().string();
  }
#elif defined(__linux__)
  char buf[4096] = {0};
  const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    const std::filesystem::path p(buf);
    return p.parent_path().string();
  }
#endif
  if (argv0) {
    const std::filesystem::path p(argv0);
    if (p.has_parent_path()) {
      const std::filesystem::path abs = std::filesystem::absolute(p, ec);
      if (!ec) return abs.parent_path().string();
    }
  }
  return std::string();
}

// Первый каталог, куда мы можем писать. Порядок: рядом с exe (удобно для
// «прислать лог»), потом %LOCALAPPDATA% / ~/.local/share, потом рабочий
// каталог (установленная игра в Program Files пишет только туда, куда
// разрешено, — поэтому fallback'и важны).
std::string writableLogDir(const char* argv0) {
  std::error_code ec;
  const std::string exeDir = executableDir(argv0);
  if (!exeDir.empty()) {
    const std::filesystem::path probe = std::filesystem::path(exeDir) / ".av-write-test";
    {
      std::ofstream f(probe, std::ios::app);
      if (f.good()) {
        f.close();
        std::filesystem::remove(probe, ec);
        return exeDir;
      }
    }
    std::filesystem::remove(probe, ec);
  }
#ifdef _WIN32
  if (const char* local = std::getenv("LOCALAPPDATA")) {
    const std::filesystem::path dir = std::filesystem::path(local) / "astro-voyager";
    std::filesystem::create_directories(dir, ec);
    if (!ec) return dir.string();
  }
#else
  if (const char* home = std::getenv("HOME")) {
    const std::filesystem::path dir =
        std::filesystem::path(home) / ".local" / "share" / "astro-voyager";
    std::filesystem::create_directories(dir, ec);
    if (!ec) return dir.string();
  }
#endif
  return std::string();
}

}  // namespace

void initLaunchLog(const char* argv0) {
  if (g_log.is_open()) return;
  const std::string dir = writableLogDir(argv0);
  std::filesystem::path path =
      dir.empty() ? std::filesystem::path("astro-voyager.log")
                  : std::filesystem::path(dir) / "astro-voyager.log";
  g_log.open(path, std::ios::out | std::ios::app);
  if (g_log.is_open()) {
    g_logPath = path.string();
    const std::time_t now = std::time(nullptr);
    char stamp[32] = {0};
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    g_log << "---- astro-voyager launch " << stamp << " ----\n";
    g_log.flush();
  }
}

const std::string& launchLogPath() { return g_logPath; }

void logLaunch(const std::string& line) {
  std::cout << line << "\n";
  std::cout.flush();
  if (g_log.is_open()) {
    g_log << line << "\n";
    g_log.flush();
  }
}

std::string glCapabilitiesString() {
  std::string out;
  const auto add = [&out](const char* label, const GLubyte* value) {
    if (!value) return;
    out += label;
    out += ": ";
    out += reinterpret_cast<const char*>(value);
    out += "\n";
  };
  add("GL version", glGetString(GL_VERSION));
  add("GL renderer", glGetString(GL_RENDERER));
  add("GL vendor", glGetString(GL_VENDOR));
  add("GLSL version", glGetString(GL_SHADING_LANGUAGE_VERSION));

  GLint value = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
  out += "Max texture size: " + std::to_string(value) + "\n";
  glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &value);
  out += "Max vertex uniform components: " + std::to_string(value) + "\n";
  glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &value);
  out += "Max fragment uniform components: " + std::to_string(value) + "\n";
  glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
  out += "Context profile: ";
  out += ((value & GL_CONTEXT_CORE_PROFILE_BIT) != 0) ? "core" : "compatibility";
  out += "\n";
  return out;
}

void reportFatalError(const std::string& message) {
  std::cerr << "FATAL: " << message << "\n";
  if (g_log.is_open()) {
    g_log << "FATAL: " << message << "\n";
    g_log.flush();
  }
#ifdef _WIN32
  std::string text = message;
  if (!g_logPath.empty()) {
    text += "\n\nLog: " + g_logPath;
  }
  text +=
      "\n\nThe game requires OpenGL 3.3 (core profile) and a working GPU driver.\n"
      "Try updating the graphics driver, or launch with --safe for reduced effects.";
  MessageBoxA(nullptr, text.c_str(), "Astro Voyager - fatal error",
               MB_OK | MB_ICONERROR);
#endif
}

void setSafeModeRequested(bool value) { g_safeMode = value; }
bool safeModeRequested() { return g_safeMode; }

std::string resolveAssetPath(const std::string& relativePath) {
  if (relativePath.empty()) return relativePath;
  std::error_code ec;
  // 1) Относительно текущего каталога - обычный случай (запуск из корня проекта).
  if (std::filesystem::exists(std::filesystem::u8path(relativePath), ec)) {
    return relativePath;
  }
  // 2) Относительно каталога с exe - портативный запуск из любого места.
  static const std::string exeDir = executableDir(nullptr);
  if (!exeDir.empty()) {
    const std::filesystem::path pExe(exeDir);
    const std::filesystem::path candidate = pExe / std::filesystem::u8path(relativePath);
    if (std::filesystem::exists(candidate, ec)) {
      return candidate.string();
    }
    // 3) Разработка: exe лежит в build-gl/Release, ассеты — в корне репозитория (../../).
    const std::filesystem::path devCandidate =
        pExe.parent_path().parent_path() / std::filesystem::u8path(relativePath);
    if (std::filesystem::exists(devCandidate, ec)) {
      return devCandidate.string();
    }
  }
  return relativePath;  // не нашли - сообщение об ошибке остаётся понятным
}

}  // namespace astro
