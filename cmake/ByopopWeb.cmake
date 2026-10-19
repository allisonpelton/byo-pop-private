# Web link options shared by every game (docs/MOD_MENU_DESIGN.md 2.3). Used from a game's CMakeLists.txt:
#   if(EMSCRIPTEN)
#     byopop_web_game(<target> <shell.html> [EXPORTS <C function>...])
#   endif()
# EXPORTS adds a game's own functions to the exported list (Bejeweled: _fflush, for its Node test runner).
# The page runs the game with callMain() (INVOKE_RUN=0) and calls the C functions of platform/WebApi.cpp and
# framework/ModMenu.cpp with ccall. Those live in the sexyframework static library, where EMSCRIPTEN_KEEPALIVE
# alone would not link them: EXPORTED_FUNCTIONS names them, which pulls them in. That list replaces
# Emscripten's default, so it names _main too.
# web/modpanel.js (the Mods panel, the rescue and restart bookkeeping, the page's file helpers: Module.byopop) is
# appended to the game's JS with --post-js, so every game's page has the same one without a file of its own.
set(BYOPOP_WEB_EXPORTED_FUNCTIONS _main _byopop_userdata_restore _byopop_mods_open _byopop_mods_state
  _byopop_mods_edit _byopop_mods_rescan _byopop_menu _byopop_restart)
set(BYOPOP_WEB_RUNTIME_METHODS FS callMain ccall)
get_filename_component(BYOPOP_WEB_POST_JS "${CMAKE_CURRENT_LIST_DIR}/../web/modpanel.js" ABSOLUTE)

function(byopop_web_game target shell)
  cmake_parse_arguments(PARSE_ARGV 2 arg "" "" "EXPORTS")
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "byopop_web_game(${target}): unknown arguments ${arg_UNPARSED_ARGUMENTS}")
  endif()
  set(export_list ${BYOPOP_WEB_EXPORTED_FUNCTIONS} ${arg_EXPORTS})
  list(REMOVE_DUPLICATES export_list)
  list(JOIN export_list "','" exports)
  list(JOIN BYOPOP_WEB_RUNTIME_METHODS "','" methods)
  set_target_properties(${target} PROPERTIES SUFFIX ".html")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${shell} ${BYOPOP_WEB_POST_JS})
  target_link_options(${target} PRIVATE
    "--shell-file=${shell}" "--post-js=${BYOPOP_WEB_POST_JS}"
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1" "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['${methods}']"
    "-sEXPORTED_FUNCTIONS=['${exports}']" "-lidbfs.js")
endfunction()
