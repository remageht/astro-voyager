#include "PostFX.h"

#include "GLCompat.h"

#include <algorithm>
#include <iostream>

namespace astro {

namespace {

// Сила свечения (post-stack pass: теперь bloom — mip-пирамида Jimenez,
// уровень 0 содержит все уровни через аддитивный upsample; порог/колено
// живут в post_down13.frag: soft-knee 0.72/0.36 по линейному HDR).
constexpr float kBloomStrength = 0.18f;
// Анаморфный стрик (смар уровня 2), доля от bloom-вклада, до умножения на
// kBloomStrength; холодный тинт задан в post_comp.frag.
constexpr float kStreakK = 0.35f;

int levelSize(int base, int level) {
  return std::max(base >> (level + 1), 1);
}

void makeColorTarget(unsigned int& fbo, unsigned int& tex, int w, int h,
                     unsigned int internalFormat, unsigned int type) {
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, type,
               nullptr);
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
  }
  delFbo(m_streakFbo);
  delTex(m_streakTex);
  delFbo(m_compFbo);
  delTex(m_compTex);
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
    makeColorTarget(m_downFbo[i], m_downTex[i], lw, lh, GL_RGBA16F,
                    GL_HALF_FLOAT);
  }
  // Стрик — на масштабе уровня 2 пирамиды bloom.
  makeColorTarget(m_streakFbo, m_streakTex, levelSize(width, 2),
                  levelSize(height, 2), GL_RGBA16F, GL_HALF_FLOAT);
  // LDR-таргет композита: уже тонамапленный гамма-кадр на входе FXAA.
  makeColorTarget(m_compFbo, m_compTex, width, height, GL_RGBA8,
                  GL_UNSIGNED_BYTE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  m_w = width;
  m_h = height;

  if (!m_shadersTried) {
    m_shadersTried = true;
    try {
      m_quad = std::make_unique<ScreenQuad>();
    } catch (...) {
      m_quad = nullptr;
      std::cerr << "[PostFX] screen quad failed, post-processing disabled\n";
    }
    try {
      m_compShader = std::make_unique<Shader>("shaders/quad.vert",
                                              "shaders/post_comp.frag");
      m_compOk = (m_quad != nullptr);
    } catch (...) {
      m_compOk = false;
      std::cerr << "[PostFX] composite shader failed, using blit fallback\n";
    }
    try {
      m_downShader = std::make_unique<Shader>("shaders/quad.vert",
                                              "shaders/post_down13.frag");
      m_upShader = std::make_unique<Shader>("shaders/quad.vert",
                                            "shaders/post_up9.frag");
      m_streakShader = std::make_unique<Shader>("shaders/quad.vert",
                                                "shaders/post_streak.frag");
      m_bloomOk = (m_quad != nullptr);
    } catch (...) {
      m_bloomOk = false;
      std::cerr << "[PostFX] bloom chain shader failed, bloom disabled\n";
    }
    try {
      m_fxaaShader = std::make_unique<Shader>("shaders/quad.vert",
                                              "shaders/post_fxaa.frag");
      m_fxaaOk = (m_quad != nullptr);
    } catch (...) {
      m_fxaaOk = false;
      std::cerr << "[PostFX] FXAA shader failed, compositing to window\n";
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
  // Downsample: scene -> down0 -> ... -> down4 (13-tap, порог и Karis
  // только на первом уровне — дальше уже извлечённые блики).
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  m_downShader->bind();
  m_downShader->setUniform1i("u_Source", 0);
  m_downShader->setUniform1f("u_Threshold", 0.72f);
  m_downShader->setUniform1f("u_Knee", 0.36f);
  glActiveTexture(GL_TEXTURE0);
  for (int i = 0; i < kLevels; ++i) {
    const int lw = levelSize(m_w, i);
    const int lh = levelSize(m_h, i);
    const int sw = (i == 0) ? m_w : levelSize(m_w, i - 1);
    const int sh = (i == 0) ? m_h : levelSize(m_h, i - 1);
    glBindFramebuffer(GL_FRAMEBUFFER, m_downFbo[i]);
    glViewport(0, 0, lw, lh);
    m_downShader->setUniform2f("u_Texel", 1.0f / static_cast<float>(sw),
                               1.0f / static_cast<float>(sh));
    m_downShader->setUniform1i("u_First", i == 0 ? 1 : 0);
    glBindTexture(GL_TEXTURE_2D, i == 0 ? m_sceneTex : m_downTex[i - 1]);
    m_quad->draw();
  }
  // Аддитивный upsample: down4 -> down3 -> ... -> down0 (9-tap tent).
  // Каждый уровень аккумулирует все младшие, уровень 0 — полный bloom.
  m_upShader->bind();
  m_upShader->setUniform1i("u_Source", 0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  for (int i = kLevels - 1; i >= 1; --i) {
    const int dw = levelSize(m_w, i - 1);
    const int dh = levelSize(m_h, i - 1);
    glBindFramebuffer(GL_FRAMEBUFFER, m_downFbo[i - 1]);
    glViewport(0, 0, dw, dh);
    m_upShader->setUniform2f("u_Texel", 1.0f / static_cast<float>(dw),
                             1.0f / static_cast<float>(dh));
    glBindTexture(GL_TEXTURE_2D, m_downTex[i]);
    m_quad->draw();
  }
  glDisable(GL_BLEND);
  // Анаморфный стрик: широкий горизонтальный смар аккумулированного
  // уровня 2 пирамиды.
  m_streakShader->bind();
  m_streakShader->setUniform1i("u_Source", 0);
  const int sw = levelSize(m_w, 2);
  const int sh = levelSize(m_h, 2);
  glBindFramebuffer(GL_FRAMEBUFFER, m_streakFbo);
  glViewport(0, 0, sw, sh);
  m_streakShader->setUniform2f("u_Texel", 1.0f / static_cast<float>(sw),
                               1.0f / static_cast<float>(sh));
  glBindTexture(GL_TEXTURE_2D, m_downTex[2]);
  m_quad->draw();
  m_upShader->unbind();
  m_streakShader->unbind();
  glActiveTexture(GL_TEXTURE0);
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

  // Композит: в LDR-таргет (дальше FXAA) или напрямую в окно без FXAA.
  const bool useFxaa = m_fxaaOk && m_fxaaOn;
  if (useFxaa) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_compFbo);
    glViewport(0, 0, m_w, m_h);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  m_compShader->bind();
  m_compShader->setUniform1i("u_Scene", 0);
  m_compShader->setUniform1i("u_Bloom", 1);
  m_compShader->setUniform1i("u_Streak", 2);
  // === Bloom === post-stack pass: mip-пирамида + анаморфный стрик.
  // Порог soft-knee 0.72/0.36 по линейному HDR — в post_down13.frag.
  m_compShader->setUniform1f("u_BloomStrength", kBloomStrength);
  m_compShader->setUniform1f("u_StreakK", kStreakK);
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
  // 0 => обе ветки в post_comp.frag пропускаются.
  m_compShader->setUniform1f("u_ChromaK", m_shipChroma);
  m_compShader->setUniform1f("u_StaticK", m_shipStaticK);
  m_compShader->setUniform1f("u_ShieldK", m_shipFlashK);
  // === Speed FX === (v3.3.0): спид-стрики вдоль вектора движения;
  // 0 => ветка в post_comp.frag пропускается.
  m_compShader->setUniform1f("u_SpeedK", m_speedK);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_sceneTex);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_downTex[0]);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_streakTex);
  m_quad->draw();
  m_compShader->unbind();

  if (useFxaa) {
    // FXAA 3.11: LDR-композит -> окно, поверх потом рисуется только HUD.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    m_fxaaShader->bind();
    m_fxaaShader->setUniform1i("u_Color", 0);
    m_fxaaShader->setUniform2f("u_TexelSize", 1.0f / static_cast<float>(m_w),
                               1.0f / static_cast<float>(m_h));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_compTex);
    m_quad->draw();
    m_fxaaShader->unbind();
  }
  glActiveTexture(GL_TEXTURE0);
}

}  // namespace astro
