#include "Mesh.h"

#include <GL/glew.h>

#include <cmath>
#include <cstdint>

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

// -------------------------------------------------------------
// StarPoints
// -------------------------------------------------------------

StarPoints::StarPoints() = default;

StarPoints::~StarPoints() {
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void StarPoints::setStars(const std::vector<glm::vec3>& positions,
                          const std::vector<float>& mags) {
  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
  }

  size_t count = positions.size();
  if (mags.size() < count) count = mags.size();
  m_vertexCount = static_cast<unsigned int>(count);

  std::vector<float> interleaved;
  interleaved.reserve(count * 4);
  for (size_t i = 0; i < count; ++i) {
    interleaved.push_back(positions[i].x);
    interleaved.push_back(positions[i].y);
    interleaved.push_back(positions[i].z);
    interleaved.push_back(mags[i]);
  }

  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float),
               interleaved.data(), GL_STATIC_DRAW);

  // Position: layout 0 (vec3), magnitude: layout 1 (float)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void StarPoints::draw() const {
  if (!m_vao || m_vertexCount == 0) return;
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glBindVertexArray(m_vao);
  glDrawArrays(GL_POINTS, 0, m_vertexCount);
  glBindVertexArray(0);
  glDisable(GL_BLEND);
}

// -------------------------------------------------------------
// SphereMesh
// -------------------------------------------------------------

SphereMesh::SphereMesh() = default;

SphereMesh::~SphereMesh() {
  if (m_ebo) glDeleteBuffers(1, &m_ebo);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void SphereMesh::setSphere(int rings, int sectors) {
  if (rings < 2) rings = 2;
  if (sectors < 3) sectors = 3;

  const float kPi = 3.14159265358979323846f;

  // Interleaved pos (vec3) + normal (vec3), unit radius.
  std::vector<float> verts;
  verts.reserve(static_cast<size_t>(rings + 1) * static_cast<size_t>(sectors + 1) * 6u);
  for (int i = 0; i <= rings; ++i) {
    float phi = kPi * static_cast<float>(i) / static_cast<float>(rings);
    float sp = std::sin(phi);
    float cp = std::cos(phi);
    for (int j = 0; j <= sectors; ++j) {
      float theta = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(sectors);
      float x = sp * std::cos(theta);
      float y = cp;
      float z = sp * std::sin(theta);
      // pos
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(z);
      // normal (unit sphere: same as pos)
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(z);
    }
  }

  std::vector<unsigned int> idx;
  idx.reserve(static_cast<size_t>(rings) * static_cast<size_t>(sectors) * 6u);
  unsigned int stride = static_cast<unsigned int>(sectors + 1);
  for (int i = 0; i < rings; ++i) {
    for (int j = 0; j < sectors; ++j) {
      unsigned int a = static_cast<unsigned int>(i) * stride + static_cast<unsigned int>(j);
      unsigned int b = a + stride;
      unsigned int c = a + 1u;
      unsigned int d = b + 1u;
      idx.push_back(a);
      idx.push_back(b);
      idx.push_back(c);
      idx.push_back(c);
      idx.push_back(b);
      idx.push_back(d);
    }
  }

  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
  }

  m_indexCount = static_cast<unsigned int>(idx.size());

  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(),
               GL_STATIC_DRAW);

  // Pos: layout 0 (vec3), normal: layout 1 (vec3), stride 6 floats.
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void SphereMesh::draw() const {
  if (!m_vao || m_indexCount == 0) return;
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

// -------------------------------------------------------------
// RingMesh
// -------------------------------------------------------------

RingMesh::RingMesh() = default;

RingMesh::~RingMesh() {
  if (m_ebo) glDeleteBuffers(1, &m_ebo);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void RingMesh::setRing(float inner, float outer, int segments) {
  if (segments < 3) segments = 3;
  if (inner < 0.0f) inner = 0.0f;
  if (outer < inner + 0.05f) outer = inner + 0.05f;

  const float kPi = 3.14159265358979323846f;

  // Interleaved pos (vec3) + a_R (float). y = 0 (XZ plane).
  std::vector<float> verts;
  verts.reserve(static_cast<size_t>(segments) * 2u * 4u);
  for (int i = 0; i < segments; ++i) {
    float a = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
    float c = std::cos(a);
    float s = std::sin(a);
    // inner edge, a_R = 0
    verts.push_back(c * inner);
    verts.push_back(0.0f);
    verts.push_back(s * inner);
    verts.push_back(0.0f);
    // outer edge, a_R = 1
    verts.push_back(c * outer);
    verts.push_back(0.0f);
    verts.push_back(s * outer);
    verts.push_back(1.0f);
  }

  std::vector<unsigned int> idx;
  idx.reserve(static_cast<size_t>(segments) * 6u);
  for (int i = 0; i < segments; ++i) {
    unsigned int i0 = static_cast<unsigned int>(i) * 2u;
    unsigned int i1 = i0 + 1u;
    unsigned int i2 = static_cast<unsigned int>((i + 1) % segments) * 2u;
    unsigned int i3 = i2 + 1u;
    // Winding for +Y (up) facing.
    idx.push_back(i0);
    idx.push_back(i2);
    idx.push_back(i1);
    idx.push_back(i1);
    idx.push_back(i2);
    idx.push_back(i3);
  }

  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
  }

  m_indexCount = static_cast<unsigned int>(idx.size());

  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(),
               GL_STATIC_DRAW);

  // Pos: layout 0 (vec3), a_R: layout 1 (float), stride 4 floats.
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        reinterpret_cast<void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void RingMesh::draw() const {
  if (!m_vao || m_indexCount == 0) return;
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

// -------------------------------------------------------------
// BeltPoints
// -------------------------------------------------------------

BeltPoints::BeltPoints() = default;

BeltPoints::~BeltPoints() {
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void BeltPoints::setBelt(int count, float rInner, float rOuter) {
  if (count < 0) count = 0;
  if (rInner < 0.0f) rInner = 0.0f;
  if (rOuter < rInner + 0.05f) rOuter = rInner + 0.05f;

  const float kPi = 3.14159265358979323846f;

  m_vertexCount = static_cast<unsigned int>(count);
  if (count == 0) return;

  // Deterministic LCG (fixed seed, stable run to run).
  uint32_t state = 0xC0FFEEu;
  auto next01 = [&state]() -> float {
    state = state * 1664525u + 1013904223u;
    // Upper 24 bits -> [0, 1).
    return static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
  };

  std::vector<float> verts;
  verts.reserve(static_cast<size_t>(count) * 3u);
  float rIn2 = rInner * rInner;
  float rOut2 = rOuter * rOuter;
  for (int i = 0; i < count; ++i) {
    float u = next01();
    float v = next01();
    float w = next01();
    // Uniform-area radius in annulus.
    float r = std::sqrt(rIn2 + (rOut2 - rIn2) * u);
    float a = 2.0f * kPi * v;
    float y = (w * 2.0f - 1.0f) * 0.25f;
    verts.push_back(std::cos(a) * r);
    verts.push_back(y);
    verts.push_back(std::sin(a) * r);
  }

  if (!m_vao) {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
  }

  glBindVertexArray(m_vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
}

void BeltPoints::draw() const {
  if (!m_vao || m_vertexCount == 0) return;
  glBindVertexArray(m_vao);
  glPointSize(2.0f);
  glDrawArrays(GL_POINTS, 0, m_vertexCount);
  glBindVertexArray(0);
}

}  // namespace astro
