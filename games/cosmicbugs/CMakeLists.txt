# Cosmic Bugs (Retro64, 2007; games/cosmicbugs) on the shared platform layer. Cosmic Bugs runs on Retro64's own
# engine (plain C over global state: images and banks, software blitters, fonts, dirty rectangles, XML data files),
# not the SexyAppFramework: engine/ is that engine, translated and mapped onto the platform, and src/ is the game on
# top of it (games/cosmicbugs/docs). The SexyAppFramework only hosts the app: its window, clock, input and sound
# device, and the 800x600 16-bit screen the game draws into.
file(GLOB COSMICBUGS_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB COSMICBUGS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# The host (the app and main) goes in the exe; the headless runner has a host of its own. SiteProfiles.cpp (the web page's
# players, framework/WebProfiles.h) is for the web exe only: it needs the framework, which this library does not link
list(FILTER COSMICBUGS_SOURCES EXCLUDE REGEX "/(CosmicBugsApp|main|SiteProfiles)\\.cpp$")

add_library(cosmicbugsengine STATIC ${COSMICBUGS_ENGINE_SOURCES})
target_include_directories(cosmicbugsengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(cosmicbugsengine PUBLIC stb)
if(NOT MSVC)
  target_compile_options(cosmicbugsengine PRIVATE -Wall)
endif()

# The game's code over the engine
add_library(cosmicbugsgame STATIC ${COSMICBUGS_SOURCES})
target_include_directories(cosmicbugsgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(cosmicbugsgame PUBLIC cosmicbugsengine)
if(NOT MSVC)
  target_compile_options(cosmicbugsgame PRIVATE -Wall)
endif()

# The game under the framework's app host
add_executable(cosmicbugs ${CMAKE_CURRENT_SOURCE_DIR}/src/CosmicBugsApp.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(cosmicbugs PRIVATE cosmicbugsgame cosmicbugsengine sexyframework sexycompat2004 sexyopenmpt stb)
if(NOT MSVC)
  target_compile_options(cosmicbugs PRIVATE -Wall)
endif()
if(EMSCRIPTEN)
  target_sources(cosmicbugs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/SiteProfiles.cpp)
  byopop_web_game(cosmicbugs TITLE "Cosmic Bugs"
    COPYRIGHT "Cosmic Bugs © 2007 PopCap Games, Inc. All rights reserved. Cosmic Bugs was developed by Retro64, Inc."
    LICENSES libopenmpt)
else()
  add_custom_command(TARGET cosmicbugs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:cosmicbugs>
    VERBATIM)
endif()

# Headless checks against the user's game files (no window, no sound): the engine's banks, and the whole game on a
# clock of its own with scripted input
if(NOT EMSCRIPTEN)
  add_executable(cosmicbugs_banktest test/banktest.cpp)
  target_link_libraries(cosmicbugs_banktest PRIVATE cosmicbugsgame cosmicbugsengine stb)
  add_executable(cosmicbugs_run test/gamerun.cpp)
  target_link_libraries(cosmicbugs_run PRIVATE cosmicbugsgame cosmicbugsengine stb)
  if(WIN32)
    add_custom_command(TARGET cosmicbugs_banktest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:cosmicbugs_banktest>
      VERBATIM)
    add_custom_command(TARGET cosmicbugs_run POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:cosmicbugs_run>
      VERBATIM)
  endif()
endif()
