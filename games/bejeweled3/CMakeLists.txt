# Bejeweled 3 (PopCap, 2010; games/bejeweled3). The port is a translation of the original's game classes
# (docs/PORT_PLAN.md) on the shared framework and platform layer, with the framework generation the game was built on
# in engines/prime (shared with Zuma's Revenge, docs/ENGINES.md).
#
# Headless tests of engines/prime against the staged game files: b3_curvetest evaluates the curve strings of a staged
# Bejeweled3.exe (or the ones given), b3_pieffecttest loads and runs the particle effects, b3_pamtest the animations,
# b3_restest reads the manifest and loads the game's groups with the generated code (src/Res.cpp)
# (tools/<name>/<name>.cpp).
if(NOT EMSCRIPTEN)
  foreach(B3_TEST curvetest pieffecttest pamtest restest)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/${B3_TEST}/${B3_TEST}.cpp)
      add_executable(b3_${B3_TEST} tools/${B3_TEST}/${B3_TEST}.cpp)
      target_link_libraries(b3_${B3_TEST} PRIVATE prime)
      if(NOT MSVC)
        target_compile_options(b3_${B3_TEST} PRIVATE -Wall)
      endif()
      add_custom_command(TARGET b3_${B3_TEST} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:b3_${B3_TEST}>
        VERBATIM)
    endif()
  endforeach()
  target_sources(b3_restest PRIVATE src/Res.cpp)
  target_link_libraries(b3_restest PRIVATE sexyopenjpeg)  # most of the pictures are JPEG 2000
  target_link_libraries(b3_pamtest PRIVATE sexyopenjpeg)
endif()

# bejeweled3: the game (src/). Native: bejeweled3.exe [-gamedir=<dir>] [-userdir=<dir>] [-modsdir=<dir>] [-nomods]
# [-600|-768|-1200] (src/main.cpp); it finds assets/bejeweled3/ by itself. The web build comes with its milestone
# (docs/PORT_PLAN.md).
if(NOT EMSCRIPTEN)
  set(B3_SOURCES src/main.cpp src/Bej3App.cpp src/Res.cpp src/Bej3Music.cpp src/EncodingParser.cpp src/HighScores.cpp
	src/Background.cpp src/Bej3Dialog.cpp src/Board.cpp src/BoardDraw.cpp src/BoardEffects.cpp src/BoardLevels.cpp src/BoardMoves.cpp src/CrystalBall.cpp src/GemSilhouettes.cpp src/TimeBonusEffect.cpp src/Effects.cpp src/EndLevelDialog.cpp src/EndLevelDialog.cpp src/MainMenu.cpp src/Math3D.cpp src/OptionsDialog.cpp src/PlayMenu.cpp src/Profile.cpp src/QuestData.cpp src/TestCommands.cpp src/ZenBoard.cpp src/LightningBoard.cpp src/QuestBoard.cpp src/ButterflyBoard.cpp src/IceStormBoard.cpp src/DiamondMineBoard.cpp
    src/PokerBoard.cpp src/QuestGoal.cpp src/QuestMenu.cpp src/TimeLimitBoard.cpp src/MoveLimitBoard.cpp)
  add_executable(bejeweled3 ${B3_SOURCES})
  target_include_directories(bejeweled3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(bejeweled3 PRIVATE prime sexyframework sexyopenmpt sexyopenjpeg)
  if(NOT MSVC)
    target_compile_options(bejeweled3 PRIVATE -Wall -Wno-overloaded-virtual)
  endif()
  add_custom_command(TARGET bejeweled3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL2::SDL2> ${PORT_RUNTIME_DLLS} $<TARGET_FILE_DIR:bejeweled3>
    VERBATIM)
endif()
