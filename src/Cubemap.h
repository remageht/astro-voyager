#pragma once

#include <string>

namespace astro {

class Cubemap {
 public:
  Cubemap();
  explicit Cubemap(const std::string& hdrPath);
  ~Cubemap();

  Cubemap(const Cubemap&) = delete;
  Cubemap& operator=(const Cubemap&) = delete;

  void bind(unsigned int slot = 0) const;
  void unbind() const;

  unsigned int getId() const { return m_textureId; }

  void regenerate(float starThreshold, float mwStrength, unsigned seed);

  // Mip-цепочка кубемапа: нужна для IBL корабля (textureLod по rough).
  // По умолчанию выключена, чтобы лодированный сэмплинг существующих
  // шейдеров (sky.frag/bh.frag) не менял эталонные кадры.
  void setMipmaps(bool on) { m_mipmaps = on; }

 private:
  void generateProceduralSkybox(float starThreshold = 0.9985f, float mwStrength = 0.40f,
                                unsigned seed = 0u);
  bool loadHdrSkybox(const std::string& path);

  unsigned int m_textureId = 0;
  bool m_mipmaps = false;
};

}  // namespace astro
