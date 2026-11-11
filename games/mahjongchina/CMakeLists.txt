# Mahjong Escape: Ancient China (JollyBear Games; games/mahjongchina) on the shared platform layer, built on two shared
# engines (docs/ENGINES.md): jollybear (engines/jollybear, JollyBear's own engine, docs/ENGINE.md) and mahjongescape
# (engines/mahjongescape, the game's code, one source for both editions: Ancient Japan's too). This folder holds what
# is Ancient China's own: src/ (the edition's numbers, tables and the few functions the two exes do differently),
# the data, the docs, the tools and the web page.
include(${PROJECT_SOURCE_DIR}/engines/mahjongescape/mahjongescape.cmake)
mahjongescape_game(mahjongchina TITLE "Mahjong Escape: Ancient China"
  COPYRIGHT "Mahjong Escape: Ancient China © JollyBear Games. All rights reserved.")
