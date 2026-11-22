  // Banana Bugs' settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bananabugs",
    shortName: "Banana Bugs",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/bananabugspersist",
    lock: "banana-bugs-instance",
    // The pictures and a font are packed in main.pak, which the import unpacks into these folders; the sounds, the music
    // and the resource manifest are loose (games/bananabugs/tools/stage_bananabugs_assets.py stages the same).
    // BananaBugs.exe is the 2010 DRM launcher and the game never reads it, so it is not copied.
    gameDirs: ["fonts", "images", "properties", "sounds"],
    gameFiles: [],
    paks: ["main.pak"],
    markers: ["main.pak", "sounds/mainmenu.ogg"],
    markerText: "main.pak and sounds/mainmenu.ogg",
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Banana Bugs</span>) or a .zip of it. Only <span class="mono">main.pak</span> (unpacked), the ' +
      '<span class="mono">sounds</span> folder and the <span class="mono">properties</span> folder are copied.',
    modsIntro: "Mods replace game files: images, sounds or music in the same folders the game uses (as main.pak holds " +
      "them), plus an optional mod.json.",
    seeds: false
  };
