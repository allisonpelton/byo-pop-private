// The desktop themes' table (web/themes.js, THEMES): by game id, the background its theme shows as the wallpaper and the sound of
// each window event, every path a file of the game's own folder that its code loads. Our own choice of which file; the files are
// the player's.
var ByopopThemeData = {
  bejeweled: { wallpaper: "images/titlescreen6.jpg",
    sounds: { open: "sounds/menuclick.ogg", close: "sounds/select.ogg", minimize: "sounds/gemongem2.ogg", restore: "sounds/go1.ogg",
      maximize: "sounds/gotsetbig2.ogg", menu: "sounds/select.ogg", ding: "sounds/bad2.ogg" } },
  // Alchemy
  alchemy: { wallpaper: "images/title.jpg",
    sounds: { open: "sounds/click.ogg", close: "sounds/thunk.ogg", minimize: "sounds/melt.ogg", restore: "sounds/forgedown.ogg", maximize: "sounds/gemchange.ogg", menu: "sounds/click.ogg", ding: "sounds/bad.ogg" } },
  // Astropop
  astropop: { wallpaper: "images/gameselector.jpg",
    sounds: { open: "sounds/click2.ogg", close: "sounds/mouseover.ogg", minimize: "sounds/magzap1.ogg", restore: "sounds/magzap3.ogg", maximize: "sounds/activated.ogg", menu: "sounds/mouseover.ogg", ding: "sounds/magzap1.ogg" } },
  // Atomica
  atomica: { wallpaper: "images/titlescreen.jpg",
    sounds: { open: "sounds/dooropen.ogg", close: "sounds/doorslam.ogg", minimize: "sounds/lowwhoosh.ogg", restore: "sounds/swoop.ogg", maximize: "sounds/whoosh.ogg", menu: "sounds/capclick.ogg", ding: "sounds/selectatom.ogg" } },
  // Bejeweled 2
  bejeweled2: { wallpaper: "images/backdrops/backdrop00.jpg",
    sounds: { open: "sounds/menuclick.ogg", close: "sounds/mainmenu_mouseoff.ogg", minimize: "sounds/panelslide1.ogg", restore: "sounds/mainmenu_mouseover.ogg", maximize: "sounds/panelslide1.ogg", menu: "sounds/menuclick2.ogg", ding: "sounds/bad2.ogg" } },
  // Bejeweled Twist
  bejeweledtwist: {
    sounds: { open: "sounds/click.ogg", close: "sounds/click_dialog.ogg", minimize: "sounds/menu_slide.ogg", restore: "sounds/doorlock2.ogg", maximize: "sounds/steamlock1.ogg", menu: "sounds/mouseover1.ogg", ding: "sounds/badmove.ogg" } },
  // Big Money
  bigmoney: { wallpaper: "images/titleback.jpg",
    sounds: { open: "sounds/buttonclick.ogg", close: "sounds/cointube.ogg", minimize: "sounds/moveup.ogg", restore: "sounds/gotbag.ogg", maximize: "sounds/bonus.ogg", menu: "sounds/buttonclick.ogg", ding: "sounds/warning.ogg" } },
  // Bonnie's Bookstore
  bonnies: { wallpaper: "Graphics/Credits.jpg",
    sounds: { open: "Sound/Button_Press.ogg", close: "Sound/Button_Release.ogg", minimize: "Sound/Cancel.ogg", restore: "Sound/Place_01.ogg", maximize: "Sound/Place_03.ogg", menu: "Sound/PaperButton_Press.ogg", ding: "Sound/Error.ogg" } },
  // BookWorm
  bookworm: { wallpaper: "images/backdrop_title.jpg",
    sounds: { open: "sounds/nextselect.ogg", close: "sounds/chomp.ogg", minimize: "sounds/up1.ogg", restore: "sounds/up3.ogg", maximize: "sounds/up5.ogg", menu: "sounds/nextselect.ogg", ding: "sounds/buzzer.ogg" } },
  // Bookworm Adventures
  bookwormadv: { wallpaper: "images/mainmenu/mainmenu_bg.png",
    sounds: { open: "sounds/click1.ogg", close: "sounds/click2.ogg", minimize: "sounds/click1.ogg", restore: "sounds/click2.ogg", maximize: "sounds/lex_chomp.ogg", menu: "sounds/click2.ogg", ding: "sounds/buzzer.ogg" } },
  // Bookworm Adventures Volume 2
  bookwormadv2: { wallpaper: "images/minigames/tentbackground.jpg",
    sounds: { open: "sounds/click1.ogg", close: "sounds/click2.ogg", minimize: "sounds/tile_undo.ogg", restore: "sounds/click1.ogg", maximize: "sounds/lex_chomp.ogg", menu: "sounds/click2.ogg", ding: "sounds/buzzer.ogg" } },
  // Cosmic Bugs
  cosmicbugs: {
    sounds: { open: "audio/click.wav", close: "audio/clank.wav", minimize: "audio/powerdn.wav", restore: "audio/powerup.wav", maximize: "audio/fill_end.wav", menu: "audio/click.wav", ding: "audio/clank.wav" } },
  // Dynomite
  dynomite: { wallpaper: "images/title.jpg",
    sounds: { open: "sounds/click.ogg", close: "sounds/egglands.wav", minimize: "sounds/breakegg.wav", restore: "sounds/popbonus.ogg", maximize: "sounds/rubbershot.ogg", menu: "sounds/click.ogg", ding: "sounds/warning.ogg" } },
  // Mahjong Escape: Ancient China
  mahjongchina: { wallpaper: "Resource.dll|pe:JPG:109",
    sounds: { open: "Resource.dll|pe:WAVE:105", close: "Resource.dll|pe:WAVE:351", minimize: "Resource.dll|pe:WAVE:349", restore: "Resource.dll|pe:WAVE:350", maximize: "Resource.dll|pe:WAVE:352", menu: "Resource.dll|pe:WAVE:175", ding: "Resource.dll|pe:WAVE:174" } },
  // Mahjong Escape: Ancient Japan
  mahjongjapan: { wallpaper: "Resource.dll|pe:JPG:109",
    sounds: { open: "Resource.dll|pe:WAVE:105", close: "Resource.dll|pe:WAVE:351", minimize: "Resource.dll|pe:WAVE:349", restore: "Resource.dll|pe:WAVE:350", maximize: "Resource.dll|pe:WAVE:352", menu: "Resource.dll|pe:WAVE:175", ding: "Resource.dll|pe:WAVE:174" } },
  // Mummy Maze
  mummymaze: { wallpaper: "images/menuback.jpg",
    sounds: { open: "sounds/click.ogg", close: "sounds/tombslide.ogg", minimize: "sounds/gate.ogg", restore: "sounds/expwalk30a.ogg", maximize: "sounds/finishedlevel.ogg", menu: "sounds/click.ogg", ding: "sounds/badankh.ogg" } },
  // Ning Po
  ningpo: { wallpaper: "ningpo.pyz|zip:loading_np.jpg",
    sounds: { open: "ningpo.pyz|zip:nm_guiclick.wav", close: "ningpo.pyz|zip:nm_unclick.wav", minimize: "ningpo.pyz|zip:nm_hint.ogg", restore: "ningpo.pyz|zip:nm_click.wav", maximize: "ningpo.pyz|zip:nm_match.ogg", menu: "ningpo.pyz|zip:nm_guiclick.wav", ding: "ningpo.pyz|zip:nm_blocked.ogg" } },
  // Noah's Ark
  noahsark: { wallpaper: "images/title.jpg",
    sounds: { open: "sounds/click.ogg", close: "sounds/deselect.ogg", minimize: "sounds/fallingclick.ogg", restore: "sounds/select.ogg", maximize: "sounds/pop.ogg", menu: "sounds/click.ogg", ding: "sounds/warning.ogg" } },
  // Pixelus
  pixelus: { wallpaper: "images/gameselect.jpg",
    sounds: { open: "sounds/ui menu click 01c.ogg", close: "sounds/ui little beep 05b.ogg", minimize: "sounds/ui menu mouseover 02.ogg", restore: "sounds/ui island click 01.ogg", maximize: "sounds/ui island mouseover 02.ogg", menu: "sounds/ui menu click 01c.ogg", ding: "sounds/illegal blocked 03.ogg" } },
  // Rocket Mania
  rocketmania: { wallpaper: "images/selectback.jpg",
    sounds: { open: "sounds/ui little beep 05b.ogg", close: "sounds/block rotating 03.ogg", minimize: "sounds/tubebump.ogg", restore: "sounds/lantern 01.ogg", maximize: "sounds/hint popup 01.ogg", menu: "sounds/ui little beep 05b.ogg", ding: "sounds/hint popup 01.ogg" } },
  // Seven Seas
  sevenseas: { wallpaper: "images/title.jpg",
    sounds: { open: "sounds/click.ogg", close: "sounds/whirlout.ogg", minimize: "sounds/whirlin.ogg", restore: "sounds/reappear.ogg", maximize: "sounds/moveevil.ogg", menu: "sounds/click.ogg", ding: "sounds/explode.ogg" } },
  // Tip Top
  tiptop: { wallpaper: "TipTop.dat|tiptop:art/MenuBkgd.JPG",
    sounds: { open: "TipTop.dat|tiptop:sounds/MouseClick.ogg", close: "TipTop.dat|tiptop:sounds/BlocksFall.ogg", minimize: "TipTop.dat|tiptop:sounds/LaunchBlockMoves.ogg", restore: "TipTop.dat|tiptop:sounds/BlockLaunch.ogg", maximize: "TipTop.dat|tiptop:sounds/GameStart.ogg", menu: "TipTop.dat|tiptop:sounds/MouseOver.ogg", ding: "TipTop.dat|tiptop:sounds/VolumeAdjust.ogg" } },
  // Typer Shark
  typershark: { wallpaper: "images/titlescreen.jpg",
    sounds: { open: "sounds/buttonclick.ogg", close: "sounds/shipmove.ogg", minimize: "sounds/type.ogg", restore: "sounds/type3.ogg", maximize: "sounds/divebell1.ogg", menu: "sounds/buttonclick.ogg", ding: "sounds/wrong.ogg" } },
  // Water Bugs
  waterbugs: {
    sounds: { open: "audio/click.wav", close: "audio/pop.wav", minimize: "audio/clank.wav", restore: "audio/fill.wav", maximize: "audio/fill_end.wav", menu: "audio/click.wav", ding: "audio/buzzer.wav" } },
  // Word Harmony
  wordharmony: { wallpaper: "res/splash/sproutSplash.jpg",
    sounds: { open: "res/sounds/menu_clicked.wav", close: "res/sounds/ball_pop.wav", minimize: "res/sounds/ball_rustle.wav", restore: "res/sounds/ball_rustle2.WAV", maximize: "res/sounds/level_begin.wav", menu: "res/sounds/menu_mouseover.wav", ding: "res/sounds/level_intro.wav" } }
};
if (typeof module !== "undefined" && module.exports) module.exports = ByopopThemeData;
