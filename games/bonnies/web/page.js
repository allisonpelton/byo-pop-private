  // Bonnie's Bookstore Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bonnies",
    shortName: "Bonnie's Bookstore Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Bonnie's and the other games can
    // be served from the same site without sharing files or saves
    persist: "/bbpersist",
    lock: "bonnies-bookstore-deluxe-instance",
    // The game's pictures and sounds, its pak, dictionary and texts; BonniesBookstore.exe holds the game's tables (it is
    // read, never run)
    gameDirs: ["data", "graphics", "sound"],
    gameFiles: ["bonniesbookstore.exe"],
    markers: ["bonniesbookstore.exe", "data/bonnie.pak"],
    markerText: "BonniesBookstore.exe and Data/Bonnie.pak",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Bonnie\'s Bookstore Deluxe</span>) or a .zip of it. The <span class="mono">Data</span>, ' +
      '<span class="mono">Graphics</span> and <span class="mono">Sound</span> folders and ' +
      '<span class="mono">BonniesBookstore.exe</span> (the game\'s tables are read from it; it is never run) are copied.',
    modsIntro: "Its mods are a mod.json of settings: a mod cannot replace the game's pictures or sounds yet. The fixes mod " +
      "is on by default; turn it off to play the original as it was.",
    seeds: false
  };
