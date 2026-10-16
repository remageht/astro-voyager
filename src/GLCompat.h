#pragma once
// === GLCompat === (v3.4.0): единая точка входа для GL-заголовков.
// Windows/Linux: GLEW (как было). macOS: системный OpenGL 3.3 core —
// GLEW там не нужен (функции линкуются из OpenGL.framework), поэтому
// ниже — мини-совместимость: glewInit()/GLEW_OK/glewExperimental станут
// no-op, а Window.cpp остаётся общим для всех платформ.
// На macOS GLFW обязан собираться с GLFW_INCLUDE_NONE (см. CMakeLists),
// чтобы glfw3.h не подтянул legacy gl.h рядом с gl3.h.
#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>

#define GLEW_OK 0
inline bool glewExperimental = false;
inline unsigned int glewInit() { return GLEW_OK; }
inline const char* glewGetErrorString(unsigned int) {
  return "GLEW is not used on macOS (system OpenGL)";
}
#else
#include <GL/glew.h>
#endif
