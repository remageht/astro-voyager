#include "Cubemap.h"

#include "GLCompat.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace astro {

namespace {

// Integer cell hash (fast, deterministic, no trig). Salt selects an
// independent stream for presence / jitter / brightness / color.
static unsigned hashCellI(int x, int y, int z, unsigned salt) {
  unsigned h = static_cast<unsigned>(x) * 374761393u +
               static_cast<unsigned>(y) * 668265263u +
               static_cast<unsigned>(z) * 1440662683u + salt * 2654435761u;
  h = (h ^ (h >> 13)) * 1274126177u;
  h ^= h >> 16;
  return h;
}
static float hashCell01(int x, int y, int z, unsigned salt) {
  return static_cast<float>(hashCellI(x, y, z, salt)) / 4294967295.0f;
}

glm::vec3 getDirectionForFace(int face, float u, float v) {
  switch (face) {
    case 0: return glm::normalize(glm::vec3( 1.0f, -v, -u)); // +X
    case 1: return glm::normalize(glm::vec3(-1.0f, -v,  u)); // -X
    case 2: return glm::normalize(glm::vec3( u,  1.0f,  v)); // +Y
    case 3: return glm::normalize(glm::vec3( u, -1.0f, -v)); // -Y
    case 4: return glm::normalize(glm::vec3( u, -v,  1.0f)); // +Z
    case 5: return glm::normalize(glm::vec3(-u, -v, -1.0f)); // -Z
    default: return glm::vec3(0.0f, 0.0f, 1.0f);
  }
}

}  // namespace

Cubemap::Cubemap() {
  const std::string defaultHdr = "res/textures/starmap_2020_4k_gal.hdr";
  if (!loadHdrSkybox(defaultHdr)) {
    generateProceduralSkybox();
  }
}

Cubemap::Cubemap(const std::string& hdrPath) {
  if (!loadHdrSkybox(hdrPath)) {
    generateProceduralSkybox();
  }
}

Cubemap::~Cubemap() {
  if (m_textureId != 0) {
    glDeleteTextures(1, &m_textureId);
  }
}

void Cubemap::bind(unsigned int slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);
}

void Cubemap::unbind() const {
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

bool Cubemap::loadHdrSkybox(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) {
    std::string fallback = "../" + path;
    f.open(fallback, std::ios::binary);
    if (!f.is_open()) {
      return false;
    }
  }
  f.close();

  int width = 0, height = 0, nrComponents = 0;
  float* data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
  if (!data) {
    return false;
  }

  // If equirectangular HDR is provided, we convert it to cubemap faces
  const int faceSize = 512;
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

  std::vector<float> facePixels(faceSize * faceSize * 3);
  const float PI = 3.14159265358979323846f;

  for (int face = 0; face < 6; ++face) {
    for (int y = 0; y < faceSize; ++y) {
      for (int x = 0; x < faceSize; ++x) {
        float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize) * 2.0f - 1.0f;
        float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize) * 2.0f - 1.0f;
        glm::vec3 dir = getDirectionForFace(face, u, v);

        // Spherical coordinates
        float theta = std::atan2(dir.z, dir.x);
        float phi = std::asin(glm::clamp(dir.y, -1.0f, 1.0f));

        float eqU = (theta + PI) / (2.0f * PI);
        float eqV = (phi + PI * 0.5f) / PI;

        int sx = glm::clamp(static_cast<int>(eqU * width), 0, width - 1);
        int sy = glm::clamp(static_cast<int>((1.0f - eqV) * height), 0, height - 1);
        int srcIdx = (sy * width + sx) * nrComponents;

        int dstIdx = (y * faceSize + x) * 3;
        facePixels[dstIdx + 0] = data[srcIdx + 0];
        facePixels[dstIdx + 1] = data[srcIdx + 1];
        facePixels[dstIdx + 2] = data[srcIdx + 2];
      }
    }

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F,
                 faceSize, faceSize, 0, GL_RGB, GL_FLOAT, facePixels.data());
  }

  stbi_image_free(data);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return true;
}

void Cubemap::generateProceduralSkybox(float starThreshold, float mwStrength, unsigned seed) {
  regenerate(starThreshold, mwStrength, seed);
}

