# Web link options shared by every game (docs/MOD_MENU_DESIGN.md 2.3). Used from a game's CMakeLists.txt:
#   if(EMSCRIPTEN)
#     byopop_web_game(<target> <shell.html> TITLE <name> COPYRIGHT <notice> [LICENSES <name>...]
#       [EXPORTS <C function>...])
#   endif()
# TITLE, COPYRIGHT and LICENSES make the game's copyright page, linked from its page's footer; see
# byopop_web_shell() below.
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
get_filename_component(BYOPOP_WEB_FOOTER "${CMAKE_CURRENT_LIST_DIR}/../web/footer.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER_CSS "${CMAKE_CURRENT_LIST_DIR}/../web/footer.css" ABSOLUTE)

# byopop_web_shell(<var> <target> <shell.html> TITLE <name> COPYRIGHT <notice> [LICENSES <name>...]): the game's
# page template, with the shared footer (web/footer.html, its style web/footer.css before </head>) at the shell's
# "<!-- byopop:footer -->" line. The footer links to the game's copyright page, <target>-copyright.html next to the
# game, written here from web/copyright.html: the game's <name> and <notice> (plain text: the copyright and
# trademark lines of the readme that comes with the game), what the player is, and the license of every library
# in web/licenses/ that the page is built from. LICENSES adds a game's own libraries (BookWorm: libopenmpt).
# Sets <var> to the generated template in the build folder, for --shell-file. Both files are rewritten only when
# their text changes, and CMake runs again when one of their inputs changes. A game that links without
# byopop_web_game() (BookWorm) uses this directly.
set(BYOPOP_WEB_LICENSES popcap-framework sdl2 stb libxmp-lite tremor)
set(BYOPOP_WEB_TOOLCHAIN_LICENSES emscripten musl)

function(byopop_write_if_changed path text)
  set(old "")
  if(EXISTS ${path})
    file(READ ${path} old)
  endif()
  if(NOT old STREQUAL text)
    file(WRITE ${path} "${text}")
  endif()
endfunction()

function(byopop_web_shell var target shell)
  cmake_parse_arguments(PARSE_ARGV 3 arg "" "TITLE;COPYRIGHT" "LICENSES")
  if(arg_UNPARSED_ARGUMENTS OR NOT arg_TITLE OR NOT arg_COPYRIGHT)
    message(FATAL_ERROR "byopop_web_shell(${target}): TITLE <the game's name> and COPYRIGHT <its notice> are required")
  endif()
  get_filename_component(web ${BYOPOP_WEB_FOOTER} DIRECTORY)
  set(inputs ${shell} ${BYOPOP_WEB_FOOTER} ${BYOPOP_WEB_FOOTER_CSS} ${web}/copyright.html)
  set(licenses "")
  foreach(name ${BYOPOP_WEB_LICENSES} ${arg_LICENSES} ${BYOPOP_WEB_TOOLCHAIN_LICENSES})
    set(file ${web}/licenses/${name}.txt)
    if(NOT EXISTS ${file})
      message(FATAL_ERROR "byopop_web_shell(${target}): no license text ${file}")
    endif()
    list(APPEND inputs ${file})
    # a license file: its title line, a blank line, the text
    file(READ ${file} text)
    string(REPLACE "&" "&amp;" text "${text}")
    string(REPLACE "<" "&lt;" text "${text}")
    string(REPLACE ">" "&gt;" text "${text}")
    string(FIND "${text}" "
" eol)
    string(SUBSTRING "${text}" 0 ${eol} title)
    math(EXPR eol "${eol} + 1")
    string(SUBSTRING "${text}" ${eol} -1 text)
    string(STRIP "${text}" text)
    string(APPEND licenses "  <details><summary>${title}</summary>
<pre>${text}</pre></details>
")
  endforeach()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${inputs})
  foreach(field TITLE COPYRIGHT)
    string(REPLACE "&" "&amp;" arg_${field} "${arg_${field}}")
    string(REPLACE "<" "&lt;" arg_${field} "${arg_${field}}")
  endforeach()

  set(copyright_page ${target}-copyright.html)
  file(READ ${web}/copyright.html text)
  string(REPLACE "@BYOPOP_TITLE@" "${arg_TITLE}" text "${text}")
  string(REPLACE "@BYOPOP_COPYRIGHT@" "${arg_COPYRIGHT}" text "${text}")
  string(REPLACE "@BYOPOP_GAME_PAGE@" "${target}.html" text "${text}")
  string(REPLACE "@BYOPOP_LICENSES@
" "${licenses}" text "${text}")
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${copyright_page} "${text}")

  file(READ ${shell} page)
  file(READ ${BYOPOP_WEB_FOOTER} footer)
  file(READ ${BYOPOP_WEB_FOOTER_CSS} css)
  string(FIND "${page}" "<!-- byopop:footer -->" at)
  if(at EQUAL -1)
    message(FATAL_ERROR "${shell} has no \"<!-- byopop:footer -->\" line for the shared footer")
  endif()
  string(REPLACE "@BYOPOP_COPYRIGHT_PAGE@" "${copyright_page}" footer "${footer}")
  string(REPLACE "<!-- byopop:footer -->" "${footer}" page "${page}")
  string(REPLACE "</head>" "<style>
${css}</style>
</head>" page "${page}")
  set(out ${CMAKE_CURRENT_BINARY_DIR}/${target}-shell.html)
  byopop_write_if_changed(${out} "${page}")
  set(${var} ${out} PARENT_SCOPE)
endfunction()

function(byopop_web_game target shell)
  cmake_parse_arguments(PARSE_ARGV 2 arg "" "TITLE;COPYRIGHT" "EXPORTS;LICENSES")
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "byopop_web_game(${target}): unknown arguments ${arg_UNPARSED_ARGUMENTS}")
  endif()
  byopop_web_shell(page ${target} ${shell} TITLE "${arg_TITLE}" COPYRIGHT "${arg_COPYRIGHT}" LICENSES ${arg_LICENSES})
  set(export_list ${BYOPOP_WEB_EXPORTED_FUNCTIONS} ${arg_EXPORTS})
  list(REMOVE_DUPLICATES export_list)
  list(JOIN export_list "','" exports)
  list(JOIN BYOPOP_WEB_RUNTIME_METHODS "','" methods)
  set_target_properties(${target} PROPERTIES SUFFIX ".html")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS ${page} ${BYOPOP_WEB_POST_JS})
  target_link_options(${target} PRIVATE
    "--shell-file=${page}" "--post-js=${BYOPOP_WEB_POST_JS}"
    "-sALLOW_MEMORY_GROWTH=1" "-sFORCE_FILESYSTEM=1" "-sINVOKE_RUN=0"
    "-sEXPORTED_RUNTIME_METHODS=['${methods}']"
    "-sEXPORTED_FUNCTIONS=['${exports}']" "-lidbfs.js")
endfunction()
