cmake_minimum_required(VERSION 3.16)
project(astro-voyager VERSION 2.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ASTROVOYAGER_ENABLE_GL "Build full OpenGL renderer (needs GLFW/GLEW/GLM/ImGui)" OFF)

add_executable(astro-voyager
  src/main.cpp
  src/Catalog.cpp
  src/Camera.cpp
  src/Geodesic.cpp
  src/Binet.cpp
  src/Interaction.cpp
  src/Constellation.cpp
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

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_interaction.cpp")
  add_executable(test_interaction
    tests/test_interaction.cpp
    src/Interaction.cpp
  )
  target_include_directories(test_interaction PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_interaction PRIVATE /W4 /permissive-)
  else()
    target_compile_options(test_interaction PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(ASTROVOYAGER_ENABLE_GL)
  message(STATUS "GL renderer enabled (expects Dependencies/ + src/vendor/ locally)")
  target_compile_definitions(astro-voyager PRIVATE ASTROVOYAGER_ENABLE_GL=1)

  # GLFW
  if(EXISTS "${CMAKE_SOURCE_DIR}/Dependencies/GLFW/CMakeLists.txt")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    if(UNIX AND NOT APPLE)
      # Linux distribution builds (AppImage) ship X11 only: the Wayland backend
      # would add libwayland/libdecor/libxkbcommon runtime dependencies that are
      # not present on X11-only hosts (XWayland covers Wayland desktops).
      set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
      set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(Dependencies/GLFW)
    target_link_libraries(astro-voyager PRIVATE glfw)
  else()
    find_package(glfw3 REQUIRED)
    target_link_libraries(astro-voyager PRIVATE glfw)
  endif()

  # GLM
  if(EXISTS "${CMAKE_SOURCE_DIR}/src/vendor/glm")
    target_include_directories(astro-voyager PRIVATE "${CMAKE_SOURCE_DIR}/src/vendor/glm")
  else()
    find_package(glm REQUIRED)
    target_link_libraries(astro-voyager PRIVATE glm::glm)
  endif()

  # GLEW & OpenGL
  if(WIN32)
    target_include_directories(astro-voyager PRIVATE "${CMAKE_SOURCE_DIR}/Dependencies/GLEW/include")
    target_link_libraries(astro-voyager PRIVATE
      "${CMAKE_SOURCE_DIR}/Dependencies/GLEW/lib/Release/x64/glew32s.lib"
      opengl32
    )
    target_compile_definitions(astro-voyager PRIVATE GLEW_STATIC)
  else()
    find_package(GLEW REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(astro-voyager PRIVATE GLEW::GLEW OpenGL::GL)
  endif()

  # ImGui and stb_image
  target_include_directories(astro-voyager PRIVATE
    "${CMAKE_SOURCE_DIR}/src/vendor"
    "${CMAKE_SOURCE_DIR}/src/vendor/imgui"
    "${CMAKE_SOURCE_DIR}/src/vendor/imgui/backends"
    "${CMAKE_SOURCE_DIR}/src/vendor/stb_image"
  )

  target_sources(astro-voyager PRIVATE
    src/Window.cpp
    src/Shader.cpp
    src/Mesh.cpp
    src/Cubemap.cpp
    src/SceneManager.cpp
    src/AppGL.cpp
    src/vendor/imgui/imgui.cpp
    src/vendor/imgui/imgui_draw.cpp
    src/vendor/imgui/imgui_tables.cpp
    src/vendor/imgui/imgui_widgets.cpp
    src/vendor/imgui/backends/imgui_impl_glfw.cpp
    src/vendor/imgui/backends/imgui_impl_opengl3.cpp
  )
endif()

enable_testing()
if(TARGET test_interaction)
  add_test(NAME test_interaction COMMAND test_interaction)
endif()
