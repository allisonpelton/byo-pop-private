# Water Bugs (Retro64, 2007; games/waterbugs) on the shared platform layer. Water Bugs runs on Retro64's own
# engine (plain C over global state: images and banks, software blitters, fonts, dirty rectangles, XML data files),
# not the SexyAppFramework: engine/ is that engine, translated and mapped onto the platform, and src/ is the game on
# top of it (games/waterbugs/docs). The SexyAppFramework only hosts the app: its window, clock, input and sound
# device, and the 800x600 16-bit screen the game draws into.
file(GLOB WATERBUGS_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB WATERBUGS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# The host (the app and main) goes in the exe; the headless runner has a host of its own
# (SiteProfiles.cpp, the web page's players in framework/WebProfiles.h, is for the web exe only: it needs the framework, which
# this library does not link)
list(FILTER WATERBUGS_SOURCES EXCLUDE REGEX "/(WaterBugsApp|main|SiteProfiles)\\.cpp$")

add_library(waterbugsengine STATIC ${WATERBUGS_ENGINE_SOURCES})
target_include_directories(waterbugsengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(waterbugsengine PUBLIC stb)
if(NOT MSVC)
  target_compile_options(waterbugsengine PRIVATE -Wall)
endif()

# The game's code over the engine
add_library(waterbugsgame STATIC ${WATERBUGS_SOURCES})
target_include_directories(waterbugsgame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(waterbugsgame PUBLIC waterbugsengine)
if(NOT MSVC)
  target_compile_options(waterbugsgame PRIVATE -Wall)
endif()

# The game under the framework's app host
add_executable(waterbugs ${CMAKE_CURRENT_SOURCE_DIR}/src/WaterBugsApp.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(waterbugs PRIVATE waterbugsgame waterbugsengine sexyframework sexycompat2004 sexyopenmpt stb)
if(NOT MSVC)
  target_compile_options(waterbugs PRIVATE -Wall)
endif()
if(EMSCRIPTEN)
  target_sources(waterbugs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/SiteProfiles.cpp)
  byopop_web_game(waterbugs TITLE "Water Bugs"
    COPYRIGHT "Water Bugs © 2007 PopCap Games, Inc. All rights reserved. Water Bugs was developed by Retro64, Inc."
    LICENSES libopenmpt)
else()
  add_custom_command(TARGET waterbugs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:waterbugs>
    VERBATIM)
endif()

# Headless checks against the user's game files (no window, no sound): the engine's banks, and the whole game on a
# clock of its own with scripted input
if(NOT EMSCRIPTEN)
  add_executable(waterbugs_banktest test/banktest.cpp)
  target_link_libraries(waterbugs_banktest PRIVATE waterbugsgame waterbugsengine stb)
  add_executable(waterbugs_run test/gamerun.cpp)
  target_link_libraries(waterbugs_run PRIVATE waterbugsgame waterbugsengine stb)
  if(WIN32)
    add_custom_command(TARGET waterbugs_banktest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:waterbugs_banktest>
      VERBATIM)
    add_custom_command(TARGET waterbugs_run POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:waterbugs_run>
      VERBATIM)
  endif()
endif()
