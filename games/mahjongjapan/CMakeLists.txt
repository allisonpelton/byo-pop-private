# Mahjong Escape: Ancient Japan (JollyBear Games; games/mahjongjapan) on the shared platform layer. The game runs on
# JollyBear's own engine (the manager, layers, objects, the display and the BASS sound object; docs/ENGINE.md):
# engine/ is that engine, translated and mapped onto the platform, and src/ is the translation of the game on top of
# it. The SexyAppFramework only hosts the app: its window, clock, input and sound device, and the 800x600 screen the
# game draws into.
file(GLOB MAHJONGJAPAN_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB MAHJONGJAPAN_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(mahjongjapanengine STATIC ${MAHJONGJAPAN_ENGINE_SOURCES})
target_include_directories(mahjongjapanengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(mahjongjapanengine PUBLIC sexyframework sexyopenmpt stb)
if(NOT MSVC)
  target_compile_options(mahjongjapanengine PRIVATE -Wall -Wno-unused-private-field)
endif()

# The game's translation is a library of its own, so it builds before the host app (MahjongApp.cpp, main.cpp) exists
set(MAHJONGJAPAN_HOST_SOURCES ${MAHJONGJAPAN_SOURCES})
list(FILTER MAHJONGJAPAN_SOURCES EXCLUDE REGEX "/(MahjongApp|main)\\.cpp$")
list(FILTER MAHJONGJAPAN_HOST_SOURCES INCLUDE REGEX "/(MahjongApp|main)\\.cpp$")
add_library(mahjongjapangame STATIC ${MAHJONGJAPAN_SOURCES})
target_include_directories(mahjongjapangame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mahjongjapangame PUBLIC mahjongjapanengine)
if(NOT MSVC)
  target_compile_options(mahjongjapangame PRIVATE -Wall -Wno-unused-private-field)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MahjongApp.cpp)
  add_executable(mahjongjapan ${MAHJONGJAPAN_HOST_SOURCES})
  target_link_libraries(mahjongjapan PRIVATE mahjongjapangame mahjongjapanengine sexyframework sexyopenmpt stb)
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
