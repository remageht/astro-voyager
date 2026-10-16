#pragma once

#include <memory>

#include "Mesh.h"
#include "Shader.h"

namespace astro {

// === PostFX === (этап 2а: scene FBO + identity-компоузер, PNG байт-в-байт;
// этап 2б: яркостный пасс + blur-цепочка + аддитивное свечение;
// v3.0.0: scene FBO и цепочка — float16 HDR-linear, тонмаппинг ACES + гамма
// и зерно живут только в композите, станционные шейдеры отдают линейный HDR).
// Рендер-петля и харнесс скриншотов идут через SceneManager::render(),
// поэтому FBO и композит живут внутри него: beginScene() в начале render(),
// endScene() в конце. При u_BloomStrength == 0.0 композит обязан отдавать
// пиксель-в-пиксель исходный кадр (texelFetch, без фильтрации).
class PostFX {
 public:
  PostFX() = default;
  ~PostFX();

  PostFX(const PostFX&) = delete;
  PostFX& operator=(const PostFX&) = delete;

  void beginScene(int width, int height);
  void endScene(int width, int height);

  // Время симуляции для зерна в композите (u_Time; харнесс фиксирует
  // его, поэтому PNG детерминированы). Вызывается каждый кадр из render().
  void setTime(float t) { m_time = t; }

  // === Relativity === параметры наблюдателя для доплера-тинта в композите.
  // Вызывается каждый кадр из SceneManager::render; beta == 0 => тинт выкл.
  void setFlight(const float camFwd[3], const float camRight[3],
                 const float camUp[3], float fovY, float aspect,
                 const float velDir[3], float beta);

  // === Ship survival === (v3.3.0): low-shield chroma/static amounts (0 = off).
  void setShipFx(float chromaK, float staticK);

 private:
  void ensureTargets(int width, int height);
  void release();
  void runBloomChain();

  unsigned int m_sceneFbo = 0;
  unsigned int m_sceneTex = 0;
  unsigned int m_sceneDepth = 0;

  static constexpr int kLevels = 5;
  unsigned int m_downFbo[kLevels] = {};
  unsigned int m_downTex[kLevels] = {};
  unsigned int m_blurFbo[kLevels] = {};
  unsigned int m_blurTex[kLevels] = {};

  int m_w = 0;
  int m_h = 0;
  float m_time = 0.0f;
  bool m_fboOk = false;
  bool m_active = false;
  bool m_compOk = false;
  bool m_bloomOk = false;
  bool m_shadersTried = false;

  // Relativity flight state (see setFlight).
  float m_camFwd[3] = {0.0f, 0.0f, -1.0f};  float m_camRight[3] = {1.0f, 0.0f, 0.0f};
  float m_camUp[3] = {0.0f, 1.0f, 0.0f};
  float m_fovY = 1.0f;
  float m_aspect = 1.777f;
  float m_velDir[3] = {0.0f, 0.0f, 1.0f};
  float m_beta = 0.0f;

  // Ship survival low-shield FX state (see setShipFx); 0 = off.
  float m_shipChroma = 0.0f;
  float m_shipStaticK = 0.0f;

  std::unique_ptr<Shader> m_compShader;
  std::unique_ptr<Shader> m_downShader;
  std::unique_ptr<Shader> m_blurShader;
  std::unique_ptr<ScreenQuad> m_quad;
};

}  // namespace astro
