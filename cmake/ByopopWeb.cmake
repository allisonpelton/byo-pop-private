# Web link options shared by every game (docs/MOD_MENU_DESIGN.md 2.3). Used from a game's CMakeLists.txt:
#   if(EMSCRIPTEN)
#     byopop_web_game(<target> TITLE <name> COPYRIGHT <notice> [LICENSES <name>...] [EXPORTS <C function>...]
#       [PAGE <folder>])
#   endif()
# Every game's page is web/shell.html with the game's parts from PAGE (by default the game's web/ folder): its
# page.js (its settings: storage folder, lock, game files, texts; the list is in web/shell.html), and optionally
# page.css (its colours and brand mark) and page.html (its own panels). TITLE is the game's name on the page.
# TITLE, COPYRIGHT and LICENSES make the game's copyright page, linked from its page's footer; see
# byopop_web_shell() below. TITLE is also the game's name on the listing page (build/web/index.html) when the game is
# in BYOPOP_WEB_FINISHED, below.
# EXPORTS adds a game's own functions to the exported list (Bejeweled: _fflush, for its Node test runner).
# The page runs the game with callMain() (INVOKE_RUN=0) and calls the C functions of platform/WebApi.cpp,
# framework/ModMenu.cpp, framework/WebCursor.cpp and framework/WebSettings.cpp with ccall. Those live in the
# sexyframework static library, where EMSCRIPTEN_KEEPALIVE alone would not link them: EXPORTED_FUNCTIONS names them,
# which pulls them in. That list replaces Emscripten's default, so it names _main too.
# web/modpanel.js (the Mods panel, the rescue and restart bookkeeping, the page's file helpers: Module.byopop) is
# appended to the game's JS with --post-js, so every game's page has the same one without a file of its own.
set(BYOPOP_WEB_EXPORTED_FUNCTIONS _main _byopop_userdata_restore _byopop_mods_open _byopop_mods_state
  _byopop_mods_edit _byopop_mods_rescan _byopop_menu _byopop_restart _byopop_cursor _byopop_cursor_copy
  _byopop_audio_mute _byopop_audio_muted _byopop_site_apply)
set(BYOPOP_WEB_RUNTIME_METHODS FS callMain ccall)
get_filename_component(BYOPOP_WEB_SHELL "${CMAKE_CURRENT_LIST_DIR}/../web/shell.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_POST_JS "${CMAKE_CURRENT_LIST_DIR}/../web/modpanel.js" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER "${CMAKE_CURRENT_LIST_DIR}/../web/footer.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER_CSS "${CMAKE_CURRENT_LIST_DIR}/../web/footer.css" ABSOLUTE)
# where each game can be bought (BYOPOP_BUY_<target>), shown under its terms
include(${CMAKE_CURRENT_LIST_DIR}/ByopopStores.cmake)
# the site's logo and icons (web/brand; docs/WEB_PLAYER.md), copied next to the pages, which link them
set(BYOPOP_WEB_BRAND favicon.svg favicon.ico apple-touch-icon.png logo.svg logo-dark.svg)
# web/settings.js, the site's settings and the record of which games have their files (docs/WEB_PLAYER.md, "Site
# settings", "The listing page"): one file that the listing page and every game page load. web/profiles.js keeps the site's
# players and what the games report of theirs ("Player profiles"); the listing page and the game pages load it too.
# web/exeicon.js reads a game's icon out of its exe (the game pages load it). All three are copied next to the pages.
set(BYOPOP_WEB_SHARED_JS settings.js profiles.js exeicon.js)
# the listing page's own scripts: the Players section (web/players.js), copied next to index.html only
set(BYOPOP_WEB_INDEX_JS players.js)
# The listing page, build/web/index.html (made from web/index.html by byopop_web_index() at the end of this file): one
# tile for each game in BYOPOP_WEB_FINISHED, in this order, with the game's TITLE and a badge. The list is the
# coordinator's: the coordinator adds a game's target at that game's final merge. A game with a web page that is not
# in the list is not shown (fwtest and smoke are test pages and never are); a name in the list with no web page is a
# configure error.
set(BYOPOP_WEB_FINISHED bejeweled bigmoney bookworm noahsark sevenseas atomica mummymaze tiptop bookwormadv ningpo
  pixelus alchemy wordharmony cosmicbugs waterbugs rocketmania dynomite bookwormadv2)
# A game's badge on the listing page, <target>|<initials>|<colour>: the initials in white on a rounded square of the
# colour. Original to byo-pop, never the games' art. The colours keep white text above a 4.5 contrast ratio. A game
# without a row here gets the initials of its TITLE and the accent colour.
set(BYOPOP_WEB_BADGES
  "bejeweled|BJ|#95233d" "bigmoney|BM|#1f8438" "bookworm|BW|#955c23" "noahsark|NA|#234995" "sevenseas|7S|#1e8080"
  "atomica|AT|#602395" "mummymaze|MM|#8c7021" "tiptop|TT|#954123" "bookwormadv|BA|#45801e" "ningpo|NP|#1f845f"
  "pixelus|PX|#952395" "alchemy|AL|#95236b" "wordharmony|WH|#232795" "cosmicbugs|CB|#412395" "waterbugs|WB|#236f95"
  "rocketmania|RM|#952723" "dynomite|DY|#78781c" "bookwormadv2|BA2|#637c1d")
