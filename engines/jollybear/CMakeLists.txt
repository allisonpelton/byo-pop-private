# JollyBear's engine (the manager, layers, objects, the display and the BASS sound object; docs/ENGINE.md), translated and
# mapped onto the platform. One copy for the games built on it: Mahjong Escape: Ancient Japan and Ancient China, whose
# own code (engines/mahjongescape and each game's folder) sits on it. The SexyAppFramework only hosts the app: its
# window, clock, input and sound device, and the 800x600 screen the game draws into.
file(GLOB JOLLYBEAR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(jollybearengine STATIC ${JOLLYBEAR_SOURCES})
target_include_directories(jollybearengine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(jollybearengine PUBLIC sexyframework sexyopenmpt stb)
if(NOT MSVC)
  target_compile_options(jollybearengine PRIVATE -Wall -Wno-unused-private-field)
endif()
