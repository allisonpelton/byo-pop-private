  // Mahjong Escape: Ancient Japan's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "mahjongjapan",
    shortName: "Mahjong Escape: Ancient Japan",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so this game and the others can be
    // served from the same site without sharing files or saves
    persist: "/mjpersist",
    lock: "mahjongjapan-instance",
    // Resource.dll holds the pictures, sounds, music, layouts and texts; MahjongEscapeAJ.exe the rest of the texts.
    // SplashLogo and MenuLogo are the publisher's pictures beside them.
    gameDirs: [],
    gameFiles: ["resource.dll", "mahjongescapeaj.exe", "splashlogo.bmp", "menulogo.bmp"],
    markers: ["resource.dll", "mahjongescapeaj.exe"],
    markerText: "Resource.dll and MahjongEscapeAJ.exe",
    importHtml: 'Choose the game\'s install folder (in the PopCap Game Collection: <span class="mono">Popcap Game ' +
      'Collection/Mahjong Escape Ancient Japan</span>) or a .zip of it. Only <span class="mono">Resource.dll</span>, ' +
      '<span class="mono">MahjongEscapeAJ.exe</span> (read for its resources; it is never run) and the two logos ' +
      'are copied.',
    modsIntro: "Its mods are a mod.json of settings (the pictures, sounds and music stay in Resource.dll). The fixes mod is on " +
      "by default; turn it off to play the original as it was.",
    seeds: true
  };
