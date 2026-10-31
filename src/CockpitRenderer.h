#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "CockpitController.h"

namespace astro {

class Shader;
class ScreenQuad;

class CockpitRenderer {
 public:
  CockpitRenderer();
  ~CockpitRenderer();

  CockpitRenderer(const CockpitRenderer&) = delete;
  CockpitRenderer& operator=(const CockpitRenderer&) = delete;

  // Initialize OpenGL shader and geometry resources
  bool init();

  // Draw the 3D cockpit interior, canopy glass, and collimated HUD
  void render(int fbWidth,
              int fbHeight,
              float time,
              const CockpitHUDData& hud,
              const glm::vec3& headOffset,
              const glm::vec3& headTilt,
              float rumble,
              int shipStyle);

  bool isReady() const { return m_ready; }

 private:
  std::unique_ptr<Shader> m_shader;
  std::unique_ptr<ScreenQuad> m_quad;
  bool m_ready = false;
  bool m_initAttempted = false;
};

}  // namespace astro
