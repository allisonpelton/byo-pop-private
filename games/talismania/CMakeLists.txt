# Talismania Deluxe (RocketJump Games, 2006; games/talismania) on the shared platform layer. The game runs on Sprout's
# second-generation engine (namespaces engine and dxdriver; shared with Feeding Frenzy 2), not the SexyAppFramework:
# engines/sprout2 is that engine, translated, and its host/ the platform's services under it; src/ is the app that
# hosts the engine on the framework (games/talismania/docs). The SexyAppFramework only hosts the app: its window,
# clock, input and sound device, and the 800x600 screen the game draws into.
file(GLOB TALISMANIA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# Native: talismania.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp); it finds assets/talismania/
# (tools/stage_talismania_assets.py) by itself. The web build comes with the first playable port.
if(NOT EMSCRIPTEN)
  add_executable(talismania ${TALISMANIA_SOURCES})
  target_include_directories(talismania PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(talismania PRIVATE sprout2host sprout2 sexyframework stb)
  if(NOT MSVC)
    target_compile_options(talismania PRIVATE -Wall -Wno-unused-private-field)
  endif()
  add_custom_command(TARGET talismania POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:talismania>
    VERBATIM)
endif()
