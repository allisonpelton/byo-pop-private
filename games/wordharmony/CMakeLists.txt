# Word Harmony (Sprout Games, 2006; games/wordharmony) on the shared platform layer. Word Harmony runs on Sprout's own
# first-generation engine (namespace engine; shared with Pizza Frenzy and Iggle Pop), not the SexyAppFramework:
# engine/ is that engine, translated and mapped onto the platform through a host interface, game/ is the game's own
# code on top of it, and src/ the app that hosts both (games/wordharmony/docs). The SexyAppFramework only hosts the app: its window, clock, input and sound
# device, and the 800x600 screen the game draws into.
file(GLOB WORDHARMONY_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB WORDHARMONY_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/game/*.cpp)
file(GLOB WORDHARMONY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(wordharmonyengine STATIC ${WORDHARMONY_ENGINE_SOURCES})
target_include_directories(wordharmonyengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
if(NOT MSVC)
  target_compile_options(wordharmonyengine PRIVATE -Wall -Wno-unused-private-field)
endif()

add_library(wordharmonygame STATIC ${WORDHARMONY_GAME_SOURCES})
target_include_directories(wordharmonygame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/game)
target_link_libraries(wordharmonygame PUBLIC wordharmonyengine)
if(NOT MSVC)
  target_compile_options(wordharmonygame PRIVATE -Wall -Wno-unused-private-field)
endif()

# The engine's check: reads WHArchive.saf and parses every XML file in it (tools/enginetest.cpp)
if(NOT EMSCRIPTEN)
  add_executable(wordharmony_enginetest ${CMAKE_CURRENT_SOURCE_DIR}/tools/enginetest.cpp)
  target_link_libraries(wordharmony_enginetest PRIVATE wordharmonygame)
  add_custom_command(TARGET wordharmony_enginetest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:wordharmony_enginetest>
    VERBATIM)
endif()
