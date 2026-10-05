cmake_minimum_required(VERSION 3.16)
project(astro-voyager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ASTROVOYAGER_ENABLE_GL "Build full OpenGL renderer (needs GLFW/GLEW/GLM/ImGui)" OFF)

add_executable(astro-voyager
  src/main.cpp
  src/Catalog.cpp
  src/Camera.cpp
  src/Geodesic.cpp
)

target_include_directories(astro-voyager PRIVATE "${CMAKE_SOURCE_DIR}/src")

find_package(Threads REQUIRED)
target_link_libraries(astro-voyager PRIVATE Threads::Threads)

if(WIN32)
  target_link_libraries(astro-voyager PRIVATE ws2_32)
endif()

if(MSVC)
  target_compile_options(astro-voyager PRIVATE /W4 /permissive-)
else()
  target_compile_options(astro-voyager PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ASTROVOYAGER_ENABLE_GL)
  message(STATUS "GL renderer enabled (expects Dependencies/ + src/vendor/ locally)")
endif()

enable_testing()
