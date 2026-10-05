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

 private:
  void generateProceduralSkybox();
  bool loadHdrSkybox(const std::string& path);

  unsigned int m_textureId = 0;
};

}  // namespace astro
