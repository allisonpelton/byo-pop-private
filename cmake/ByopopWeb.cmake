# Web link options shared by every game (docs/MOD_MENU_DESIGN.md 2.3). Used from a game's CMakeLists.txt:
#   if(EMSCRIPTEN)
#     byopop_web_game(<target> TITLE <name> COPYRIGHT <notice> [LICENSES <name>...] [EXPORTS <C function>...]
#       [PAGE <folder>])
#   endif()
# Every game's page is web/shell.html with the game's parts from PAGE (by default the game's web/ folder): its
# page.js (its settings: storage folder, lock, game files, texts; the list is in web/shell.html), and optionally
# page.css (its colours and brand mark) and page.html (its own panels). TITLE is the game's name on the page.
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
get_filename_component(BYOPOP_WEB_SHELL "${CMAKE_CURRENT_LIST_DIR}/../web/shell.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_POST_JS "${CMAKE_CURRENT_LIST_DIR}/../web/modpanel.js" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER "${CMAKE_CURRENT_LIST_DIR}/../web/footer.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER_CSS "${CMAKE_CURRENT_LIST_DIR}/../web/footer.css" ABSOLUTE)
# where each game can be bought (BYOPOP_BUY_<target>), shown under its terms
include(${CMAKE_CURRENT_LIST_DIR}/ByopopStores.cmake)
# the site's logo and icons (web/brand; docs/WEB_PLAYER.md), copied next to the pages, which link them
set(BYOPOP_WEB_BRAND favicon.svg favicon.ico apple-touch-icon.png logo.svg logo-dark.svg)

# byopop_web_shell(<var> <target> <page folder> TITLE <name> COPYRIGHT <notice> [LICENSES <name>...]): the game's
# page template: web/shell.html with the game's page.js, page.css and page.html (from <page folder>) at its
# byopop:game-js, byopop:game-css and byopop:game-panels markers, its <name> for @BYOPOP_TITLE@, and the shared
# footer (web/footer.html, its style web/footer.css before </head>) at its "<!-- byopop:footer -->" line. The footer links to the game's copyright page, <target>-copyright.html next to the
# game, written here from web/copyright.html: the game's <name> and <notice> (plain text: the copyright and
# trademark lines of the readme that comes with the game), what the player is, and the license of every library
# in web/licenses/ that the page is built from. LICENSES adds a game's own libraries (BookWorm: libopenmpt).
# Sets <var> to the generated template in the build folder, for --shell-file. Both files are rewritten only when
# their text changes, and CMake runs again when one of their inputs changes. byopop_web_game() calls it.
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

# Puts <text> into the page text in <var> at <marker>, which web/shell.html has once
function(byopop_web_insert var marker text)
  string(FIND "${${var}}" "${marker}" first)
  string(FIND "${${var}}" "${marker}" last REVERSE)
  if(first EQUAL -1 OR NOT first EQUAL last)
    message(FATAL_ERROR "${BYOPOP_WEB_SHELL} needs one \"${marker}\"")
  endif()
  string(REPLACE "${marker}" "${text}" inserted "${${var}}")
  set(${var} "${inserted}" PARENT_SCOPE)
endfunction()

