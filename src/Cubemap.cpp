#include "Cubemap.h"

#include <GL/glew.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace astro {

namespace {

// Simple hash for pseudo-random star generation
float hash3(const glm::vec3& p) {
  float h = glm::dot(p, glm::vec3(127.1f, 311.7f, 74.7f));
  return std::abs(std::sin(h) * 43758.5453123f - std::floor(std::sin(h) * 43758.5453123f));
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

void Cubemap::generateProceduralSkybox() {
  const int faceSize = 512;
  glGenTextures(1, &m_textureId);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureId);

  std::vector<float> facePixels(faceSize * faceSize * 3);

  for (int face = 0; face < 6; ++face) {
    for (int y = 0; y < faceSize; ++y) {
      for (int x = 0; x < faceSize; ++x) {
        float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize) * 2.0f - 1.0f;
        float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize) * 2.0f - 1.0f;
        glm::vec3 dir = getDirectionForFace(face, u, v);

        // Background space dust / nebula
        float mw = std::exp(-std::abs(dir.y) * 4.0f) * 0.35f;
        float r = 0.02f + mw * 0.7f;
        float g = 0.025f + mw * 0.5f;
        float b = 0.04f + mw * 0.9f;

        // Discrete star field
        glm::vec3 quantized = glm::floor(dir * 240.0f);
        float rnd = hash3(quantized);
        if (rnd > 0.985f) {
          float intensity = std::pow((rnd - 0.985f) / 0.015f, 3.0f) * 2.5f;
          float starColorVar = hash3(quantized + glm::vec3(1.0f, 2.0f, 3.0f));
          if (starColorVar > 0.6f) {
            r += intensity * 0.8f;
            g += intensity * 0.9f;
            b += intensity * 1.2f; // blue star
          } else if (starColorVar > 0.3f) {
            r += intensity * 1.2f;
            g += intensity * 1.1f;
            b += intensity * 0.8f; // yellow star
          } else {
            r += intensity * 1.3f;
            g += intensity * 0.7f;
            b += intensity * 0.6f; // reddish star
          }
        }

        // Galactic core glow towards (1, 0, 0)
        float coreAngle = glm::dot(dir, glm::vec3(1.0f, 0.0f, 0.0f));
        if (coreAngle > 0.0f) {
          float coreGlow = std::pow(coreAngle, 8.0f) * 0.6f;
          r += coreGlow * 1.1f;
          g += coreGlow * 0.8f;
          b += coreGlow * 0.6f;
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
