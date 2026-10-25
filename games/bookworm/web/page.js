  // BookWorm's settings for the shared page (web/shell.html lists the fields; docs/WEB_PLAYER.md)
  var PAGE = {
    id: "bookworm",
    shortName: "BookWorm",
    // Its own IndexedDB folder (IDBFS names the database after the mount point), so BookWorm, Big Money and
    // Bejeweled can be served from the same site without sharing files or saves
    persist: "/bwpersist",
    lock: "bookworm-deluxe-instance",
    gameDirs: ["images", "fonts", "sounds", "music", "properties"],
    // The word files and Bookworm.exe (the game's texts are read from it; it is never run) sit at the top of the
    // game folder
    gameFiles: ["wordlist.txt", "bonus_word_dictionary.txt", "popdefs.txt", "bookworm.exe"],
    markers: ["music/bookworm1.mo3", "wordlist.txt", "bookworm.exe"],
    markerText: "wordlist.txt, music/Bookworm1.mo3 and Bookworm.exe",
    // the original's runtime sound cache
    skip: [/^sounds\/cached_/i],
    importHtml: 'Choose the game\'s install folder (for the retail version: <span class="mono">Program Files/PopCap ' +
      'Games/BookWorm Deluxe</span>) or a .zip of it. Only the <span class="mono">images, fonts, sounds, music</span> and ' +
      '<span class="mono">properties</span> folders, the word files (<span class="mono">wordlist.txt, ' +
      'bonus_word_dictionary.txt, popdefs.txt</span>) and <span class="mono">Bookworm.exe</span> (the game\'s texts are ' +
      'read from it; it is never run) are copied.',
    modsIntro: "Mods replace game files: images, sounds, music or data in the same folders the game uses (for example " +
      "images/backdrop_board.gif, or images/backdrop_board@2x.png for an HD version, or a wordlist.txt with another " +
      "dictionary), plus an optional mod.json.",
    seeds: false,
    replays: false,
    // The game writes players/<name>/classic.dat or action.dat (classic-<key>.dat... with mods that change play)
    // only for a game in progress
    hasSave: function () {
      var fs = web().fs;
      return fs.listDir(USER + "/players").some(function (p) {
        return fs.listDir(USER + "/players/" + p).some(function (n) { return /^(classic|action).*\.dat$/i.test(n); });
      });
    }
  };
