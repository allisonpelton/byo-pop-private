# Sprout Games' second-generation engine (the C++ namespaces engine, dxdriver, simplesound and sigslot: objects and
# references, the signals, the archives and streams, the scene graph and its draw context, the screen layouts, fonts,
# particles, paths and sound; docs/ENGINES.md, "Sprout's second generation"), translated from Feeding Frenzy 2 and
# shared with Talismania. The engine's own abstract class engine::Engine is the seam: the platform's services are its
# pure slots, which host/EngineImpl implements over the framework (the original's dxdriver::EngineImpl). Game code
# includes its headers by name ("Engine.h").
file(GLOB SPROUT2_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(sprout2 STATIC ${SPROUT2_SOURCES})
target_include_directories(sprout2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(NOT MSVC)
  target_compile_options(sprout2 PRIVATE -Wall -Wno-unused-private-field)
endif()
# The images are decoded through the platform layer (Surface.cpp: libjpeg 6b and stb_image, as the originals' libjpeg
# and libpng read them)
target_link_libraries(sprout2 PUBLIC sexyframework)
# The animations' frames are Theora packets (TheoraReader.cpp), decoded by libtheora 1.1.1
target_link_libraries(sprout2 PRIVATE theoradec)

# The port's side of the engine's Windows layer (host/): the platform's services of engine::Engine over the
# framework's platform layer (EngineImpl), and the game folder's files. Each game's app runs the frames.
file(GLOB SPROUT2_HOST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/host/*.cpp)
add_library(sprout2host STATIC ${SPROUT2_HOST_SOURCES})
target_include_directories(sprout2host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host)
target_link_libraries(sprout2host PUBLIC sprout2 sexyframework)
if(NOT MSVC)
  target_compile_options(sprout2host PRIVATE -Wall -Wno-unused-private-field)
endif()
