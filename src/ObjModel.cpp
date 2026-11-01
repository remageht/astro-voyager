// === ObjModel === (агент M: runtime-загрузчик меша для стиля Model).
// Парсит Wavefront OBJ с v/vt/vn (f a/b/c). Дедуп вершины по ключу
// (pos_idx, uv_idx, nrm_idx) в один interleaved VBO:
// loc0 = position, loc1 = normal, loc2 = uv, stride 32.
// Формат выбирался под ship.vert один-в-один: u_InvScale = (1,1,1),
// u_FlatNormal = 0, нормали приходят готовыми из экспорт-тула.
// Если vn в файле нет — фолбэк на per-face плоские нормали (без дедупа).
// Если vt нет — UV = (0,0), текстуры выключаются в шейдере.
//
// Плюс PBR-набор карт (albedo/normal/rough/metal), который экспорт-тул
// кладёт рядом с .obj как 24-битные BMP: читаем их напрямую, без stb_image
// и без zlib — проект остаётся свободным от тяжёлых зависимостей.
//
// Ошибки ввода/парса -> load == false, стиль откатывается на процедуру.

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "ObjModel.h"

#include "GLCompat.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace astro {

namespace {

inline bool isSpace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

// Читает первый токен (до пробела) с позиции, возвращает длину токена.
inline int tokenAt(const char* line, int pos, char* out, int cap) {
  while (line[pos] && isSpace(line[pos])) ++pos;
  int n = 0;
  while (line[pos] && !isSpace(line[pos]) && n < cap - 1) out[n++] = line[pos++];
  out[n] = '\0';
  return pos;
}

struct FaceIdx {
  int v = 0;
  int vt = -1;
  int vn = -1;
};

inline bool parseCorner(const char* tok, int tlen, FaceIdx& out, int vCount,
                        int vtCount, int vnCount) {
  int v = 0, vt = 0, vn = 0;
  int i = 0, sign = 1;
  if (i < tlen && tok[i] == '-') { sign = -1; ++i; }
  bool any = false;
  while (i < tlen && tok[i] >= '0' && tok[i] <= '9') {
    v = v * 10 + (tok[i] - '0');
    ++i; any = true;
  }
  if (!any) return false;
  if (i < tlen && tok[i] == '/') {
    ++i;
    if (i < tlen && tok[i] != '/') {
      bool have = false;
      while (i < tlen && tok[i] >= '0' && tok[i] <= '9') {
        vt = vt * 10 + (tok[i] - '0'); ++i; have = true;
      }
      if (!have) vt = 0;
    }
    if (i < tlen && tok[i] == '/') {
      ++i;
      while (i < tlen && tok[i] >= '0' && tok[i] <= '9') {
        vn = vn * 10 + (tok[i] - '0'); ++i;
      }
    }
  }
  if (sign > 0) {
    if (v <= 0 || v > vCount) return false;
    out.v = v - 1;
    if (vt > 0 && vt <= vtCount) out.vt = vt - 1;
    if (vn > 0 && vn <= vnCount) out.vn = vn - 1;
  } else {
    if (v <= 0 || v > vCount) return false;
    out.v = vCount - v;  // OBJ: отрицательный = relative к текущему размеру
    if (vt > 0 && vt <= vtCount) out.vt = vtCount - vt;
    if (vn > 0 && vn <= vnCount) out.vn = vnCount - vn;
  }
  return true;
}

inline int32_t readI32LE(const unsigned char* p) {
  return static_cast<int32_t>(static_cast<uint32_t>(p[0]) |
                              (static_cast<uint32_t>(p[1]) << 8) |
                              (static_cast<uint32_t>(p[2]) << 16) |
                              (static_cast<uint32_t>(p[3]) << 24));
}
inline uint32_t readU32LE(const unsigned char* p) {
  return static_cast<uint32_t>(readI32LE(p));
}

// 24/32-битный несжатый BMP (BI_RGB), как его пишет tools/tex_to_bmp.ps1.
bool loadBmp(const char* path, std::vector<unsigned char>& rgb, int& w, int& h) {
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  unsigned char hdr[54];
  if (std::fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { std::fclose(f); return false; }
  if (hdr[0] != 'B' || hdr[1] != 'M') { std::fclose(f); return false; }
  const uint32_t dataOff = readU32LE(hdr + 10);
  const int32_t bw = readI32LE(hdr + 18);
  const int32_t bhRaw = readI32LE(hdr + 22);
  const uint16_t bpp = static_cast<uint16_t>(hdr[28] | (hdr[29] << 8));
  const uint32_t comp = readU32LE(hdr + 30);
  if (comp != 0 || (bpp != 24 && bpp != 32) || bw <= 0 || bhRaw == 0) {
    std::fclose(f);
    return false;
  }
  const bool topDown = bhRaw < 0;
  const int bh = topDown ? -bhRaw : bhRaw;
  if (bw > 8192 || bh > 8192) {
    std::fclose(f);
    return false;
  }
  const size_t srcStride = ((static_cast<size_t>(bw) * (bpp / 8)) + 3u) & ~size_t(3);
  std::vector<unsigned char> row(srcStride);
  rgb.resize(static_cast<size_t>(bw) * static_cast<size_t>(bh) * 3);
  if (std::fseek(f, static_cast<long>(dataOff), SEEK_SET) != 0) { std::fclose(f); return false; }
  const size_t bytesPerPx = bpp / 8;
  for (int r = 0; r < bh; ++r) {
    if (std::fread(row.data(), 1, srcStride, f) != srcStride) { std::fclose(f); return false; }
    const int y = topDown ? r : (bh - 1 - r);
    unsigned char* dst = &rgb[static_cast<size_t>(y) * static_cast<size_t>(bw) * 3];
    for (int x = 0; x < bw; ++x) {
      const unsigned char* src = &row[static_cast<size_t>(x) * bytesPerPx];
      dst[x * 3 + 0] = src[2];  // BMP хранит BGR
      dst[x * 3 + 1] = src[1];
      dst[x * 3 + 2] = src[0];
    }
  }
  std::fclose(f);
  w = bw;
  h = bh;
  return true;
}

unsigned int makeTexture(const std::vector<unsigned char>& rgb, int w, int h) {
  unsigned int id = 0;
  glGenTextures(1, &id);
  if (id == 0) return 0;
  glBindTexture(GL_TEXTURE_2D, id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               rgb.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);
  return id;
}

unsigned int loadTextureFile(const std::string& path) {
  std::vector<unsigned char> rgb;
  int w = 0, h = 0;
  if (!loadBmp(path.c_str(), rgb, w, h)) return 0;
  return makeTexture(rgb, w, h);
}

}  // namespace

unsigned int loadBmpTexture(const std::string& path) {
  return loadTextureFile(path);
}

ObjModel::ObjModel() = default;

ObjModel::~ObjModel() {
  if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
  if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
  if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
  unsigned int tex[5] = {m_texAlbedo, m_texNormal, m_texRough, m_texMetal,
                         m_texEmissive};
  for (unsigned int t : tex) {
    if (t != 0) glDeleteTextures(1, &t);
  }
}

bool ObjModel::load(const std::string& path) {
  FILE* f = nullptr;
#ifdef _MSC_VER
  fopen_s(&f, path.c_str(), "rb");
#else
  f = std::fopen(path.c_str(), "rb");
#endif
  if (!f) return false;

  std::vector<float> rawPos;
  std::vector<float> rawUV;
  std::vector<float> rawNrm;
  std::vector<FaceIdx> corners;
  rawPos.reserve(16384 * 3);
  rawUV.reserve(16384 * 2);
  rawNrm.reserve(16384 * 3);
  corners.reserve(32768 * 3);

  char buf[4096];
  char tok[64];
  FaceIdx face[16];
  bool hasNormals = false;
  bool hasUV = false;
  while (std::fgets(buf, sizeof(buf), f)) {
    const int len = static_cast<int>(std::strlen(buf));
    // Быстрый фильтр: интересны строки 'v ', 'vt', 'vn', 'f '; остальное скип.
    if (buf[0] == 'v' && len > 2) {
      if (buf[1] == ' ' || buf[1] == '\t') {
        float x, y, z;
        if (std::sscanf(buf + 2, "%f %f %f", &x, &y, &z) == 3) {
          rawPos.push_back(x); rawPos.push_back(y); rawPos.push_back(z);
        }
        continue;
      }
      if (buf[1] == 't') {
        float u, v;
        if (std::sscanf(buf + 3, "%f %f", &u, &v) == 2) {
          rawUV.push_back(u); rawUV.push_back(v);
          hasUV = true;
        }
        continue;
      }
      if (buf[1] == 'n') {
        float x, y, z;
        if (std::sscanf(buf + 3, "%f %f %f", &x, &y, &z) == 3) {
          rawNrm.push_back(x); rawNrm.push_back(y); rawNrm.push_back(z);
          hasNormals = true;
        }
        continue;
      }
      continue;
    }
    if (buf[0] != 'f' || (buf[1] != ' ' && buf[1] != '\t')) continue;
    int pos = 1;
    int n = 0;
    bool bad = false;
    while (pos < len && n < 16) {
      pos = tokenAt(buf, pos, tok, 64);
      if (tok[0] == '\0') break;
      if (tok[0] == '#' || tok[0] == '/') { bad = true; break; }
      FaceIdx& c = face[n];
      if (!parseCorner(tok, static_cast<int>(std::strlen(tok)), c,
                       static_cast<int>(rawPos.size() / 3),
                       static_cast<int>(rawUV.size() / 2),
                       static_cast<int>(rawNrm.size() / 3))) {
        bad = true;  // битая грань: пропускаем целиком
        break;
      }
      ++n;
    }
    if (bad || n < 3) continue;
    for (int i = 2; i < n; ++i) {
      corners.push_back(face[0]);
      corners.push_back(face[i - 1]);
      corners.push_back(face[i]);
    }
  }
  std::fclose(f);

  if (corners.empty() || rawPos.empty()) return false;

  // Собираем interleaved pos+normal+uv. Дедуп по (v, vt, vn): три индекса по
  // 21 биту. При отсутствии vt/vn в файле слот занимает маркер "нет".
  struct Inter { float x, y, z, nx, ny, nz, u, v; };
  constexpr unsigned long long kNoIdx = 0x1FFFFFull;
  std::vector<Inter> verts;
  std::vector<unsigned int> indices(corners.size());
  verts.reserve(corners.size() / 2 + 16);
  std::unordered_map<unsigned long long, unsigned int> dedup;
  dedup.reserve(corners.size() / 2 + 16);

  glm::vec3 mn(1e30f), mx(-1e30f);
  int fi = 0;
  for (const FaceIdx& c : corners) {
    float nx = 0.0f, ny = 0.0f, nz = 1.0f;
    float u = 0.0f, v = 0.0f;
    if (c.vt >= 0) {
      const float* tr = &rawUV[static_cast<size_t>(c.vt) * 2];
      u = tr[0];
      v = tr[1];
    }
    const bool flat = (c.vn < 0 || !hasNormals);
    if (!flat) {
      const float* nr = &rawNrm[static_cast<size_t>(c.vn) * 3];
      nx = nr[0]; ny = nr[1]; nz = nr[2];
    } else {
      // Плоская нормаль по трём вершинам текущей грани; дедуп выключаем,
      // каждая вершина грани получает свою (одинаковую) нормаль.
      const int b = fi - (fi % 3);  // старт текущего треугольника в corners
      const FaceIdx* tri[3] = {&corners[b], &corners[b + 1], &corners[b + 2]};
      const float* p0 = &rawPos[static_cast<size_t>(tri[0]->v) * 3];
      const float* p1 = &rawPos[static_cast<size_t>(tri[1]->v) * 3];
      const float* p2 = &rawPos[static_cast<size_t>(tri[2]->v) * 3];
      const float ux = p1[0] - p0[0], uy = p1[1] - p0[1], uz = p1[2] - p0[2];
      const float wx = p2[0] - p0[0], wy = p2[1] - p0[1], wz = p2[2] - p0[2];
      nx = uy * wz - uz * wy;
      ny = uz * wx - ux * wz;
      nz = ux * wy - uy * wx;
      const float l = std::sqrt(nx * nx + ny * ny + nz * nz);
      if (l > 1e-9f) { nx /= l; ny /= l; nz /= l; }
    }

    unsigned int idx;
    if (!flat) {
      const unsigned long long key =
          (static_cast<unsigned long long>(static_cast<unsigned int>(c.v)) << 42) |
          (static_cast<unsigned long long>(
               c.vt >= 0 ? static_cast<unsigned int>(c.vt) : kNoIdx) << 21) |
          static_cast<unsigned long long>(
              c.vn >= 0 ? static_cast<unsigned int>(c.vn) : kNoIdx);
      auto it = dedup.find(key);
      if (it != dedup.end()) {
        idx = it->second;
      } else {
        idx = static_cast<unsigned int>(verts.size());
        dedup.emplace(key, idx);
        const float* p = &rawPos[static_cast<size_t>(c.v) * 3];
        verts.push_back({p[0], p[1], p[2], nx, ny, nz, u, v});
        mn = glm::min(mn, glm::vec3(p[0], p[1], p[2]));
        mx = glm::max(mx, glm::vec3(p[0], p[1], p[2]));
      }
    } else {
      idx = static_cast<unsigned int>(verts.size());
      const float* p = &rawPos[static_cast<size_t>(c.v) * 3];
      verts.push_back({p[0], p[1], p[2], nx, ny, nz, u, v});
      mn = glm::min(mn, glm::vec3(p[0], p[1], p[2]));
      mx = glm::max(mx, glm::vec3(p[0], p[1], p[2]));
    }
    indices[fi] = idx;
    ++fi;
  }

  if (verts.empty()) return false;

  // GL-аплоад. (Контекст существует: стиль 3 активируется только из рендер-потока.)
  unsigned int vao = 0, vbo = 0, ebo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(verts.size() * sizeof(Inter)),
               verts.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
               indices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Inter),
                        reinterpret_cast<const void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Inter),
                        reinterpret_cast<const void*>(12));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Inter),
                        reinterpret_cast<const void*>(24));
  glBindVertexArray(0);

  if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
  if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
  if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
  m_vao = vao;
  m_vbo = vbo;
  m_ebo = ebo;
  m_indexCount = static_cast<unsigned int>(indices.size());
  m_vertexCount = static_cast<unsigned int>(verts.size());
  m_center = (mn + mx) * 0.5f;
  (void)hasUV;
  return true;
}

