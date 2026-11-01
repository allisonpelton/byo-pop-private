  // Bejeweled Twist's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bejeweledtwist",
    shortName: "Bejeweled Twist",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/bjtpersist",
    lock: "bejeweled-twist-instance",
    // The pictures, fonts and sounds are packed in main.pak, which the import unpacks into these folders; music/,
    // properties/ and the 3D meshes (*.p3d) are loose. BejeweledTwist.exe holds the game's texts and curves (it is
    // never run).
    gameDirs: ["fonts", "images", "music", "properties", "sounds"],
    gameFiles: ["bejeweledtwist.exe", "crystal1.p3d", "crystal2.p3d", "crystal3.p3d", "crystal4.p3d", "crystal5.p3d",
      "crystal6.p3d", "crystal7.p3d", "outerspace.p3d", "planet1.p3d", "planet2.p3d", "planetrings.p3d", "saucer.p3d",
      "saucerlights.p3d", "sidebar.p3d", "space.p3d", "stars.p3d", "warpcone.p3d"],
    paks: ["main.pak"],
    markers: ["main.pak", "bejeweledtwist.exe"],
    markerText: "main.pak and BejeweledTwist.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Bejeweled Twist</span>; for Steam: <span class="mono">steamapps/common/Bejeweled Twist</span>) or a .zip ' +
      'of it. Only <span class="mono">main.pak</span> (unpacked), the <span class="mono">music</span> and ' +
      '<span class="mono">properties</span> folders, the .p3d meshes and <span class="mono">BejeweledTwist.exe</span> ' +
      '(the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (as " +
      "main.pak holds them), plus an optional mod.json.",
    seeds: false
  };
