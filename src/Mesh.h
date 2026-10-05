#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace astro {

class ScreenQuad {
 public:
  ScreenQuad();
  ~ScreenQuad();

  ScreenQuad(const ScreenQuad&) = delete;
  ScreenQuad& operator=(const ScreenQuad&) = delete;

  void draw() const;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
};

class LineMesh {
 public:
  LineMesh();
  LineMesh(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices);
  ~LineMesh();

  LineMesh(const LineMesh&) = delete;
  LineMesh& operator=(const LineMesh&) = delete;

  void setGeometry(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices);
  void drawLines() const;
  void drawPoints() const;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
  unsigned int m_indexCount = 0;
  unsigned int m_vertexCount = 0;
};

}  // namespace astro