bool ObjModel::loadTextures(const std::string& base) {
  const std::string albedo = base + "_albedo.bmp";
  const std::string normal = base + "_normal.bmp";
  const std::string rough = base + "_rough.bmp";
  const std::string metal = base + "_metal.bmp";
  const std::string emissive = base + "_emissive.bmp";  // опциональна
  unsigned int a = loadTextureFile(albedo);
  unsigned int n = loadTextureFile(normal);
  unsigned int r = loadTextureFile(rough);
  unsigned int m = loadTextureFile(metal);
  unsigned int e = loadTextureFile(emissive);
  if (a == 0) {
    // Без albedo набор бессмысленен: чистим частично загруженное.
    unsigned int extra[4] = {n, r, m, e};
    for (unsigned int t : extra) {
      if (t != 0) glDeleteTextures(1, &t);
    }
    return false;
  }
  if (m_texAlbedo != 0) glDeleteTextures(1, &m_texAlbedo);
  if (m_texNormal != 0) glDeleteTextures(1, &m_texNormal);
  if (m_texRough != 0) glDeleteTextures(1, &m_texRough);
  if (m_texMetal != 0) glDeleteTextures(1, &m_texMetal);
  if (m_texEmissive != 0) glDeleteTextures(1, &m_texEmissive);
  m_texAlbedo = a;
  m_texNormal = n;
  m_texRough = r;
  m_texMetal = m;
  m_texEmissive = e;
  return true;
}

void ObjModel::bindTextures(int firstUnit) const {
  const unsigned int tex[5] = {m_texAlbedo, m_texNormal, m_texRough, m_texMetal,
                               m_texEmissive};
  for (int i = 0; i < 5; ++i) {
    glActiveTexture(GL_TEXTURE0 + firstUnit + i);
    glBindTexture(GL_TEXTURE_2D, tex[i]);
  }
}

void ObjModel::draw() const {
  if (m_indexCount == 0 || m_vao == 0) return;
  glBindVertexArray(m_vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount),
                 GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

}  // namespace astro
