  // Ning Po Mahjong Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "ningpo",
    shortName: "Ning Po Mahjong Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Ning Po and the other games can
    // be served from the same site without sharing files or saves
    persist: "/nppersist",
    lock: "ningpo-mahjong-deluxe-instance",
    // Ning Po keeps its pictures, sounds, music and layouts in one file, ningpo.pyz; Ningpo.exe holds the texts
    gameDirs: [],
    gameFiles: ["ningpo.pyz", "ningpo.exe"],
    markers: ["ningpo.pyz", "ningpo.exe"],
    markerText: "ningpo.pyz and Ningpo.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/NingPo MahJong Deluxe</span>) or a .zip of it. Only <span class="mono">ningpo.pyz</span> and ' +
      '<span class="mono">Ningpo.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Ning Po keeps its pictures, sounds and music inside ningpo.pyz, and this port does not load mods yet.",
    seeds: false
  };
