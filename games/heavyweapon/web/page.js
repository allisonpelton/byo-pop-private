  // Heavy Weapon Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "heavyweapon",
    shortName: "Heavy Weapon",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/hwpersist",
    lock: "heavy-weapon-deluxe-instance",
    // The game's exe sits at the top of the install: the game's texts are read from it, and it is never run
    gameDirs: ["data", "fonts", "images", "music", "properties", "sounds"],
    gameFiles: ["heavy weapon deluxe.exe", "heavyweapon.exe"],
    markers: ["music/atomictank.mo3", "images/intro.swf"],
    markerText: "music/atomictank.mo3 and images/intro.swf",
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Heavy Weapon Deluxe</span>; for Steam: <span class="mono">steamapps/common/Heavy Weapon Deluxe</span>) or ' +
      'a .zip of it. Only the <span class="mono">data, fonts, images, music, properties</span> and ' +
      '<span class="mono">sounds</span> folders and the game\'s exe (its texts are read from it; it is never run) ' +
      'are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses, plus an " +
      "optional mod.json.",
    seeds: false
  };
