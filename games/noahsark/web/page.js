  // Noah's Ark Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "noahsark",
    shortName: "Noah's Ark Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Noah's Ark, BookWorm, Big Money
    // and Bejeweled can be served from the same site without sharing files or saves
    persist: "/napersist",
    lock: "noahs-ark-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music"],
    // WinNoah.exe (the game's texts are read from it; it is never run) sits at the top of the game folder
    gameFiles: ["winnoah.exe"],
    markers: ["music/mainloop.ogg", "images/title.jpg", "winnoah.exe"],
    markerText: "music/MainLoop.ogg, images/title.jpg and WinNoah.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Noah\'s Ark Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music</span> ' +
      'folders and <span class="mono">WinNoah.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/title.jpg), plus an optional mod.json.",
    seeds: false
  };
