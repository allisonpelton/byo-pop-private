# Mahjong Escape: Ancient Japan (JollyBear Games; games/mahjongjapan) on the shared platform layer. The game runs on
# JollyBear's own engine (the manager, layers, objects, the display and the BASS sound object; docs/ENGINE.md):
# engine/ is that engine, translated and mapped onto the platform, and src/ is the translation of the game on top of
# it. The SexyAppFramework only hosts the app: its window, clock, input and sound device, and the 800x600 screen the
# game draws into.
file(GLOB MAHJONGJAPAN_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# The game's translation is a library of its own, so it builds before the host app (MahjongApp.cpp, main.cpp) exists
set(MAHJONGJAPAN_HOST_SOURCES ${MAHJONGJAPAN_SOURCES})
list(FILTER MAHJONGJAPAN_SOURCES EXCLUDE REGEX "/(MahjongApp|main|TestCommands|ModFeatures)\\.cpp$")
list(FILTER MAHJONGJAPAN_HOST_SOURCES INCLUDE REGEX "/(MahjongApp|main|TestCommands|ModFeatures)\\.cpp$")
add_library(mahjongjapangame STATIC ${MAHJONGJAPAN_SOURCES})
target_include_directories(mahjongjapangame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mahjongjapangame PUBLIC jollybearengine)
if(NOT MSVC)
  target_compile_options(mahjongjapangame PRIVATE -Wall -Wno-unused-private-field)
endif()

# The layouts and the deal, checked without a window (tools/dealtest.cpp)
if(NOT EMSCRIPTEN)
  add_executable(mahjongjapan_dealtest ${CMAKE_CURRENT_SOURCE_DIR}/tools/dealtest.cpp)
  target_link_libraries(mahjongjapan_dealtest PRIVATE mahjongjapangame)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MahjongApp.cpp)
  add_executable(mahjongjapan ${MAHJONGJAPAN_HOST_SOURCES})
  target_link_libraries(mahjongjapan PRIVATE mahjongjapangame jollybearengine sexyframework sexyopenmpt stb)
  if(NOT MSVC)
    target_compile_options(mahjongjapan PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(mahjongjapan TITLE "Mahjong Escape: Ancient Japan"
      COPYRIGHT "Mahjong Escape: Ancient Japan © JollyBear Games. All rights reserved.")
  else()
    add_custom_command(TARGET mahjongjapan POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:mahjongjapan>
      VERBATIM)
  endif()
endif()
