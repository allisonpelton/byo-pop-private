// The music of each game, for the listing page's music player (web/music.js; docs/WEB_PLAYER.md, "The music player"), by the
// game's id. Paths are in the game's folder as the import writes it, matched without regard to case, where * stands for any part
// of a file name.
//   files   the files that are tunes: a module's subsongs are its tunes, an Ogg Vorbis or WAV file is one tune
//   parts   for a module the game plays in ranges of its orders: {"<file name>": [[first order, last order], ...]}, its tunes
//           instead of its subsongs
//   joined  tunes the game joins from pieces by a playlist: [{playlist: "<path>", piece: "<path with {} for the playlist's word>"}],
//           or from files in order: [{id: "<name>", pieces: ["<path>", ...], loopFrom: <the piece its loop goes back to>}]
//   cuts    {"<file name>#<n>": [order, ...]}: a subsong split where the game's code starts a different tune partway
//           through; each later piece is "<file name>#<n>@<order>" (an order the subsong never reaches: from it to the end)
//   chains  {"<file name>#<n>": ["<file name>#<m>", ...]}: parts the game only plays as jumps inside one tune, one track
//   skip    ["<file name>#<n>", ...]: tunes that are not tunes (n counts from 0 in the file's subsongs or parts)
//   names   {"<file name>#<n>" or "<playlist name>": "<name>"}: plain names, never invented titles and never the game's own text
//           at length; without one a tune is named after its file
//   inside  tunes kept inside a game's file: [{file: "<path>", magic: "<first bytes>"}] its resources (a Windows module's)
//           that start with those bytes, the magic naming their format, or [{file: "<path>", zip: ["<entry>", ...]}] those
//           entries of a zip archive; each is then a file as above
//   where   {"<file name>#<n>", "<file name>" (an Ogg or WAV file) or "<playlist name>": "<place>"}: where in the game its code plays
//           the tune, in the style of a game music rip ("Main Menu", "Danger 2": Title Case, one place, at most 20 characters,
//           tunes of one place numbered as the game reaches them), from the game's code (its session's answer) or, for an Ogg
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
    where: { "alch_intro#0": "Title Screen", "1985-hehasnoface#0": "In-Game" },
    unused: ["1985-hehasnoface#1"]
  },
  // AstroPop's one module holds every tune, which the game starts at an order of it (PlaySong); its subsongs stand for them,
  // each starting at one of the orders the game plays (0 loading, 5 the menus, 9 a level, 35 the danger siren, 39 the game's
  // end, 41 a level's tally and level-up: games/astropop/src, checked 2026-11-11 once its music played)
  astropop: {
    files: ["music/*.mo3"],
    where: { "music#0": "Loading", "music#1": "Main Menu", "music#2": "In-Game", "music#3": "Danger", "music#4": "Game Over",
      "music#5": "Level Complete" }
  },
  atomica: {
    files: ["music/*.it"],
    where: { "atomica#2": "Title Screen", "atomica#3": "Level Theme 1", "atomica#5": "Level Theme 2", "atomica#7": "Main Menu", "atomica#9": "Level Theme 3",
      "atomica#8": "Game Over 1", "atomica#10": "Game Over 2" },
    unused: ["atomica#1", "atomica#4", "atomica#6"]
  },
  bejeweled: {
    files: ["music/*.s3m"],
    where: { "1980-Autonomus#0": "Title Screen", "1984-Network#0": "Normal Mode", "1981-DataJack#0": "Timed Mode" },
    unused: ["1981-DataJack#1", "1981-DataJack#2", "1981-DataJack#3"]
  },
  // one module, BeyondNetwork.mo3: the game plays it from a starting order for each screen (properties/music.xml), so its
  // subsongs are the tunes
  bejeweled2: {
    files: ["music/*.mo3"],
    cuts: { "BeyondNetwork#1": [122] },
    where: { "BeyondNetwork#1": "Classic Mode", "BeyondNetwork#1@122": "Action Mode", "BeyondNetwork#2": "Main Menu", "BeyondNetwork#5": "Title Screen",
      "BeyondNetwork#6": "Finale" },
    unused: ["BeyondNetwork#3", "BeyondNetwork#4", "BeyondNetwork#7"]
  },
  bejeweledtwist: {
    files: ["music/*.mo3"],
    cuts: { "bejewel twist 06.10#3": [72], "bejewel twist 06.10#7": [73], "bejeweledtwist24#0": [24] },
    skip: ["bejewel twist 06.10#3@72"],
    where: { "bejewel twist 06.10#0": "In-Game 1", "bejewel twist 06.10#1": "In-Game 2", "bejewel twist 06.10#2": "In-Game 3",
      "bejewel twist 06.10#6": "In-Game 4", "bejewel twist 06.10#7@73": "In-Game 5", "bejewel twist 06.10#8": "In-Game 6", "bejewel twist 06.10#7": "Loading",
      "bejewel twist 06.10#3": "Game Over", "bejewel twist 06.10#4": "Victory", "bejewel twist 06.10#5": "Main Menu",
      "bejeweledtwist24#0": "Blitz", "bejeweledtwist24#0@24": "Blitz 3 Minutes", "bejeweledtwist24#1": "Blitz Results 1", "bejeweledtwist24#2": "Blitz Results 2",
      "bejeweledtwist24#3": "Blitz Results 3", "bejeweledtwist24#4": "Challenge Menu", "bejeweledtwist24#5": "Challenge 1",
      "bejeweledtwist24#6": "Challenge 2", "bejeweledtwist24#7": "Bonus" },
    unused: ["bejewel twist 06.10#9", "bejewel twist 06.10#10", "bejewel twist 06.10#11", "bejewel twist 06.10#12",
      "bejewel twist 06.10#13", "bejeweledtwist24#8"]
  },
  bigmoney: {
    files: ["music/*.it"],
    cuts: { "QualityTime_BigMoney#1": [24], "QualityTime_BigMoney#3": [41], "QualityTime_BigMoney#5": [64] },
    where: { "QualityTime_BigMoney#0": "Puzzle Mode", "QualityTime_BigMoney#1": "Puzzle Level Up", "QualityTime_BigMoney#1@24": "Puzzle Pause",
      "QualityTime_BigMoney#2": "Action Mode", "QualityTime_BigMoney#3": "Action Level Up", "QualityTime_BigMoney#3@41": "Action Pause",
      "QualityTime_BigMoney#4": "Strategy Mode", "QualityTime_BigMoney#5": "Strategy Level Up", "QualityTime_BigMoney#5@64": "Strategy Pause",
      "QualityTime_BigMoney#6": "Game Over", "QualityTime_BigMoney#7": "Title Screen" }
  },
  bookworm: {
    files: ["music/*.mo3"],
    where: { "bookworm1#0": "Classic Mode", "bookworm1#1": "Title Screen", "bookworm1#2": "Action Mode",
      "bookworm1#3": "Danger 1", "bookworm1#4": "Hall of Fame", "bookworm1#5": "Game Over 2", "bookworm1#6": "Game Over 1",
      "bookworm1#7": "Danger 2" }
  },
  // Bookworm Adventures sets its music's amplify to 12.0, which BASS holds at 100
  bookwormadv: {
    files: ["sounds/music.mo3"],
    level: { bassAmplify: 100 },
    where: { "music#0": "Greek Battle", "music#1": "Greek Map", "music#2": "Greek Boss", "music#3": "Level Complete",
      "music#4": "Boss Defeated", "music#5": "Arabian Battle", "music#6": "Arabian Map", "music#7": "Arabian Boss",
      "music#8": "Credits", "music#9": "Monster Battle", "music#10": "Monster Map", "music#11": "Monster Boss",
      "music#12": "Main Menu", "music#14": "Mini-Games", "music#15": "Game Over" },
    unused: ["music#13"]
  },
  // as Water Bugs (its session's reading of CBLevel.cpp, CBMain.cpp and bugs.xml, 2026-11-11): cosmo1 through the logos,
  // title and menus; a board the song of its level set, 0-9 cosmo4, 10-19 cosmo3, 20-49 cosmo2, 50-99 cosmo5, then again by
  // hundreds from cosmo4 at 100; a new song only at a set's first level, and cosmo1 again on leaving a board
  cosmicbugs: {
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Main Menu", "cosmo4#0": "Level Theme 1", "cosmo3#0": "Level Theme 2",
      "cosmo2#0": "Level Theme 3", "cosmo5#0": "Level Theme 4" }
  },
  // the menus play cosmo1; each set of levels its own song, the sets' songs repeating (its session's reading, 2026-11-11):
  // 0-9 cosmo4, 10-19 cosmo3, 20-49 cosmo2, 50-99 cosmo5, then again from cosmo4 at 100 (Level_SetBackground's own song
  // table is never played)
  waterbugs: {
    files: ["audio/*.mo3"],
    where: { "cosmo1#0": "Main Menu", "cosmo4#0": "Level Theme 1", "cosmo3#0": "Level Theme 2",
      "cosmo2#0": "Level Theme 3", "cosmo5#0": "Level Theme 4" }
  },
  dynomite: {
    files: ["music/*.it"],
    cuts: { "amazonas#2": [64] },
    // the places from the Dynomite session (2026-11-11): #2@64 is where the won challenge starts, and the game over (#2) and
    // Time Trial's clear (#4) jump into it; #7 is Endless (the port's DynomitePanic); #9, two seconds, nothing starts or jumps to
    where: { "amazonas#0": "Fossil Challenge", "amazonas#1": "Title Screen", "amazonas#2": "Game Over", "amazonas#4": "Time Trial Clear",
      "amazonas#5": "Game Start", "amazonas#6": "Time Trial", "amazonas#2@64": "Challenge Won", "amazonas#7": "Endless", "amazonas#8": "Museum",
      "omniphilia#0": "Stomped" },
    unused: ["amazonas#3", "amazonas#9"]
  },
  sevenseas: {
    files: ["music/*.it"],
    where: { "game#0": "In-Game", "game#1": "Level Complete", "game#2": "Game Over", "game#3": "Treasure Bonus",
      "game#4": "Bonus End", "game#5": "Tutorial", "game#6": "Title Screen", "game#7": "Intro", "game#8": "High Scores",
      "game#9": "Bonus Level" },
    unused: ["game#10"]
  },
  // Typer Shark's one module (wordshar.mo3) holds every tune, which the game starts at an order of it (ChangeMusic); its subsongs stand for them
  typershark: {
    files: ["music/*.mo3"],
    cuts: { "wordshar#1": [20] },
    where: { "wordshar#0": "In-Game", "wordshar#1": "Main Menu", "wordshar#1@20": "Map", "wordshar#2": "Game Over", "wordshar#3": "Level Complete",
      "wordshar#4": "Danger 1", "wordshar#6": "Danger 2", "wordshar#7": "Treasure", "wordshar#8": "Boss Fight" },
    unused: ["wordshar#5"]
  },
  mummymaze: {
    files: ["music/*.it"],
    cuts: { "game#9": [67], "game#10": [77] },
    where: { "game#0": "Main Menu", "game#1": "Tutorial", "game#2": "In-Game", "game#3": "Stage Clear",
      "game#5": "Chamber End 1", "game#6": "Chamber End 2", "game#7": "Victory", "game#9@67": "Map", "game#10@77": "High Scores" },
    unused: ["game#9", "game#10"]
  },
  // tunes inside the games' own files: the Mahjong Escapes keep their MO3 modules among Resource.dll's resources (the game's
  // module, 339, starts at one of two orders by the player's progress and jumps to others while gold tiles are in play), Ning
  // Po its one music stream and three jingles in its Python archive; the layout select's tune turns with each visit (LayoutSelect's
  // gMusicTurn): China 338, 336, 337, Japan 337, 336, 337, numbered as the game reaches them
  mahjongchina: {
    inside: [{ file: "resource.dll", magic: "MO3" }],
    chains: { "Resource 339#0": ["Resource 339#1", "Resource 339#2", "Resource 339#3"] },
    where: { "Resource 335#0": "Main Menu", "Resource 338#0": "Layout Select 1", "Resource 336#0": "Layout Select 2",
      "Resource 337#0": "Layout Select 3", "Resource 339#0": "In-Game" }
  },
  mahjongjapan: {
    inside: [{ file: "resource.dll", magic: "MO3" }],
    chains: { "Resource 339#0": ["Resource 339#1", "Resource 339#2", "Resource 339#3"] },
    where: { "Resource 335#0": "Main Menu", "Resource 337#0": "Layout Select 1", "Resource 336#0": "Layout Select 2",
      "Resource 339#0": "In-Game" }
  },
  // Word Harmony's music is made as it plays, from WAV clips: the menu's loop, and in the rising-water mode one drum track
  // stepping through the intense pieces as the water rises, intense-between its bridge from each to the next (which piece goes
  // with which height is not known), here as one tune, each piece once; its other modes pick single instrument samples at
  // random, which are not tunes and are left out, as is intense-levelend (a sound); intenseEnd is the game over's jingle (Game::gameOver)
  wordharmony: {
    files: ["res/music/menuMusic.wav", "res/music/intenseEnd.wav"],
    joined: [{ id: "intense", pieces: ["res/music/intense0.wav", "res/music/intense-between.wav", "res/music/intense1.wav",
      "res/music/intense-between.wav", "res/music/intense1b.wav", "res/music/intense-between.wav", "res/music/intense2.wav",
      "res/music/intense-between.wav", "res/music/intense3.wav", "res/music/intense-between.wav", "res/music/intense4.wav",
      "res/music/intense-between.wav", "res/music/intense5.wav", "res/music/intense-between.wav", "res/music/intense6.wav"] }],
    where: { menuMusic: "Main Menu", intense: "Rising Water", intenseEnd: "Game Over" }
  },
  ningpo: { inside: [{ file: "ningpo.pyz", zip: ["theme_np.ogg", "nm_splash.ogg", "nm_gameover.ogg", "nm_youwin.ogg"] }],
    where: { theme_np: "In-Game", nm_splash: "Splash Screen", nm_gameover: "Game Over", nm_youwin: "Victory" } },
  // the Ogg games
  bookwormadv2: {
    files: ["music/*.ogg"],
    where: { chinese_map: "Chinese Map", chinese_level: "Chinese Levels", chinese_boss: "Chinese Boss",
      fairy_tale_map: "Fairy Tale Map", fairy_tale_level: "Fairy Tale Levels", fairy_tale_boss: "Fairy Tale Boss",
      scifi_map: "Sci-Fi Map", scifi_level: "Sci-Fi Levels", scifi_boss: "Sci-Fi Boss", dance_battle_level: "Dance Battle" }
  },
  // Noah's Ark: mainintro plays once as play starts and runs on into mainloop, which loops: one tune, repeating from the loop
  noahsark: {
    files: ["music/loading.ogg", "music/gameover.ogg"],
    joined: [{ id: "mainloop", pieces: ["music/mainintro.ogg", "music/mainloop.ogg"], loopFrom: 1 }],
    where: { mainloop: "In-Game", loading: "Title Screen", gameover: "Game Over" }
  },
  // Bonnie's Bookstore plays its one module as tracks, each a range of orders (its track table, games/bonnies/game/Music.cpp:
  // the first for the menu, the rest in a game), and a logo tune
  bonnies: {
    files: ["sound/logo.ogg", "sound/music.oxm"],
    level: { fmod3XM: true },
    parts: { music: [[1, 18], [20, 52], [54, 82], [84, 99], [101, 126], [128, 150], [152, 168], [170, 196], [198, 220]] },
    where: { logo: "Logo", "music#0": "Main Menu", "music#1": "In-Game 1", "music#2": "In-Game 2", "music#3": "In-Game 3",
      "music#4": "In-Game 4", "music#5": "In-Game 5", "music#6": "In-Game 6", "music#7": "In-Game 7", "music#8": "In-Game 8" }
  },
  // Pixelus and Rocket Mania join their in-game (and Rocket Mania its menu) music from pieces by a playlist, as the games do
  pixelus: {
    files: ["music/pixelus *.ogg"],
    joined: [{ playlist: "music/pixelus in-game playlist.txt", piece: "music/{}.ogg" }],
    where: { "pixelus main menu loop v02": "Main Menu", "pixelus story screen loop v01": "Story Screen",
      "pixelus temple screen loop v01": "Temple Screen", "pixelus in-game": "In-Game" }
  },
  rocketmania: {
    files: ["music/* loop *.ogg"],
    joined: [{ playlist: "music/tubetricks menu one v02 playlist.txt", piece: "music/tubetricks menu one v02 {}.ogg" },
      { playlist: "music/tubetricks in-game one v02 playlist.txt", piece: "music/tubetricks in-game one v02 {}.ogg" }],
    where: { "tubetricks menu one v02": "Main Menu", "tubetricks in-game one v02": "In-Game",
      "tubetricks level up loop one v01": "Level Complete", "tubetricks game over loop one v01": "Game Over" }
  }
};
