  // Bejeweled 3's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bejeweled3",
    shortName: "Bejeweled 3",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/bj3persist",
    lock: "bejeweled-3-instance",
    // The pictures, effects, fonts, sounds, music, properties and the 3D meshes at its top (*.p3d, *.p3danim) are packed
    // in main.pak, which the import unpacks; properties/ also has loose files, and Zen mode's affirmations/, ambient/ and
    // binaural/ are loose. Bejeweled3.exe holds the game's texts and tables (it is never run).
    gameDirs: ["affirmations", "ambient", "binaural", "effects", "fonts", "images", "music", "properties", "sounds"],
    gameFiles: ["bejeweled3.exe", "board.p3d", "gemblue.p3d", "gemgreen.p3d", "gemorange.p3d", "gempurple.p3d", "gemred.p3d",
      "gemwhite.p3d", "gemyellow.p3d", "hyperspace_main_0.p3danim", "hyperspace_main_1.p3danim", "hyperspace_main_2.p3danim",
      "hyperspace_main_3.p3danim", "hyperspace_zen_0.p3danim", "questtemple.p3d", "temple.p3d", "warptube.p3d",
      "warptube_cap.p3d"],
    paks: ["main.pak"],
    markers: ["main.pak", "bejeweled3.exe"],
    markerText: "main.pak and Bejeweled3.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Bejeweled 3</span>; for Steam: <span class="mono">steamapps/common/Bejeweled 3</span>) or a .zip of it. ' +
      'Only <span class="mono">main.pak</span> (unpacked), the <span class="mono">properties</span>, ' +
      '<span class="mono">affirmations</span>, <span class="mono">ambient</span> and <span class="mono">binaural</span> ' +
      'folders and <span class="mono">Bejeweled3.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (as " +
      "main.pak holds them), plus an optional mod.json.",
    seeds: false
  };
