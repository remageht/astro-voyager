#include "PostFX.h"

#include <GL/glew.h>

#include <algorithm>
#include <iostream>

namespace astro {

namespace {

// Сила свечения (этап 2б). Порог яркости — u_Threshold = 0.72 в post_bright
// (v3.0.0: порог по линейному HDR, сцена и цепочка — float16).
constexpr float kBloomStrength = 0.30f;

int levelSize(int base, int level) {
  return std::max(base >> (level + 1), 1);
}

void makeColorTarget(unsigned int& fbo, unsigned int& tex, int w, int h) {
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA,
               GL_HALF_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tex, 0);
}

}  // namespace

PostFX::~PostFX() { release(); }

void PostFX::release() {
  auto delFbo = [](unsigned int& id) {
    if (id) {
      glDeleteFramebuffers(1, &id);
      id = 0;
    }
  };
  auto delTex = [](unsigned int& id) {
    if (id) {
      glDeleteTextures(1, &id);
      id = 0;
    }
  };
  for (int i = 0; i < kLevels; ++i) {
    delFbo(m_downFbo[i]);
    delTex(m_downTex[i]);
    delFbo(m_blurFbo[i]);
    delTex(m_blurTex[i]);
  }
  delFbo(m_sceneFbo);
  delTex(m_sceneTex);
  if (m_sceneDepth) {
    glDeleteRenderbuffers(1, &m_sceneDepth);
    m_sceneDepth = 0;
  }
  m_w = 0;
  m_h = 0;
}

