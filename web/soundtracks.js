// The music of each game, for the listing page's music player (web/music.js; docs/WEB_PLAYER.md, "The music player"), by the
// game's id. files: the module files that hold its tunes, as paths in the game's folder as the import writes them (folders in
// lower case), where * stands for any part of a file name; each file's subsongs are its tunes, those under two seconds left out
// (the silences and stings between tunes). Where a game's split comes out wrong, or a tune needs a name, a game's entry may add
// skip: ["<file>#<n>", ...], subsongs that are not tunes, and names: {"<file>#<n>": "<name>"}, plain names, never invented
// titles and never the game's own text at length. Every game's tunes were listed from the user's copies on 2026-11-07.
var ByopopSoundtracks = {
  alchemy: { files: ["music/*.s3m"] },
  atomica: { files: ["music/*.it"] },
  bejeweled: { files: ["music/*.s3m"] },
  bejeweledtwist: { files: ["music/*.mo3"] },
  bigmoney: { files: ["music/*.it"] },
  bookworm: { files: ["music/*.mo3"] },
  bookwormadv: { files: ["sounds/music.mo3"] },
  cosmicbugs: { files: ["audio/*.mo3"] },
  waterbugs: { files: ["audio/*.mo3"] },
  dynomite: { files: ["music/*.it"] },
  sevenseas: { files: ["music/*.it"] },
  mummymaze: { files: ["music/*.it"] }
};
