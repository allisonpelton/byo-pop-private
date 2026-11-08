# Mahjong Escape: Ancient China (JollyBear Games; games/mahjongchina) on the shared platform layer. The game runs on
# JollyBear's own engine (the manager, layers, objects, the display and the BASS sound object; docs/ENGINE.md):
# engine/ is that engine, translated and mapped onto the platform, and src/ is the translation of the game on top of
# it. The SexyAppFramework only hosts the app: its window, clock, input and sound device, and the 800x600 screen the
# game draws into.
file(GLOB MAHJONGCHINA_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB MAHJONGCHINA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(mahjongchinaengine STATIC ${MAHJONGCHINA_ENGINE_SOURCES})
target_include_directories(mahjongchinaengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(mahjongchinaengine PUBLIC sexyframework sexyopenmpt stb)
if(NOT MSVC)
  target_compile_options(mahjongchinaengine PRIVATE -Wall -Wno-unused-private-field)
endif()

# The game's translation is a library of its own, so it builds before the host app (MahjongApp.cpp, main.cpp) exists
set(MAHJONGCHINA_HOST_SOURCES ${MAHJONGCHINA_SOURCES})
list(FILTER MAHJONGCHINA_SOURCES EXCLUDE REGEX "/(MahjongApp|main|TestCommands|ModFeatures)\\.cpp$")
list(FILTER MAHJONGCHINA_HOST_SOURCES INCLUDE REGEX "/(MahjongApp|main|TestCommands|ModFeatures)\\.cpp$")
add_library(mahjongchinagame STATIC ${MAHJONGCHINA_SOURCES})
target_include_directories(mahjongchinagame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(mahjongchinagame PUBLIC mahjongchinaengine)
if(NOT MSVC)
  target_compile_options(mahjongchinagame PRIVATE -Wall -Wno-unused-private-field)
endif()

# The layouts and the deal, checked without a window (tools/dealtest.cpp)
if(NOT EMSCRIPTEN)
  add_executable(mahjongchina_dealtest ${CMAKE_CURRENT_SOURCE_DIR}/tools/dealtest.cpp)
  target_link_libraries(mahjongchina_dealtest PRIVATE mahjongchinagame)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/MahjongApp.cpp)
  add_executable(mahjongchina ${MAHJONGCHINA_HOST_SOURCES})
  target_link_libraries(mahjongchina PRIVATE mahjongchinagame mahjongchinaengine sexyframework sexyopenmpt stb)
  if(NOT MSVC)
    target_compile_options(mahjongchina PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(mahjongchina TITLE "Mahjong Escape: Ancient China"
      COPYRIGHT "Mahjong Escape: Ancient China © JollyBear Games. All rights reserved.")
  else()
    add_custom_command(TARGET mahjongchina POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:mahjongchina>
      VERBATIM)
  endif()
endif()
