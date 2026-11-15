  // Pizza Frenzy's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "pizzafrenzy",
    shortName: "Pizza Frenzy",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so Pizza Frenzy and the other games
    // can be served from the same site without sharing files or saves
    persist: "/pfpersist",
    lock: "pizza-frenzy-instance",
    // PizzaFrenzy.saf holds the pictures, fonts, screens, sounds and manifests; splash/ the splash screen's picture;
    // PizzaFrenzy.exe the texts
    gameDirs: ["splash"],
    gameFiles: ["pizzafrenzy.saf", "pizzafrenzy.exe"],
    markers: ["PizzaFrenzy.saf", "PizzaFrenzy.exe"],
    markerText: "PizzaFrenzy.saf and PizzaFrenzy.exe",
    importHtml: 'Choose the game\'s install folder (for the Steam version: <span class="mono">steamapps/common/Pizza ' +
      'Frenzy</span>) or a .zip of it. Only <span class="mono">PizzaFrenzy.saf</span>, the ' +
      '<span class="mono">splash</span> folder and <span class="mono">PizzaFrenzy.exe</span> (the game\'s texts are ' +
      'read from it; it is never run) are copied.',
    modsIntro: "Pizza Frenzy has no mods yet.",
    seeds: false
  };
