  // Alchemy Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "alchemy",
    shortName: "Alchemy Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Alchemy, Atomica, Noah's Ark,
    // BookWorm, Big Money and Bejeweled can be served from the same site without sharing files or saves
    persist: "/alpersist",
    lock: "alchemy-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties"],
    // The game's texts are in properties/default.xml; Alchemy.exe is not needed
    gameFiles: [],
    markers: ["properties/default.xml", "music/alch_intro.s3m", "images/title.jpg"],
    markerText: "properties/default.xml, music/alch_intro.s3m and images/title.jpg",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Alchemy Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music, ' +
      'properties</span> folders are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/title.jpg), plus an optional mod.json.",
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    seeds: false
  };
