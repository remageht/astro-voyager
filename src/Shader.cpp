#include "Shader.h"

#include "EmbeddedShaders.h"  // v3.6.0: шейдеры, вшитые в бинарник
#include "GLCompat.h"
#include "LaunchDiagnostics.h"
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace astro {

namespace {

// Аварийные шейдеры: если исходный не компилируется (экзотический драйвер,
// например старый AMD), игра продолжает работать — объект рисуется
// простым цветом или не рисуется вовсе. Раньше любая такая ошибка
// уничтожала процесс: игрок видел белый экран и закрытие.
const char* kFallbackFrag = R"(#version 330 core
// Аварийный фрагментный шейдер: ровный тёмный цвет вместо эффекта.
out vec4 fragColor;
void main() { fragColor = vec4(0.04, 0.06, 0.10, 1.0); }
)";

const char* kFallbackVert = R"(#version 330 core
// Аварийный вершинный шейдер: геометрия уходит за пределы NDC и
// отсекается — объект не виден, но сцена продолжает рисоваться.
layout(location = 0) in vec3 position;
void main() { gl_Position = vec4(0.0, 0.0, 2.0, 1.0); }
)";

}  // namespace

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
  std::string vertexSource = readFile(vertexPath);
  std::string fragmentSource = readFile(fragmentPath);
  try {
    m_rendererId = createProgram(vertexSource, fragmentSource);
  } catch (const std::exception& e) {
    // Мягкая деградация: падать нельзя — иначе у игрока белый экран и
    // закрытие игры. Имя файла в сообщении: без него не понять, какой
    // шейдер не собрался (у разных драйверов отваливаются разные).
    const std::string detail = std::string(e.what()) +
                               "\nShaders: " + vertexPath + " + " + fragmentPath;
    logLaunch("[Shader] FALLBACK (" + fragmentPath + "): " + detail);
    try {
      // Сначала пробуем оставить рабочий вершинный шейдер и подменить
      // только фрагментный — геометрия останется на месте.
      m_rendererId = createProgram(vertexSource, kFallbackFrag);
    } catch (...) {
      try {
        m_rendererId = createProgram(kFallbackVert, kFallbackFrag);
      } catch (...) {
        throw std::runtime_error(detail);
      }
    }
  }
}

Shader::~Shader() {
  if (m_rendererId != 0) {
    glDeleteProgram(m_rendererId);
  }
}

void Shader::bind() const {
  glUseProgram(m_rendererId);
}

void Shader::unbind() const {
  glUseProgram(0);
}

void Shader::setUniform1i(const std::string& name, int value) {
  int loc = getUniformLocation(name);
  if (loc != -1) {
    glUniform1i(loc, value);
  }
}

void Shader::setUniform1f(const std::string& name, float value) {
  int loc = getUniformLocation(name);
  if (loc != -1) {
    glUniform1f(loc, value);
  }
}

void Shader::setUniform3f(const std::string& name, float v0, float v1, float v2) {
  int loc = getUniformLocation(name);
  if (loc != -1) {
    glUniform3f(loc, v0, v1, v2);
  }
}

void Shader::setUniform2f(const std::string& name, float v0, float v1) {
  int loc = getUniformLocation(name);
  if (loc != -1) {
    glUniform2f(loc, v0, v1);
  }
}

void Shader::setUniform3f(const std::string& name, const glm::vec3& v) {
  setUniform3f(name, v.x, v.y, v.z);
}

void Shader::setUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
  int loc = getUniformLocation(name);
  if (loc != -1) {
    glUniform4f(loc, v0, v1, v2, v3);
  }
}

void Shader::setUniformMat4f(const std::string& name, const glm::mat4& matrix) {
  int loc = getUniformLocation(name);
  if (loc != -1) {
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
  }
}

int Shader::getUniformLocation(const std::string& name) {
  auto it = m_uniformLocationCache.find(name);
  if (it != m_uniformLocationCache.end()) {
    return it->second;
  }

  int loc = glGetUniformLocation(m_rendererId, name.c_str());
  if (loc == -1) {
    // Note: uniforms can be optimized out by GLSL compiler if unused
  }
  m_uniformLocationCache[name] = loc;
  return loc;
}

std::string Shader::readFile(const std::string& filepath) {
  const std::string resolved = resolveAssetPath(filepath);
  std::ifstream file(resolved);
  if (!file.is_open()) {
    // Try with res/ prefix or relative to executable
    std::string fallback = "../" + filepath;
    file.open(fallback);
  }
  if (file.is_open()) {
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
  }
  // v3.6.0 (portable release): диска нет рядом (одиночный exe «с коробки») —
  // берём шейдер из таблицы, вшитой в бинарник на этапе configure
  // (packaging/embed_shaders.cmake -> EmbeddedShaders.cpp).
  if (const char* embedded = embeddedShader(filepath)) {
    return embedded;
  }
  throw std::runtime_error("Shader::readFile - Failed to open shader file: " + filepath);
}

unsigned int Shader::compileShader(unsigned int type, const std::string& source) {
  unsigned int id = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);

  int result = 0;
  glGetShaderiv(id, GL_COMPILE_STATUS, &result);
  if (result == GL_FALSE) {
    int length = 0;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> message(length + 1);
    glGetShaderInfoLog(id, length, &length, message.data());
    glDeleteShader(id);
    std::string stage = (type == GL_VERTEX_SHADER ? "vertex" : "fragment");
    throw std::runtime_error("Failed to compile " + stage + " shader:\n" + message.data());
  }

  return id;
}

unsigned int Shader::createProgram(const std::string& vertexShader,
                                   const std::string& fragmentShader) {
  unsigned int program = glCreateProgram();
  unsigned int vs = 0;
  unsigned int fs = 0;
  try {
    vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
  } catch (...) {
    if (vs != 0) glDeleteShader(vs);
    if (fs != 0) glDeleteShader(fs);
    if (program != 0) glDeleteProgram(program);
    throw;
  }

  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  int linkStatus = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus == GL_FALSE) {
    int length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::vector<char> message(length + 1);
    glGetProgramInfoLog(program, length, &length, message.data());
    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteProgram(program);
    throw std::runtime_error(std::string("Shader linking failed:\n") + message.data());
  }

  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}

}  // namespace astro
