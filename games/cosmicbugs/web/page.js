  // Cosmic Bugs' settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "cosmicbugs",
    shortName: "Cosmic Bugs",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Cosmic Bugs and the other games
    // can be served from the same site without sharing files or saves
    persist: "/cbpersist",
    lock: "cosmic-bugs-instance",
    // Retro64's engine reads its pictures and fonts from the art banks, its boards and screens from the XML files
    // in animations/ (with the FLIC animations), its sounds and music from audio/, and the texts from CosmicBugs.exe
    gameDirs: ["animations", "audio"],
    gameFiles: ["art.bbk", "art.dat", "art8.bbk", "art8.dat", "cosmicbugs.exe"],
    markers: ["art.bbk", "art8.bbk", "cosmicbugs.exe"],
    markerText: "art.bbk, art8.bbk and CosmicBugs.exe",
    // The developer's tools left in the install
    skip: [/^animations\/[^\/]*\.(exe|bat)$/i],
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Cosmic Bugs</span>) or a .zip of it. The art banks, <span class="mono">animations</span>, ' +
      '<span class="mono">audio</span> and <span class="mono">CosmicBugs.exe</span> (the game\'s texts are read ' +
      'from it; it is never run) are copied.',
    modsIntro: "Cosmic Bugs keeps its pictures in its art banks, so a mod cannot replace them yet; its mods are a mod.json of settings. " +
      "The fixes mod is on by default; turn it off to play the original as it was.",
    seeds: false
  };
