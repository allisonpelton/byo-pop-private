# Bejeweled Twist (PopCap, 2008; games/bejeweledtwist). The port is a translation of the original's game classes
# (docs/PORT_PLAN.md) on the shared framework and platform layer.
#
# bjt_engine: the classes kept free of game code (CurvedVal, BSpline). bjt_curvetest evaluates curve strings,
# headless (tools/curvetest/curvetest.cpp).
add_library(bjt_engine STATIC ${CMAKE_CURRENT_SOURCE_DIR}/src/CurvedVal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/BSpline.cpp)
target_include_directories(bjt_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bjt_engine PUBLIC sexyframework sexycompat2004)
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

# bjt_popanim: Sexy::PopAnim, PopCap's player for .pam animations (popanim/: Bookworm Adventures 2's, with Twist's
# two scales), kept free of game code.
file(GLOB POPANIM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/popanim/*.cpp)
add_library(bjt_popanim STATIC ${POPANIM_SOURCES})
target_include_directories(bjt_popanim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/popanim)
target_link_libraries(bjt_popanim PUBLIC sexyframework)
if(NOT MSVC)
  target_compile_options(bjt_popanim PRIVATE -Wall)
endif()

# bejeweledtwist: the game (src/). Native: bejeweledtwist.exe [-gamedir=<dir>] [-userdir=<dir>] (src/main.cpp); it
# finds assets/bejeweledtwist/ by itself.
set(BJT_SOURCES
  src/main.cpp src/WinTwistApp.cpp src/CustomResourceManager.cpp src/Res.cpp src/GameText.cpp
  src/TwistWidget.cpp src/TwistButton.cpp src/TwistCheckbox.cpp src/TwistSlider.cpp src/CustomBassMusicInterface.cpp src/TwistUtil.cpp src/GraphicsRecorder.cpp src/PlayButton.cpp
  src/EffectsManager.cpp src/Logo.cpp src/LoaderScreen.cpp src/Hyperspace.cpp src/VizManager.cpp
  src/Tutorial.cpp src/WheelSpin.cpp src/ColorCycle.cpp
  src/MenuButtonWidget.cpp src/Gates.cpp src/Profile.cpp src/MenuPanels.cpp src/Menu.cpp src/Ranks.cpp
  src/TwistEditWidget.cpp src/TwistDialog.cpp src/UnderDialogWidget.cpp
  src/Piece.cpp src/Board.cpp src/BoardUpdate.cpp src/BoardFill.cpp src/BoardTurn.cpp src/BoardMatch.cpp
  src/BoardTwist.cpp src/GoodTube.cpp
  src/Traveller.cpp src/GemSoul.cpp src/LevelUpShow.cpp src/Shockwave.cpp src/SpeedText.cpp src/RankBonusWidget.cpp src/AwardTelly.cpp src/Mandala.cpp src/ModeButton.cpp src/Points.cpp src/BoardDraw.cpp
  src/FlyingText.cpp src/GameOverShow.cpp src/Challenge.cpp src/ChallengeMenu.cpp)
add_executable(bejeweledtwist ${BJT_SOURCES})
target_include_directories(bejeweledtwist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(bejeweledtwist PRIVATE bjt_engine bjt_popanim sexyframework sexyopenmpt sexyopenjpeg)
if(NOT MSVC)
  target_compile_options(bejeweledtwist PRIVATE -Wall -Wno-overloaded-virtual)
endif()
if(EMSCRIPTEN)
  byopop_web_game(bejeweledtwist TITLE "Bejeweled Twist"
    COPYRIGHT "Bejeweled Twist © 2008 PopCap Games, Inc. All rights reserved. Bejeweled is a registered trademark of PopCap Games, Inc." LICENSES libopenmpt openjpeg)
else()
  add_custom_command(TARGET bejeweledtwist POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweledtwist>
    VERBATIM)
endif()
