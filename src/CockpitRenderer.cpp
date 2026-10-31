#include "CockpitRenderer.h"
#include "GLCompat.h"
#include "Shader.h"
#include "Mesh.h"
#include <iostream>

namespace astro {

CockpitRenderer::CockpitRenderer() = default;
CockpitRenderer::~CockpitRenderer() = default;

bool CockpitRenderer::init() {
  if (m_ready) return true;
  if (m_initAttempted) return false;
  m_initAttempted = true;

  try {
    m_shader = std::make_unique<Shader>("shaders/cockpit.vert", "shaders/cockpit.frag");
    m_quad = std::make_unique<ScreenQuad>();
    m_ready = true;
  } catch (const std::exception& e) {
    std::cerr << "[CockpitRenderer] Shader initialization failed: " << e.what() << "\n";
    m_ready = false;
  }
  return m_ready;
}

void CockpitRenderer::render(int fbWidth,
                            int fbHeight,
                            float time,
                            const CockpitHUDData& hud,
                            const glm::vec3& headOffset,
                            const glm::vec3& headTilt,
                            float rumble,
                            int shipStyle) {
  if (!m_ready && !init()) return;
  if (fbWidth <= 0 || fbHeight <= 0) return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  m_shader->bind();
  m_shader->setUniform2f("u_Resolution", static_cast<float>(fbWidth), static_cast<float>(fbHeight));
  m_shader->setUniform1f("u_Time", time);
  m_shader->setUniform3f("u_HeadOffset", headOffset.x, headOffset.y, headOffset.z);
  m_shader->setUniform3f("u_HeadTilt", headTilt.x, headTilt.y, headTilt.z);
  m_shader->setUniform1f("u_Rumble", rumble);
  m_shader->setUniform1f("u_Pitch", hud.pitchDeg);
  m_shader->setUniform1f("u_Roll", hud.rollDeg);
  m_shader->setUniform1f("u_Yaw", hud.yawDeg);
  m_shader->setUniform1f("u_Speed", hud.speed);
  m_shader->setUniform1f("u_Throttle", hud.throttle);
  m_shader->setUniform1f("u_WarpActive", hud.warpActive ? 1.0f : 0.0f);
  m_shader->setUniform1f("u_WarpCharge", hud.warpCharge);
  m_shader->setUniform1f("u_Hull", hud.hullIntegrity);
  m_shader->setUniform1f("u_Shield", hud.shieldIntegrity);
  m_shader->setUniform1f("u_AutoTint", hud.autoTint);
  m_shader->setUniform1f("u_TargetAngle", hud.targetAngleDeg);
  m_shader->setUniform1f("u_TargetLocked", hud.targetLocked ? 1.0f : 0.0f);
  m_shader->setUniform1i("u_ShipStyle", shipStyle);

  m_quad->draw();

  m_shader->unbind();
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
}

}  // namespace astro
