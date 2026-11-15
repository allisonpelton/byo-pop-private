  // Hammer Heads Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "hammerheads",
    shortName: "Hammer Heads Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/hammerheadspersist",
    lock: "hammerheads-deluxe-instance",
    // The game's files are loose: the board files, the level texts, the pictures, sounds and fonts, and the resource list.
    // HammerHeads.exe is the DRM launcher (with drm.xml, drm.swf and vhwb.dat): the game never reads it, so it is not copied.
    gameDirs: ["boards", "fonts", "gnomedata", "images", "properties", "sounds"],
    markers: ["boards/board001.xml", "gnomedata/0.txt"],
    markerText: "boards/board001.xml and gnomedata/0.txt",
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    importHtml: 'Choose the game\'s install folder (for Steam: <span class="mono">steamapps/common/Hammer Heads ' +
      'Deluxe</span>) or a .zip of it. Only the <span class="mono">boards, fonts, gnomedata, images, properties</span> ' +
      'and <span class="mono">sounds</span> folders are copied.',
    modsIntro: "Mods replace game files: images, sounds, boards or level texts in the same folders the game uses, plus " +
      "an optional mod.json.",
    seeds: false
  };
