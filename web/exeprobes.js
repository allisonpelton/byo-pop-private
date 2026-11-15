// The games whose port reads texts or tables out of the game's exe, and how the import checks that a copy's exe gives them up
// (web/gameimport.js, exeUnreadable; tools/exeprobe.py is the same check). For each game: the exe's names, in the order its
// reader tries them, and the probes, each one text its port reads: its address in the game's image, its length and the FNV-1a
// hash of its bytes (never the text). An exe passes when any probe holds in it, in the plain image or in the image PopCap's
// own launcher holds after its marker. A copy whose exe is packed and encrypted by a store's wrapper fails, and is refused
// before anything is copied. The probes are entries of the ports' own tables (GameText.cpp and the like; Bonnie's, which runs
// the image itself, a text of its .rdata), each checked against the real exes of every build the reader takes.
var ByopopExeProbes = {
  alchemy: { files: ["Alchemy.exe", "AlchemyGame.exe"], probes: [{ va: 0x004e4124, len: 7, fnv: 0x2d1f32b6 }] },
  astropop: { files: ["WinAP.exe", "AstroPopGame.exe", "AstroPop.exe"], probes: [{ va: 0x0055d4e8, len: 18, fnv: 0xa1dbb54c }, { va: 0x005d84b8, len: 18, fnv: 0xa1dbb54c }] }, // the Steam build; the Collection's (the reader finds its texts by length and hash)
  atomica: { files: ["Atomica.exe"], probes: [{ va: 0x004ae4c4, len: 14, fnv: 0xab3df281 }] },
  bejeweledtwist: { files: ["BejeweledTwist.exe"], probes: [{ va: 0x007d815c, len: 37, fnv: 0x7abfe4a4 }] },
  bonnies: { files: ["BonniesBookstore.exe"], probes: [{ va: 0x004573b0, len: 10, fnv: 0xfba088ce }] },
  // the PopCap build; and the Steam build (delisted in 2016), whose texts lie elsewhere: the port reads them only at the PopCap
  // build's addresses so far, but the copy is let in rather than refused with "the Steam copy works" (for BookWorm's session)
  bookworm: { files: ["Bookworm.exe", "BookwormGame.exe"], probes: [{ va: 0x004fc384, len: 13, fnv: 0x2a9b2748 }, { va: 0x0053c8f4, len: 13, fnv: 0x2a9b2748 }] },
  bookwormadv: { files: ["BookwormAdventures.exe"], probes: [{ va: 0x007a95e8, len: 8, fnv: 0xab43d664 }] },
  bookwormadv2: { files: ["BookwormAdventuresVol2.exe"], probes: [{ va: 0x006f6ab8, len: 8, fnv: 0xab43d664 }] },
  cosmicbugs: { files: ["CosmicBugs.exe"], probes: [{ va: 0x00458a94, len: 8, fnv: 0x372c6a2f }] },
  dynomite: { files: ["Dynomite.exe"], probes: [{ va: 0x004f7594, len: 6, fnv: 0xde281feb }] },
  heavyweapon: { files: ["Heavy Weapon Deluxe.exe", "HeavyWeapon.exe"], probes: [{ va: 0x0052b190, len: 12, fnv: 0xb163b8af }] }, // PopCap's own build, its image after the launcher's marker
  mahjongchina: { files: ["MahjongEscapeAC.exe"], probes: [{ va: 0x005240fc, len: 4, fnv: 0xad6079dc }] },
  mahjongjapan: { files: ["MahjongEscapeAJ.exe"], probes: [{ va: 0x005250fc, len: 6, fnv: 0x47d8a4ab }] },
  mummymaze: { files: ["WinMM.exe"], probes: [{ va: 0x0048d1a0, len: 11, fnv: 0xb02e2b14 }] },
  ningpo: { files: ["Ningpo.exe"], probes: [{ va: 0x00467a24, len: 6, fnv: 0xfb7f6c5e }] },
  noahsark: { files: ["WinNoah.exe"], probes: [{ va: 0x0049424c, len: 10, fnv: 0xab42b91f }] },
  pixelus: { files: ["Pixelus.exe", "PixelusGame.exe"], probes: [{ va: 0x0050b4a4, len: 8, fnv: 0x57aa4a80 }] },
  rocketmania: { files: ["RocketMania.exe", "RocketManiaGame.exe"], probes: [{ va: 0x004e4170, len: 7, fnv: 0x4cd36545 }] },
  sevenseas: { files: ["WinSS.exe"], probes: [{ va: 0x004a0150, len: 64, fnv: 0xec4ac530 }] },
  tiptop: { files: ["TipTop.exe"], probes: [{ va: 0x0046c2d0, len: 71, fnv: 0x074dd849 }] },
  typershark: { files: ["WinTS.exe", "TyperSharkGame.exe", "TyperShark.exe"], probes: [{ va: 0x00529880, len: 18, fnv: 0xa1dbb54c }, { va: 0x00511870, len: 18, fnv: 0xa1dbb54c }, { va: 0x0050c870, len: 18, fnv: 0xa1dbb54c }] }, // the release; two older builds the reader finds by length and hash
  waterbugs: { files: ["WaterBugs.exe"], probes: [{ va: 0x00458870, len: 10, fnv: 0xbc8c0769 }] }
};
if (typeof module !== "undefined") module.exports = ByopopExeProbes;
