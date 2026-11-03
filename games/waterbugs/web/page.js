  // Water Bugs' settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "waterbugs",
    shortName: "Water Bugs",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Water Bugs and the other games
    // can be served from the same site without sharing files or saves
    persist: "/wbpersist",
    lock: "water-bugs-instance",
    // Retro64's engine reads its pictures and fonts from the art banks, its boards and screens from the XML files
    // in wanims/ (with the FLIC animations), its sounds and music from audio/, and the texts from WaterBugs.exe
    gameDirs: ["wanims", "audio"],
    gameFiles: ["water_art.bbk", "water_art.dat", "water_art8.bbk", "water_art8.dat", "waterbugs.exe"],
    markers: ["water_art.bbk", "water_art8.bbk", "waterbugs.exe"],
    markerText: "water_art.bbk, water_art8.bbk and WaterBugs.exe",
    // The developer's tools left in the install
    skip: [/^wanims\/[^\/]*\.(exe|bat)$/i],
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Water Bugs</span>) or a .zip of it. The art banks, <span class="mono">wanims</span>, ' +
      '<span class="mono">audio</span> and <span class="mono">WaterBugs.exe</span> (the game\'s texts are read ' +
      'from it; it is never run) are copied.',
    modsIntro: "Water Bugs keeps its pictures in its art banks, and this port does not load mods yet.",
    seeds: false
  };
