# Cosmic Bugs (Retro64, 2007; games/cosmicbugs) on the shared platform layer. Cosmic Bugs runs on Retro64's own
# engine (plain C over global state: images and banks, software blitters, fonts, dirty rectangles, XML data files),
# not the SexyAppFramework: engine/ is that engine, translated and mapped onto the platform, and src/ is the game on
# top of it (games/cosmicbugs/docs). The SexyAppFramework only hosts the app: its window, clock, input and sound
# device, and the 800x600 16-bit screen the game draws into.
file(GLOB COSMICBUGS_ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp)
file(GLOB COSMICBUGS_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(cosmicbugsengine STATIC ${COSMICBUGS_ENGINE_SOURCES})
target_include_directories(cosmicbugsengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine)
target_link_libraries(cosmicbugsengine PUBLIC stb)
if(NOT MSVC)
  target_compile_options(cosmicbugsengine PRIVATE -Wall)
endif()

# Headless checks of the engine against the user's game files (no window, no sound)
if(NOT EMSCRIPTEN)
  add_executable(cosmicbugs_banktest test/banktest.cpp)
  target_link_libraries(cosmicbugs_banktest PRIVATE cosmicbugsengine stb)
  if(WIN32)
    add_custom_command(TARGET cosmicbugs_banktest POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:cosmicbugs_banktest>
      VERBATIM)
  endif()
endif()
