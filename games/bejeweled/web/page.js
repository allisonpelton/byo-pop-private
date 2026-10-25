  // Bejeweled's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bejeweled",
    shortName: "Bejeweled",
    // The first port's folder and lock, kept so its players keep their saves
    persist: "/persist",
    lock: "bejeweled-deluxe-instance",
    gameDirs: ["images", "data", "sounds", "music", "properties"],
    markers: ["images/gemsheet6.*"],
    markerText: "images/gemsheet6 file",
    importHtml: 'Choose the game\'s install folder (for Steam: <span class="mono">steamapps/common/Bejeweled Deluxe</span>) or a ' +
      '.zip of it. Only the <span class="mono">images, data, sounds, music</span> and <span class="mono">properties</span> ' +
      'folders are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/gemsheet6.png, or images/gemsheet6@2x.png for an HD version), plus an optional mod.json.",
    // ?seed=N: the seed of every new game until one is played on it (its first match or its game over;
    // games/bejeweled/docs/CODE_MODS_DESIGN.md 4.10)
    seeds: true,
    // finished replays (features.replays)
    replays: true
  };
