  // Rocket Mania Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "rocketmania",
    shortName: "Rocket Mania Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so the games can be served from the
    // same site without sharing files or saves
    persist: "/rmpersist",
    lock: "rocket-mania-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties"],
    // RocketMania.exe (PopCap's wrapper with the game inside; the game's texts are read from it, it is never run) sits at
    // the top of the game folder
    gameFiles: ["rocketmania.exe"],
    markers: ["images/loadinggraphic.jpg", "properties/partner.xml", "rocketmania.exe"],
    markerText: "images/loadinggraphic.jpg, properties/partner.xml and RocketMania.exe",
    importHtml: 'Choose the game\'s install folder (for the Steam version: <span class="mono">steamapps/common/' +
      'Rocket Mania Deluxe</span>) or a .zip of it. Only the <span class="mono">images, data, sounds, music, properties' +
      '</span> folders and <span class="mono">RocketMania.exe</span> (the game\'s texts are read from it; it is never ' +
      'run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/loadinggraphic.jpg), plus an optional mod.json.",
    seeds: false
  };
