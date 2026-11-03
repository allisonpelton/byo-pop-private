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
# wasm-opt, which emcc runs when it links a game, uses every core by default, and a build is not under rundesk's low
# priority: one link took the user's machine to 100% (2026-11-01). BINARYEN_CORES caps its threads, so a link takes at
# most 4 of the machine's 20.
if(EMSCRIPTEN)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E env BINARYEN_CORES=4")
endif()
set(BYOPOP_WEB_EXPORTED_FUNCTIONS _main _byopop_userdata_restore _byopop_mods_open _byopop_mods_state
  _byopop_mods_edit _byopop_mods_rescan _byopop_menu _byopop_restart _byopop_cursor _byopop_cursor_copy
  _byopop_cursor_set_copy
  _byopop_audio_mute _byopop_audio_muted _byopop_site_apply _byopop_site_windowed _byopop_mixer_gain)
set(BYOPOP_WEB_RUNTIME_METHODS FS callMain ccall)
get_filename_component(BYOPOP_WEB_SHELL "${CMAKE_CURRENT_LIST_DIR}/../web/shell.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_POST_JS "${CMAKE_CURRENT_LIST_DIR}/../web/modpanel.js" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER "${CMAKE_CURRENT_LIST_DIR}/../web/footer.html" ABSOLUTE)
get_filename_component(BYOPOP_WEB_FOOTER_CSS "${CMAKE_CURRENT_LIST_DIR}/../web/footer.css" ABSOLUTE)
# where each game can be bought (BYOPOP_BUY_<target>), shown under its terms
include(${CMAKE_CURRENT_LIST_DIR}/ByopopStores.cmake)
# the site's logo and icons (web/brand; docs/WEB_PLAYER.md), copied next to the pages, which link them
set(BYOPOP_WEB_BRAND favicon.svg favicon.ico apple-touch-icon.png logo.svg logo-dark.svg cap.svg opener.svg)
# web/settings.js, the site's settings and the record of which games have their files (docs/WEB_PLAYER.md, "Site
# settings", "The listing page"): one file that the listing page and every game page load. web/profiles.js keeps the site's
# players and what the games report of theirs ("Player profiles"); the listing page and the game pages load it too.
# web/exeicon.js reads a game's icon out of its exe (the game pages and the listing page load it). web/gameimport.js holds the
# rules for importing a game's files, which the game pages' import card and the listing page's adding of games share
# (docs/WEB_PLAYER.md, "Adding games from the listing page"). web/chat.js is the chat beside the game (docs/CHAT_DESIGN.md). All
# five are copied next to the pages. chat-games.js, the chat's table of the site's games (the id
# of each game's page, its page's file name, its badge's initials and colour and its title), is written beside them by
# byopop_web_index() from the same BYOPOP_WEB_BADGES and the same titles as the listing page's tiles, so the chat keeps no list of
# games of its own.
set(BYOPOP_WEB_SHARED_JS theme.js cursors.js capanim.js settings.js profiles.js exeicon.js gameimport.js chat.js)
# the listing page's own scripts: the Players section (web/players.js), the save backup (web/saves.js) and the import of
# saves from an installed game (web/saveimport.js, with the table platform/saveimport.json as saveimport-table.js), and the
# adding of games' files (web/addgames.js, with web/modpanel.js for its zip and pak readers, and import-games.js, each listed
# game's page.js, which byopop_web_index() writes), copied next to index.html only
set(BYOPOP_WEB_INDEX_JS players.js saves.js saveimport.js modpanel.js addgames.js)
# The listing page, build/web/index.html (made from web/index.html by byopop_web_index() at the end of this file): one
# tile for each game in BYOPOP_WEB_FINISHED, in alphabetical order of TITLE (not this list's order; the page can sort by
# release date instead), with a badge and the year of its release. The list is the coordinator's: the coordinator adds a
# game's target at that game's final merge, and its release date (BYOPOP_WEB_RELEASED, below) at the same merge. A game
# with a web page that is not in the list is not shown (fwtest and smoke are test pages and never are); a name in the
# list with no web page is a configure error.
set(BYOPOP_WEB_FINISHED bejeweled bigmoney bookworm noahsark sevenseas atomica mummymaze tiptop bookwormadv ningpo
  pixelus alchemy wordharmony cosmicbugs waterbugs rocketmania dynomite bookwormadv2 mahjongjapan mahjongchina
  bonnies)
# The games still to come, <target>|<title>, each shown on the listing page as a "Coming soon" tile among the finished ones
# (the user, 2026-11-02: "Add tiles for the upcoming games to the site and have them display disabled and say coming soon"):
# the badge, the title and the year, sorted with the rest, not a link, and where to buy the game, so that a visitor can get
# it ready. The target is the one the game's port uses (or will use), the title the one its Windows release has. The list is
# the coordinator's: at a game's final merge its target moves from here to BYOPOP_WEB_FINISHED (its title then comes from
# its page's TITLE), and its badge, release date and BYOPOP_BUY entry stay. Every game here needs a row in
# BYOPOP_WEB_BADGES and BYOPOP_WEB_RELEASED and a BYOPOP_BUY entry (cmake/ByopopStores.cmake); an empty target or title, a
# target listed twice or in both lists, or a game here without a well-formed BYOPOP_BUY entry, is a configure error. Bejeweled Twist has a page already, not yet merged; the rest
# are "The ports still to make" in docs/COORDINATION.md. Zuma Deluxe, Insaniquarium and Plants vs.
# Zombies are left out: nobody ports them (the user, 2026-10-26).
set(BYOPOP_WEB_UPCOMING
  "bejeweledtwist|Bejeweled Twist" "venice|Venice Deluxe"
  "heavyweapon|Heavy Weapon Deluxe" "hammerheads|Hammer Heads Deluxe" "bejeweled2|Bejeweled 2 Deluxe"
  "astropop|AstroPop Deluxe" "typershark|Typer Shark! Deluxe" "pizzafrenzy|Pizza Frenzy Deluxe" "igglepop|Iggle Pop Deluxe"
  "feedingfrenzy|Feeding Frenzy Deluxe" "chuzzle|Chuzzle Deluxe" "feedingfrenzy2|Feeding Frenzy 2 Deluxe"
  "peggle|Peggle Deluxe" "pegglenights|Peggle Nights" "bejeweled3|Bejeweled 3" "bananabugs|Banana Bugs"
  "gyromancer|Gyromancer" "zumasrevenge|Zuma's Revenge!" "talismania|Talismania Deluxe")
# A game's badge on the listing page, <target>|<initials>|<colour>: the initials in white on a rounded square of the
# colour. Original to byo-pop, never the games' art. The colours keep white text above a 4.5 contrast ratio (the chat's game
# tags, "[BW]" before a username, use the same row and colour: tools/chattest.js checks the ratio and that the initials are
# unique, 1 to 4 letters or digits). A game without a row here gets the initials of its TITLE and the accent colour.
set(BYOPOP_WEB_BADGES
  "bejeweled|BJ|#95233d" "bigmoney|BM|#1f8438" "bookworm|BW|#955c23" "noahsark|NA|#234995" "sevenseas|7S|#1e8080"
  "atomica|AT|#602395" "mummymaze|MM|#8c7021" "tiptop|TT|#954123" "bookwormadv|BA|#45801e" "ningpo|NP|#1f845f"
  "pixelus|PX|#952395" "alchemy|AL|#95236b" "wordharmony|WH|#232795" "cosmicbugs|CB|#412395" "waterbugs|WB|#236f95"
  "rocketmania|RM|#952723" "dynomite|DY|#78781c" "bookwormadv2|BA2|#637c1d" "mahjongjapan|MJ|#4b5d73"
  "mahjongchina|MC|#655873" "bonnies|BB|#8a3a4f"
  # the games still to come, in BYOPOP_WEB_UPCOMING
  "bejeweledtwist|BT|#7a2395" "venice|VE|#1f5f8c" "heavyweapon|HW|#5c6420" "hammerheads|HH|#7a5a1e"
  "bejeweled2|BJ2|#a02a2a" "astropop|AP|#2a3fa0" "typershark|TS|#1d6a7a" "pizzafrenzy|PF|#a33a1a" "igglepop|IP|#8a238a"
  "feedingfrenzy|FF|#1d6e8e" "chuzzle|CH|#9a3c6e" "feedingfrenzy2|FF2|#16707a" "peggle|PG|#2f6f2f" "pegglenights|PN|#3a2f8a"
  "bejeweled3|BJ3|#6e2370" "bananabugs|BN|#7a6a12" "gyromancer|GY|#4a4a8e" "zumasrevenge|ZR|#2a7a46" "talismania|TA|#8a4a1e")
# When each finished or upcoming game was first released, <target>|<date> with the date as YYYY, YYYY-MM or YYYY-MM-DD, as exact as the
# sources allow (the user, 2026-10-27: the listing page can sort by release date, docs/WEB_PLAYER.md, "The listing page"). It is
# the first release of the Windows edition that the site plays, the Deluxe download where there is one, not a web, Java or
# phone version; a game that came out first under another publisher is dated by that first release. The tile shows the
# year; the page sorts by the whole date, and a year alone sorts after the full dates of its year. Each row has its
# source in the comment above it. A finished game without a row is a configure warning (it shows no year and sorts last);
# a row that is not a date is a configure error. Researched 2026-10-27 from Wikipedia and VGTimes (a game database); MobyGames,
# Giant Bomb, VideoGameGeek and PCGamingWiki refused automated requests and were not read directly, and the Internet
# Archive's copies of popcap.com could not be fetched.
set(BYOPOP_WEB_RELEASED
  # Wikipedia, "Bejeweled (video game)": the retail Bejeweled Deluxe for Windows, May 30, 2001. High.
  "bejeweled|2001-05-30"
  # The user, 2026-10-30: June 4, 2002. (Other databases disagree: Dec 23, 2002 on LaunchBox, the Mac date elsewhere, April 2003
  # on VGTimes for the USA PC edition, Aug 30, 2006 for the Steam release; Wikipedia's list of PopCap games has 2002.)
  "bigmoney|2002-06-04"
  # Wikipedia, "Bookworm (video game)": February 25, 2003. VGTimes has the USA PC edition on Feb 27, 2003. High.
  "bookworm|2003-02-25"
  # VGTimes (https://vgtimes.com/games/noahs-ark-deluxe/release-dates/): PC, October 15, 2002; Wikipedia's list and Lutris say 2002. Medium.
  "noahsark|2002-10-15"
  # VGTimes (https://vgtimes.com/games/seven-seas-deluxe/release-dates/): PC, October 16, 2001; Wikipedia's list says 2001. A
  # database that gave Oct 10, 2000 is out of line with both. Medium.
  "sevenseas|2001-10-16"
  # VGTimes (https://vgtimes.com/games/atomica-deluxe/release-dates/): PC, August 14, 2002; Wikipedia's list says 2002. Medium.
  "atomica|2002-08-14"
  # VGTimes (https://vgtimes.com/games/mummy-maze-deluxe/release-dates/): PC, June 21, 2002; Giant Bomb has the same day; Wikipedia's list says 2002. Medium.
  "mummymaze|2002-06-21"
  # VGTimes (https://vgtimes.com/games/tiptop/release-dates/): PC, December 12, 2002; Wikipedia's list says 2002. Medium.
  "tiptop|2002-12-12"
  # Wikipedia, "Bookworm Adventures": Windows, November 28, 2006 (as packaged software; Steam followed on Jan 2, 2007). High.
  "bookwormadv|2006-11-28"
  # The user, 2026-10-30: July 26, 2002. (Wikipedia's list and MyAbandonware have only the year: Windows 2002, Mac 2003.)
  "ningpo|2002-07-26"
  # VGTimes (https://vgtimes.com/games/pixelus-deluxe/release-dates/): PC, July 15, 2004; Wikipedia's list says 2004. Medium.
  "pixelus|2004-07-15"
  # VGTimes (https://vgtimes.com/games/alchemy-deluxe/release-dates/): PC (USA), August 29, 2001; Giant Bomb has the same day; Wikipedia's list says 2001. High.
  "alchemy|2001-08-29"
  # VGTimes (https://vgtimes.com/games/word-harmony/release-dates/): PC, December 27, 2003, and LaunchBox, MyAbandonware and
  # Wikipedia's list say 2003. Sprout Games made it; PopCap bought Sprout in 2005 and a PopCap edition (the Deluxe, the copyright
  # notice of games/wordharmony) followed in 2006, so the edition the site plays is later than this date. Medium.
  "wordharmony|2003-12-27"
  # The user, 2026-10-30: November 11, 2004, the first release (a fan wiki's date). Wikipedia's list of PopCap games has 2004, as
  # made by Retro64 and published by PopCap from 2007; VGTimes has PC, February 9, 2005. The PopCap edition is from 2007.
  "cosmicbugs|2004-11-11"
  # VGTimes (https://vgtimes.com/games/water-bugs/release-dates/): PC, October 5, 2005 (another database says Oct 11); Wikipedia's
  # list says 2005. As for Cosmic Bugs, the PopCap edition is later than the first release. Medium.
  "waterbugs|2005-10-05"
  # Wikipedia, "Rocket Mania!", and the user (2026-10-30): August 15, 2003. (VGTimes has PC (USA) on Aug 3, 2003; VideoGameGeek
  # has Apr 15, 2003 for the download, seen only in a search result; the 2006 dates are the Steam release.)
  "rocketmania|2003-08-15"
  # VideoGameGeek (seen only in a search result) and a Wikipedia mirror: January 25, 2002; VGTimes has the USA PC edition on
  # Feb 25, 2002; Wikipedia's list says 2002 (the 2006 dates are the Steam release). Medium to low.
  "dynomite|2002-01-25"
  # Wikipedia, "Bookworm Adventures": the sequel, July 30, 2009 (PopCap's announcement, as Gamezebo reported it, gave July 30 as
  # well). Other databases have June 3 and July 28; they are outliers. High.
  "bookwormadv2|2009-07-30"
  # VGTimes (https://vgtimes.com/games/mahjong-escape-ancient-japan/), MobyGames and VideoGameGeek: Windows, November 13, 2006;
  # Metacritic and GameFAQs have December 7, 2006, and LaunchBox's March 28, 2008 is a later Deluxe edition. Medium.
  "mahjongjapan|2006-11-13"
  # MobyGames: 2006. The days disagree: Metacritic has April 20, 2006, VGTimes and VideoGameGeek March 9, 2007, and later dates
  # are regional editions; the exe the site plays was built in August 2006 (docs/POPCAP_GAMES_SURVEY.md). Low, so the year only.
  "mahjongchina|2006"
  # VGTimes (https://vgtimes.com/games/bonnies-bookstore/release-dates/): PC, December 6, 2005 (Mac March 23, 2006); Wikipedia's
  # list has it among 2005's games. Medium.
  "bonnies|2005-12-06"
  # The games still to come (BYOPOP_WEB_UPCOMING), researched 2026-11-02 the same way: Wikipedia (each game's article, through
  # its API, and its "List of PopCap Games games"), VGTimes' release-date pages (vgtimes.com/games/<name>/release-dates/),
  # Steam's app data (its release dates are the Steam releases, 2006 and later, so they only bound a date) and Shockwave's
  # search API (its first release dates are when Shockwave added a game, weeks to years late, and are not used).
  # Wikipedia, "Bejeweled Twist": Windows, October 27, 2008, citing PopCap's press release. High.
  "bejeweledtwist|2008-10-27"
  # Wikipedia, "Venice (video game)": June 22, 2007, citing GamesIndustry ("PopCap Launches Venice For PC"); Steam's page has
  # June 25, 2007. High.
  "venice|2007-06-22"
  # Wikipedia, "Heavy Weapon": March 1, 2005, citing PopCap's "What's New" page as archived on March 6, 2005; VGTimes
  # (https://vgtimes.com/games/heavy-weapon-deluxe/release-dates/) has PC, February 24, 2005. Medium.
  "heavyweapon|2005-03-01"
  # Wikipedia, "Hammer Heads", and VGTimes (https://vgtimes.com/games/hammer-heads-deluxe/release-dates/): May 23, 2006. High.
  "hammerheads|2006-05-23"
  # Wikipedia, "Bejeweled 2": Windows, November 16, 2004 (the console and phone editions came later). High.
  "bejeweled2|2004-11-16"
  # Wikipedia, "AstroPop": April 6, 2004, the day its exe was built (docs/POPCAP_GAMES_SURVEY.md); VGTimes
  # (https://vgtimes.com/games/astropop/release-dates/) has PC, March 30, 2004. Medium to high.
  "astropop|2004-04-06"
  # VGTimes (https://vgtimes.com/games/typer-shark-deluxe/release-dates/): PC, June 10, 2003; Wikipedia's list has 2003. The exe
  # of Typer Shark Deluxe 1.02 is from April 2004, a later version. Medium.
  "typershark|2003-06-10"
  # Wikipedia, "Pizza Frenzy": February 2, 2005. Medium.
  "pizzafrenzy|2005-02-02"
  # Wikipedia's list: Windows, 2004 (Sprout Games); VGTimes has only the Mac edition (January 30, 2006), and Shockwave's March
  # 2005 is when it added the game. Low, so the year only.
  "igglepop|2004"
  # Wikipedia, "Feeding Frenzy (video game)": March 17, 2004, citing GameHouse's "Recently Released" list as archived in May
  # 2004 (Sprout Games). Medium.
  "feedingfrenzy|2004-03-17"
  # Wikipedia, "Chuzzle", and VGTimes (https://vgtimes.com/games/chuzzle-deluxe/release-dates/): PC, May 12, 2005. High.
  "chuzzle|2005-05-12"
  # Wikipedia, "Feeding Frenzy 2": PC, March 15, 2006, citing GamesIndustry ("PopCap Games Launches Feeding Frenzy 2"). High.
  "feedingfrenzy2|2006-03-15"
  # Wikipedia, "Peggle": Windows, February 27, 2007 (Steam's March 6, 2007 is the Steam release). High.
  "peggle|2007-02-27"
  # Wikipedia, "Peggle Nights": Windows, September 16, 2008 (Steam, October 15, 2008). High.
  "pegglenights|2008-09-16"
  # Wikipedia, "Bejeweled 3": Windows and OS X, December 7, 2010; Steam's page has the same day. High.
  "bejeweled3|2010-12-07"
  # Wikipedia's list: Windows, 2010; it has no article, VGTimes has no page for it, and Shockwave's August 3, 2010 may be when it
  # added the game. Low, so the year only.
  "bananabugs|2010"
  # Wikipedia, "Gyromancer", and Steam's page: November 18, 2009 (Square Enix with PopCap). High.
  "gyromancer|2009-11-18"
  # Wikipedia, "Zuma's Revenge!": September 15, 2009; Steam's page has the same day. High.
  "zumasrevenge|2009-09-15"
  # VGTimes (https://vgtimes.com/games/talismania-deluxe/release-dates/): PC, July 20, 2006; Wikipedia's list has 2006. Medium.
  "talismania|2006-07-20")
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

# The game's badge (BYOPOP_WEB_BADGES): its initials and colour into <initials_var> and <colour_var>. A game without a row
# gets the first letters of its TITLE's words, without "Deluxe", and the accent colour.
function(byopop_web_badge target title initials_var colour_var)
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
    string(REPLACE " " ";" words "${title}")
    list(REMOVE_ITEM words Deluxe)
    foreach(word ${words})
      string(SUBSTRING "${word}" 0 1 letter)
      string(TOUPPER "${letter}" letter)
      string(APPEND initials "${letter}")
    endforeach()
    message(STATUS "byo-pop: ${target} has no badge in BYOPOP_WEB_BADGES (cmake/ByopopWeb.cmake): using \"${initials}\"")
  endif()
  set(${initials_var} "${initials}" PARENT_SCOPE)
  set(${colour_var} "${colour}" PARENT_SCOPE)
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
  # (quoted, so that an empty page stays in the list and is caught)
  set(buy "${BYOPOP_BUY_${target}}")
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
      if(store STREQUAL "" OR url STREQUAL "")
        message(FATAL_ERROR "byopop_web_shell(${target}): BYOPOP_BUY_${target} has a store with an empty name or page")
      endif()
      string(REPLACE "&" "&amp;" url "${url}")
      string(REPLACE "\"" "&quot;" url "${url}")
      list(APPEND links "<a href=\"${url}\" target=\"_blank\" rel=\"noopener noreferrer\">${store}<span class=\"sr-only\"> (opens in a new tab)</span></a>")
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
  # the game's cap over its import card (web/capanim.js): its badge, as on the listing page
  byopop_web_badge(${target} "${arg_TITLE}" cap_initials cap_colour)
  string(REPLACE "@BYOPOP_CAP_INITIALS@" "${cap_initials}" page "${page}")
  string(REPLACE "@BYOPOP_CAP_COLOUR@" "${cap_colour}" page "${page}")
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
  foreach(field id persist lock)
    string(REGEX MATCH "\n[ \t]+${field}:[ \t]*\"([^\"]+)\"" ignored "${page_js}")
    if(NOT CMAKE_MATCH_1)
      message(FATAL_ERROR "byopop_web_game(${target}): ${arg_PAGE}/page.js has no ${field}: \"...\" in its PAGE")
    endif()
    set_property(GLOBAL PROPERTY BYOPOP_WEB_${field}_${target} "${CMAKE_MATCH_1}")
    # no two games share an id, a persist folder or a lock: importing one game's files would replace the other's (both
    # Mahjong Escapes had /mjpersist until 2026-11-01)
    string(MAKE_C_IDENTIFIER "${CMAKE_MATCH_1}" key)
    get_property(owner GLOBAL PROPERTY BYOPOP_WEB_owner_${field}_${key})
    if(owner AND NOT owner STREQUAL target)
      message(FATAL_ERROR "byopop_web_game(${target}): its ${field} \"${CMAKE_MATCH_1}\" (${arg_PAGE}/page.js) is ${owner}'s too")
    endif()
    set_property(GLOBAL PROPERTY BYOPOP_WEB_owner_${field}_${key} ${target})
  endforeach()
  # whether the game keeps replays (a folder of its own beside the user dir, which the save backup takes too): "replays: true"
  string(REGEX MATCH "\n[ \t]+replays:[ \t]*true" has_replays "${page_js}")
  if(has_replays)
    set_property(GLOBAL PROPERTY BYOPOP_WEB_replays_${target} "1")
  else()
    set_property(GLOBAL PROPERTY BYOPOP_WEB_replays_${target} "")
  endif()
  set_property(GLOBAL PROPERTY BYOPOP_WEB_TITLE_${target} "${arg_TITLE}")
  set_property(GLOBAL PROPERTY BYOPOP_WEB_pagejs_${target} "${arg_PAGE}/page.js")
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
# (BYOPOP_WEB_BADGES), the year of its release and, on the card, its whole release date and title as data-released and
# data-title, which the page's "Sort by" reads (BYOPOP_WEB_RELEASED; a game without a row gets a warning, no year and an
# empty data-released). It runs when the top-level CMakeLists.txt has added every game (cmake_language(DEFER), below).
# A game whose BUILD_<TARGET> option is OFF is left out; any other name in the list that has no web page is an error.
# Among them, sorted with them by title, a "Coming soon" tile for each game in BYOPOP_WEB_UPCOMING: its badge, title and
# year in a group named "<title>, <year>, coming soon", not a link, with its "where to buy" links always shown.
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
  set(chat_games "")
  set(chat_separator "")
  set(import_games "")
  set(count 0)
  set(soon_count 0)
  # the tiles go in alphabetical order of the games' titles, ignoring case (the user, 2026-10-27) and "Deluxe", so
  # that Bejeweled Deluxe comes before Bejeweled 2 (the user, 2026-11-02); a tab ends the key, so that a title sorts
  # before the longer ones it starts. A game with no web page has no title, sorts first and is dealt with below
  set(ordered "")
  foreach(target ${BYOPOP_WEB_FINISHED})
    get_property(title GLOBAL PROPERTY BYOPOP_WEB_TITLE_${target})
    string(TOLOWER "${title}" key)
    string(REPLACE " deluxe" "" key "${key}")
    list(APPEND ordered "${key}\t${target}")
  endforeach()
  # the games still to come, by the titles in the list (a game with a page not yet merged still shows as coming)
  set(upcoming "")
  foreach(row ${BYOPOP_WEB_UPCOMING})
    # both fields non-empty (a list split on "|" would count an empty title as a field)
    if(NOT row MATCHES "^([^|]+)[|]([^|]+)$")
      message(FATAL_ERROR "BYOPOP_WEB_UPCOMING (cmake/ByopopWeb.cmake) has the row \"${row}\", which is not <target>|<title>")
    endif()
    set(soon_target "${CMAKE_MATCH_1}")
    set(soon_title "${CMAKE_MATCH_2}")
    if(soon_target IN_LIST upcoming)
      message(FATAL_ERROR "BYOPOP_WEB_UPCOMING (cmake/ByopopWeb.cmake) lists ${soon_target} twice")
    endif()
    if(soon_target IN_LIST BYOPOP_WEB_FINISHED)
      message(FATAL_ERROR "${soon_target} is in both BYOPOP_WEB_FINISHED and BYOPOP_WEB_UPCOMING (cmake/ByopopWeb.cmake): "
        "a game's final merge moves it from the second to the first")
    endif()
    if(NOT DEFINED BYOPOP_BUY_${soon_target})
      message(FATAL_ERROR "BYOPOP_WEB_UPCOMING (cmake/ByopopWeb.cmake) names ${soon_target}, which has no BYOPOP_BUY_${soon_target} "
        "in cmake/ByopopStores.cmake: its tile shows where to buy it")
    endif()
    # the same shape byopop_web_shell asks of a finished game's entry: the date checked, then a store and its page each
    # (quoted, so that an empty page stays in the list and is caught)
    set(soon_buy "${BYOPOP_BUY_${soon_target}}")
    list(POP_FRONT soon_buy soon_checked)
    if(NOT soon_checked MATCHES "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$")
      message(FATAL_ERROR "BYOPOP_BUY_${soon_target} (cmake/ByopopStores.cmake) starts with \"${soon_checked}\", not a YYYY-MM-DD date")
    endif()
    list(LENGTH soon_buy soon_buy_count)
    math(EXPR odd "${soon_buy_count} % 2")
    if(odd)
      message(FATAL_ERROR "BYOPOP_BUY_${soon_target} (cmake/ByopopStores.cmake) needs a store's name and its page for each store")
    endif()
    while(soon_buy_count GREATER 0)
      list(POP_FRONT soon_buy soon_store soon_url)
      if(soon_store STREQUAL "" OR soon_url STREQUAL "")
        message(FATAL_ERROR "BYOPOP_BUY_${soon_target} (cmake/ByopopStores.cmake) has a store with an empty name or page")
      endif()
      math(EXPR soon_buy_count "${soon_buy_count} - 2")
    endwhile()
    set(soon_title_${soon_target} "${soon_title}")
    list(APPEND upcoming ${soon_target})
    string(TOLOWER "${soon_title}" key)
    string(REPLACE " deluxe" "" key "${key}")
    list(APPEND ordered "${key}\t${soon_target}")
  endforeach()
  list(SORT ordered)
  list(TRANSFORM ordered REPLACE "^.*\t" "")
  foreach(target ${ordered})
    set(soon OFF)
    if(target IN_LIST upcoming)
      set(soon ON)
      set(title "${soon_title_${target}}")
    else()
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
    endif()
    # the title as plain text, for the chat's table (the page's own copy is HTML-escaped below): backslash and quote escaped
    set(chat_title "${title}")
    set(import_title "${title}")  # the raw title, before the HTML escaping below, for the listing's import rules
    string(REPLACE "\\" "\\\\" chat_title "${chat_title}")
    string(REPLACE "\"" "\\\"" chat_title "${chat_title}")
    byopop_web_badge(${target} "${title}" initials colour)
    set(chat_initials "${initials}")
    foreach(field title initials)
      string(REPLACE "&" "&amp;" ${field} "${${field}}")
      string(REPLACE "<" "&lt;" ${field} "${${field}}")
      string(REPLACE ">" "&gt;" ${field} "${${field}}")
    endforeach()
    # the game's first release (BYOPOP_WEB_RELEASED): the year goes on the tile, the date as given into data-released
    set(released "")
    foreach(row ${BYOPOP_WEB_RELEASED})
      string(REPLACE "|" ";" fields "${row}")
      list(LENGTH fields field_count)
      if(NOT field_count EQUAL 2)
        message(FATAL_ERROR "BYOPOP_WEB_RELEASED (cmake/ByopopWeb.cmake) has the row \"${row}\", which is not <target>|<date>")
      endif()
      list(GET fields 0 row_target)
      if(row_target STREQUAL target)
        list(GET fields 1 released)
      endif()
    endforeach()
    set(year "")
    if(released STREQUAL "")
      message(WARNING "byo-pop: ${target} has no release date in BYOPOP_WEB_RELEASED (cmake/ByopopWeb.cmake): the listing page "
        "shows no year for it and sorts it last")
    elseif(NOT released MATCHES "^[0-9][0-9][0-9][0-9](-(0[1-9]|1[0-2])(-(0[1-9]|[12][0-9]|3[01]))?)?$")
      message(FATAL_ERROR "BYOPOP_WEB_RELEASED (cmake/ByopopWeb.cmake) gives ${target} the date \"${released}\", "
        "which is not YYYY, YYYY-MM or YYYY-MM-DD")
    else()
      string(SUBSTRING "${released}" 0 4 year)
    endif()
    string(REPLACE "\"" "&quot;" title_attr "${title}")
    # data-game and data-persist: the page's id for the game (the record of its files in localStorage, web/settings.js) and
    # its browser database (the persist folder), for the label under the name and its "?" (web/index.html)
    get_property(game_id GLOBAL PROPERTY BYOPOP_WEB_id_${target})
    get_property(game_persist GLOBAL PROPERTY BYOPOP_WEB_persist_${target})
    # the chat's table: one entry for each finished game on the listing page, in the tiles' order (the game's id is the page's
    # PAGE.id); a game still to come has no page to send from
    if(NOT soon)
      string(APPEND chat_games "${chat_separator}  {\"id\": \"${game_id}\", \"page\": \"${target}.html\", \"tag\": \"${chat_initials}\", "
        "\"title\": \"${chat_title}\", \"colour\": \"${colour}\"}")
      set(chat_separator ",\n")
      # the game's import rules for a folder dropped on the listing page (web/gameimport.js): its page.js, run in a function of
      # its own that returns its PAGE (the file only declares things, and none of its functions is called)
      get_property(game_page_js GLOBAL PROPERTY BYOPOP_WEB_pagejs_${target})
      file(READ ${game_page_js} import_page_js)
      string(REPLACE "\\" "\\\\" import_title "${import_title}")
      string(REPLACE "\"" "\\\"" import_title "${import_title}")
      string(APPEND import_games "ByopopImport.add(\"${target}\", \"${import_title}\", function () {\n${import_page_js}\n  return PAGE;\n});\n")
    endif()
    # the game's Web Lock (its page holds it while it runs) and whether it keeps replays, for the save backup (web/saves.js)
    get_property(game_lock GLOBAL PROPERTY BYOPOP_WEB_lock_${target})
    get_property(game_replays GLOBAL PROPERTY BYOPOP_WEB_replays_${target})
    set(replays_attr "")
    if(game_replays)
      set(replays_attr " data-replays=\"1\"")
    endif()
    # where the game can still be bought (cmake/ByopopStores.cmake, the data of the game pages' buy note): a link for each
    # store, in the card but beside the game's own link, since a link cannot hold a link; none for a game with no store
    # (hidden: the listing page shows them for a game whose files were never loaded in this browser, web/index.html; always shown
    # for a game still to come, which has no files to load yet)
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
        string(APPEND buy_html "<a href=\"${url}\" rel=\"noopener\" aria-label=\"Buy on ${on}, ${title}\">Buy on ${on}</a>")
      endwhile()
      if(NOT buy_html STREQUAL "" AND soon)
        set(buy_html "<div class=\"buyrow\">${buy_html}</div>")
      elseif(NOT buy_html STREQUAL "")
        set(buy_html "<div class=\"buyrow\" hidden>${buy_html}</div>")
      endif()
    endif()
    # a game no store sells any more: the listing page hides it unless the player shows those games or its files are here
    set(unsold_attr "")
    if(buy_html STREQUAL "")
      set(unsold_attr " data-unsold=\"1\"")
    endif()
    # the year under the name (a space between the two spans, so that the link's name reads "Bejeweled Deluxe 2001" even
    # where the spans' text is joined), and the link's name with it, "Bejeweled Deluxe, 2001"; the page adds the files' state to it
    set(year_html "")
    set(label_attr "")
    if(NOT year STREQUAL "")
      set(year_html " <span class=\"year\">${year}</span>")
      set(label_attr " aria-label=\"${title_attr}, ${year}\"")
    endif()
    if(soon)
      # not a link: a group named with the state, "AstroPop Deluxe, 2004, coming soon", and the words on the card
      set(soon_label "${title_attr}")
      if(NOT year STREQUAL "")
        string(APPEND soon_label ", ${year}")
      endif()
      string(CONCAT tile "    <li class=\"game soon\" style=\"--tile: ${colour}\" data-released=\"${released}\" data-title=\"${title_attr}\"${unsold_attr}>"
        "<div class=\"tile\" role=\"group\" aria-label=\"${soon_label}, coming soon\">"
        "<span class=\"badge\" aria-hidden=\"true\">${initials}</span>"
        "<span class=\"name\">${title}</span>${year_html}</div><div class=\"ownrow\"><span class=\"own own-soon\" aria-hidden=\"true\">Coming soon</span></div>${buy_html}</li>")
      list(APPEND tiles "${tile}")
      math(EXPR soon_count "${soon_count} + 1")
      continue()
    endif()
    string(CONCAT tile "    <li class=\"game\" style=\"--tile: ${colour}\" data-released=\"${released}\" data-title=\"${title_attr}\"${unsold_attr}>"
      "<a class=\"tile\" href=\"${target}.html\" data-game=\"${game_id}\" data-persist=\"${game_persist}\" data-lock=\"${game_lock}\"${replays_attr}${label_attr}>"
      "<span class=\"badge\" aria-hidden=\"true\">${initials}</span>"
      "<span class=\"name\">${title}</span>${year_html}</a><div class=\"ownrow\"><span class=\"own\" aria-hidden=\"true\"></span></div>${buy_html}</li>")
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
  # the table of what is imported from an installed game (platform/saveimport.json, docs/WEB_PLAYER.md, "Importing saves from an
  # installed game"): the native games compile the same file in (framework/CMakeLists.txt)
  get_filename_component(saveimport_json "${BYOPOP_WEB_BRAND_DIR}/../../platform/saveimport.json" ABSOLUTE)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${saveimport_json})
  file(READ ${saveimport_json} saveimport_text)
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/saveimport-table.js "var ByopopSaveTable = ${saveimport_text};
")
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/index.html "${page}")
  # the chat's table of games (web/chat.js, docs/CHAT_DESIGN.md), loaded by every game page before chat.js
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/chat-games.js "var ByopopGames = [\n${chat_games}\n];\n")
  # the listed games' import rules, for the listing page's adding of games (web/gameimport.js), loaded after it
  byopop_write_if_changed(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/import-games.js
    "// The listed games' page.js files, for web/gameimport.js (written by byopop_web_index(), cmake/ByopopWeb.cmake)\n${import_games}")
  message(STATUS "byo-pop: the listing page shows ${count} games and ${soon_count} coming soon")
endfunction()
if(EMSCRIPTEN)
  # (a deferred call runs at the end of this directory, the top level, after every game's byopop_web_game())
  get_filename_component(BYOPOP_WEB_BRAND_DIR "${CMAKE_CURRENT_LIST_DIR}/../web/brand" ABSOLUTE)
  cmake_language(DEFER CALL byopop_web_index)
endif()
