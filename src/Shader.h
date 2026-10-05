#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace astro {

class Shader {
 public:
  Shader(const std::string& vertexPath, const std::string& fragmentPath);
  ~Shader();

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  void bind() const;
  void unbind() const;

  void setUniform1i(const std::string& name, int value);
  void setUniform1f(const std::string& name, float value);
  void setUniform3f(const std::string& name, float v0, float v1, float v2);
  void setUniform3f(const std::string& name, const glm::vec3& v);
  void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

  unsigned int getProgramId() const { return m_rendererId; }

 private:
  static std::string readFile(const std::string& filepath);
  static unsigned int compileShader(unsigned int type, const std::string& source);
  static unsigned int createProgram(const std::string& vertexShader,
                                    const std::string& fragmentShader);
  int getUniformLocation(const std::string& name);

  unsigned int m_rendererId = 0;
  std::unordered_map<std::string, int> m_uniformLocationCache;
};

}  // namespace astro
