  // Pixelus Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "pixelus",
    shortName: "Pixelus Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so the games can be served from
    // the same site without sharing files or saves
    persist: "/pxpersist",
    lock: "pixelus-deluxe-instance",
    gameDirs: ["images", "data", "levels", "sounds", "music", "properties", "ncframework"],
    // Pixelus.exe (the game's texts are read from it; it is never run) sits at the top of the game folder
    gameFiles: ["pixelus.exe"],
    markers: ["levels/levels.txt", "images/playfield.jpg", "pixelus.exe"],
    markerText: "levels/levels.txt, images/playfield.jpg and Pixelus.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Pixelus Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, levels, sounds, music, ' +
      'properties, NcFramework</span> folders and <span class="mono">Pixelus.exe</span> (the game\'s texts are read from ' +
      'it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or levels in the same folders the game uses (for example " +
      "levels/levels.txt), plus an optional mod.json.",
    seeds: false
  };
