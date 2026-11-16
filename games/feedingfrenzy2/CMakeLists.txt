# Feeding Frenzy 2 (Sprout Games, 2006; games/feedingfrenzy2) on the shared platform layer. The game runs on Sprout's
# second-generation engine (namespaces engine and dxdriver; shared with Talismania), not the SexyAppFramework:
# engines/sprout2 is that engine, translated, and its host/ the platform's services under it; src/ is the app that
# hosts the engine on the framework (games/feedingfrenzy2/docs). The SexyAppFramework only hosts the app: its window,
# clock, input and sound device, and the 800x600 screen the game draws into.
file(GLOB FEEDINGFRENZY2_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Native: feedingfrenzy2.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp); it finds assets/feedingfrenzy2/
# (tools/stage_feedingfrenzy2_assets.py) by itself. The web build comes with the first playable port.
if(NOT EMSCRIPTEN)
  add_executable(feedingfrenzy2 ${FEEDINGFRENZY2_SOURCES})
  target_include_directories(feedingfrenzy2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(feedingfrenzy2 PRIVATE sprout2host sprout2 sexyframework stb)
  if(NOT MSVC)
    target_compile_options(feedingfrenzy2 PRIVATE -Wall -Wno-unused-private-field)
  endif()
  add_custom_command(TARGET feedingfrenzy2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:feedingfrenzy2>
    VERBATIM)
endif()
