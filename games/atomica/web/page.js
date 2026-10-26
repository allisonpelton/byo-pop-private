  // Atomica Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "atomica",
    shortName: "Atomica Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Atomica, Noah's Ark, BookWorm, Big Money
    // and Bejeweled can be served from the same site without sharing files or saves
    persist: "/atpersist",
    lock: "atomica-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music"],
    // Atomica.exe (the game's texts are read from it; it is never run) sits at the top of the game folder
    gameFiles: ["atomica.exe"],
    markers: ["music/atomica.it", "images/titlescreen.jpg", "atomica.exe"],
    markerText: "music/atomica.it, images/titlescreen.jpg and Atomica.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Atomica Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music</span> ' +
      'folders and <span class="mono">Atomica.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/titlescreen.jpg), plus an optional mod.json.",
    seeds: false
  };