get_filename_component(BYOPOP_WEB_INDEX "${CMAKE_CURRENT_LIST_DIR}/../web/index.html" ABSOLUTE)
# Whether a game's page asks the player to agree to its terms (the terms card and its checkbox) before the game's
# files are imported or loaded. OFF for now (the user, 2026-10-26), and the pages must not be deployed until it is
# ON again (docs/WEB_PLAYER.md, "The terms"). The copyright page shows the terms either way. A plain variable, not
# a cache option, so that changing it here reaches every build folder.
set(BYOPOP_WEB_TERMS_GATE OFF)
if(NOT BYOPOP_WEB_TERMS_GATE)
  message(STATUS "byo-pop: the game pages show no terms card (BYOPOP_WEB_TERMS_GATE): do not deploy them")
endif()

# byopop_web_shell(<var> <target> <page folder> TITLE <name> COPYRIGHT <notice> [LICENSES <name>...]): the game's
# page template: web/shell.html with the game's page.js, page.css and page.html (from <page folder>) at its
# byopop:game-js, byopop:game-css and byopop:game-panels markers, its <name> for @BYOPOP_TITLE@, and the shared
# footer (web/footer.html, its style web/footer.css before </head>) at its "<!-- byopop:footer -->" line. The footer links to the game's copyright page, <target>-copyright.html next to the
# game, written here from web/copyright.html: the game's <name> and <notice> (plain text: the copyright and
# trademark lines of the readme that comes with the game), what the player is, and the license of every library
# in web/licenses/ that the page is built from. LICENSES adds a game's own libraries (BookWorm: libopenmpt).
# Sets <var> to the generated template in the build folder, for --shell-file. Both files are rewritten only when
# their text changes, and CMake runs again when one of their inputs changes. byopop_web_game() calls it.
set(BYOPOP_WEB_LICENSES popcap-framework sdl2 stb libxmp-lite tremor minimp3)
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
  foreach(name ${BYOPOP_WEB_SHARED_JS})
    configure_file(${web}/${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name} COPYONLY)
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
  if(BYOPOP_WEB_TERMS_GATE)
    string(REPLACE "@BYOPOP_TERMS_GATE@" "true" page "${page}")
  else()
    string(REPLACE "@BYOPOP_TERMS_GATE@" "false" page "${page}")
  endif()
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
  # the listing page (byopop_web_index() below) reads the game's name from here, and its id and persist folder from the
  # page's settings (games/<game>/web/page.js, "id" and "persist"): the folder is the name of the game's browser
  # database, where the listing page looks for game files that were imported before the page kept a record of them
  file(READ ${arg_PAGE}/page.js page_js)
  foreach(field id persist)
    string(REGEX MATCH "\n[ \t]+${field}:[ \t]*\"([^\"]+)\"" ignored "${page_js}")
    if(NOT CMAKE_MATCH_1)
      message(FATAL_ERROR "byopop_web_game(${target}): ${arg_PAGE}/page.js has no ${field}: \"...\" in its PAGE")
    endif()
    set_property(GLOBAL PROPERTY BYOPOP_WEB_${field}_${target} "${CMAKE_MATCH_1}")
  endforeach()
  set_property(GLOBAL PROPERTY BYOPOP_WEB_TITLE_${target} "${arg_TITLE}")
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

