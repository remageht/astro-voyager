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

class StarPoints {
 public:
  StarPoints();
  ~StarPoints();

  StarPoints(const StarPoints&) = delete;
  StarPoints& operator=(const StarPoints&) = delete;

  void setStars(const std::vector<glm::vec3>& positions, const std::vector<float>& mags);
  void clear() { m_vertexCount = 0; }
  void draw() const;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_vertexCount = 0;
};

// Unit sphere, indexed. Interleaved pos (vec3) + normal (vec3).
// Radius is 1.0; caller scales via model matrix.
class SphereMesh {
 public:
  SphereMesh();
  ~SphereMesh();

  SphereMesh(const SphereMesh&) = delete;
  SphereMesh& operator=(const SphereMesh&) = delete;

  void setSphere(int rings = 48, int sectors = 96);
  void draw() const;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
  unsigned int m_indexCount = 0;
};

// Flat annulus in XZ plane (y = 0), indexed.
// Attributes: pos (vec3, layout 0) + a_R (float, layout 1).
// a_R is 0 at inner edge, 1 at outer edge.
class RingMesh {
 public:
  RingMesh();
  ~RingMesh();

  RingMesh(const RingMesh&) = delete;
  RingMesh& operator=(const RingMesh&) = delete;

  void setRing(float inner = 0.0f, float outer = 1.0f, int segments = 128);
  void draw() const;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
  unsigned int m_indexCount = 0;
};

// Asteroid-belt style point cloud in XZ annulus, GL_POINTS.
// Single VBO of vec3 positions, deterministic PRNG (stable run to run).
class BeltPoints {
 public:
  BeltPoints();
  ~BeltPoints();

  BeltPoints(const BeltPoints&) = delete;
  BeltPoints& operator=(const BeltPoints&) = delete;

  void setBelt(int count = 2200, float rInner = 19.5f, float rOuter = 22.5f);
  void draw() const;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_vertexCount = 0;
};

}  // namespace astro
