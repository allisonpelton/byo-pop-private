  // TipTop Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "tiptop",
    shortName: "TipTop Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so TipTop and the other games can be
    // served from the same site without sharing files or saves
    persist: "/ttpersist",
    lock: "tiptop-deluxe-instance",
    // TipTop keeps its pictures, sounds, music and settings in one file, TipTop.dat; TipTop.exe holds the texts
    gameDirs: [],
    gameFiles: ["tiptop.dat", "tiptop.exe"],
    markers: ["tiptop.dat", "tiptop.exe"],
    markerText: "TipTop.dat and TipTop.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/TipTop Deluxe</span>) or a .zip of it. Only <span class="mono">TipTop.dat</span> and ' +
      '<span class="mono">TipTop.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "TipTop keeps its pictures, sounds and music inside TipTop.dat, and this port does not load mods yet.",
    seeds: false
  };
