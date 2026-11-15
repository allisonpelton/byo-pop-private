# cmake -DSHELL=<game>-shell.html -DPANEL=<web/modpanel.js> -DNAME=<target> -DOUT=<target>.html
#       [-DNODE=<node> -DMINIFIER=<html-minifier-terser's cli.js>] -P ByopopWebPage.cmake
#
# Writes a game's page apart from its link (byopop_web_game() in ByopopWeb.cmake), so that an edit to the page or to
# web/modpanel.js rebuilds every page in seconds and relinks no game. The game's template (byopop_web_shell()) has its
# {{{ SCRIPT }}} line replaced, as emcc's --shell-file does, by:
#   - web/modpanel.js inline. It only defines Module.byopop on the page's Module, which the page and the game's script
#     read later, so it runs first, as on the listing page (web/index.html), which loads it without a game;
#   - the game's script, as emcc writes it: <script async type="text/javascript" src="<target>.js">.
# With MINIFIER the page is then minified as emcc minifies a page it writes at -O1 and over without -g
# (Emscripten's tools/link.py, minify_html), with the same html-minifier-terser options.
foreach(arg SHELL PANEL NAME OUT)
  if(NOT DEFINED ${arg})
    message(FATAL_ERROR "ByopopWebPage.cmake: -D${arg}=... is missing")
  endif()
endforeach()

file(READ "${SHELL}" page)
file(READ "${PANEL}" panel)
set(marker "{{{ SCRIPT }}}")
string(FIND "${page}" "${marker}" first)
string(FIND "${page}" "${marker}" last REVERSE)
if(first EQUAL -1 OR NOT first EQUAL last)
  message(FATAL_ERROR "${SHELL} needs one \"${marker}\"")
endif()
# an inline script ends at the first "</script" in it
string(FIND "${panel}" "</script" closing)
if(NOT closing EQUAL -1)
  message(FATAL_ERROR "${PANEL} holds \"</script\", which would end its inline script on the page")
endif()
string(REPLACE "${marker}" "<script>
${panel}</script>
<script async type=\"text/javascript\" src=\"${NAME}.js\"></script>" page "${page}")

if(NOT MINIFIER)
  file(WRITE "${OUT}" "${page}")
  return()
endif()
set(raw "${OUT}.raw")
set(minified "${OUT}.min")
file(WRITE "${raw}" "${page}")
execute_process(COMMAND "${NODE}" "${MINIFIER}" "${raw}" -o "${minified}"
    --collapse-whitespace --remove-comments --remove-tag-whitespace --sort-attributes --sort-class-name
    --decode-entities --collapse-boolean-attributes --remove-attribute-quotes --remove-redundant-attributes
    --remove-script-type-attributes --remove-style-link-type-attributes --use-short-doctype
    --minify-css true --minify-js true --ignore-custom-fragments "[]"
  RESULT_VARIABLE result)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "ByopopWebPage.cmake: html-minifier-terser failed on ${raw} (${result})")
endif()
file(REMOVE "${raw}")
file(RENAME "${minified}" "${OUT}")
