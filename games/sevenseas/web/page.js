  // Seven Seas Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "sevenseas",
    shortName: "Seven Seas Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so the games can be served from
    // the same site without sharing files or saves
    persist: "/sspersist",
    lock: "seven-seas-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music"],
    // WinSS.exe (the game's texts are read from it; it is never run) sits at the top of the game folder
    gameFiles: ["winss.exe"],
    markers: ["music/game.it", "images/title.jpg", "winss.exe"],
    markerText: "music/game.it, images/title.jpg and WinSS.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Seven Seas Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music</span> ' +
      'folders and <span class="mono">WinSS.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/title.jpg), plus an optional mod.json.",
    seeds: false
  };
