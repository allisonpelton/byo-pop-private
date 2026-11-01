# Bejeweled Twist (PopCap, 2008; games/bejeweledtwist). The port is a translation of the original's game classes
# (docs/PORT_PLAN.md) on the shared framework and platform layer.
#
# bjt_engine: the classes kept free of game code (CurvedVal, BSpline). bjt_curvetest evaluates curve strings,
# headless (tools/curvetest/curvetest.cpp).
add_library(bjt_engine STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/CurvedVal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/BSpline.cpp)
target_include_directories(bjt_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bjt_engine PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(bjt_engine PRIVATE -Wall)
endif()
if(NOT EMSCRIPTEN)
  add_executable(bjt_curvetest tools/curvetest/curvetest.cpp)
  target_link_libraries(bjt_curvetest PRIVATE bjt_engine)
  add_custom_command(TARGET bjt_curvetest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bjt_curvetest>
    VERBATIM)
endif()

# bejeweledtwist: the game (src/). Native: bejeweledtwist.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp); it
# finds assets/bejeweledtwist/ by itself.
set(BJT_SOURCES
  src/main.cpp src/WinTwistApp.cpp src/CustomResourceManager.cpp src/Res.cpp src/GameText.cpp
  src/TwistWidget.cpp src/TwistButton.cpp src/TwistUtil.cpp src/GraphicsRecorder.cpp src/PlayButton.cpp
  src/EffectsManager.cpp src/Logo.cpp src/LoaderScreen.cpp)
add_executable(bejeweledtwist ${BJT_SOURCES})
target_include_directories(bejeweledtwist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bejeweledtwist PRIVATE bjt_engine sexyframework sexyopenmpt sexyopenjpeg)
if(NOT MSVC)
  target_compile_options(bejeweledtwist PRIVATE -Wall -Wno-overloaded-virtual)
endif()
if(NOT EMSCRIPTEN)
  add_custom_command(TARGET bejeweledtwist POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweledtwist>
    VERBATIM)
endif()
