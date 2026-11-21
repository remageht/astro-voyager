#pragma once

// Runtime OBJ loader for the Model hull style (style 3): Wavefront OBJ files
// from tools/fbx_ship_export.mjs (v/vt/vn indices) plus the PBR texture set
// that the same tool bakes to 24-bit BMP (albedo, normal, rough, metal).
// Layout matches ship.vert: location 0 = position, 1 = normal, 2 = uv
// (interleaved, stride 32 B).
//
// Heavy-binary-free policy: the .obj and the .bmp set live in res/models/ next
// to the exe, are gitignored, and regenerate from the Tripo FBX; loader
// failure falls back to the procedural Ring Ship silently, and a missing
// texture set falls back to the flat PBR-lite hull.

#include <string>
#include <glm/glm.hpp>

namespace astro {

// Общий загрузчик 24-битных BMP в GL-текстуру (mipmaps + REPEAT): PBR-набор
// стиля Model и тайловый скин Ring Ship (tools/make_ring_skin.mjs) читаются
// одним и тем же путём, без stb_image и zlib. 0 = файла нет / битый.
unsigned int loadBmpTexture(const std::string& path);

class ObjModel {
 public:
  ObjModel();
  ~ObjModel();

  ObjModel(const ObjModel&) = delete;
  ObjModel& operator=(const ObjModel&) = delete;

  //false при отсутствии/битом файле или неинициализированном GL-контексте.
  bool load(const std::string& path);

  // PBR-набор рядом с .obj: <base>_albedo.bmp / _normal.bmp / _rough.bmp /
  // _metal.bmp / (опционально) _emissive.bmp. Возвращает false, если не
  // нашлось ни одной карты (меш при этом остаётся годным — плоский металл).
  bool loadTextures(const std::string& base);

  // Share PBR texture handles from another model (same UV atlas files).
  // No duplicate GL uploads; this model does not own the shared textures.
  void shareTexturesFrom(const ObjModel& other);

  bool ready() const { return m_indexCount > 0; }
  bool textured() const { return m_texAlbedo != 0; }
  bool emissive() const { return m_texEmissive != 0; }

  // Единичный масштаб: baked экспорт сцентрирован в 0.0 и растянут в LENGTH.
  void draw() const;

  // Привязка карт к юнитам firstUnit..firstUnit+4 (albedo, normal, rough,
  // metal, emissive).
  void bindTextures(int firstUnit) const;

  const glm::vec3& center() const { return m_center; }
  const glm::vec3& bboxMin() const { return m_bboxMin; }
  const glm::vec3& bboxMax() const { return m_bboxMax; }
  glm::vec3 dimensions() const { return m_bboxMax - m_bboxMin; }

  unsigned int vertexCount() const { return m_vertexCount; }
  unsigned int indexCount() const { return m_indexCount; }
  unsigned int triangleCount() const { return m_indexCount / 3; }
  unsigned int albedoTex() const { return m_texAlbedo; }
  unsigned int normalTex() const { return m_texNormal; }
  unsigned int roughTex() const { return m_texRough; }
  unsigned int metalTex() const { return m_texMetal; }
  unsigned int emissiveTex() const { return m_texEmissive; }

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_ebo = 0;
  unsigned int m_indexCount = 0;
  unsigned int m_vertexCount = 0;
  unsigned int m_texAlbedo = 0;
  unsigned int m_texNormal = 0;
  unsigned int m_texRough = 0;
  unsigned int m_texMetal = 0;
  unsigned int m_texEmissive = 0;
  bool m_sharedTex = false;
  glm::vec3 m_center{0.0f};
  glm::vec3 m_bboxMin{0.0f};
  glm::vec3 m_bboxMax{0.0f};
};

}  // namespace astro
