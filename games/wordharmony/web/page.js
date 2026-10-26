  // Word Harmony's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "wordharmony",
    shortName: "Word Harmony",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Word Harmony and the other games
    // can be served from the same site without sharing files or saves
    persist: "/whpersist",
    lock: "word-harmony-instance",
    // WHArchive.saf holds the pictures, fonts, screens, levels and word lists; res/ the sounds, the music and the
    // splash; WordHarmony.exe the texts
    gameDirs: ["res"],
    gameFiles: ["WHArchive.saf", "WordHarmony.exe"],
    markers: ["WHArchive.saf", "WordHarmony.exe"],
    markerText: "WHArchive.saf and WordHarmony.exe",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Word Harmony</span>) or a .zip of it. Only <span class="mono">WHArchive.saf</span>, the ' +
      '<span class="mono">res</span> folder and <span class="mono">WordHarmony.exe</span> (the game\'s texts are ' +
      'read from it; it is never run) are copied.',
    modsIntro: "Word Harmony keeps its pictures and screens inside WHArchive.saf, and this port does not load mods yet.",
    seeds: false
  };
