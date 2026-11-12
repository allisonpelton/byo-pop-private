# Mahjong Escape's game code, built once for each edition (Ancient Japan, Ancient China). The code of the two exes is one
# source, engines/mahjongescape/src (the boards and layouts, the tiles' rules, the bot, the dialogs, the map, the play
# layer, the host app), on JollyBear's engine (engines/jollybear). What differs between the exes is each game's own
# files in its src/: Edition.h and Edition.cpp (the names, the numbers, the tables, the switches of what only one
# edition has), Res.h (the resource ids), GameTextTable.inc (the strings read from the exe) and the few functions the two
# exes do differently (LevelDone, the panel's Draw). The game's src/ comes first on the include path, so the shared code
# finds its Edition.h and Res.h. Each game's folder keeps its data, docs, tools, web page and mods.
set(MAHJONGESCAPE_DIR ${CMAKE_CURRENT_LIST_DIR})

# mahjongescape_game(<game> TITLE <title> COPYRIGHT <line>): the library <game>game, the deal's test <game>_dealtest, the
# exe (or the web build) <game>; called from the game's own CMakeLists.txt
function(mahjongescape_game GAME)
  cmake_parse_arguments(ARG "" "TITLE;COPYRIGHT" "" ${ARGN})
  string(TOUPPER ${GAME} UPPER)
  file(GLOB ${UPPER}_SOURCES CONFIGURE_DEPENDS ${MAHJONGESCAPE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

  # The game's translation is a library of its own, so it builds before the host app (MahjongApp.cpp, main.cpp) exists
  set(${UPPER}_HOST_SOURCES ${${UPPER}_SOURCES})
  list(FILTER ${UPPER}_SOURCES EXCLUDE REGEX "/(MahjongApp|main|TestCommands)\.cpp$")
  list(FILTER ${UPPER}_HOST_SOURCES INCLUDE REGEX "/(MahjongApp|main|TestCommands)\.cpp$")
  add_library(${GAME}game STATIC ${${UPPER}_SOURCES})
  target_include_directories(${GAME}game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${MAHJONGESCAPE_DIR}/src)
  target_link_libraries(${GAME}game PUBLIC jollybearengine)
  if(NOT MSVC)
    target_compile_options(${GAME}game PRIVATE -Wall -Wno-unused-private-field)
  endif()

  # The layouts and the deal, checked without a window (tools/dealtest.cpp)
  if(NOT EMSCRIPTEN)
    add_executable(${GAME}_dealtest ${CMAKE_CURRENT_SOURCE_DIR}/tools/dealtest.cpp)
    target_link_libraries(${GAME}_dealtest PRIVATE ${GAME}game)
  endif()

  add_executable(${GAME} ${${UPPER}_HOST_SOURCES})
  target_link_libraries(${GAME} PRIVATE ${GAME}game jollybearengine sexyframework sexyopenmpt stb)
  if(NOT MSVC)
    target_compile_options(${GAME} PRIVATE -Wall -Wno-unused-private-field)
  endif()
  if(EMSCRIPTEN)
    byopop_web_game(${GAME} TITLE "${ARG_TITLE}" COPYRIGHT "${ARG_COPYRIGHT}")
  else()
    add_custom_command(TARGET ${GAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:${GAME}>
      VERBATIM)
  endif()
endfunction()
