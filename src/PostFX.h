#pragma once

#include <memory>

#include "Mesh.h"
#include "Shader.h"

namespace astro {

// === PostFX === (этап 2а: scene FBO + identity-компоузер, PNG байт-в-байт;
// этап 2б: яркостный пасс + blur-цепочка + аддитивное свечение).
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
  bool m_fboOk = false;
  bool m_active = false;
  bool m_compOk = false;
  bool m_bloomOk = false;
  bool m_shadersTried = false;

  std::unique_ptr<Shader> m_compShader;
  std::unique_ptr<Shader> m_downShader;
  std::unique_ptr<Shader> m_blurShader;
  std::unique_ptr<ScreenQuad> m_quad;
};

}  // namespace astro
