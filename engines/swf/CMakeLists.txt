# sexyswf: Sexy::SwfMovie, a small player for Flash (.swf) movies written from the public SWF file format
# specification, kept free of game code: the one copy for every port that plays a game's Flash cutscenes through it
# (both Bookworm Adventures games, Heavy Weapon). Each game keeps its own widget over it and its own swftest tool.
file(GLOB SWF_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(sexyswf STATIC ${SWF_SOURCES})
target_include_directories(sexyswf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sexyswf PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(sexyswf PRIVATE -Wall)
endif()
