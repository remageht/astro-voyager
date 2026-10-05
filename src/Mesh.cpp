#include "Mesh.h"

#include <GL/glew.h>

namespace astro {

ScreenQuad::ScreenQuad() {
  // Vertex positions + texcoords
  // layout 0: pos (vec3), layout 1: uv (vec2)
  const float vertices[] = {
      // pos              // uv
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
      -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
  };

  const unsigned int indices[] = {
      0, 1, 2,
      2, 3, 0
  };

  glGenVertexArrays(1, &m_vao);
  glGenBuffers(1, &m_vbo);
  glGenBuffers(1, &m_ebo);

  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  // Position: layout 0
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  // TexCoord: layout 1
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

ScreenQuad::~ScreenQuad() {
  if (m_ebo) glDeleteBuffers(1, &m_ebo);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void ScreenQuad::draw() const {
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

// -------------------------------------------------------------
// LineMesh
// -------------------------------------------------------------

LineMesh::LineMesh() = default;

LineMesh::LineMesh(const std::vector<glm::vec3>& vertices,
                   const std::vector<unsigned int>& indices) {
  setGeometry(vertices, indices);
}

LineMesh::~LineMesh() {
  if (m_ebo) glDeleteBuffers(1, &m_ebo);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void LineMesh::setGeometry(const std::vector<glm::vec3>& vertices,
                           const std::vector<unsigned int>& indices) {
  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
  }

  m_vertexCount = static_cast<unsigned int>(vertices.size());
  m_indexCount = static_cast<unsigned int>(indices.size());

  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

void LineMesh::drawLines() const {
  if (m_vao && m_indexCount > 0) {
    glBindVertexArray(m_vao);
    glLineWidth(2.0f);
    glDrawElements(GL_LINES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }
}

void LineMesh::drawPoints() const {
  if (m_vao && m_vertexCount > 0) {
    glBindVertexArray(m_vao);
    glPointSize(5.0f);
    glDrawArrays(GL_POINTS, 0, m_vertexCount);
    glBindVertexArray(0);
  }
}

}  // namespace astro
