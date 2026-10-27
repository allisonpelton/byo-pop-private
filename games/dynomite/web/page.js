  // Dynomite Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "dynomite",
    shortName: "Dynomite Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/dypersist",
    lock: "dynomite-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties", "stomped"],
    // DEFAULT.EGG holds the puzzles; Dynomite.exe is read for a few of the game's texts and is never run
    gameFiles: ["default.egg", "dynomite.exe"],
    markers: ["properties/default.xml", "music/amazonas.it", "images/title.jpg", "default.egg"],
    markerText: "properties/default.xml, music/amazonas.it, images/title.jpg and DEFAULT.EGG",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Dynomite Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music, ' +
      'properties, stomped</span> folders, <span class="mono">DEFAULT.EGG</span> and <span class="mono">Dynomite.exe' +
      '</span> (a few of the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/title.jpg), plus an optional mod.json.",
    seeds: false
  };
