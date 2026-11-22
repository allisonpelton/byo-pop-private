# prime: what PopCap's post-2010 internal framework (the "prime" generation of Bejeweled 3, and the 2009 one of Zuma's
# Revenge) has beyond the shared framework/, translated from Bejeweled 3 and kept free of game code so that both games
# link one copy (docs/ENGINES.md): CurvedVal, and as they come the PopAnim player for .pam versions 4 and 5, PIEffect
# particles, the manifest-2 resource manager and the 3D layer. Game code includes its headers by name ("CurvedVal.h").
file(GLOB PRIME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(prime STATIC ${PRIME_SOURCES})
target_include_directories(prime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(prime PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(prime PRIVATE -Wall)
endif()