function(byopop_web_shell var target pagedir)
  cmake_parse_arguments(PARSE_ARGV 3 arg "" "TITLE;COPYRIGHT" "LICENSES")
  if(arg_UNPARSED_ARGUMENTS OR NOT arg_TITLE OR NOT arg_COPYRIGHT)
    message(FATAL_ERROR "byopop_web_shell(${target}): TITLE <the game's name> and COPYRIGHT <its notice> are required")
  endif()
  if(NOT EXISTS ${pagedir}/page.js)
    message(FATAL_ERROR "byopop_web_shell(${target}): no ${pagedir}/page.js (the game's settings for web/shell.html)")
  endif()
  get_filename_component(web ${BYOPOP_WEB_FOOTER} DIRECTORY)
  set(inputs ${BYOPOP_WEB_SHELL} ${pagedir}/page.js ${BYOPOP_WEB_FOOTER} ${BYOPOP_WEB_FOOTER_CSS} ${web}/copyright.html
    ${web}/terms.html)
  # the game's parts: page.js, and page.css and page.html where it has them
  foreach(name js css html)
    set(game_${name} "")
    if(EXISTS ${pagedir}/page.${name})
      list(APPEND inputs ${pagedir}/page.${name})
      file(READ ${pagedir}/page.${name} game_${name})
    endif()
  endforeach()
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

  # The game's terms (web/terms.html after its comment), with its name and notice. A hash of them is the version the
  # page keeps with a player's agreement, so changed terms ask again.
  file(READ ${web}/terms.html terms)
  string(FIND "${terms}" "-->" eoc)
  math(EXPR eoc "${eoc} + 3")
  string(SUBSTRING "${terms}" ${eoc} -1 terms)
  string(STRIP "${terms}" terms)
  string(REPLACE "@BYOPOP_TITLE@" "${arg_TITLE}" terms "${terms}")
  string(REPLACE "@BYOPOP_COPYRIGHT@" "${arg_COPYRIGHT}" terms "${terms}")
  string(SHA1 terms_version "${terms}")
  string(SUBSTRING "${terms_version}" 0 12 terms_version)

  # Where to buy the game (cmake/ByopopStores.cmake), under the terms on both pages; not part of their version, so a
  # change of stores asks nobody to agree again
  if(NOT DEFINED BYOPOP_BUY_${target})
    message(FATAL_ERROR "byopop_web_shell(${target}): no BYOPOP_BUY_${target} in cmake/ByopopStores.cmake (the date "
      "the digital stores were checked, then each store's name and page; a shared file: message the coordinator)")
  endif()
  set(buy ${BYOPOP_BUY_${target}})
  list(POP_FRONT buy checked)
  if(NOT checked MATCHES "^([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9])$")
    message(FATAL_ERROR "byopop_web_shell(${target}): BYOPOP_BUY_${target} starts with \"${checked}\", not a YYYY-MM-DD date")
  endif()
  # (each REGEX REPLACE resets CMAKE_MATCH_*, so all three are taken first)
  set(year ${CMAKE_MATCH_1})
  set(month ${CMAKE_MATCH_2})
  set(day ${CMAKE_MATCH_3})
  string(REGEX REPLACE "^0" "" month "${month}")
  string(REGEX REPLACE "^0" "" day "${day}")
  set(months January February March April May June July August September October November December)
  math(EXPR month "${month} - 1")
  list(GET months ${month} month)
  set(checked "${month} ${day}, ${year}")
  list(LENGTH buy count)
  math(EXPR odd "${count} % 2")
  if(odd)
    message(FATAL_ERROR "byopop_web_shell(${target}): BYOPOP_BUY_${target} needs a store's name and its page for each store")
  endif()
  if(count EQUAL 0)
    set(buy_html "<p class=\"terms-buy\">${arg_TITLE} is not for sale as of ${checked}.</p>")
  else()
    set(links "")
    while(buy)
      list(POP_FRONT buy store url)
      string(REPLACE "&" "&amp;" url "${url}")
      string(REPLACE "\"" "&quot;" url "${url}")
      list(APPEND links "<a href=\"${url}\" target=\"_blank\" rel=\"noopener noreferrer\">${store}</a>")
    endwhile()
    list(JOIN links ", " links)
    set(buy_html "<p class=\"terms-buy\">Where to buy ${arg_TITLE} digitally, as of ${checked}: ${links}.</p>")
  endif()

  foreach(name ${BYOPOP_WEB_BRAND})
    configure_file(${web}/brand/${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name} COPYONLY)
  endforeach()

  set(copyright_page ${target}-copyright.html)
  file(READ ${web}/copyright.html text)
  string(REPLACE "@BYOPOP_TITLE@" "${arg_TITLE}" text "${text}")
  string(REPLACE "@BYOPOP_COPYRIGHT@" "${arg_COPYRIGHT}" text "${text}")
  string(REPLACE "@BYOPOP_GAME_PAGE@" "${target}.html" text "${text}")
  string(REPLACE "@BYOPOP_TERMS@" "${terms}" text "${text}")
  string(REPLACE "@BYOPOP_BUY@" "${buy_html}" text "${text}")
  string(REPLACE "@BYOPOP_LICENSES@
" "${licenses}" text "${text}")
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${copyright_page} "${text}")

  file(READ ${BYOPOP_WEB_SHELL} page)
  byopop_web_insert(page "/* byopop:game-js */" "${game_js}")
  byopop_web_insert(page "/* byopop:game-css */" "${game_css}")
  byopop_web_insert(page "<!-- byopop:game-panels -->" "${game_html}")
  byopop_web_insert(page "<!-- byopop:terms -->" "${terms}")
  byopop_web_insert(page "<!-- byopop:buy -->" "${buy_html}")
  string(REPLACE "@BYOPOP_TERMS_VERSION@" "${terms_version}" page "${page}")
  string(REPLACE "@BYOPOP_TITLE@" "${arg_TITLE}" page "${page}")
  file(READ ${BYOPOP_WEB_FOOTER} footer)
  file(READ ${BYOPOP_WEB_FOOTER_CSS} css)
  string(REPLACE "@BYOPOP_COPYRIGHT_PAGE@" "${copyright_page}" footer "${footer}")
  byopop_web_insert(page "<!-- byopop:footer -->" "${footer}")
  string(REPLACE "</head>" "<style>
${css}</style>
</head>" page "${page}")
  set(out ${CMAKE_CURRENT_BINARY_DIR}/${target}-shell.html)
  byopop_write_if_changed(${out} "${page}")
  set(${var} ${out} PARENT_SCOPE)
endfunction()

function(byopop_web_game target)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "TITLE;COPYRIGHT;PAGE" "EXPORTS;LICENSES")
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "byopop_web_game(${target}): unknown arguments ${arg_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT arg_PAGE)
    set(arg_PAGE ${CMAKE_CURRENT_SOURCE_DIR}/web)
  endif()
  byopop_web_shell(page ${target} ${arg_PAGE} TITLE "${arg_TITLE}" COPYRIGHT "${arg_COPYRIGHT}" LICENSES ${arg_LICENSES})
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
