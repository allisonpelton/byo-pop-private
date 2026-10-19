# Web link options shared by every game (docs/MOD_MENU_DESIGN.md 2.3). Used from a game's CMakeLists.txt:
#   if(EMSCRIPTEN)
#     byopop_web_game(<target> <shell.html>)
#   endif()
# The page runs the game with callMain() (INVOKE_RUN=0) and calls the C functions of platform/WebApi.cpp with
# ccall. Those live in the sexyframework static library, where EMSCRIPTEN_KEEPALIVE alone would not link them:
# EXPORTED_FUNCTIONS names them, which pulls them in. That list replaces Emscripten's default, so it names
# _main too.
set(BYOPOP_WEB_EXPORTED_FUNCTIONS _main _byopop_userdata_restore)
set(BYOPOP_WEB_RUNTIME_METHODS FS callMain ccall)

function(byopop_web_game target shell)
  list(JOIN BYOPOP_WEB_EXPORTED_FUNCTIONS "','" exports)
  list(JOIN BYOPOP_WEB_RUNTIME_METHODS "','" methods)
  set_target_properties(${target} PROPERTIES SUFFIX ".html")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${shell})
  target_link_options(${target} PRIVATE
    "--shell-file=${shell}"
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1" "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['${methods}']"
    "-sEXPORTED_FUNCTIONS=['${exports}']" "-lidbfs.js")
endfunction()