void Cubemap::regenerate(float starThreshold, float mwStrength, unsigned seed) {
  if (m_textureId != 0) {
    glDeleteTextures(1, &m_textureId);
    m_textureId = 0;
  }
  const int faceSize = 512;
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

  std::vector<float> facePixels(faceSize * faceSize * 3);

  unsigned int seedOff = seed * 7u;

  for (int face = 0; face < 6; ++face) {
    for (int y = 0; y < faceSize; ++y) {
      for (int x = 0; x < faceSize; ++x) {
        float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize) * 2.0f - 1.0f;
        float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize) * 2.0f - 1.0f;
        glm::vec3 dir = getDirectionForFace(face, u, v);

        // Deep-space base plus a granular Milky-Way band. The band keeps the
        // exp(-|y|*5) profile that sky.frag/bh.frag bandW gain (u_BandK)
        // assumes; clump harmonics + an off-plane dust lane break the
        // perfectly smooth ramp of the old field into a real galactic band.
        float az = std::atan2(dir.z, dir.x);
        float bandW = std::exp(-std::abs(dir.y) * 5.0f);
        float clump = 0.62f + 0.24f * std::sin(2.0f * az + 1.3f) +
                      0.14f * std::sin(5.0f * az + 4.2f);
        float laneQ = (std::abs(dir.y) - 0.035f) / 0.018f;
        float laneK = 1.0f - 0.55f * std::exp(-laneQ * laneQ) *
                                (0.5f + 0.5f * std::sin(3.0f * az + 2.1f));
        float mw = bandW * mwStrength * clump * laneK;
        float r = 0.006f + mw * 0.26f;
        float g = 0.007f + mw * 0.24f;
        float b = 0.012f + mw * 0.19f;

        // Discrete star field, sparse two-tier population (v3.5.0). The old
        // field held ~23k stars with a fat bright tail (pow9*8): a confetti
        // sky that gravitational lensing at the BH stretched into a speckle
        // tunnel. Now ~5-7k faint stars plus a small bright tail (two-tier
        // power law), tighter Gaussian points, white-leaning color mix.
        // Stars sit at JITTERED sub-cell positions (not cell centers): a
        // coarse grid would lens into regularly dotted columns near the
        // photon ring. Neighbor search + Gaussian points give Poisson-like
        // randomness while staying fully deterministic run to run. Star
        // presence is boosted near the Milky-Way plane (cell-center y in
        // cube space), so the band reads as a dense faint sprinkle.
        const float kStarInv = 1.0f / 450.0f;
        const float kStarSig = 0.60f * kStarInv;
        const float kStarSig2 = kStarSig * kStarSig;
        int ccx = static_cast<int>(std::floor(dir.x * 450.0f));
        int ccy = static_cast<int>(std::floor(dir.y * 450.0f));
        int ccz = static_cast<int>(std::floor(dir.z * 450.0f));
        for (int ox = -1; ox <= 1; ++ox) {
          for (int oy = -1; oy <= 1; ++oy) {
            for (int oz = -1; oz <= 1; ++oz) {
              int nx = ccx + ox, ny = ccy + oy, nz = ccz + oz;
              // Presence: base (1 - starThreshold) of cells, boosted up to
              // ~6x inside the band (|cell y| < ~0.14).
              float cy = (static_cast<float>(ny) + 0.5f) * kStarInv;
              float p = (1.0f - starThreshold) *
                        (1.0f + 5.0f * std::exp(-(cy * cy) / 0.02f));
              if (p <= 0.0f) continue;
              if (hashCell01(nx, ny, nz, 0u + seedOff) < 1.0f - p) continue;
              float jx = hashCell01(nx, ny, nz, 1u + seedOff);
              float jy = hashCell01(nx, ny, nz, 2u + seedOff);
              float jz = hashCell01(nx, ny, nz, 3u + seedOff);
              float px = (static_cast<float>(nx) + 0.05f + 0.9f * jx) * kStarInv;
              float py = (static_cast<float>(ny) + 0.05f + 0.9f * jy) * kStarInv;
              float pz = (static_cast<float>(nz) + 0.05f + 0.9f * jz) * kStarInv;
              float dx = dir.x - px, dy = dir.y - py, dz = dir.z - pz;
              float d2 = dx * dx + dy * dy + dz * dz;
              if (d2 > 9.0f * kStarSig2) continue;
              float w = std::exp(-d2 / (2.0f * kStarSig2));
              // Two-tier brightness: ~97% faint stars, 3% bright tail.
              float tier = hashCell01(nx, ny, nz, 4u + seedOff);
              float hI = hashCell01(nx, ny, nz, 6u + seedOff);
              bool isBright = tier > 0.97f;
              float intensity = isBright
                                    ? (1.2f + 7.0f * std::pow(hI, 8.0f))
                                    : (0.04f + 0.34f * std::pow(hI, 3.0f));
              intensity *= w;
              if (isBright) {
                // Bright stars carry a soft halo (wider Gaussian) and are
                // dimmed inside the band so it stays a subtle glow, not a
                // row of hot dots.
                intensity += std::exp(-d2 / (6.0f * kStarSig2)) * 0.10f;
                intensity *= 1.0f - 0.65f * std::exp(-std::abs(py) * 5.0f);
              }
              float starColorVar = hashCell01(nx, ny, nz, 5u + seedOff);
              float sr = 1.0f;
              float sg = 1.0f;
              float sb = 1.0f;
              if (starColorVar > 0.84f) {
                sr = 0.82f;
                sg = 0.88f;
                sb = 1.0f;  // blue-white, 16%
              } else if (starColorVar > 0.58f) {
                sr = 1.0f;
                sg = 0.93f;
                sb = 0.82f;  // warm white, 26%
              } else if (starColorVar > 0.33f) {
                sr = 1.0f;
                sg = 0.97f;
                sb = 0.90f;  // white, 25%
              } else if (starColorVar > 0.13f) {
                sr = 1.0f;
                sg = 0.74f;
                sb = 0.52f;  // orange, 20%
              } else {
                sr = 1.0f;
                sg = 0.58f;
                sb = 0.42f;  // red, 13%
              }
              r += intensity * sr;
              g += intensity * sg;
              b += intensity * sb;
            }
          }
        }

        // Galactic core glow towards (1, 0, 0), softened.
        float coreAngle = glm::dot(dir, glm::vec3(1.0f, 0.0f, 0.0f));
        if (coreAngle > 0.0f) {
          float coreGlow = std::pow(coreAngle, 6.0f) * 0.30f;
          r += coreGlow * 1.15f;
          g += coreGlow * 0.85f;
          b += coreGlow * 0.55f;
        }

        int idx = (y * faceSize + x) * 3;
        facePixels[idx + 0] = r;
        facePixels[idx + 1] = g;
        facePixels[idx + 2] = b;
      }
    }

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F,
                 faceSize, faceSize, 0, GL_RGB, GL_FLOAT, facePixels.data());
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

}  // namespace astro