# byopop_web_index(): writes the listing page, build/web/index.html, from web/index.html: for each target in
# BYOPOP_WEB_FINISHED, a tile linking to <target>.html, with the TITLE the game gave byopop_web_game() and its badge
# (BYOPOP_WEB_BADGES). It runs when the top-level CMakeLists.txt has added every game (cmake_language(DEFER), below).
# A game whose BUILD_<TARGET> option is OFF is left out; any other name in the list that has no web page is an error.
function(byopop_web_index)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BYOPOP_WEB_INDEX})
  file(READ ${BYOPOP_WEB_INDEX} page)
  set(marker "<!-- byopop:games -->")
  string(FIND "${page}" "${marker}" first)
  string(FIND "${page}" "${marker}" last REVERSE)
  if(first EQUAL -1 OR NOT first EQUAL last)
    message(FATAL_ERROR "${BYOPOP_WEB_INDEX} needs one \"${marker}\"")
  endif()
  set(tiles "")
  set(count 0)
  foreach(target ${BYOPOP_WEB_FINISHED})
    get_property(has_title GLOBAL PROPERTY BYOPOP_WEB_TITLE_${target} SET)
    if(NOT has_title)
      string(TOUPPER ${target} upper)
      if(DEFINED BUILD_${upper} AND NOT BUILD_${upper})
        message(STATUS "byo-pop: ${target} is off (BUILD_${upper}), so the listing page leaves it out")
        continue()
      endif()
      message(FATAL_ERROR "BYOPOP_WEB_FINISHED (cmake/ByopopWeb.cmake) names ${target}, which has no web page: "
        "no byopop_web_game(${target} ...) was configured")
    endif()
    get_property(title GLOBAL PROPERTY BYOPOP_WEB_TITLE_${target})
    set(initials "")
    set(colour "#6b3fd1")
    set(found OFF)
    foreach(badge ${BYOPOP_WEB_BADGES})
      string(REPLACE "|" ";" fields "${badge}")
      list(GET fields 0 badge_target)
      if(badge_target STREQUAL target)
        list(GET fields 1 initials)
        list(GET fields 2 colour)
        set(found ON)
      endif()
    endforeach()
    if(NOT found)
      # no row in the table: the first letters of the TITLE's words, without "Deluxe"
      string(REPLACE " " ";" words "${title}")
      list(REMOVE_ITEM words Deluxe)
      foreach(word ${words})
        string(SUBSTRING "${word}" 0 1 letter)
        string(TOUPPER "${letter}" letter)
        string(APPEND initials "${letter}")
      endforeach()
      message(STATUS "byo-pop: ${target} has no badge in BYOPOP_WEB_BADGES (cmake/ByopopWeb.cmake): using \"${initials}\"")
    endif()
    foreach(field title initials)
      string(REPLACE "&" "&amp;" ${field} "${${field}}")
      string(REPLACE "<" "&lt;" ${field} "${${field}}")
      string(REPLACE ">" "&gt;" ${field} "${${field}}")
    endforeach()
    # data-game and data-persist: the page's id for the game (the record of its files in localStorage, web/settings.js) and
    # its browser database (the persist folder), for the label under the name and its "?" (web/index.html)
    get_property(game_id GLOBAL PROPERTY BYOPOP_WEB_id_${target})
    get_property(game_persist GLOBAL PROPERTY BYOPOP_WEB_persist_${target})
    # where the game can still be bought (cmake/ByopopStores.cmake, the data of the game pages' buy note): a link for each
    # store, in the card but beside the game's own link, since a link cannot hold a link; none for a game with no store
    # (hidden: the listing page shows them for a game whose files were never loaded in this browser, web/index.html)
    set(buy_html "")
    if(DEFINED BYOPOP_BUY_${target})
      set(buy ${BYOPOP_BUY_${target}})
      list(POP_FRONT buy checked)
      while(buy)
        list(POP_FRONT buy store url)
        string(REPLACE "&" "&amp;" url "${url}")
        string(REPLACE "\"" "&quot;" url "${url}")
        foreach(field store)
          string(REPLACE "&" "&amp;" ${field} "${${field}}")
          string(REPLACE "<" "&lt;" ${field} "${${field}}")
          string(REPLACE ">" "&gt;" ${field} "${${field}}")
        endforeach()
        # "Buy on Steam", "Buy on the EA app": a store named as an app takes "the"
        set(on "${store}")
        if(store MATCHES " app$")
          set(on "the ${store}")
        endif()
        string(APPEND buy_html "<a href=\"${url}\" rel=\"noopener\" aria-label=\"Buy ${title} on ${on}\">Buy on ${on}</a>")
      endwhile()
      if(NOT buy_html STREQUAL "")
        set(buy_html "<div class=\"buyrow\" hidden>${buy_html}</div>")
      endif()
    endif()
    string(CONCAT tile "    <li class=\"game\" style=\"--tile: ${colour}\"><a class=\"tile\" href=\"${target}.html\" "
      "data-game=\"${game_id}\" data-persist=\"${game_persist}\"><span class=\"badge\" aria-hidden=\"true\">${initials}</span>"
      "<span class=\"name\">${title}</span></a><div class=\"ownrow\"><span class=\"own\"></span></div>${buy_html}</li>")
    list(APPEND tiles "${tile}")
    math(EXPR count "${count} + 1")
  endforeach()
  # (one line for each tile; a title with a semicolon would break the list, and none has one)
  list(JOIN tiles "\n" tiles)
  string(REPLACE "${marker}" "${tiles}" page "${page}")
  # (the logo and icons are copied by byopop_web_shell(); this keeps the page whole if no game is listed)
  foreach(name ${BYOPOP_WEB_BRAND})
    configure_file(${BYOPOP_WEB_BRAND_DIR}/${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name} COPYONLY)
  endforeach()
  foreach(name ${BYOPOP_WEB_SHARED_JS} ${BYOPOP_WEB_INDEX_JS})
    configure_file(${BYOPOP_WEB_BRAND_DIR}/../${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name} COPYONLY)
  endforeach()
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html "${page}")
  message(STATUS "byo-pop: the listing page shows ${count} games")
endfunction()
if(EMSCRIPTEN)
  # (a deferred call runs at the end of this directory, the top level, after every game's byopop_web_game())
  get_filename_component(BYOPOP_WEB_BRAND_DIR "${CMAKE_CURRENT_LIST_DIR}/../web/brand" ABSOLUTE)
  cmake_language(DEFER CALL byopop_web_index)
endif()
