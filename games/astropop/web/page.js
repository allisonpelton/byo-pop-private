  // AstroPop Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "astropop",
    shortName: "AstroPop",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/appersist",
    lock: "astropop-deluxe-instance",
    // The pictures, fonts, sounds, music (one module) and level files are loose in the install folder. WinAP.exe (the Steam
    // copy) or AstroPop.exe (the PopCap Game Collection's copy) holds the game's texts (they are read from it; it is never
    // run). Either exe is a marker ("a|b": any one of them).
    gameDirs: ["data", "fonts", "images", "music", "properties", "sounds"],
    gameFiles: ["winap.exe", "astropop.exe"],
    markers: ["data/bonus-1.xcf", "images/_galaxy1.gif", "WinAP.exe|AstroPop.exe"],
    markerText: "data/bonus-1.xcf, images/_galaxy1.gif and WinAP.exe or AstroPop.exe",
    importHtml: 'Choose the game\'s install folder (for the Steam version: <span class="mono">steamapps/common/' +
      'AstroPop Deluxe</span>) or a .zip of it. Only the <span class="mono">data, fonts, images, music, properties, ' +
      'sounds</span> folders and <span class="mono">WinAP.exe</span> (<span class="mono">AstroPop.exe</span> in the ' +
      'PopCap Game Collection\'s copy; the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/_galaxy1.gif), plus an optional mod.json.",
    seeds: false
  };
