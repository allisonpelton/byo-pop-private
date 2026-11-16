# Bejeweled 3 (PopCap, 2010; games/bejeweled3). The port is a translation of the original's game classes
# (docs/PORT_PLAN.md) on the shared framework and platform layer, with the framework generation the game was built on
# in engines/prime (shared with Zuma's Revenge, docs/ENGINES.md).
#
# b3_curvetest evaluates the curve strings of a staged Bejeweled3.exe (or the ones given), headless
# (tools/curvetest/curvetest.cpp).
if(NOT EMSCRIPTEN)
  add_executable(b3_curvetest tools/curvetest/curvetest.cpp)
  target_link_libraries(b3_curvetest PRIVATE prime)
  if(NOT MSVC)
    target_compile_options(b3_curvetest PRIVATE -Wall)
  endif()
  add_custom_command(TARGET b3_curvetest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:b3_curvetest>
    VERBATIM)
endif()
