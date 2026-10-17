#pragma once
#include <string>

namespace astro {

// === Portable release (v3.6.0) ===: все shaders/*.vert|frag вшиты в бинарник
// сгенерированным EmbeddedShaders.cpp (packaging/embed_shaders.cmake) — так
// одиночный astro-voyager.exe запускается «с коробки» без папки shaders/
// рядом. Shader::readFile сначала пробует диск (dev-режим и pack-архивы),
// затем эту таблицу. Имена в таблице — те же относительные пути, что
// передают вызовы: "shaders/quad.vert", "shaders/bh.frag" и т.д.
const char* embeddedShader(const std::string& name);

}  // namespace astro
