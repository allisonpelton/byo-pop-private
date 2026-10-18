# Big Money Deluxe (PopCap, 2005) on the same portable framework as Bejeweled. The game code is a
# translation of the original's game classes (games/bigmoney/docs).
# Native: bigmoney.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] (see main.cpp); it finds
# assets/bigmoney/ (games/bigmoney/tools/stage_bigmoney_assets.py) or the Steam install by itself.
# Web: bigmoney.html/.js/.wasm; games/bigmoney/web/shell.html imports the user's game files into /bmpersist
# (IDBFS) and starts the game with callMain().
file(GLOB BIGMONEY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(bigmoney ${BIGMONEY_SOURCES} ${CMAKE_SOURCE_DIR}/src/SteamPath.cpp)
target_include_directories(bigmoney PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(bigmoney PRIVATE sexyframework)
# Next to bejeweled.exe / bejeweled.html in the build folder
set_target_properties(bigmoney PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
if(NOT MSVC)
  target_compile_options(bigmoney PRIVATE -Wall -Wno-unused-private-field)
endif()
if(EMSCRIPTEN)
  set_target_properties(bigmoney PROPERTIES SUFFIX ".html")
  set_property(TARGET bigmoney APPEND PROPERTY LINK_DEPENDS ${CMAKE_SOURCE_DIR}/games/bigmoney/web/shell.html)
  target_link_options(bigmoney PRIVATE
    "--shell-file=${CMAKE_SOURCE_DIR}/games/bigmoney/web/shell.html"
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1" "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['FS','callMain']" "-lidbfs.js")
else()
  add_custom_command(TARGET bigmoney POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bigmoney>
    VERBATIM)
endif()