void PostFX::ensureTargets(int width, int height) {
  if (m_sceneFbo && m_w == width && m_h == height) return;
  release();

  glGenFramebuffers(1, &m_sceneFbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
  glGenTextures(1, &m_sceneTex);
  glBindTexture(GL_TEXTURE_2D, m_sceneTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
               GL_HALF_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_sceneTex, 0);
  glGenRenderbuffers(1, &m_sceneDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, m_sceneDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, m_sceneDepth);
  m_fboOk =
      (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

  for (int i = 0; i < kLevels; ++i) {
    const int lw = levelSize(width, i);
    const int lh = levelSize(height, i);
    makeColorTarget(m_downFbo[i], m_downTex[i], lw, lh);
    makeColorTarget(m_blurFbo[i], m_blurTex[i], lw, lh);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  m_w = width;
  m_h = height;

  if (!m_shadersTried) {
    m_shadersTried = true;
    try {
      m_compShader = std::make_unique<Shader>("shaders/quad.vert",
                                               "shaders/post_comp.frag");
      m_compOk = true;
    } catch (...) {
      m_compOk = false;
      std::cerr << "[PostFX] composite shader failed, using blit fallback\n";
    }
    try {
      m_downShader = std::make_unique<Shader>("shaders/quad.vert",
                                               "shaders/post_bright.frag");
      m_blurShader = std::make_unique<Shader>("shaders/quad.vert",
                                               "shaders/post_blur.frag");
      m_quad = std::make_unique<ScreenQuad>();
      m_bloomOk = true;
    } catch (...) {
      m_bloomOk = false;
      std::cerr << "[PostFX] bloom chain shader failed, bloom disabled\n";
    }
  }

  if (!m_fboOk) {
    std::cerr << "[PostFX] scene FBO incomplete, post-processing disabled\n";
  }
}

void PostFX::beginScene(int width, int height) {
  m_active = false;
  if (width <= 0 || height <= 0) return;
  ensureTargets(width, height);
  if (!m_fboOk) return;
  // Дизеринг (GL_DITHER, по умолчанию включён) драйвер применяет при
  // записи в текстуру FBO, но не в окно: без этого гейта PNG разъезжаются
  // на ±1..4 LSB. Окно дизерингу не поддаётся (проверено), выключение
  // глобально безопасно.
  glDisable(GL_DITHER);
  glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFbo);
  glViewport(0, 0, width, height);
  // v3.0.0: float16-таргет поднимает неинициализированную VRAM в видимый
  // лифт после гаммы композита (станции ori/uma не перекрывают весь экран),
  // поэтому кадр всегда начинается с детерминированного чёрного. Заодно
  // чистим depth: раньше он тоже был неопределён между кадрами.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  m_active = true;
}

void PostFX::runBloomChain() {
  // Bright pass + progressive downsample: scene -> down0 -> ... -> down4.
  m_downShader->bind();
  m_downShader->setUniform1i("u_Source", 0);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  for (int i = 0; i < kLevels; ++i) {
    const int lw = levelSize(m_w, i);
    const int lh = levelSize(m_h, i);
    glBindFramebuffer(GL_FRAMEBUFFER, m_downFbo[i]);
    glViewport(0, 0, lw, lh);
    m_downShader->setUniform1f("u_Threshold", i == 0 ? 0.72f : 0.0f);
    glBindTexture(GL_TEXTURE_2D, i == 0 ? m_sceneTex : m_downTex[i - 1]);
    m_quad->draw();
  }
  // Separable blur per level: down[i] -> blur[i] (H) -> down[i] (V).
  m_blurShader->bind();
  m_blurShader->setUniform1i("u_Source", 0);
  for (int i = 0; i < kLevels; ++i) {
    const int lw = levelSize(m_w, i);
    const int lh = levelSize(m_h, i);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[i]);
    glViewport(0, 0, lw, lh);
    m_blurShader->setUniform2f("u_Dir", 1.0f / static_cast<float>(lw), 0.0f);
    glBindTexture(GL_TEXTURE_2D, m_downTex[i]);
    m_quad->draw();
    glBindFramebuffer(GL_FRAMEBUFFER, m_downFbo[i]);
    m_blurShader->setUniform2f("u_Dir", 0.0f, 1.0f / static_cast<float>(lh));
    glBindTexture(GL_TEXTURE_2D, m_blurTex[i]);
    m_quad->draw();
  }
  m_blurShader->unbind();
}

void PostFX::setFlight(const float camFwd[3], const float camRight[3],
                       const float camUp[3], float fovY, float aspect,
                       const float velDir[3], float beta) {
  for (int i = 0; i < 3; ++i) {
    m_camFwd[i] = camFwd[i];
    m_camRight[i] = camRight[i];
    m_camUp[i] = camUp[i];
    m_velDir[i] = velDir[i];
  }
  m_fovY = fovY;
  m_aspect = aspect;
  m_beta = beta;
}

void PostFX::setShipFx(float chromaK, float staticK, float flashK) {
  m_shipChroma = chromaK;
  m_shipStaticK = staticK;
  m_shipFlashK = flashK;
}

void PostFX::setSpeedFx(float speedK) {
  m_speedK = speedK;
}

void PostFX::endScene(int width, int height) {
  if (!m_active) return;
  m_active = false;

  if (!m_compOk) {
    // Точный blit-фолбэк: тот же пиксель-в-пиксель результат.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_w, m_h, 0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    return;
  }

  if (m_bloomOk) {
    runBloomChain();
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, width, height);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  m_compShader->bind();
  m_compShader->setUniform1i("u_Scene", 0);
  m_compShader->setUniform1i("u_Bloom0", 1);
  m_compShader->setUniform1i("u_Bloom1", 2);
  m_compShader->setUniform1i("u_Bloom2", 3);
  m_compShader->setUniform1i("u_Bloom3", 4);
  m_compShader->setUniform1i("u_Bloom4", 5);
  // === Bloom === этап 2б: свечение включено. Порог 0.72 по линейному
  // HDR (диск ЧД, квазар, пульсар, звёзды, солнце); тонмаппинг — в композите.
  m_compShader->setUniform1f("u_BloomStrength", kBloomStrength);
  m_compShader->setUniform1f("u_Time", m_time);
  // === Relativity === доплер-тинт (post_comp.frag); u_Beta == 0 => выкл.
  m_compShader->setUniform3f("u_CamFwd", m_camFwd[0], m_camFwd[1], m_camFwd[2]);
  m_compShader->setUniform3f("u_CamRight", m_camRight[0], m_camRight[1],
                             m_camRight[2]);
  m_compShader->setUniform3f("u_CamUp", m_camUp[0], m_camUp[1], m_camUp[2]);
  m_compShader->setUniform1f("u_FovY", m_fovY);
  m_compShader->setUniform1f("u_Aspect", m_aspect);
  m_compShader->setUniform3f("u_VelDir", m_velDir[0], m_velDir[1], m_velDir[2]);
  m_compShader->setUniform1f("u_Beta", m_beta);
  // === Ship survival === (v3.3.0): низкий щит — хрома-аберрация и помехи;
  // 0 => обе ветки в post_comp.frag пропускаются, кадр байт-в-байт.
  m_compShader->setUniform1f("u_ChromaK", m_shipChroma);
  m_compShader->setUniform1f("u_StaticK", m_shipStaticK);
  m_compShader->setUniform1f("u_ShieldK", m_shipFlashK);
  // === Speed FX === (v3.3.0): спид-стрики вдоль вектора движения;
  // 0 => ветка в post_comp.frag пропускается, кадр байт-в-байт.
  m_compShader->setUniform1f("u_SpeedK", m_speedK);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_sceneTex);
  for (int i = 0; i < kLevels; ++i) {
    glActiveTexture(GL_TEXTURE1 + i);
    glBindTexture(GL_TEXTURE_2D, m_downTex[i]);
  }
  m_quad->draw();
  m_compShader->unbind();
  glActiveTexture(GL_TEXTURE0);
}

}  // namespace astro
