  // Feeding Frenzy's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "feedingfrenzy",
    shortName: "Feeding Frenzy",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Feeding Frenzy and the other games
    // can be served from the same site without sharing files or saves
    persist: "/ffpersist",
    lock: "feeding-frenzy-instance",
    // FFArchive.saf holds the pictures, fonts, screens, stages and fish; resources/ the sounds and music, properties/ the
    // game's own settings files; FeedingFrenzy.exe the texts
    gameDirs: ["properties", "resources"],
    gameFiles: ["ffarchive.saf", "feedingfrenzy.exe"],
    markers: ["FFArchive.saf", "FeedingFrenzy.exe"],
    markerText: "FFArchive.saf and FeedingFrenzy.exe",
    importHtml: 'Choose the game\'s install folder (for the PopCap Game Collection: <span class="mono">Popcap Game ' +
      'Collection/Feeding Frenzy Deluxe</span>) or a .zip of it. Only <span class="mono">FFArchive.saf</span>, the ' +
      '<span class="mono">resources</span> and <span class="mono">properties</span> folders and ' +
      '<span class="mono">FeedingFrenzy.exe</span> (the game\'s texts are read from it; it is never run) are copied.',
    modsIntro: "Feeding Frenzy has no mods yet.",
    seeds: false
  };
