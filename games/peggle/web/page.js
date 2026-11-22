  // Peggle Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "peggle",
    shortName: "Peggle Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/peggledeluxepersist",
    lock: "peggle-deluxe-instance",
    // The pictures, sounds, characters, level files and fonts are packed in main.pak, which the import unpacks into these
    // folders; the music and the distributor's settings are loose. Peggle.exe is the DRM wrapper: the game never reads
    // it, so it is not copied.
    gameDirs: ["characters", "fonts", "images", "leveleditor", "levels", "music", "properties", "sounds", "trailer"],
    gameFiles: [],
    paks: ["main.pak"],
    markers: ["main.pak", "music/pegglebeats.mo3"],
    markerText: "main.pak and music/PeggleBeats.mo3",
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Peggle Deluxe</span>; for Steam: <span class="mono">steamapps/common/Peggle Deluxe</span>) or a .zip of ' +
      'it. Only <span class="mono">main.pak</span> (unpacked), the <span class="mono">music</span> folder and the ' +
      '<span class="mono">properties</span> folder are copied.',
    modsIntro: "Mods replace game files: images, sounds, music, levels or characters in the same folders the game uses " +
      "(as main.pak holds them), plus an optional mod.json.",
    seeds: false
  };
