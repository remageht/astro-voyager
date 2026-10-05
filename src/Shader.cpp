#include "Shader.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace astro {

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
  std::string vertexSource = readFile(vertexPath);
  std::string fragmentSource = readFile(fragmentPath);
  m_rendererId = createProgram(vertexSource, fragmentSource);
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

void Shader::setUniform3f(const std::string& name, const glm::vec3& v) {
  setUniform3f(name, v.x, v.y, v.z);
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
  std::ifstream file(filepath);
  if (!file.is_open()) {
    // Try with res/ prefix or relative to executable
    std::string fallback = "../" + filepath;
    file.open(fallback);
    if (!file.is_open()) {
      throw std::runtime_error("Shader::readFile - Failed to open shader file: " + filepath);
    }
  }
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
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
  unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
  unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

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
