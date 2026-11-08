  // Bejeweled 2 Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bejeweled2",
    shortName: "Bejeweled 2",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/bej2persist",
    lock: "bejeweled-2-instance",
    gameDirs: ["images", "data", "edata", "sounds", "music", "properties", "puzzles"],
    markers: ["properties/resources.xml", "music/beyondnetwork.mo3"],
    markerText: "properties/resources.xml and music/BeyondNetwork.mo3",
    importHtml: 'Choose the game\'s install folder (for Steam: <span class="mono">steamapps/common/Bejeweled 2 Deluxe</span>) ' +
      'or a .zip of it. Only the <span class="mono">images, data, edata, sounds, music, properties</span> and ' +
      '<span class="mono">puzzles</span> folders are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses, plus an " +
      "optional mod.json.",
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    seeds: false
  };
