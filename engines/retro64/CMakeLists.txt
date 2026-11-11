# Retro64's engine (2007), the one copy that Cosmic Bugs and Water Bugs both run on (engines/retro64). Plain C over
# global state: images and banks, software blitters, fonts, dirty rectangles, XML data files, translated from the
# games' exes and mapped onto the platform. Where the two exes' engines differ, the library has both
# (docs/ENGINES.md): a game sets which at its start.
file(GLOB RETRO64_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(retro64 STATIC ${RETRO64_SOURCES})
target_include_directories(retro64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(retro64 PUBLIC stb)
if(NOT MSVC)
  target_compile_options(retro64 PRIVATE -Wall)
endif()
