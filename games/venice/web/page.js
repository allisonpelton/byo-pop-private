  // Venice Deluxe's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "venice",
    shortName: "Venice Deluxe",
    // Its own IndexedDB folder (IDBFS names the database after the mount point; src/main.cpp uses the same), so the
    // games served from the same site never share files or saves
    persist: "/venicepersist",
    lock: "venice-deluxe-instance",
    // The pictures, sounds, level files and object files are packed in main.pak, which the import unpacks into these
    // folders; config.xml (the game's rules), developer_logo.png and properties/partner.xml are loose. Venice.exe is the
    // DRM wrapper: the game never reads it, so it is not copied.
    gameDirs: ["audio", "gfx", "levels", "objects", "properties"],
    gameFiles: ["config.xml", "developer_logo.png"],
    paks: ["main.pak"],
    markers: ["main.pak", "config.xml"],
    markerText: "main.pak and config.xml",
    // the game stores no exe: the import reads its icon out of the install's exe (web/shell.html, importIcon)
    importIcon: true,
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/Venice Deluxe</span>; for Steam: <span class="mono">steamapps/common/Venice Deluxe</span>) or a .zip of ' +
      'it. Only <span class="mono">main.pak</span> (unpacked), <span class="mono">config.xml</span>, ' +
      '<span class="mono">developer_logo.png</span> and the <span class="mono">properties</span> folder are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or level files in the same folders the game uses (as " +
      "main.pak holds them), plus an optional mod.json.",
    seeds: false
  };
