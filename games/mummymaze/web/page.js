  // Mummy Maze Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "mummymaze",
    shortName: "Mummy Maze Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so the games can be served from
    // the same site without sharing files or saves
    persist: "/mmpersist",
    lock: "mummy-maze-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music"],
    // WinMM.exe (the game's texts are read from it; it is never run) sits at the top of the game folder
    gameFiles: ["winmm.exe"],
    markers: ["music/game.it", "images/title.jpg", "winmm.exe"],
    markerText: "music/game.it, images/title.jpg and WinMM.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Mummy Maze Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music</span> ' +
      'folders and <span class="mono">WinMM.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/title.jpg), plus an optional mod.json.",
    seeds: false
  };
