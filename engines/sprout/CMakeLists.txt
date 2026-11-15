# Sprout Games' first-generation engine (the C++ namespace engine: objects and references, the scene graph, the screen
# layouts, fonts, particles, the music system, the .saf archive and the resource manager; docs/ENGINES.md, "Sprout"),
# translated and mapped onto the platform through its host interface (Host.h). One copy for the games built on it:
# Word Harmony and Pizza Frenzy. Game code includes its headers by name ("Node.h"), a host beside the framework's
# headers by the folder's ("sprout/Font.h").
file(GLOB SPROUT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(sprout STATIC ${SPROUT_SOURCES})
target_include_directories(sprout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(NOT MSVC)
  target_compile_options(sprout PRIVATE -Wall -Wno-unused-private-field)
endif()

# The port's side of the engine's Windows layer (host/): its images and the render context over the framework's screen
# (Render), its sound system over the platform's mixer (SoundHost). The game's app implements the rest of the host.
file(GLOB SPROUT_HOST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/host/*.cpp)
add_library(sprouthost STATIC ${SPROUT_HOST_SOURCES})
target_link_libraries(sprouthost PUBLIC sprout sexyframework)
if(NOT MSVC)
  target_compile_options(sprouthost PRIVATE -Wall -Wno-unused-private-field)
endif()
