cmake_minimum_required(VERSION 3.16)
project(astro-voyager VERSION 3.8.3 LANGUAGES CXX)

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
  src/Kerr.cpp
  src/Interaction.cpp
  src/Constellation.cpp
  src/SolarSystem.cpp
  src/Journey.cpp
  src/Achievements.cpp
  src/Tour.cpp
  src/SaveGame.cpp  # === SaveGame === (v3.4.0): JSON-профиль, чистый C++17
  src/StoryJournal.cpp  # === StoryJournal === (v3.5.0): бортовой журнал, чистый C++17
  src/Expedition.cpp  # === Expedition === (v3.5.0): сюжетная арка, чистый C++17
)

target_include_directories(astro-voyager PRIVATE "${CMAKE_SOURCE_DIR}/src")

find_package(Threads REQUIRED)
target_link_libraries(astro-voyager PRIVATE Threads::Threads)

if(WIN32)
  target_link_libraries(astro-voyager PRIVATE ws2_32)
endif()

if(MSVC)
  # /utf-8: русские строковые литералы (кинотур/карточки/HUD) — и в исходниках,
  # и в рантайме строго UTF-8, иначе ImGui покажет "?" вместо кириллицы.
  target_compile_options(astro-voyager PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(astro-voyager PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_interaction.cpp")
  add_executable(test_interaction
    tests/test_interaction.cpp
    src/Interaction.cpp
    src/Catalog.cpp
    src/WarpJump.cpp
  )
  target_include_directories(test_interaction PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_interaction PRIVATE /W4 /permissive-)
  else()
    target_compile_options(test_interaction PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_journey.cpp")
  add_executable(test_journey
    tests/test_journey.cpp
    src/Journey.cpp
  )
  target_include_directories(test_journey PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_journey PRIVATE /W4 /permissive-)
  else()
    target_compile_options(test_journey PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_achievements.cpp")
  add_executable(test_achievements
    tests/test_achievements.cpp
    src/Achievements.cpp
  )
  target_include_directories(test_achievements PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_achievements PRIVATE /W4 /permissive-)
  else()
    target_compile_options(test_achievements PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_save.cpp")
  add_executable(test_save
    tests/test_save.cpp
    src/SaveGame.cpp
  )
  target_include_directories(test_save PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_save PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(test_save PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_story.cpp")
  add_executable(test_story
    tests/test_story.cpp
    src/StoryJournal.cpp
  )
  target_include_directories(test_story PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_story PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(test_story PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_expedition.cpp")
  add_executable(test_expedition
    tests/test_expedition.cpp
    src/Expedition.cpp
  )
  target_include_directories(test_expedition PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_expedition PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(test_expedition PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_rkf45.cpp")
  add_executable(test_rkf45
    tests/test_rkf45.cpp
    src/Geodesic.cpp
  )
  target_include_directories(test_rkf45 PRIVATE "${CMAKE_SOURCE_DIR}/src")

  if(MSVC)
    target_compile_options(test_rkf45 PRIVATE /W4 /permissive-)
  else()
    target_compile_options(test_rkf45 PRIVATE -Wall -Wextra -Wpedantic)
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
  elseif(APPLE)
    # === macOS GL (v3.4.0) ===: системный OpenGL 3.3 core без GLEW —
    # функции линкуются из OpenGL.framework (src/GLCompat.h даёт no-op
    # glewInit, поэтому Window.cpp общий для всех платформ).
    # GLFW_INCLUDE_NONE: glfw3.h не подтягивает legacy gl.h рядом с gl3.h.
    # CoreAudio/AudioToolbox/CoreFoundation — для miniaudio, Cocoa/IOKit/
    # CoreVideo — для GLFW.
    find_package(OpenGL REQUIRED)
    target_compile_definitions(astro-voyager PRIVATE GLFW_INCLUDE_NONE)
    target_link_libraries(astro-voyager PRIVATE
      OpenGL::GL
      "-framework Cocoa"
      "-framework IOKit"
      "-framework CoreVideo"
      "-framework CoreFoundation"
      "-framework CoreAudio"
      "-framework AudioToolbox"
    )
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
    src/ObjModel.cpp
    src/Cubemap.cpp
    src/PostFX.cpp
    src/SceneManager.cpp
    src/AppGL.cpp
    src/StorySlides.cpp  # === Expedition === (v3.5.0): слайды + вектор-арт
    src/WarpJump.cpp  # === WarpJump === (агент J: чистая логика катсцены)
    src/vendor/imgui/imgui.cpp
    src/vendor/imgui/imgui_draw.cpp
    src/vendor/imgui/imgui_tables.cpp
    src/vendor/imgui/imgui_widgets.cpp
    src/vendor/imgui/backends/imgui_impl_glfw.cpp
    src/vendor/imgui/backends/imgui_impl_opengl3.cpp
  )

  # === Audio === (агент AU: процедурный звук только для GL-таргета).
  # Core-таргет (без GL) звук не тянет и собирается без аудио-кода.
  # miniaudio.h лежит в src/audio (НЕ в src/vendor — он в gitignore).
  # MINIAUDIO_IMPLEMENTATION ровно в одном .cpp (src/audio/AudioEngine.cpp).
  target_sources(astro-voyager PRIVATE src/audio/AudioEngine.cpp)
  target_include_directories(astro-voyager PRIVATE "${CMAKE_SOURCE_DIR}/src")
  if(UNIX)
    target_link_libraries(astro-voyager PRIVATE pthread dl m)
  endif()
  # === Audio === end

  # === Portable release (v3.6.0) ===: все shaders/*.vert|frag вшиваются в
  # бинарник на этапе configure (packaging/embed_shaders.cmake) — одиночный
  # astro-voyager.exe запускается «с коробки» без папки shaders/ рядом.
  # Shader::readFile сначала пробует диск (dev-режим), затем встроенную
  # таблицу. Генерация на каждом configure: правки шейдеров подхватываются
  # после любой переконфигурации.
  set(ASTRO_EMBED_CPP "${CMAKE_BINARY_DIR}/generated/EmbeddedShaders.cpp")
  execute_process(
    COMMAND "${CMAKE_COMMAND}"
      "-DSHADER_DIR=${CMAKE_SOURCE_DIR}/shaders"
      "-DOUT=${ASTRO_EMBED_CPP}"
      -P "${CMAKE_SOURCE_DIR}/packaging/embed_shaders.cmake"
    RESULT_VARIABLE astro_embed_rc)
  if(NOT astro_embed_rc EQUAL 0)
    message(FATAL_ERROR "embed_shaders.cmake failed (rc=${astro_embed_rc})")
  endif()
  target_sources(astro-voyager PRIVATE "${ASTRO_EMBED_CPP}")

  # === AV hygiene (v3.8.1) ===: иконка + VERSIONINFO в exe. Генерик-эвристики
  # AV (VirusTotal 2/71 на v3.8.0) чаще помечают неподписанный exe без
  # метаданных; версия подставляется из project(... VERSION ...).
  if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/packaging/app.win.rc.in")
    # rc.exe — ANSI (не UTF-8): абсолютные пути с кириллицей пользовательской
    # папки валят его с rc 2. В ресурс пишем ASCII-путь ОТНОСИТЕЛЬНО каталога
    # generated/ (каталог рабочей копии компилятора ресурсов).
    set(AV_ICON_PATH "${CMAKE_SOURCE_DIR}/packaging/app.ico")
    file(RELATIVE_PATH AV_ICON_PATH "${CMAKE_BINARY_DIR}/generated"
         "${AV_ICON_PATH}")
    # v3.8.2: манифест (asInvoker + supportedOS Win7..Win11 + long paths)
    # вшивается ресурсом RT_MANIFEST: без него Windows считает exe legacy-
    # бинарником, и SmartScreen/эвристики AV относятся к неподписанной сборке
    # ещё подозрительнее.
    set(AV_MANIFEST_PATH "${CMAKE_SOURCE_DIR}/packaging/app.manifest")
    file(RELATIVE_PATH AV_MANIFEST_PATH "${CMAKE_BINARY_DIR}/generated"
         "${AV_MANIFEST_PATH}")
    configure_file(
      "${CMAKE_SOURCE_DIR}/packaging/app.win.rc.in"
      "${CMAKE_BINARY_DIR}/generated/app.win.rc"
      @ONLY)
    target_sources(astro-voyager PRIVATE "${CMAKE_BINARY_DIR}/generated/app.win.rc")
  endif()
endif()

# === Win32GUI === begin (агент UI): GL-приложение без консольного окна.
# При запуске двойным кликом больше нет пустой консоли. Только GL-таргет;
# core-таргет остаётся консольным. Точка входа main() сохраняется
# (MSVC /ENTRY:mainCRTStartup, MinGW -mwindows): CLI-флаги (--version,
# --demo-*, --screenshot-all) парсятся как раньше, файлы пишутся, код
# возврата в норме — без консоли stdout просто не виден. Опция
# ASTROVOYAGER_WIN32_GUI=OFF возвращает консоль для отладки.
option(ASTROVOYAGER_WIN32_GUI "Build GL app as Windows GUI without console" ON)
if(ASTROVOYAGER_ENABLE_GL AND WIN32 AND ASTROVOYAGER_WIN32_GUI)
  if(MSVC)
    # /MANIFEST:NO — манифест уже вшит ресурсом RT_MANIFEST (app.win.rc), иначе
    # link.exe синтезирует второй манифест и CVTRES падает с CVT1100 (duplicate).
    target_link_options(astro-voyager PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup" "/MANIFEST:NO")
  else()
    target_link_options(astro-voyager PRIVATE "-mwindows")
  endif()
endif()
# === Win32GUI === end

enable_testing()
if(TARGET test_interaction)
  add_test(NAME test_interaction COMMAND test_interaction)
endif()
if(TARGET test_journey)
  add_test(NAME test_journey COMMAND test_journey)
endif()
if(TARGET test_achievements)
  add_test(NAME test_achievements COMMAND test_achievements)
endif()
if(TARGET test_save)
  add_test(NAME test_save COMMAND test_save)
endif()
if(TARGET test_story)
  add_test(NAME test_story COMMAND test_story)
endif()
if(TARGET test_expedition)
  add_test(NAME test_expedition COMMAND test_expedition)
endif()

if(TARGET test_rkf45)
  add_test(NAME test_rkf45 COMMAND test_rkf45)
endif()

# Runtime resources next to the binary (double-click launch support):
# the app loads shaders/ and res/ via relative paths, so they must sit
# beside the exe, not only in the source tree. Guarded: minimal source
# checkouts (e.g. the CLI-only docker build without shaders/) skip it.
if(EXISTS "${CMAKE_SOURCE_DIR}/shaders")
  add_custom_command(TARGET astro-voyager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:astro-voyager>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:astro-voyager>/res"
  )
endif()
