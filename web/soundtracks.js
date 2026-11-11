// The music of each game, for the listing page's music player (web/music.js; docs/WEB_PLAYER.md, "The music player"), by the
// game's id. Paths are in the game's folder as the import writes it, matched without regard to case, where * stands for any part
// of a file name.
//   files   the files that are tunes: a module's subsongs are its tunes, an Ogg Vorbis file is one tune
//   parts   for a module the game plays in ranges of its orders: {"<file name>": [[first order, last order], ...]}, its tunes
//           instead of its subsongs
//   joined  tunes the game joins from pieces by a playlist: [{playlist: "<path>", piece: "<path with {} for the playlist's word>"}]
//   skip    ["<file name>#<n>", ...]: tunes that are not tunes (n counts from 0 in the file's subsongs or parts)
//   names   {"<file name>#<n>" or "<playlist name>": "<name>"}: plain names, never invented titles and never the game's own text
//           at length; without one a tune is named after its file
//   where   {"<file name>#<n>", "<file name>" (an Ogg file) or "<playlist name>": "<place>"}: where in the game its code plays
//           the tune, as a short value ("Main menu", "Levels 1-5"), from the game's code (its session's answer) or, for an Ogg
//           game, its file's name; shown beside the tune and in the display
//   unused  ["<file name>#<n>", ...]: tunes the game's code never plays (left in its files), shown as such
//   level   the level the game plays its modules at, where it is not the renderer's own (platform/MusicLevels.h):
//           {bassAmplify: n} the BASS amplify it sets (0 to 100), {fmod3XM: true} FMOD 3's XM pre-amp. The renderer is the
//           game's own (libopenmpt for MO3 and OXM, libxmp-lite for the rest: web/music/openmpt_web.c)
// Tunes under two seconds are left out (the silences and stings between tunes). Every game's tunes were listed from the user's
// copies on 2026-11-07.
var ByopopSoundtracks = {
  alchemy: { files: ["music/*.s3m"] },
  // AstroPop's one module holds every tune, which the game starts at an order of it (PlaySong); its subsongs stand for them
  astropop: { files: ["music/*.mo3"] },
  atomica: { files: ["music/*.it"] },
  bejeweled: { files: ["music/*.s3m"] },
  // one module, BeyondNetwork.mo3: the game plays it from a starting order for each screen (properties/music.xml), so its
  // subsongs are the tunes
  bejeweled2: { files: ["music/*.mo3"] },
  bejeweledtwist: { files: ["music/*.mo3"] },
  bigmoney: { files: ["music/*.it"] },
  bookworm: { files: ["music/*.mo3"] },
  // Bookworm Adventures sets its music's amplify to 12.0, which BASS holds at 100
  bookwormadv: { files: ["sounds/music.mo3"], level: { bassAmplify: 100 } },
  cosmicbugs: { files: ["audio/*.mo3"] },
  waterbugs: { files: ["audio/*.mo3"] },
  dynomite: { files: ["music/*.it"] },
  sevenseas: { files: ["music/*.it"] },
  // Typer Shark's one module (wordshar.mo3) holds every tune, which the game starts at an order of it (ChangeMusic); its subsongs stand for them
  typershark: { files: ["music/*.mo3"] },
  mummymaze: { files: ["music/*.it"] },
  // the Ogg games
  bookwormadv2: {
    files: ["music/*.ogg"],
    where: { chinese_map: "Chinese world, map", chinese_level: "Chinese world, levels", chinese_boss: "Chinese world, boss",
      fairy_tale_map: "Fairy tale world, map", fairy_tale_level: "Fairy tale world, levels", fairy_tale_boss: "Fairy tale world, boss",
      scifi_map: "Sci-fi world, map", scifi_level: "Sci-fi world, levels", scifi_boss: "Sci-fi world, boss" }
  },
  noahsark: { files: ["music/mainintro.ogg", "music/mainloop.ogg", "music/loading.ogg", "music/gameover.ogg"] },
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
