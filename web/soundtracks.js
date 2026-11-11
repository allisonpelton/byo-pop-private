// The music of each game, for the listing page's music player (web/music.js; docs/WEB_PLAYER.md, "The music player"), by the
// game's id. Paths are in the game's folder as the import writes it, matched without regard to case, where * stands for any part
// of a file name.
//   files   the files that are tunes: a module's subsongs are its tunes, an Ogg Vorbis or WAV file is one tune
//   parts   for a module the game plays in ranges of its orders: {"<file name>": [[first order, last order], ...]}, its tunes
//           instead of its subsongs
//   joined  tunes the game joins from pieces by a playlist: [{playlist: "<path>", piece: "<path with {} for the playlist's word>"}]
//   skip    ["<file name>#<n>", ...]: tunes that are not tunes (n counts from 0 in the file's subsongs or parts)
//   names   {"<file name>#<n>" or "<playlist name>": "<name>"}: plain names, never invented titles and never the game's own text
//           at length; without one a tune is named after its file
//   inside  tunes kept inside a game's file: [{file: "<path>", magic: "<first bytes>"}] its resources (a Windows module's)
//           that start with those bytes, the magic naming their format, or [{file: "<path>", zip: ["<entry>", ...]}] those
//           entries of a zip archive; each is then a file as above
//   where   {"<file name>#<n>", "<file name>" (an Ogg or WAV file) or "<playlist name>": "<place>"}: where in the game its code plays
//           the tune, as a short value ("Main menu", "Levels 1-5"), from the game's code (its session's answer) or, for an Ogg
//           game, its file's name; shown beside the tune and in the display
//   unused  ["<file name>#<n>", ...]: tunes the game's code never plays (left in its files), shown as such
//   level   the level the game plays its modules at, where it is not the renderer's own (platform/MusicLevels.h):
//           {bassAmplify: n} the BASS amplify it sets (0 to 100), {fmod3XM: true} FMOD 3's XM pre-amp. The renderer is the
//           game's own (libopenmpt for MO3 and OXM, libxmp-lite for the rest: web/music/openmpt_web.c)
// Tunes under two seconds are left out (the silences and stings between tunes). Every game's tunes were listed from the user's
// copies on 2026-11-07.
var ByopopSoundtracks = {
  // Places and unused tunes from each port's code (the coordinator's reading, 2026-11-11); a place only inferred is left blank
  alchemy: {
    files: ["music/*.s3m"],
    where: { "alch_intro#0": "Title and loading", "1985-hehasnoface#0": "In a game" },
    unused: ["1985-hehasnoface#1"]
  },
  // AstroPop's one module holds every tune, which the game starts at an order of it (PlaySong); its subsongs stand for them
  astropop: {
    files: ["music/*.mo3"],
    where: { "music#0": "Start-up and loading", "music#1": "Menu, pilot select, story", "music#2": "In a game",
      "music#3": "Brick warning siren", "music#4": "Game end and results", "music#5": "Level start and complete" }
  },
  atomica: {
    files: ["music/*.it"],
    where: { "atomica#2": "Title screen", "atomica#3": "Levels 1, 6, 11 and so on", "atomica#5": "Every 5th level",
      "atomica#6": "Main menu", "atomica#8": "Game over results", "atomica#10": "Fly-away after game over" },
    unused: ["atomica#1", "atomica#4", "atomica#7", "atomica#9"]
  },
  bejeweled: {
    files: ["music/*.s3m"],
    where: { "1980-Autonomus#0": "Title screen", "1984-Network#0": "Normal game", "1981-DataJack#0": "Timed game" },
    unused: ["1981-DataJack#1", "1981-DataJack#2", "1981-DataJack#3"]
  },
  // one module, BeyondNetwork.mo3: the game plays it from a starting order for each screen (properties/music.xml), so its
  // subsongs are the tunes
  bejeweled2: {
    files: ["music/*.mo3"],
    where: { "BeyondNetwork#1": "Classic, action and puzzle", "BeyondNetwork#2": "Main menu", "BeyondNetwork#5": "Loading and title",
      "BeyondNetwork#6": "Finale, last planet cleared" },
    unused: ["BeyondNetwork#3", "BeyondNetwork#4", "BeyondNetwork#7"]
  },
  bejeweledtwist: {
    files: ["music/*.mo3"],
    where: { "bejewel twist 06.10#0": "In a game, by level", "bejewel twist 06.10#1": "In a game, by level",
      "bejewel twist 06.10#2": "In a game, by level", "bejewel twist 06.10#6": "In a game, by level",
      "bejewel twist 06.10#8": "In a game, by level", "bejewel twist 06.10#7": "Loading screen",
      "bejewel twist 06.10#3": "Game over, lost", "bejewel twist 06.10#4": "Game over, won", "bejewel twist 06.10#5": "Main menu",
      "bejeweledtwist24#0": "Blitz", "bejeweledtwist24#1": "Blitz end, moderate score", "bejeweledtwist24#2": "Blitz end, good score",
      "bejeweledtwist24#3": "Blitz end, excellent score", "bejeweledtwist24#4": "Challenge menu",
      "bejeweledtwist24#5": "Turn-based challenge", "bejeweledtwist24#6": "Untimed challenge", "bejeweledtwist24#7": "Mega fruit bonus" },
    unused: ["bejewel twist 06.10#10", "bejewel twist 06.10#13", "bejeweledtwist24#8"]
  },
  bigmoney: {
    files: ["music/*.it"],
    where: { "QualityTime_BigMoney#0": "Puzzle game", "QualityTime_BigMoney#1": "Puzzle level-up",
      "QualityTime_BigMoney#2": "Action game", "QualityTime_BigMoney#3": "Action level-up",
      "QualityTime_BigMoney#4": "Strategy game", "QualityTime_BigMoney#5": "Strategy level-up",
      "QualityTime_BigMoney#6": "Game over", "QualityTime_BigMoney#7": "Title and game select" }
  },
  bookworm: {
    files: ["music/*.mo3"],
    where: { "bookworm1#0": "Classic game", "bookworm1#1": "Title and menu", "bookworm1#2": "Action game",
      "bookworm1#3": "Danger, classic", "bookworm1#4": "Hall of fame", "bookworm1#5": "Game over, action",
      "bookworm1#6": "Game over, classic", "bookworm1#7": "Danger, action" }
  },
  // Bookworm Adventures sets its music's amplify to 12.0, which BASS holds at 100
  bookwormadv: {
    files: ["sounds/music.mo3"],
    level: { bassAmplify: 100 },
    where: { "music#0": "Greek battles, timed battles", "music#1": "Greek map, timed map, intro", "music#2": "Greek boss, timed boss",
      "music#3": "Level won", "music#4": "Boss won", "music#5": "Arabian battles", "music#6": "Arabian map", "music#7": "Arabian boss",
      "music#8": "Credits", "music#9": "Monster book battles", "music#10": "Monster book map", "music#11": "Monster book boss",
      "music#12": "Main menu", "music#14": "Mini-games", "music#15": "Player's death" }
  },
  // the game changes song by level (cosmo1 from the start)
  cosmicbugs: {
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Start", "cosmo3#0": "Levels 10-19, 200-299", "cosmo2#0": "Levels 20-49, 300-399",
      "cosmo5#0": "Levels 50-99, 400-499", "cosmo4#0": "Levels 100-199, 500 on" }
  },
  waterbugs: {
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Start", "cosmo3#0": "Levels 10-19, 200-299", "cosmo2#0": "Levels 20-49, 300-399",
      "cosmo5#0": "Levels 50-99, 400-499", "cosmo4#0": "Levels 100-199, 500 on" }
  },
  dynomite: {
    files: ["music/*.it"],
    where: { "amazonas#0": "Classic game", "amazonas#1": "Title and intro", "amazonas#2": "Losing sequence",
      "amazonas#4": "Time Trial cleared", "amazonas#5": "Game start jingle", "amazonas#6": "Time Trial", "amazonas#7": "Panic",
      "amazonas#8": "Fossil mode", "omniphilia#0": "Stomped" },
    unused: ["amazonas#3"]
  },
  sevenseas: {
    files: ["music/*.it"],
    where: { "game#0": "Levels 1-11", "game#1": "Level complete", "game#2": "Game over", "game#3": "Treasure bonus",
      "game#4": "Bonus over", "game#5": "Tutorial and title", "game#6": "End of the tutorial tune", "game#7": "Pirate intro",
      "game#8": "High scores", "game#9": "Every 10th level" },
    unused: ["game#10"]
  },
  // Typer Shark's one module (wordshar.mo3) holds every tune, which the game starts at an order of it (ChangeMusic); its subsongs stand for them
  typershark: {
    files: ["music/*.mo3"],
    where: { "wordshar#0": "Play", "wordshar#1": "Title, menu, bonus, map", "wordshar#2": "Game over", "wordshar#3": "Level done",
      "wordshar#4": "Danger, a shark near", "wordshar#6": "Danger, a shark very near" }
  },
  mummymaze: {
    files: ["music/*.it"],
    where: { "game#0": "Intro, menu, adventure start", "game#1": "Tutorial", "game#2": "Chambers", "game#3": "Chamber complete",
      "game#5": "Chamber ending", "game#6": "Chamber ending", "game#7": "Victory", "game#9": "Adventure map", "game#10": "High scores" }
  },
  // tunes inside the games' own files: the Mahjong Escapes keep their MO3 modules among Resource.dll's resources (the game's
  // module, 339, starts at one of two orders by the player's progress and jumps to others while gold tiles are in play), Ning
  // Po its one music stream in its Python archive
  mahjongchina: {
    inside: [{ file: "resource.dll", magic: "MO3" }],
    where: { "Resource 335#0": "Main menu", "Resource 336#0": "Layout select", "Resource 337#0": "Layout select",
      "Resource 338#0": "Layout select", "Resource 339#0": "In a game", "Resource 339#1": "Gold tile pair free",
      "Resource 339#2": "Gold tiles matched", "Resource 339#3": "In a game" }
  },
  mahjongjapan: {
    inside: [{ file: "resource.dll", magic: "MO3" }],
    where: { "Resource 335#0": "Main menu", "Resource 336#0": "Layout select", "Resource 337#0": "Layout select",
      "Resource 339#0": "In a game", "Resource 339#1": "Gold tile pair free", "Resource 339#2": "Gold tiles matched",
      "Resource 339#3": "In a game" }
  },
  // Word Harmony's music is made as it plays, from WAV clips: the menu's loop, and in the rising-water mode one drum track
  // stepping through the intense pieces as the water rises (which piece goes with which height is not known); its other modes
  // pick single instrument samples at random, which are not tunes and are left out, as is intense-levelend (a sound)
  wordharmony: {
    files: ["res/music/menuMusic.wav", "res/music/intense0.wav", "res/music/intense1.wav", "res/music/intense1b.wav",
      "res/music/intense2.wav", "res/music/intense3.wav", "res/music/intense4.wav", "res/music/intense5.wav",
      "res/music/intense6.wav", "res/music/intense-between.wav", "res/music/intenseEnd.wav"],
    where: { menuMusic: "Main menu", intense0: "Rising water", intense1: "Rising water", intense1b: "Rising water",
      intense2: "Rising water", intense3: "Rising water", intense4: "Rising water", intense5: "Rising water",
      intense6: "Rising water", "intense-between": "Rising water, between steps" }
  },
  ningpo: { inside: [{ file: "ningpo.pyz", zip: ["theme_np.ogg"] }], where: { theme_np: "In play" } },
  // the Ogg games
  bookwormadv2: {
    files: ["music/*.ogg"],
    where: { chinese_map: "Chinese world, map", chinese_level: "Chinese world, levels, menu", chinese_boss: "Chinese world, boss",
      fairy_tale_map: "Fairy tale world, map", fairy_tale_level: "Fairy tale levels, menu", fairy_tale_boss: "Fairy tale world, boss",
      scifi_map: "Sci-fi world, map", scifi_level: "Sci-fi world, levels, menu", scifi_boss: "Sci-fi world, boss",
      dance_battle_level: "Dance battle" }
  },
  // Noah's Ark: mainintro plays once as play starts and runs on into mainloop
  noahsark: {
    files: ["music/mainintro.ogg", "music/mainloop.ogg", "music/loading.ogg", "music/gameover.ogg"],
    where: { mainintro: "Start of play", mainloop: "In play", loading: "Title and menu", gameover: "Game over, high scores" }
  },
  // Bonnie's Bookstore plays its one module as tracks, each a range of orders (its track table, games/bonnies/game/Music.cpp:
  // the first for the menu, the rest in a game), and a logo tune
  bonnies: {
    files: ["sound/logo.ogg", "sound/music.oxm"],
    level: { fmod3XM: true },
    parts: { music: [[1, 18], [20, 52], [54, 82], [84, 99], [101, 126], [128, 150], [152, 168], [170, 196], [198, 220]] },
    where: { logo: "Logo", "music#0": "Main menu", "music#1": "In a game", "music#2": "In a game", "music#3": "In a game",
      "music#4": "In a game", "music#5": "In a game", "music#6": "In a game", "music#7": "In a game", "music#8": "In a game" }
  },
  // Pixelus and Rocket Mania join their in-game (and Rocket Mania its menu) music from pieces by a playlist, as the games do
  pixelus: {
    files: ["music/pixelus *.ogg"],
    joined: [{ playlist: "music/pixelus in-game playlist.txt", piece: "music/{}.ogg" }],
    where: { "pixelus main menu loop v02": "Main menu", "pixelus story screen loop v01": "Story screen",
      "pixelus temple screen loop v01": "Temple screen", "pixelus in-game": "In a game" }
  },
  rocketmania: {
    files: ["music/* loop *.ogg"],
    joined: [{ playlist: "music/tubetricks menu one v02 playlist.txt", piece: "music/tubetricks menu one v02 {}.ogg" },
      { playlist: "music/tubetricks in-game one v02 playlist.txt", piece: "music/tubetricks in-game one v02 {}.ogg" }],
    where: { "tubetricks menu one v02": "Main menu", "tubetricks in-game one v02": "In a game",
      "tubetricks level up loop one v01": "Level up", "tubetricks game over loop one v01": "Game over" }
  }
};
