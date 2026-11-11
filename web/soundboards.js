// The sound effects of each game, for Desktop 95's Sounds window (web/sounds.js; docs/WEB_PLAYER.md, "The soundboard"), by the
// game's id. A game is listed in the window only when it has an entry here and its files are in this browser. Paths are in the
// game's folder as the import writes it, matched without regard to case.
//   groups  [{name, sounds: [[path, name, when], ...]}, ...]: the window's group boxes, in order, each with its buttons in order
//           path  the file
//           name  the button's label: a plain name of our own; a voice line is its words, in quotes (a file name gives
//                 only part of a line until someone corrects it by ear); a sound the game never plays says so; a
//                 silent file is left out
//           when  the tooltip's second line: when the game plays it, found in the port's source (every PlaySample of the
//                 sound), never a passage of the game's text
//   presets [{name, when, steps: [[file, ms, {v, p, r, loop, end, ramp, pans}], ...]}, ...]: sequences the game plays, as it
//           plays them, in a group box of their own before the others (the user, 2026-11-11). Each is a stretch of the
//           port's sound log from a game its bot played, made by tools/soundpreset.py: a step is a file (its extension may
//           be left off) at its time in milliseconds, with its volume v and pan p in hundredths of a decibel as logged,
//           its pitch r as a rate, loop, end (when it is stopped), and ramp and pans ([[ms, value], ...]: changes while it
//           plays). The name is rip style ("Cascade x5", "Level Up": Title Case, at most 20 characters)
// BookWorm's 45 are its sound table (games/bookworm/src/BookWormApp.cpp, gSoundNames; Res.h, SoundId), every file of
// sounds/ but blank.au, which is silent and never played. Three are never played: bad, gameovertone and up9 (the rising
// notes stop at up8). The user corrected two labels by ear on 2026-11-09: lex_welcome says "Welcome to BookWorm!" and
// bonusword is Lex saying "Bonus!".
var ByopopSoundboards = {
  alchemy: {
    presets: [
      { name: "Row Clear", when: "A tile fills a row or column", steps: [["sounds/finalvanish", 0], ["sounds/thunk", 0], ["sounds/forgedown", 0]]},
      { name: "Board Complete", when: "The last square turns gold", steps: [["sounds/finalvanish", 0], ["sounds/thunk", 0], ["sounds/finalgong", 1010], ["sounds/v4-levelcomplete", 4540], ["sounds/finalvanish", 4680], ["sounds/gemchange", 5350]]},
      { name: "Game Over", when: "The forge overflows", steps: [["sounds/melt", 0], ["sounds/warning", 10], ["sounds/endofgame", 340], ["sounds/v4-gameover", 1350]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed: the title, intro, high scores and options screens, the sound button"],
      ["sounds/button.ogg", "Button (unused)", "Never played"],
      ["sounds/slideback.ogg", "Slide back (unused)", "Never played"],
      ["sounds/v4-welcome.ogg", "Welcome voice", "The intro screen, a second after it opens, before the first game"]
    ] },
    { name: "Placing tiles", sounds: [
      ["sounds/bad.ogg", "Bad move", "A tile is clicked where it cannot go"],
      ["sounds/thunk.ogg", "Tile placed", "A tile is set on the board"],
      ["sounds/skull.ogg", "Skull bomb", "A bomb tile is set down and blows"],
      ["sounds/melt.ogg", "Tile melts", "A tile is thrown into the discard"]
    ] },
    { name: "The forge", sounds: [
      ["sounds/forgedown.ogg", "Forge down", "A placed tile lowers the forge's level (not in the timed game)"],
      ["sounds/warning.ogg", "Forge warning", "The forge is nearly full; repeats every few seconds while it stays so"],
      ["sounds/gemchange.ogg", "Gems change", "The next board's tiles fade in after a board is cleared"]
    ] },
    { name: "Board end", sounds: [
      ["sounds/finalvanish.ogg", "Row clears", "A full row or column clears; the board is wiped for the next one"],
      ["sounds/finalgong.ogg", "Final gong", "Every tile of the board is lit"],
      ["sounds/endofgame.ogg", "End of game", "The game ends"],
      ["sounds/v4-levelcomplete.ogg", "Level complete voice", "The wizard appears after a board is cleared"],
      ["sounds/v4-gameover.ogg", "Game over voice", "The wizard appears when the game ends"]
    ] },
  ] },
  astropop: {
    presets: [
      { name: "Combo x2", when: "Two sets in one chain: a rising ping for each, then the voice", steps: [["sounds/explode1", 0, {"v": -46, "p": 1200}], ["sounds/gotset2", 0, {"p": 1600}], ["sounds/star9", 0], ["sounds/combo_tone", 0], ["sounds/magzap3", 100, {"v": -145}], ["sounds/magzap2", 170, {"v": -145}], ["sounds/gemongem2", 170, {"v": -46, "p": -1920}], ["sounds/blockhit", 400, {"v": -145, "p": -1920}], ["sounds/magzap3", 500, {"v": -145}], ["sounds/magzap2", 570, {"v": -145}], ["sounds/gemongem2", 570, {"v": -46}], ["sounds/blockhit", 690, {"v": -145}], ["sounds/magzap3", 790, {"v": -145}], ["sounds/magzap2", 860, {"v": -145}], ["sounds/gemongem2", 860, {"v": -46, "p": -1920}], ["sounds/v_combo1", 1000]]},
      { name: "Combo x3", when: "Three sets in one chain", steps: [["sounds/star9", 0], ["sounds/combo_tone", 0], ["sounds/laser", 0, {"p": -2000}], ["sounds/magzap3", 0, {"v": -145}], ["sounds/star9", 420], ["sounds/laser", 420, {"p": -1920}], ["sounds/magzap3", 420, {"v": -145}], ["sounds/laser", 840, {"p": -1840}], ["sounds/magzap3", 840, {"v": -145}], ["sounds/laser", 1200, {"p": 1200}], ["sounds/magzap3", 1200, {"v": -145}], ["sounds/v_combo2", 1420]]},
      { name: "Combo x4", when: "Four sets in one chain", steps: [["sounds/combo_tone", 0], ["sounds/explode1", 0, {"v": -46, "p": -1880}], ["sounds/gotset2", 0, {"p": -1920}], ["sounds/blockhit", 350, {"v": -145, "p": -1920}], ["sounds/magzap3", 480, {"v": -145}], ["sounds/magzap2", 550, {"v": -145}], ["sounds/gemongem2", 550, {"v": -46, "p": -1960}], ["sounds/combo_tone", 660, {"r": 1.0595}], ["sounds/explode1", 660, {"v": -46, "p": -1960}], ["sounds/gotset2", 660, {"p": -2000}], ["sounds/blockhit", 1020, {"v": -145, "p": -2000}], ["sounds/magzap3", 1100, {"v": -145}], ["sounds/magzap2", 1170, {"v": -145}], ["sounds/gemongem2", 1170, {"v": -46, "p": -1880}], ["sounds/combo_tone", 1290, {"r": 1.1225}], ["sounds/explode1", 1290, {"v": -46, "p": -1880}], ["sounds/gotset2", 1290, {"p": -1880}], ["sounds/blockhit", 1640, {"v": -145, "p": -1840}], ["sounds/magzap3", 1720, {"v": -145}], ["sounds/magzap2", 1790, {"v": -145}], ["sounds/gemongem2", 1790, {"v": -46, "p": 1600}], ["sounds/blockhit", 1880, {"v": -145, "p": 1600}], ["sounds/magzap3", 2010, {"v": -145}], ["sounds/magzap2", 2080, {"v": -145}], ["sounds/gemongem2", 2080, {"v": -46, "p": -1880}], ["sounds/blockhit", 2200, {"v": -145, "p": -1880}], ["sounds/v_combo3", 2290]]},
      { name: "Combo x5", when: "Five sets in one chain", steps: [["sounds/combo_tone", 0], ["sounds/explode1", 0, {"v": -46, "p": -1880}], ["sounds/gotset2", 0], ["sounds/star9", 0], ["sounds/blockhit", 410, {"v": -145, "p": -1840}], ["sounds/magzap3", 440, {"v": -145}], ["sounds/magzap2", 510, {"v": -145}], ["sounds/gemongem2", 510, {"v": -46, "p": -1840}], ["sounds/blockhit", 630, {"v": -145, "p": -1840}], ["sounds/magzap3", 730, {"v": -145}], ["sounds/magzap2", 800, {"v": -145}], ["sounds/gemongem2", 800, {"v": -46, "p": -1880}], ["sounds/combo_tone", 920, {"r": 1.1225}], ["sounds/explode1", 920, {"v": -46, "p": -1880}], ["sounds/gotset2", 920, {"p": -1880}], ["sounds/magzap3", 1020, {"v": -145}], ["sounds/magzap2", 1090, {"v": -145}], ["sounds/gemongem2", 1090, {"v": -46, "p": -1840}], ["sounds/blockhit", 1280, {"v": -145, "p": -1840}], ["sounds/magzap3", 1310, {"v": -145}], ["sounds/magzap2", 1380, {"v": -145}], ["sounds/gemongem2", 1380, {"v": -46, "p": -1840}], ["sounds/blockhit", 1500, {"v": -145, "p": -1840}], ["sounds/magzap3", 1600, {"v": -145}], ["sounds/magzap2", 1670, {"v": -145}], ["sounds/gemongem2", 1670, {"v": -46, "p": -1880}], ["sounds/combo_tone", 1790, {"r": 1.1892}], ["sounds/explode1", 1790, {"v": -46, "p": -1880}], ["sounds/bubbly", 1790, {"p": -1920}], ["sounds/blockhit", 2150, {"v": -145, "p": -1840}], ["sounds/magzap3", 2220, {"v": -145}], ["sounds/magzap2", 2290, {"v": -145}], ["sounds/gemongem2", 2290, {"v": -46, "p": -1920}], ["sounds/blockhit", 2420, {"v": -145, "p": -1920}], ["sounds/magzap3", 2510, {"v": -145}], ["sounds/magzap2", 2580, {"v": -145}], ["sounds/gemongem2", 2580, {"v": -46, "p": -1880}], ["sounds/blockhit", 2710, {"v": -145, "p": -1880}], ["sounds/v_combo4", 2790]]},
      { name: "Combo x6", when: "Six sets in one chain", steps: [["sounds/explode1", 0, {"v": -46, "p": 1600}], ["sounds/gotset2", 0, {"p": 1600}], ["sounds/star9", 0], ["sounds/combo_tone", 0], ["sounds/explode1", 400, {"v": -46, "p": 1400}], ["sounds/gotset2", 400, {"p": 1400}], ["sounds/magzap3", 440, {"v": -145}], ["sounds/magzap2", 510, {"v": -145}], ["sounds/gemongem2", 510, {"v": -46, "p": -1920}], ["sounds/combo_tone", 750, {"r": 1.1225}], ["sounds/explode1", 750, {"v": -46, "p": -1920}], ["sounds/gotset2", 750, {"p": -1880}], ["sounds/magzap1", 840, {"v": -515}], ["sounds/magzap2", 910, {"v": -145}], ["sounds/magzap3", 1130, {"v": -145}], ["sounds/magzap2", 1200, {"v": -145}], ["sounds/gemongem2", 1200, {"v": -46}], ["sounds/blockhit", 1320, {"v": -145}], ["sounds/magzap3", 1420, {"v": -145}], ["sounds/magzap2", 1490, {"v": -145}], ["sounds/gemongem2", 1490, {"v": -46, "p": 1200}], ["sounds/combo_tone", 1610, {"r": 1.1892}], ["sounds/explode1", 1610, {"v": -46, "p": 1200}], ["sounds/star9", 1610], ["sounds/bubbly", 1610], ["sounds/magzap3", 1710, {"v": -145}], ["sounds/magzap2", 1780, {"v": -145}], ["sounds/gemongem2", 1780, {"v": -46, "p": -1840}], ["sounds/blockhit", 2030, {"v": -145, "p": -1840}], ["sounds/magzap3", 2110, {"v": -145}], ["sounds/magzap2", 2180, {"v": -145}], ["sounds/gemongem2", 2180, {"v": -46, "p": 1600}], ["sounds/blockhit", 2310, {"v": -145, "p": 1600}], ["sounds/magzap3", 2400, {"v": -145}], ["sounds/magzap2", 2470, {"v": -145}], ["sounds/gemongem2", 2470, {"v": -46}], ["sounds/blockhit", 2600, {"v": -145}], ["sounds/v_combo5", 2610]]},
      { name: "Combo x8", when: "Eight sets in one chain", steps: [["sounds/explode1", 0, {"v": -46, "p": 1400}], ["sounds/gotset2", 0, {"p": 1200}], ["sounds/star9", 0], ["sounds/combo_tone", 0], ["sounds/wordswoosh", 340], ["sounds/brickmilestone", 340, {"v": -94}], ["sounds/blockhit", 410, {"v": -145, "p": 1200}], ["sounds/magzap3", 440, {"v": -145}], ["sounds/magzap2", 510, {"v": -145}], ["sounds/gemongem2", 510, {"v": -46, "p": -1920}], ["sounds/combo_tone", 610, {"r": 1.0595}], ["sounds/explode1", 610, {"v": -46, "p": -1920}], ["sounds/gotset2", 610, {"p": -2000}], ["sounds/blockhit", 960, {"v": -145, "p": -1960}], ["sounds/magzap3", 1060, {"v": -145}], ["sounds/magzap2", 1130, {"v": -145}], ["sounds/gemongem2", 1130, {"v": -46, "p": 1600}], ["sounds/combo_tone", 1240, {"r": 1.1225}], ["sounds/explode1", 1240, {"v": -46, "p": 1600}], ["sounds/gotset2", 1240, {"p": 1800}], ["sounds/blockhit", 1600, {"v": -145, "p": 1800}], ["sounds/magzap3", 1680, {"v": -145}], ["sounds/magzap2", 1750, {"v": -145}], ["sounds/gemongem2", 1750, {"v": -46, "p": 1600}], ["sounds/combo_tone", 1870, {"r": 1.1892}], ["sounds/explode1", 1870, {"v": -46, "p": 1600}], ["sounds/gotset2", 1870, {"p": 1600}], ["sounds/star9", 1870], ["sounds/explode1", 2270, {"v": -46, "p": 1200}], ["sounds/star9", 2270], ["sounds/bubbly", 2270], ["sounds/blockhit", 2680, {"v": -145}], ["sounds/magzap3", 2740, {"v": -145}], ["sounds/magzap2", 2810, {"v": -145}], ["sounds/gemongem2", 2810, {"v": -46, "p": -1840}], ["sounds/blockhit", 2940, {"v": -145, "p": -1840}], ["sounds/magzap3", 3030, {"v": -145}], ["sounds/magzap2", 3100, {"v": -145}], ["sounds/gemongem2", 3100, {"v": -46, "p": 1200}], ["sounds/blockhit", 3230, {"v": -145, "p": 1200}], ["sounds/v_combo65", 3270]]},
      { name: "Combo x9", when: "Nine sets or more in one chain", steps: [["sounds/explode1", 0, {"v": -46, "p": -1960}], ["sounds/gotset2", 0, {"p": -2000}], ["sounds/star9", 0], ["sounds/combo_tone", 0], ["sounds/magzap3", 90, {"v": -145}], ["sounds/magzap2", 160, {"v": -145}], ["sounds/gemongem2", 160, {"v": -46, "p": -1880}], ["sounds/explode1", 400, {"v": -46, "p": -1880}], ["sounds/gotset2", 400, {"p": -1960}], ["sounds/star9", 400], ["sounds/magzap3", 490, {"v": -145}], ["sounds/magzap2", 560, {"v": -145}], ["sounds/gemongem2", 560, {"v": -46, "p": -1880}], ["sounds/combo_tone", 810, {"r": 1.1892}], ["sounds/explode1", 810, {"v": -46, "p": -1880}], ["sounds/bubbly", 810, {"p": -1880}], ["sounds/blockhit", 1160, {"v": -145}], ["sounds/magzap3", 1220, {"v": -145}], ["sounds/magzap2", 1290, {"v": -145}], ["sounds/gemongem2", 1290, {"v": -46, "p": 1800}], ["sounds/combo_tone", 1410, {"r": 1.2599}], ["sounds/explode1", 1410, {"v": -46, "p": 1800}], ["sounds/gotset2", 1410, {"p": 1800}], ["sounds/magzap1", 1510, {"v": -515}], ["sounds/magzap2", 1580, {"v": -145}], ["sounds/level_up2", 1750, {"v": -94}], ["sounds/magzap3", 1800, {"v": -145}], ["sounds/magzap2", 1870, {"v": -145}], ["sounds/gemongem2", 1870, {"v": -46, "p": 1400}], ["sounds/combo_tone", 1990, {"r": 1.3348}], ["sounds/explode1", 1990, {"v": -46, "p": 1400}], ["sounds/gotset2", 1990, {"p": 1400}], ["sounds/wordswoosh", 2330], ["sounds/brickmilestone", 2330, {"v": -94}], ["sounds/blockhit", 2350, {"v": -145, "p": 1200}], ["sounds/magzap3", 2420, {"v": -145}], ["sounds/magzap2", 2490, {"v": -145}], ["sounds/gemongem2", 2490, {"v": -46, "p": -1920}], ["sounds/combo_tone", 2600, {"r": 1.4142}], ["sounds/explode1", 2600, {"v": -46, "p": -1920}], ["sounds/gotset2", 2600, {"p": -1960}], ["sounds/star9", 2600], ["sounds/magzap1", 2710, {"v": -515}], ["sounds/magzap2", 2780, {"v": -145}], ["sounds/magzap3", 3000, {"v": -145}], ["sounds/magzap2", 3070, {"v": -145}], ["sounds/gemongem2", 3070, {"v": -46, "p": -1840}], ["sounds/blockhit", 3200, {"v": -145, "p": -1840}], ["sounds/magzap3", 3290, {"v": -145}], ["sounds/magzap2", 3360, {"v": -145}], ["sounds/gemongem2", 3360, {"v": -46, "p": -1960}], ["sounds/blockhit", 3490, {"v": -145, "p": -1960}], ["sounds/magzap3", 3580, {"v": -145}], ["sounds/v_combo7", 3600]]},
      { name: "Level Complete", when: "The level's last set: the ship warps to the next level", steps: [["sounds/combo_tone", 0, {"r": 1.7818}], ["sounds/explode1", 0, {"v": -46, "p": 1200}], ["sounds/gotset2", 0, {"p": -1840}], ["sounds/blockhit", 360, {"v": -145}], ["sounds/v_levelcomplete", 370], ["sounds/wordswoosh", 370], ["sounds/warp3", 1110], ["sounds/hyperspaceloop1", 1640, {"v": -376, "loop": 1, "end": 7990}], ["sounds/v_warpengaged", 2050], ["sounds/comptext", 3120, {"v": -515}], ["sounds/comptext", 3170, {"v": -515}], ["sounds/comptext", 3220, {"v": -515}], ["sounds/comptext", 3270, {"v": -515}], ["sounds/comptext", 3320, {"v": -515}], ["sounds/comptext", 3370, {"v": -515}], ["sounds/comptext", 3420, {"v": -515}], ["sounds/comptext", 3470, {"v": -515}], ["sounds/comptext", 3520, {"v": -515}], ["sounds/comptext", 3570, {"v": -515}], ["sounds/comptext", 3620, {"v": -515}], ["sounds/comptext", 3670, {"v": -515}], ["sounds/comptext", 3720, {"v": -515}], ["sounds/hyperspacethunder1", 3760, {"v": -376, "p": 1200}], ["sounds/comptext", 3770, {"v": -515}], ["sounds/comptext", 3820, {"v": -515}], ["sounds/comptext", 3870, {"v": -515}], ["sounds/comptext", 3920, {"v": -515}], ["sounds/comptext", 3970, {"v": -515}], ["sounds/comptext", 4020, {"v": -515}], ["sounds/comptext", 4070, {"v": -515}], ["sounds/comptext", 4120, {"v": -515}], ["sounds/comptext", 4170, {"v": -515}], ["sounds/comptext", 4220, {"v": -515}], ["sounds/comptext", 4270, {"v": -515}], ["sounds/comptext", 4320, {"v": -515}], ["sounds/comptext", 4370, {"v": -515}], ["sounds/comptext", 4420, {"v": -515}], ["sounds/comptext", 4470, {"v": -515}], ["sounds/comptext", 4520, {"v": -515}], ["sounds/comptext", 4570, {"v": -515}], ["sounds/comptext", 4620, {"v": -515}], ["sounds/comptext", 4670, {"v": -515}], ["sounds/comptext", 4720, {"v": -515}], ["sounds/comptext", 4770, {"v": -515}], ["sounds/comptext", 4820, {"v": -515}], ["sounds/comptext", 4870, {"v": -515}], ["sounds/comptext", 4920, {"v": -515}], ["sounds/comptext", 4970, {"v": -515}], ["sounds/comptext", 5020, {"v": -515}], ["sounds/comptext", 5070, {"v": -515}], ["sounds/comptext", 5120, {"v": -515}], ["sounds/comptext", 5170, {"v": -515}], ["sounds/comptext", 5220, {"v": -515}], ["sounds/comptext", 5270, {"v": -515}], ["sounds/comptext", 5320, {"v": -515}], ["sounds/comptext", 5370, {"v": -515}], ["sounds/comptext", 5420, {"v": -515}], ["sounds/comptext", 5470, {"v": -515}], ["sounds/comptext", 5520, {"v": -515}], ["sounds/comptext", 5570, {"v": -515}], ["sounds/comptext", 5620, {"v": -515}], ["sounds/comptext", 5670, {"v": -515}], ["sounds/comptext", 5720, {"v": -515}], ["sounds/comptext", 5770, {"v": -515}], ["sounds/hyperspacethunder2", 5820, {"v": -376, "p": -1920}], ["sounds/comptext", 5820, {"v": -515}], ["sounds/comptext", 5870, {"v": -515}], ["sounds/comptext", 5920, {"v": -515}], ["sounds/comptext", 5970, {"v": -515}], ["sounds/comptext", 6020, {"v": -515}], ["sounds/comptext", 6070, {"v": -515}], ["sounds/comptext", 6120, {"v": -515}], ["sounds/comptext", 6170, {"v": -515}], ["sounds/comptext", 6220, {"v": -515}], ["sounds/comptext", 6270, {"v": -515}], ["sounds/comptext", 6320, {"v": -515}], ["sounds/comptext", 6370, {"v": -515}], ["sounds/comptext", 6420, {"v": -515}], ["sounds/comptext", 6470, {"v": -515}], ["sounds/comptext", 6520, {"v": -515}], ["sounds/comptext", 6570, {"v": -515}], ["sounds/comptext", 6620, {"v": -515}], ["sounds/comptext", 6670, {"v": -515}], ["sounds/comptext", 6720, {"v": -515}], ["sounds/comptext", 6770, {"v": -515}], ["sounds/comptext", 6820, {"v": -515}], ["sounds/comptext", 6870, {"v": -515}], ["sounds/comptext", 6920, {"v": -515}], ["sounds/comptext", 6970, {"v": -515}], ["sounds/comptext", 7020, {"v": -515}], ["sounds/comptext", 7070, {"v": -515}], ["sounds/comptext", 7120, {"v": -515}], ["sounds/comptext", 7170, {"v": -515}], ["sounds/click2", 7560, {"v": -313}], ["sounds/engineloop", 7990, {"v": -1246, "loop": 1, "end": 176520}], ["sounds/v_getready", 7990]]},
      { name: "Game Over", when: "Survival: the blocks reach the ship", steps: [["sounds/klaxon", 0, {"loop": 1, "end": 3490}], ["sounds/magzap3", 10, {"v": -145}], ["sounds/gemongem2", 40, {"v": -46, "p": 1200}], ["sounds/magzap2", 80, {"v": -145}], ["sounds/blockhit", 150, {"v": -145, "p": 1600}], ["sounds/magzap3", 300, {"v": -145}], ["sounds/gemongem2", 320, {"v": -46, "p": 1600}], ["sounds/magzap2", 370, {"v": -145}], ["sounds/blockhit", 390, {"v": -145, "p": 1200}], ["sounds/magzap3", 590, {"v": -145}], ["sounds/gemongem2", 610, {"v": -46, "p": 1200}], ["sounds/magzap2", 660, {"v": -145}], ["sounds/blockhit", 680, {"v": -145, "p": 1600}], ["sounds/magzap3", 880, {"v": -145}], ["sounds/gemongem2", 900, {"v": -46, "p": 1600}], ["sounds/magzap2", 950, {"v": -145}], ["sounds/blockhit", 970, {"v": -145, "p": 1200}], ["sounds/magzap3", 1170, {"v": -145}], ["sounds/gemongem2", 1190, {"v": -46, "p": 1200}], ["sounds/magzap2", 1240, {"v": -145}], ["sounds/blockhit", 1260, {"v": -145, "p": 1600}], ["sounds/magzap3", 1460, {"v": -145}], ["sounds/gemongem2", 1480, {"v": -46, "p": 1600}], ["sounds/magzap2", 1530, {"v": -145}], ["sounds/blockhit", 1550, {"v": -145, "p": 1200}], ["sounds/magzap3", 1750, {"v": -145}], ["sounds/gemongem2", 1770, {"v": -46, "p": 1200}], ["sounds/magzap2", 1820, {"v": -145}], ["sounds/blockhit", 1840, {"v": -145, "p": 1600}], ["sounds/magzap3", 2040, {"v": -145}], ["sounds/gemongem2", 2060, {"v": -46, "p": 1600}], ["sounds/magzap2", 2110, {"v": -145}], ["sounds/blockhit", 2130, {"v": -145, "p": 1200}], ["sounds/magzap3", 2330, {"v": -145}], ["sounds/gemongem2", 2350, {"v": -46, "p": 1200}], ["sounds/magzap2", 2400, {"v": -145}], ["sounds/blockhit", 2420, {"v": -145, "p": 1600}], ["sounds/magzap3", 2620, {"v": -145}], ["sounds/gemongem2", 2640, {"v": -46, "p": 1600}], ["sounds/magzap2", 2690, {"v": -145}], ["sounds/blockhit", 2710, {"v": -145, "p": 1200}], ["sounds/magzap3", 2910, {"v": -145}], ["sounds/gemongem2", 2930, {"v": -46, "p": 1200}], ["sounds/magzap2", 2980, {"v": -145}], ["sounds/blockhit", 3000, {"v": -145, "p": 1600}], ["sounds/magzap3", 3200, {"v": -145}], ["sounds/gemongem2", 3220, {"v": -46, "p": 1600}], ["sounds/magzap2", 3270, {"v": -145}], ["sounds/blockhit", 3290, {"v": -145, "p": 1200}], ["sounds/v_gameover", 3490], ["sounds/megaexplosion", 3490]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/mouseover.ogg", "Menu hover", "The pointer enters a main menu button (pitched by button); the quit box and the options window open"],
      ["sounds/click2.ogg", "Button click", "Any button goes down: menus, dialogs, pilot choice, story screens, the warp panels"],
      ["sounds/warpbutton.ogg", "Warp gate click", "A gate is clicked on the checkpoint box"],
      ["sounds/v_chooseyourpilot.ogg", "\"Choose your pilot\"", "The pilot choice screen opens"],
      ["sounds/timer1.ogg", "Tick", "The sound volume slider is let go; the weapon box's OK (a fix leaves it out)"]
    ] },
    { name: "Ship", sounds: [
      ["sounds/engineloop.ogg", "Engine loop", "A game is under way and the field is not in the warp (loops)"],
      ["sounds/hyperspaceloop1.ogg", "Hyperspace loop", "The field is in the warp (loops)"],
      ["sounds/magzap3.ogg", "Pick up", "A brick is picked up from the field"],
      ["sounds/magzap2.ogg", "Fire", "The ship fires the bricks it holds (right click)"],
      ["sounds/magzap1.ogg", "Can't do that", "A click that cannot pick up or fire"],
      ["sounds/laser.ogg", "Laser", "A laser shot removes a brick"],
      ["sounds/psychic.ogg", "Psychic", "The psychic tool is used"],
      ["sounds/activated.ogg", "Tool picked up", "A tool brick is picked up"]
    ] },
    { name: "Bricks", sounds: [
      ["sounds/gemongem2.ogg", "Brick lands", "A brick falls to the floor"],
      ["sounds/blockhit.ogg", "Brick hit", "A brick next to a set is hit as the set is made"],
      ["sounds/explode1.ogg", "Brick pops", "A brick is taken out beside a set, a kill-all brick goes off, or a slow-mo brick spins"],
      ["sounds/steelclang1.ogg", "Steel clang", "A whole steel brick is hit for the first time"],
      ["sounds/mutator1.ogg", "Mutation", "A brick mutates"],
      ["sounds/mudestroy3.ogg", "Mutant dies", "A mutated brick is destroyed"],
      ["sounds/hyperbrick_sound.ogg", "Booster pulse", "A booster brick pulses"],
      ["sounds/freezeblock1.ogg", "Freeze", "A freeze brick goes off"]
    ] },
    { name: "Sets and combos", sounds: [
      ["sounds/gotset2.ogg", "Set", "A set of bricks is made"],
      ["sounds/gotsetbig.ogg", "Big set", "A big set of bricks is made"],
      ["sounds/bubbly.ogg", "Set with a mark", "A set is made that holds a marked brick (instead of the set sounds)"],
      ["sounds/combo_tone.ogg", "Combo ping", "A chain goes past its first set, pitched higher up the chain; not twice within 45 updates"],
      ["sounds/v_combo1.ogg", "Combo voice 1", "A chain reaches 2"],
      ["sounds/v_combo2.ogg", "Combo voice 2", "A chain reaches 3"],
      ["sounds/v_combo3.ogg", "Combo voice 3", "A chain reaches 4"],
      ["sounds/v_combo4.ogg", "Combo voice 4", "A chain reaches 5"],
      ["sounds/v_combo5.ogg", "Combo voice 5", "A chain reaches 6"],
      ["sounds/v_combo6.ogg", "Combo voice 6", "A chain reaches 7"],
      ["sounds/v_combo65.ogg", "Combo voice 7", "A chain reaches 8"],
      ["sounds/v_combo7.ogg", "Combo voice 8", "A chain reaches 9 or more"]
    ] },
    { name: "Power-ups", sounds: [
      ["sounds/killall_explode.ogg", "Kill-all blast", "A kill-all brick goes off"],
      ["sounds/star_explode.ogg", "Star blast", "A star brick goes off"],
      ["sounds/nukeexplosion1.ogg", "Row nuke", "A row nuke goes off"],
      ["sounds/ckiller4.ogg", "Column nuke", "A column nuke goes off; a column killer is fired"],
      ["sounds/galaxy.ogg", "Galaxy", "A galaxy brick goes off (with the column nuke)"],
      ["sounds/star9.ogg", "Fuel cell", "A fuel cell is destroyed"],
      ["sounds/v_weaponready.ogg", "\"Weapon ready\"", "The power meter fills"],
      ["sounds/v_poweractivated.ogg", "\"Power activated\"", "A power is started"],
      ["sounds/explosion.ogg", "Weapon blast", "A weapon explodes; a power's blast goes off"],
      ["sounds/sloooowdown.ogg", "Slow down", "The slow-mo power starts; the ships screen opens"],
      ["sounds/powerupwearsoff.ogg", "Power wears off", "A power runs out"],
      ["sounds/v_weaponupgrade.ogg", "\"Weapon upgrade\"", "The upgrade box shows"]
    ] },
    { name: "Levels and warp", sounds: [
      ["sounds/v_welcome.ogg", "\"Welcome\"", "A game begins"],
      ["sounds/v_getready.ogg", "\"Get ready\"", "A story or warp ends and the stage starts"],
      ["sounds/warp3.ogg", "Warp", "The warp starts and ends"],
      ["sounds/v_warpengaged.ogg", "\"Warp engaged\"", "The warp starts"],
      ["sounds/comptext.ogg", "Computer text", "The warp's text is typed out, a character at a time"],
      ["sounds/hyperspacethunder1.ogg", "Thunder 1", "In the warp, a lightning flash (one of two at random)"],
      ["sounds/hyperspacethunder2.ogg", "Thunder 2", "In the warp, a lightning flash (one of two at random)"],
      ["sounds/level_up1.ogg", "Hundred left", "The last hundred bricks of a stage begin"],
      ["sounds/level_up2.ogg", "Fifty left", "The last fifty bricks of a stage begin"],
      ["sounds/brickmilestone.ogg", "Milestone", "A part of the stage's bricks is cleared"],
      ["sounds/v_levelcomplete.ogg", "\"Level complete\"", "A level is won"],
      ["sounds/wordswoosh.ogg", "Word swoosh", "A big word flies across the field (level complete, milestones)"],
      ["sounds/v_bonusround.ogg", "\"Bonus round\"", "A bonus round starts"],
      ["sounds/v_perfect.ogg", "\"Perfect!\"", "The field is cleared; the perfect bonus is shown"],
      ["sounds/count.ogg", "Score count", "The bonus points are counted up on the level summary"]
    ] },
    { name: "Danger and game over", sounds: [
      ["sounds/klaxon.ogg", "Klaxon", "The field has no room for the next piece (loops)"],
      ["sounds/v_caution.ogg", "\"Caution\"", "The field's siren sounds as it fills up"],
      ["sounds/v_danger.ogg", "\"Danger\"", "Loaded, never played"],
      ["sounds/v_nomoremoves.ogg", "\"No more moves\"", "No move is left in the field"],
      ["sounds/v_gameover.ogg", "\"Game over\"", "The game ends"],
      ["sounds/megaexplosion.ogg", "Mega explosion", "The game ends"],
      ["sounds/v_newhighscore.ogg", "\"New high score\"", "A game ends with a new high score"]
    ] }
  ] },
  atomica: {
    presets: [
      { name: "Get Ready", when: "A game starts", steps: [["sounds/capclick", 0], ["sounds/swoop", 0], ["sounds/dooropen", 450], ["sounds/voices/getready", 510], ["sounds/whoosh", 1000], ["sounds/voices/go", 2540]]},
      { name: "Super Combo", when: "Molecules made one after another", steps: [["sounds/createmolecule", 0], ["sounds/geiger1", 10, {"v": -1687, "loop": 1, "end": 5570, "ramp": [[2650, -1354]]}], ["sounds/whoosh", 10], ["sounds/voices/Combo", 310], ["sounds/createmolecule", 2640], ["sounds/whoosh", 2650], ["sounds/voices/supercombo", 2950]]},
      { name: "Level Up", when: "The meter fills", steps: [["sounds/createmolecule", 0], ["sounds/whoosh", 10], ["sounds/voices/levelup", 360], ["sounds/whoosh", 2000]]},
      { name: "Game Over", when: "The board fills", steps: [["sounds/whoosh", 0], ["sounds/voices/gameover", 260], ["sounds/doorslam", 3460], ["sounds/voices/anewhighscore", 3990], ["sounds/swoop", 4520]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/capclick.ogg", "Cap click", "A button is pressed: the main menu, high scores box, quit box, hint box check box, most options buttons; also the bomb button lighting up and the bonus molecule frame sliding in or out"],
      ["sounds/swoop.ogg", "Swoop", "A menu panel slides in or out of the main menu (and when a game is started from it)"],
      ["sounds/doorslam.ogg", "Door slam", "The main menu's door shuts as the board closes"],
      ["sounds/dooropen.ogg", "Door open", "The main menu's door opens once its panel has slid away"]
    ] },
    { name: "Atoms", sounds: [
      ["sounds/atomappear.ogg", "Atom appears", "A new atom finishes starting to appear on the grid"],
      ["sounds/selectatom.ogg", "Select atom", "An atom is picked up; the options dialog's reset-hints button"],
      ["sounds/moveatom.ogg", "Move atom", "A picked atom is sent along a path to a new cell"],
      ["sounds/movebioatom.ogg", "Move biohazard", "A biohazard atom is sent along a path to a new cell"],
      ["sounds/whoosh.ogg", "Whoosh", "Atoms fly away (a molecule made, an element atom shatters, the grid is cleared, a bonus ring starts); a new level's atoms arrive"],
      ["sounds/lowwhoosh.ogg", "Low whoosh", "Each atom a swirl carries off disappears"],
      ["sounds/magnet.ogg", "Magnet", "Atoms are pulled along toward each other after a move"]
    ] },
    { name: "Molecules and combos", sounds: [
      ["sounds/createmolecule.ogg", "Molecule created", "Atoms join to make a molecule"],
      ["sounds/electric1.ogg", "Electric 1", "Lightning bonds between neighbouring atoms of one type; one of the three picked at random; the infection bolt plays all three at once and then one every 10 updates"],
      ["sounds/electric2.ogg", "Electric 2", "Lightning bonds between neighbouring atoms of one type; one of the three picked at random; the infection bolt plays all three at once and then one every 10 updates"],
      ["sounds/electric3.ogg", "Electric 3", "Lightning bonds between neighbouring atoms of one type; one of the three picked at random; the infection bolt plays all three at once and then one every 10 updates"],
      ["sounds/geiger1.ogg", "Geiger counter 1", "Loops while the combo is 2 or more, its volume following the combo; stops when it fades to 0 or the grid is reset"],
      ["sounds/geiger2.ogg", "Geiger counter 2", "Loops while the combo is above 3, its volume following the combo; stops when it fades to 0 or the grid is reset"],
      ["sounds/voices/combo.ogg", "Combo voice", "The combo reaches 2"],
      ["sounds/voices/supercombo.ogg", "Super combo voice", "The combo reaches 3"],
      ["sounds/voices/megacombo.ogg", "Mega combo voice", "The combo reaches 4"],
      ["sounds/voices/ultracombo.ogg", "Ultra combo voice", "The combo reaches 5"],
      ["sounds/voices/hypercombo.ogg", "Hyper combo voice", "The combo reaches 6"],
      ["sounds/voices/gigacombo.ogg", "Giga combo voice", "The combo reaches 7"],
      ["sounds/voices/maxcombo.ogg", "Max combo voice", "The combo reaches 8"]
    ] },
    { name: "Bombs and biohazards", sounds: [
      ["sounds/nukeblast.ogg", "Nuke blast", "An atom is destroyed by the atomic bomb or an element's blast"],
      ["sounds/heartbeat.ogg", "Heartbeat", "A biohazard atom's glow reaches its brightest; repeats with the pulse"],
      ["sounds/voices/atomicbombready.ogg", "Bomb ready voice", "The atomic bomb button finishes lighting"],
      ["sounds/voices/atomicbomb.ogg", "Atomic bomb voice", "The bomb becomes available with atoms on the grid"],
      ["sounds/voices/biohazard.ogg", "Biohazard voice", "The first biohazard atom of a grid finishes appearing (one of two choices, both this voice)"],
      ["sounds/voices/biohazardalert.ogg", "Biohazard alert (unused)", "Never played"]
    ] },
    { name: "Elements and bonus molecules", sounds: [
      ["sounds/freeze.ogg", "Freeze", "A freeze takes hold of the board (the freeze progress reaches 128)"],
      ["sounds/voices/fire.ogg", "Fire voice", "A fire element atom is placed"],
      ["sounds/voices/ice.ogg", "Ice voice", "A water (ice) element atom is placed"],
      ["sounds/voices/lightning.ogg", "Lightning voice", "An earth (lightning) element atom is placed"],
      ["sounds/voices/wind.ogg", "Wind voice", "An air (wind) element atom is placed"],
      ["sounds/voices/ether.ogg", "Ether voice", "An ether element atom is placed"],
      ["sounds/voices/bonusmolecule.ogg", "Bonus molecule voice", "A bonus molecule is made"],
      ["sounds/voices/bonusmoleculeready.ogg", "Bonus ready voice", "The bonus molecule frame has slid into place with its pattern ready"],
      ["sounds/voices/bonusmoleculeupgraded.ogg", "Bonus upgraded voice", "A bonus molecule is upgraded"]
    ] },
    { name: "Level start and end", sounds: [
      ["sounds/voices/getready.ogg", "Get ready voice", "A level begins"],
      ["sounds/voices/go.ogg", "Go voice", "The grid's field is revealed and play starts"],
      ["sounds/voices/levelup.ogg", "Level up voice", "The meter fills and the level goes up"],
      ["sounds/voices/gameover.ogg", "Game over voice", "The game ends (300 updates into the game-over count)"],
      ["sounds/voices/anewhighscore.ogg", "New high score voice", "The name entry box opens for a score that earns a place"],
      ["sounds/voices/cool.ogg", "Cool voice", "Okay is pressed on a high score name: one of six voices at random"],
      ["sounds/voices/excellent.ogg", "Excellent voice", "Okay is pressed on a high score name: one of six voices at random"],
      ["sounds/voices/great.ogg", "Great voice", "Okay is pressed on a high score name: one of six voices at random"],
      ["sounds/voices/nifty.ogg", "Nifty voice", "Okay is pressed on a high score name: one of six voices at random"],
      ["sounds/voices/terrific.ogg", "Terrific voice", "Okay is pressed on a high score name: one of six voices at random"],
      ["sounds/voices/wicked.ogg", "Wicked voice", "Okay is pressed on a high score name: one of six voices at random"],
      ["sounds/voices/awesome.ogg", "Awesome voice (unused)", "Never played"],
      ["sounds/voices/levelcomplete.ogg", "Level complete voice (unused)", "Never played"],
      ["sounds/voices/beautiful.ogg", "Beautiful voice (unused)", "Never played"]
    ] },
  ] },
  bejeweled: {
    presets: [
      { name: "Swap", when: "A swap that makes one match", steps: [["sounds/select", 0, {"p": 299}], ["sounds/gotset2", 380, {"p": 98}], ["sounds/combo22", 380, {"p": 98}], ["sounds/gemongem2", 770, {"p": 98}]]},
      { name: "Cascade x2", when: "A swap whose falling gems make a second match", steps: [["sounds/select", 0, {"p": -102}], ["sounds/gotset2", 380, {"p": -102}], ["sounds/combo22", 380, {"p": -102}], ["sounds/gemongem2", 940, {"p": -102}], ["sounds/gemongem2", 1040, {"p": -102}], ["sounds/gotset2", 1050, {"p": 98}], ["sounds/combo32", 1050, {"p": 98}], ["sounds/gemongem2", 1430, {"p": 98}]]},
      { name: "Cascade x3", when: "Three matches from one swap", steps: [["sounds/select", 0, {"p": 299}], ["sounds/gotset2", 380, {"p": 299}], ["sounds/combo22", 380, {"p": 299}], ["sounds/gemongem2", 940, {"p": 299}], ["sounds/gemongem2", 1040, {"p": 299}], ["sounds/gotset2", 1050, {"p": 299}], ["sounds/combo32", 1050, {"p": 299}], ["sounds/gemongem2", 1600, {"p": 299}], ["sounds/gemongem2", 1700, {"p": 299}], ["sounds/gotset2", 1710, {"p": 500}], ["sounds/combo42", 1710, {"p": 500}], ["sounds/gemongem2", 2090, {"p": 500}]]},
      { name: "Cascade x4", when: "Four matches from one swap", steps: [["sounds/select", 0, {"p": -303}], ["sounds/gotset2", 380, {"p": -303}], ["sounds/combo22", 380, {"p": -303}], ["sounds/gemongem2", 940, {"p": -303}], ["sounds/gemongem2", 1040, {"p": -303}], ["sounds/gotset2", 1050, {"p": -102}], ["sounds/combo32", 1050, {"p": -102}], ["sounds/gemongem2", 1430, {"p": -102}], ["sounds/gotset2", 1510, {"p": 299}], ["sounds/combo42", 1510, {"p": 299}], ["sounds/gemongem2", 1880, {"p": 299}], ["sounds/gotset2", 1960, {"p": -102}], ["sounds/combo52", 1960, {"p": -102}], ["sounds/gemongem2", 2400, {"p": -102}]]},
      { name: "Cascade x5", when: "Five matches from one swap, with the praise", steps: [["sounds/select", 0, {"p": 500}], ["sounds/gotset2", 380, {"p": 500}], ["sounds/combo22", 380, {"p": 500}], ["sounds/gemongem2", 940, {"p": 500}], ["sounds/gemongem2", 1040, {"p": 500}], ["sounds/gotset2", 1050, {"p": 500}], ["sounds/combo32", 1050, {"p": 500}], ["sounds/gemongem2", 1600, {"p": 500}], ["sounds/gemongem2", 1700, {"p": 500}], ["sounds/gotset2", 1710, {"p": 500}], ["sounds/combo42", 1710, {"p": 500}], ["sounds/gemongem2", 2090, {"p": 500}], ["sounds/gotset2", 2170, {"p": 299}], ["sounds/combo52", 2170, {"p": 299}], ["sounds/gemongem2", 2610, {"p": 299}], ["sounds/excellent1", 2620], ["sounds/gotset2", 2620, {"p": 500}], ["sounds/combo52", 2620, {"p": 500}], ["sounds/gemongem2", 3060, {"p": 500}]]},
      { name: "Level Up", when: "The level is won: the board drains away and the next one falls in", steps: [["sounds/whirlpool1", 0, {"loop": 1, "end": 3200, "ramp": [[0, -9854], [20, -1804], [50, -1527], [90, -1246], [190, -767], [320, -363], [490, 0], [2540, -14], [2810, -471], [2910, -711], [3000, -990], [3090, -1377], [3160, -1831], [3170, -9854]]}], ["sounds/levelup1", 1730], ["sounds/firecrackle", 2530], ["sounds/gemongem2", 4870, {"p": 299}], ["sounds/gemongem2", 4980, {"p": 98}], ["sounds/gemongem2", 5090, {"p": 701}], ["sounds/gemongem2", 5200, {"p": -102}], ["sounds/gemongem2", 5300, {"p": -303}], ["sounds/gemongem2", 5410, {"p": 98}], ["sounds/gemongem2", 5510, {"p": -405}]]},
      { name: "Game Over", when: "No moves are left: the board shakes and the gems fall away", steps: [["sounds/explode2", 0], ["sounds/gameover1", 1290]]},
      { name: "Time Up", when: "Timed mode's bar runs out", steps: [["sounds/warning1", 0], ["sounds/warning1", 940], ["sounds/warning1", 1790], ["sounds/warning1", 2590], ["sounds/warning1", 3390], ["sounds/warning1", 4190], ["sounds/warning1", 4990], ["sounds/warning1", 5790], ["sounds/warning1", 6590], ["sounds/warning1", 7390], ["sounds/warning1", 8190], ["sounds/warning1", 8990], ["sounds/warning1", 9790], ["sounds/warning1", 10590], ["sounds/warning1", 11390], ["sounds/warning1", 12190], ["sounds/explode2", 14640], ["sounds/gameover1", 15850]]}
    ],
    groups: [
    { name: "Matches", sounds: [
      ["sounds/select.ogg", "Select", "A gem is clicked"],
      ["sounds/gotset2.ogg", "Match", "A match of three"],
      ["sounds/gotsetbig2.ogg", "Big match", "A match of four or more"],
      ["sounds/multishot.ogg", "Two matches", "Two or more matches at once"],
      ["sounds/bad2.ogg", "Bad swap", "A swap that makes no match"],
      ["sounds/gemongem2.ogg", "Gems land", "Falling gems land"],
      ["sounds/excellent1.ogg", "\"Excellent!\"", "One move scores 120 points or more (times the level's multiplier)"]
    ] },
    { name: "Cascades", sounds: [
      ["sounds/combo22.ogg", "Cascade 1", "Every match a move makes first"],
      ["sounds/combo32.ogg", "Cascade 2", "The second match of a move"],
      ["sounds/combo42.ogg", "Cascade 3", "The third match of a move"],
      ["sounds/combo52.ogg", "Cascade 4", "The fourth match of a move and every one after"],
      ["sounds/combo62.ogg", "Cascade 5 (unused)", "Never played"],
      ["sounds/combo72.ogg", "Cascade 6 (unused)", "Never played"]
    ] },
    { name: "Levels", sounds: [
      ["sounds/go1.ogg", "\"Go!\"", "A new game's board has fallen in"],
      ["sounds/whirlpool1.ogg", "Whirlpool", "The level is won: the board drains away (loops)"],
      ["sounds/levelup1.ogg", "Level up", "The draining gems reach the middle"],
      ["sounds/firecrackle.ogg", "Crackle", "The board has drained: the next level's starts falling"],
      ["sounds/warning1.ogg", "Warning", "Timed mode's bar is low, faster as it empties"]
    ] },
    { name: "Game over", sounds: [
      ["sounds/explode2.ogg", "Explosion", "The board shakes, then the gems fall away"],
      ["sounds/gameover1.ogg", "\"Game over\"", "The gems have fallen away"]
    ] },
    { name: "Menus", sounds: [
      ["sounds/menuclick.ogg", "Click", "A button is pressed"],
      ["sounds/click2.ogg", "Click 2 (unused)", "Loaded, never played"],
      ["sounds/sidebarclick2.ogg", "Side bar click (unused)", "Never played"],
      ["sounds/welcome1.ogg", "Welcome (unused)", "Never played"],
      ["sounds/goodbye1.ogg", "Goodbye (unused)", "Never played"]
    ] }
  ] },
  bejeweled2: {
    presets: [
      { name: "Swap", when: "A swap that makes one match", steps: [["sounds/select", 0, {"p": 300}], ["sounds/gotset2", 380, {"p": 100}], ["sounds/combo22", 380, {"p": 100}], ["sounds/gemongem2", 810, {"p": 100}]]},
      { name: "Cascade x2", when: "A swap whose falling gems make a second match", steps: [["sounds/select", 0, {"p": 300}], ["sounds/gotset2", 380, {"p": 100}], ["sounds/combo22", 380, {"p": 100}], ["sounds/gemongem2", 810, {"p": 100}], ["sounds/gotset2", 880, {"p": -100}], ["sounds/combo32", 880, {"p": -100}], ["sounds/gemongem2", 1400, {"p": -100}]]},
      { name: "Cascade x3", when: "Three matches from one swap", steps: [["sounds/select", 0, {"p": 100}], ["sounds/gotset2", 380, {"p": 300}], ["sounds/combo22", 380, {"p": 300}], ["sounds/gemongem2", 810, {"p": 300}], ["sounds/gotset2", 880, {"p": 100}], ["sounds/combo32", 880, {"p": 100}], ["sounds/gemongem2", 1400, {"p": 100}], ["sounds/gotset2", 1410, {"p": 100}], ["sounds/combo42", 1410, {"p": 100}], ["sounds/gemongem2", 1940, {"p": 100}]]},
      { name: "Cascade x4", when: "Four matches from one swap", steps: [["sounds/select", 0, {"p": 100}], ["sounds/gotset2", 380, {"p": 100}], ["sounds/combo22", 380, {"p": 100}], ["sounds/gemongem2", 990, {"p": 100}], ["sounds/gemongem2", 1110, {"p": 100}], ["sounds/gotset2", 1120, {"p": 100}], ["sounds/combo32", 1120, {"p": 100}], ["sounds/gemongem2", 1740, {"p": 100}], ["sounds/gemongem2", 1860, {"p": 100}], ["sounds/gotset2", 1870, {"p": 100}], ["sounds/combo42", 1870, {"p": 100}], ["sounds/gemongem2", 2300, {"p": 100}], ["sounds/gotset2", 2370, {"p": 100}], ["sounds/combo52", 2370, {"p": 100}], ["sounds/gemongem2", 2970, {"p": 100}], ["sounds/gemongem2", 3090, {"p": 100}]]},
      { name: "Flame Gem", when: "A match that sets off a flame gem", steps: [["sounds/select", 0, {"p": 300}], ["sounds/gotset2", 380, {"p": 500}], ["sounds/combo22", 380, {"p": 500}], ["sounds/bombexplode", 380], ["sounds/gemongem2", 800, {"p": 700}], ["sounds/gemongem2", 980, {"p": 300}], ["sounds/gemongem2", 1090, {"p": 300}]]},
      { name: "Level Complete", when: "The level bar fills: the board drains away and the next one falls in", steps: [["sounds/Level_Complete", 0], ["sounds/whirlpool1", 30], ["sounds/Get_ready", 6210], ["sounds/gemongem2", 8260, {"p": 500}], ["sounds/gemongem2", 8360, {"p": -500}], ["sounds/gemongem2", 8460, {"p": -100}], ["sounds/gemongem2", 8570, {"p": -500}], ["sounds/gemongem2", 8670, {"p": 100}]]},
      { name: "Game Over", when: "No moves are left", steps: [["sounds/No_More_Moves", 0], ["sounds/explode2", 2000]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/mainmenu_mouseover.ogg", "Menu hover", "The pointer enters a main menu or dialog button"],
      ["sounds/mainmenu_mouseoff.ogg", "Menu leave", "The pointer leaves a main menu or dialog button"],
      ["sounds/mainmenu_mouseclick.ogg", "Menu click", "A main menu game button is pressed"],
      ["sounds/landinglights.ogg", "Landing lights", "The main menu's lights run one after the other"],
      ["sounds/mystery.ogg", "Fairy dust", "The secret button on the main menu is switched on"],
      ["sounds/mainmenu_gamestart.ogg", "Game start", "A game starts from the main menu"],
      ["sounds/menuclick.ogg", "Click", "A button goes down: the menu, the board, the galaxy map, the high scores"],
      ["sounds/menuclick2.ogg", "Dialog click", "A button goes down in a dialog; an option is switched"],
      ["sounds/click2.ogg", "Click 2 (unused)", "Never played"],
      ["sounds/panelslide1.ogg", "Panel slide", "A panel slides in or out: the high scores, the galaxy map, the options window, the puzzle dome"],
      ["sounds/Welcome.ogg", "\"Welcome\"", "The main menu opens with no player yet"],
      ["sounds/Welcome_Back.ogg", "\"Welcome back\"", "The main menu opens again with a player"],
      ["sounds/Goodbye.ogg", "\"Goodbye\"", "Quitting is confirmed"]
    ] },
    { name: "Swaps and matches", sounds: [
      ["sounds/select.ogg", "Select", "A gem is clicked"],
      ["sounds/bad2.ogg", "Bad swap", "A swap is refused or makes no match; a locked planet is clicked"],
      ["sounds/gotset2.ogg", "Match", "A match of three; the sound volume slider is let go"],
      ["sounds/gotsetbig2.ogg", "Big match", "Two or more matches at once"],
      ["sounds/multishot.ogg", "Multi match", "A move makes several matches"],
      ["sounds/gemongem2.ogg", "Gems land", "Falling gems land"],
      ["sounds/combo22.ogg", "Cascade 1", "The first match a move makes"],
      ["sounds/combo32.ogg", "Cascade 2", "The second match of a move"],
      ["sounds/combo42.ogg", "Cascade 3", "The third match of a move"],
      ["sounds/combo52.ogg", "Cascade 4", "The fourth match of a move"],
      ["sounds/combo62.ogg", "Cascade 5", "The fifth match of a move"],
      ["sounds/combo72.ogg", "Cascade 6", "The sixth match of a move and every one after"]
    ] },
    { name: "Special gems", sounds: [
      ["sounds/hypergem_creation.ogg", "Hypercube made", "A hypercube is made; the logo's gem forms"],
      ["sounds/hypergem_destroyed.ogg", "Hypercube used", "A hypercube is used and bursts"],
      ["sounds/electro_start.ogg", "Lightning start", "A hypercube is swapped, the lightning starts"],
      ["sounds/electro_path.ogg", "Lightning", "The lightning jumps to a gem"],
      ["sounds/electro_explode.ogg", "Lightning blast", "The lightning's gems explode"],
      ["sounds/bombexplode.ogg", "Gem explodes", "A flame gem is matched, or a bomb gem's count runs out"]
    ] },
    { name: "Praise", sounds: [
      ["sounds/Good.ogg", "\"Good\"", "A single chain after a long quiet stretch"],
      ["sounds/excellent1.ogg", "\"Excellent\"", "A chain of two; the logo's gem finishes forming"],
      ["sounds/Incredible.ogg", "\"Incredible\"", "A chain of three or more"]
    ] },
    { name: "Levels", sounds: [
      ["sounds/Get_ready.ogg", "\"Get ready\"", "A level's warp finishes"],
      ["sounds/Go.ogg", "\"Go\"", "A level or game begins"],
      ["sounds/whirlpool1.ogg", "Whirlpool", "The level is won and the board drains away; a new game's board fills"],
      ["sounds/Level_Complete.ogg", "\"Level complete\"", "The level is won"],
      ["sounds/firecrackle.ogg", "Fire crackle", "Loaded, never played"],
      ["sounds/warning1.ogg", "Warning", "A timed game's bar is low, faster as it empties"]
    ] },
    { name: "Puzzles", sounds: [
      ["sounds/planetselect.ogg", "Planet select", "A planet is picked on the galaxy map"],
      ["sounds/galaxymapbutton.ogg", "Galaxy map", "The puzzle dome's button opens the galaxy map"],
      ["sounds/Puzzle_Solved.ogg", "\"Puzzle solved\"", "A puzzle is solved"],
      ["sounds/Warp_Drive_Ready.ogg", "\"Warp drive ready\"", "The fourth puzzle of a planet is solved"],
      ["sounds/World_Complete.ogg", "\"World complete\"", "The fifth puzzle of a planet is solved"]
    ] },
    { name: "Game over", sounds: [
      ["sounds/explode2.ogg", "Explosion", "The board blows up after the game ends"],
      ["sounds/No_More_Moves.ogg", "\"No more moves\"", "No move is left"],
      ["sounds/Time_Up.ogg", "\"Time up\"", "A timed game's bar runs out"],
      ["sounds/Game_Over.ogg", "\"Game over\"", "Loaded, never played"],
      ["sounds/A_New_High_Score.ogg", "\"A new high score\"", "A game ends with a score on the list"]
    ] }
  ] },
  bejeweledtwist: {
    presets: [
      { name: "Start Classic", when: "Classic is chosen", steps: [["sounds/doorlock2", 0, {"v": -650}], ["sounds/voice_classic", 0, {"v": -650}], ["sounds/trans_spacenoise", 0], ["sounds/buttonpress2", 0, {"v": -650}], ["sounds/trans_arrival", 3610, {"v": -650}]]},
      { name: "Level Complete", when: "The level bar fills", steps: [["sounds/voice_levelcomplete", 0, {"v": -650}], ["sounds/trans_spacenoise", 300, {"v": -650, "ramp": [[2600, -563], [2640, -274], [2690, 0]]}], ["sounds/rowblow", 1080, {"v": -1707}], ["sounds/rowblow", 1310, {"v": -1707}], ["sounds/rowblow", 1430, {"v": -1707, "ramp": [[2600, -1628], [2670, -1197]]}], ["sounds/rowblow", 1550, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 1670, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 1790, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 1910, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 2030, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/trans_preignition", 2360, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/geode", 2440, {"v": -1573, "r": 1.0234, "ramp": [[2600, -1494], [2640, -1226], [2690, -968]]}], ["sounds/trans_engineignition", 3600, {"v": -851}], ["sounds/trans_engineloop1", 3600, {"v": -1176, "loop": 1, "end": 8350, "ramp": [[3600, -526], [8260, -627], [8290, -1009], [8310, -1377], [8320, -1628], [8330, -1962], [8340, -2465]]}], ["sounds/trans_arrival", 7210, {"v": -650}], ["sounds/voice_go", 9580, {"v": -650}]]},
      { name: "Power Gem", when: "A power gem is made and set off", steps: [["sounds/powergem_created", 0, {"v": -1102, "p": -600}], ["sounds/doubleset", 0, {"v": -796, "p": 1}], ["sounds/preblast", 1090, {"v": -650}], ["sounds/gem_shatters", 1090, {"v": -968, "p": -600}], ["sounds/combo_3", 1090, {"v": -650}], ["sounds/voice_good", 1100, {"v": -650}]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/voice_welcometobejeweledtwist.ogg", "\"Welcome to Bejeweled Twist\"", "The loading screen finishes"],
      ["sounds/mouseover1.ogg", "Hover 1", "The pointer enters a button or dialog button"],
      ["sounds/mouseover2.ogg", "Hover 2", "The pointer enters a challenge button"],
      ["sounds/mouseover3.ogg", "Hover 3", "The pointer enters a game mode button"],
      ["sounds/click.ogg", "Click", "A button goes down: the main menu, the mode screen, the high scores, the award screens, the board's buttons"],
      ["sounds/click_dialog.ogg", "Dialog click", "A button goes down in a dialog or the options; an option is switched"],
      ["sounds/menu_button_click.ogg", "Menu click", "A main menu button is pressed; a name is accepted in the profile box"],
      ["sounds/buttonpress1.ogg", "Replay button", "A button on the replay pod is let go"],
      ["sounds/buttonpress2.ogg", "Button press", "A button is let go on the mode screen, the challenge screen or beside the level bar"],
      ["sounds/menu_slide.ogg", "Menu slide", "A high scores page slides; the zen tip slides in"],
      ["sounds/doorlock2.ogg", "Door lock", "A main menu door is opened or shut; the profile list opens; the mode doors open"],
      ["sounds/doorlock3.ogg", "Door lock, locked", "The profile button is pressed and cannot open"],
      ["sounds/steamlock1.ogg", "Steam lock", "The main menu's gates open to start a game"],
      ["sounds/lock1.ogg", "Gate lock", "The gates are a third of the way open or shut"],
      ["sounds/innerlock1.ogg", "Inner lock, in", "The mode screen opens"],
      ["sounds/innerlock2.ogg", "Inner lock, out", "The mode screen is left for the main menu"],
      ["sounds/voice_chooseyourgame.ogg", "\"Choose your game\"", "The mode screen opens"],
      ["sounds/voice_classic.ogg", "\"Classic\"", "The Classic mode is pressed on the mode screen"],
      ["sounds/voice_challenge.ogg", "\"Challenge\"", "The Challenge mode is pressed on the mode screen"],
      ["sounds/voice_blitz.ogg", "\"Blitz\"", "The Blitz mode is pressed on the mode screen"],
      ["sounds/voice_zen.ogg", "\"Zen\"", "The Zen mode is pressed on the mode screen"],
      ["sounds/zenstart.ogg", "Zen start", "A Zen game begins"],
      ["sounds/typewriter1.ogg", "Typewriter", "A character is typed into a name box"],
      ["sounds/screenprint.ogg", "Screen print", "A high scores page is printed on"],
      ["sounds/transfer.ogg", "Volume check", "The sound volume slider is let go"],
      ["sounds/bubblepop.ogg", "Dialog pop", "A dialog pops up"],
      ["sounds/voice_goodbye.ogg", "\"Goodbye\"", "The game is quit"]
    ] },
    { name: "Starting and the cursor", sounds: [
      ["sounds/voice_go.ogg", "\"Go!\"", "A game starts after the board settles"],
      ["sounds/select.ogg", "Cursor tick", "The cursor rests on the board and ticks"],
      ["sounds/start_rotate.ogg", "Twist start", "A group of four gems starts to turn"],
      ["sounds/gem_prelock.ogg", "Gem about to lock", "A gem is about to be locked"],
      ["sounds/badmove.ogg", "Bad move", "A twist that makes no match, or a click that cannot be used"],
      ["sounds/tutorial_snap.ogg", "Tutorial snap", "The cursor jumps to a gem in the tutorial"],
      ["sounds/tutorial-match.ogg", "Tutorial match", "A match is made in the tutorial"],
      ["sounds/tutorialcomplete.ogg", "Tutorial done", "The tutorial is finished"],
      ["sounds/reshuffle1.ogg", "Reshuffle", "A challenge is restarted and the gems are shuffled"],
      ["sounds/purr.ogg", "Purr", "A tooltip about a coal gem appears (when coal is a snack)"]
    ] },
    { name: "Matches and chains", sounds: [
      ["sounds/combo_1.ogg", "Match 1", "A match with no cascade before it"],
      ["sounds/combo_2.ogg", "Match 2", "The second match of a cascade; the wheel's bomb is disarmed"],
      ["sounds/combo_3.ogg", "Match 3", "The third match of a cascade"],
      ["sounds/combo_4.ogg", "Match 4", "The fourth match of a cascade"],
      ["sounds/combo_5.ogg", "Match 5", "The fifth match of a cascade"],
      ["sounds/combo_6.ogg", "Match 6", "The sixth match of a cascade"],
      ["sounds/combo_7.ogg", "Match 7", "The seventh match of a cascade and after"],
      ["sounds/sixmatch.ogg", "Six-gem match", "A match of six or more gems (the pitch rises with the length)"],
      ["sounds/doubleset.ogg", "Double set", "Two sets are matched at once"],
      ["sounds/chainx1.ogg", "Chain note", "A cascade chain is counted: two notes an octave apart"],
      ["sounds/chainparticle1.ogg", "Chain smoke", "A puff of smoke from a chain moves along"],
      ["sounds/gem_hit.ogg", "Gem lands", "A falling gem lands"],
      ["sounds/speedbonus.ogg", "Speed bonus", "A speed bonus text is shown"],
      ["sounds/multiplier_up2_1.ogg", "Level up 1", "The level bar's multiplier goes up to level 1"],
      ["sounds/multiplier_up2_2.ogg", "Level up 2", "The multiplier goes up to level 2"],
      ["sounds/multiplier_up2_3.ogg", "Level up 3", "The multiplier goes up to level 3"],
      ["sounds/multiplier_up2_4.ogg", "Level up 4", "The multiplier goes up to level 4 and above"],
      ["sounds/multiplier_down.ogg", "Multiplier down", "The multiplier drops back"],
      ["sounds/voice_good.ogg", "\"Good\"", "A praise for a small cascade"],
      ["sounds/voice_excellent.ogg", "\"Excellent\"", "A praise for a bigger cascade"],
      ["sounds/voice_awesome.ogg", "\"Awesome\"", "A praise for a big cascade"],
      ["sounds/voice_incredible.ogg", "\"Incredible\"", "A praise for a bigger one"],
      ["sounds/voice_spectacular.ogg", "\"Spectacular\"", "A praise for a huge one"],
      ["sounds/voice_unbelievable.ogg", "\"Unbelievable\"", "A praise for the biggest"]
    ] },
    { name: "Special gems", sounds: [
      ["sounds/powergem_created.ogg", "Power gem made", "A match of four makes a power gem"],
      ["sounds/preblast.ogg", "Power gem charging", "A power gem is about to go off"],
      ["sounds/gem_shatters.ogg", "Power gem blast", "A power gem goes off"],
      ["sounds/electrogem_created.ogg", "Electro gem made", "A match of five makes an electro gem"],
      ["sounds/electro_start1.ogg", "Electro start", "An electro gem is matched and its lightning starts"],
      ["sounds/electro_explode.ogg", "Electro strike", "The lightning of an electro gem hits"],
      ["sounds/flamelightning.ogg", "Flame lightning", "A power gem's lightning strike is set off"],
      ["sounds/supergem_create.ogg", "Super gem made", "Loaded, never played"],
      ["sounds/wildcard_destroyed.ogg", "Wildcard used", "A wildcard gem is matched"],
      ["sounds/bonuslaser.ogg", "Fruit laser", "A fruit gem's shockwave starts"],
      ["sounds/fruit_appears.ogg", "Fruit gem appears", "A fruit gem is put on the board"],
      ["sounds/gem_fruit_destroyed.ogg", "Fruit gem matched", "A fruit gem is matched"],
      ["sounds/fruitbonuscount.ogg", "Fruit count", "A match adds to the fruit bonus colour count"],
      ["sounds/rowblow.ogg", "Row blows", "A row of gems blows up in a jump or a fruit blast"],
      ["sounds/gem_lock_start.ogg", "Lock starts", "A gem is locked"],
      ["sounds/gem_lock_end.ogg", "Lock ends", "A locked gem's lock wears off"],
      ["sounds/gem_lock_break.ogg", "Lock breaks", "A match breaks the lock on a gem"],
      ["sounds/coal_blasted.ogg", "Coal blasted", "A coal gem is blown up"],
      ["sounds/geode.ogg", "Geode", "A geode breaks open; a light of the rank bonus"],
      ["sounds/soulify.ogg", "Gem becomes a soul", "A gem turns into a soul"],
      ["sounds/gemsoul-lands3.ogg", "Soul lands", "A soul lands on the board"]
    ] },
    { name: "Bombs and counters", sounds: [
      ["sounds/gem_countdown.ogg", "Counter gem falls", "A gem with a countdown lands"],
      ["sounds/counter_warning.ogg", "Counter warning", "A countdown gem's number goes down (the pitch rises as it nears zero)"],
      ["sounds/gem_countdown_destroyed.ogg", "Counter gem destroyed", "A countdown gem is matched; a high scores row slides in"],
      ["sounds/bomb_explode.ogg", "Bomb explodes", "A countdown gem reaches zero or a bomb blows"],
      ["sounds/doomgem_appears.ogg", "Doom gem appears", "A doom gem is put on the board"],
      ["sounds/doomgem_countdown.ogg", "Doom gem ticks", "A doom gem's count changes"],
      ["sounds/doomgem_destroyed.ogg", "Doom gem destroyed", "A doom gem is matched"],
      ["sounds/doomgemgameover.ogg", "Doom gem game over", "The game ends because a doom gem went off"],
      ["sounds/endfinal6b.ogg", "Board blows up", "The game ends because a bomb went off"]
    ] },
    { name: "Time and levels", sounds: [
      ["sounds/voice_thirtyseconds.ogg", "\"Thirty seconds\"", "Thirty seconds are left in Blitz"],
      ["sounds/voice_oneminute.ogg", "\"One minute\"", "A Blitz minute mark is reached"],
      ["sounds/voice_twominutes.ogg", "\"Two minutes\"", "A Blitz minute mark is reached"],
      ["sounds/voice_threeminutes.ogg", "\"Three minutes\"", "A Blitz minute mark is reached"],
      ["sounds/voice_fourminutes.ogg", "\"Four minutes\"", "A Blitz minute mark is reached"],
      ["sounds/voice_nine.ogg", "\"Nine\"", "Nine seconds are left in Blitz"],
      ["sounds/voice_eight.ogg", "\"Eight\"", "Eight seconds are left in Blitz"],
      ["sounds/voice_seven.ogg", "\"Seven\"", "Seven seconds are left in Blitz"],
      ["sounds/voice_six.ogg", "\"Six\"", "Six seconds are left in Blitz"],
      ["sounds/voice_five.ogg", "\"Five\"", "Five seconds are left in Blitz"],
      ["sounds/voice_four.ogg", "\"Four\"", "Four seconds are left in Blitz"],
      ["sounds/voice_three.ogg", "\"Three\"", "Three seconds are left in Blitz"],
      ["sounds/voice_two.ogg", "\"Two\"", "Two seconds are left in Blitz"],
      ["sounds/voice_one.ogg", "\"One\"", "One second is left in Blitz"],
      ["sounds/voice_levelcomplete.ogg", "\"Level complete\"", "A level is cleared"],
      ["sounds/whirlpool1.ogg", "Whirlpool", "The board drains away after a level"],
      ["sounds/trans_spacenoise.ogg", "Space noise", "The jump to the next level begins"],
      ["sounds/trans_preignition.ogg", "Engine warms", "The jump's engine is about to start"],
      ["sounds/trans_engineignition.ogg", "Engine starts", "The jump's engine starts"],
      ["sounds/trans_engineloop1.ogg", "Engine runs", "The jump's engine runs (loops)"],
      ["sounds/trans_arrival.ogg", "Arrival", "The jump ends at the next level"],
      ["sounds/zentransition1.ogg", "Zen change 1", "A Zen wheel picture changes to a new one"],
      ["sounds/zentransition2.ogg", "Zen change 2", "A Zen wheel picture changes"],
      ["sounds/zentransition3.ogg", "Zen change 3", "A Zen wheel picture changes"],
      ["sounds/spinsequencer1.ogg", "Spin sequence", "The replay pod starts to spin"],
      ["sounds/rewind.ogg", "Rewind", "A move is run backwards in a replay"],
      ["sounds/replay_popup.ogg", "Replay pops up", "The replay pod pops up"]
    ] },
    { name: "The wheel", sounds: [
      ["sounds/x_spinnerappears1.ogg", "Wheel appears", "The skull wheel appears"],
      ["sounds/siren.ogg", "Siren", "The wheel's siren runs while the wheel spins (loops)"],
      ["sounds/temp_timpani.ogg", "Timpani", "A beat as the wheel's slots are lit"],
      ["sounds/x_wheelspin.ogg", "Wheel click 1", "A slot passes the wheel's pointer as it speeds up"],
      ["sounds/x_wheelspin2.ogg", "Wheel click 2", "A slot passes the pointer as it slows down"],
      ["sounds/skull-added.ogg", "Skull added", "A skull is added to the wheel"],
      ["sounds/x_win.ogg", "Wheel win", "The wheel stops on a good slot"],
      ["sounds/x_win2.ogg", "Wheel win, sparkle", "The wheel's win is tallied"],
      ["sounds/x_lose.ogg", "Wheel lose", "The wheel stops on a bad slot"],
      ["sounds/voice_bombdisarmed.ogg", "\"Bomb disarmed\"", "The wheel's bomb is disarmed"],
      ["sounds/spinner_unlock.ogg", "Wheel unlocks", "The wheel unfolds on the board"]
    ] },
    { name: "Game end and awards", sounds: [
      ["sounds/voice_gameover.ogg", "\"Game over\"", "A game ends"],
      ["sounds/tally_zoomin.ogg", "Score zooms in", "The final score or the end stats come in"],
      ["sounds/blitz-countup.ogg", "Score counts up", "A tally counts up: the end stats, a fruit blast, the wheel"],
      ["sounds/voice_anewhighscore.ogg", "\"A new high score\"", "The high scores show a new record"],
      ["sounds/voice_challengecomplete.ogg", "\"Challenge complete\"", "A challenge goal is reached"],
      ["sounds/challengecompleted.ogg", "Challenge done", "A challenge goal is reached"],
      ["sounds/challenge_unlock.ogg", "Challenge unlocks", "A challenge button unlocks on the challenge screen"],
      ["sounds/voice_newchallengeunlocked.ogg", "\"New challenge unlocked\"", "A new challenge button lights up"],
      ["sounds/gameunlocked.ogg", "Mode unlocked", "A new game mode is unlocked"],
      ["sounds/lightbar_blink.ogg", "Rank light", "A light of the rank bonus bar blinks"],
      ["sounds/rank_completion.ogg", "Rank complete", "A rank is completed"],
      ["sounds/trophy_awarded.ogg", "Trophy", "A trophy is awarded; the credits and the fruit award screen"],
      ["sounds/eclipseintro1.ogg", "Eclipse intro", "Loaded, never played"],
      ["sounds/badloop.ogg", "Bad loop", "Loaded, never played"],
      ["sounds/goodloop.ogg", "Good loop", "Loaded, never played"],
      ["sounds/trialclick.ogg", "Trial click", "Loaded, never played"],
      ["sounds/triallimit.ogg", "Trial limit", "Loaded, never played"],
      ["sounds/trialunlock.ogg", "Trial unlock", "Loaded, never played"]
    ] },
  ] },
  bigmoney: {
    presets: [
      { name: "Get Ready", when: "A game starts", steps: [["sounds/getready", 0], ["sounds/welcome", 370], ["sounds/go", 1390]]},
      { name: "Big Clear", when: "A clear of many coins", steps: [["sounds/bigmoney", 0], ["sounds/clearcoins3", 0], ["sounds/tone1", 0], ["sounds/unbelievable", 130], ["sounds/clearcoins3", 130], ["sounds/tone9", 130], ["sounds/awesome", 260], ["sounds/clearcoins2", 260], ["sounds/tone9", 260]]},
      { name: "Coin Chain", when: "Clears follow one another", steps: [["sounds/clearcoins2", 0], ["sounds/tone1", 0], ["sounds/clearcoins", 130], ["sounds/tone1", 130], ["sounds/clearcoins2", 260], ["sounds/tone1", 260], ["sounds/cointube", 270], ["sounds/cointube", 290], ["sounds/cointube", 330], ["sounds/cointube", 390], ["sounds/clearcoins2", 390], ["sounds/tone2", 390], ["sounds/cointube", 410], ["sounds/cointube", 450], ["sounds/cointube", 510], ["sounds/cointube", 520], ["sounds/cointube", 520], ["sounds/clearcoins", 520], ["sounds/tone3", 520], ["sounds/cointube", 540], ["sounds/cointube", 570], ["sounds/cointube", 590], ["sounds/cointube", 630], ["sounds/cointube", 630], ["sounds/clearcoins2", 650], ["sounds/tone3", 650], ["sounds/cointube", 660], ["sounds/cointube", 700], ["sounds/cointube", 730], ["sounds/cointube", 760], ["sounds/clearcoins2", 780], ["sounds/tone4", 780], ["sounds/cointube", 790], ["sounds/cointube", 850], ["sounds/cointube", 860], ["sounds/cointube", 860], ["sounds/cointube", 880], ["sounds/cointube", 900], ["sounds/clearcoins", 910], ["sounds/tone5", 910], ["sounds/cointube", 940], ["sounds/cointube", 970], ["sounds/cointube", 970], ["sounds/cointube", 1000], ["sounds/cointube", 1030], ["sounds/clearcoins2", 1040], ["sounds/tone5", 1040], ["sounds/cointube", 1070], ["sounds/cointube", 1100], ["sounds/cointube", 1130], ["sounds/clearcoins", 1170], ["sounds/tone6", 1170], ["sounds/cointube", 1190], ["sounds/cointube", 1200], ["sounds/cointube", 1220], ["sounds/cointube", 1260], ["sounds/cointube", 1300], ["sounds/clearcoins", 1300], ["sounds/tone6", 1300], ["sounds/cointube", 1330], ["sounds/cointube", 1350], ["sounds/cointube", 1420], ["sounds/clearcoins", 1430], ["sounds/tone7", 1430], ["sounds/cointube", 1440], ["sounds/cointube", 1470], ["sounds/cointube", 1540], ["sounds/clearcoins", 1560], ["sounds/tone7", 1560], ["sounds/cointube", 1570], ["sounds/cointube", 1640], ["sounds/cointube", 1690], ["sounds/clearcoins", 1690], ["sounds/tone7", 1690], ["sounds/cointube", 1760], ["sounds/cointube", 1810], ["sounds/clearcoins2", 1820], ["sounds/tone7", 1820], ["sounds/cointube", 1880], ["sounds/cointube", 1910], ["sounds/cointube", 1940], ["sounds/clearcoins2", 1950], ["sounds/tone8", 1950], ["sounds/cointube", 1970], ["sounds/cointube", 2020], ["sounds/cointube", 2020], ["sounds/cointube", 2030], ["sounds/cointube", 2040], ["sounds/cointube", 2040], ["sounds/cointube", 2060], ["sounds/cointube", 2080], ["sounds/clearcoins", 2080], ["sounds/tone9", 2080], ["sounds/cointube", 2120], ["sounds/cointube", 2130], ["sounds/cointube", 2150], ["sounds/cointube", 2190], ["sounds/clearcoins", 2210], ["sounds/tone9", 2210], ["sounds/cointube", 2230]]},
      { name: "Level Up", when: "The last bag of a level is won", steps: [["sounds/clearcoins", 0], ["sounds/tone9", 0], ["sounds/cointube", 20], ["sounds/cointube", 40], ["sounds/levelup", 70], ["sounds/cointube", 80], ["sounds/cointube", 160]]},
      { name: "Game Over", when: "The coins reach the top", steps: [["sounds/gameovertone", 0], ["sounds/gameover", 2800], ["sounds/anewhighscore", 5590]]}
    ],
    groups: [
    { name: "Selector", sounds: [
      ["sounds/buttonclick.ogg", "Button click", "Any button is pressed: the menus, the board, dialogs, the high scores"],
      ["sounds/welcome.ogg", "\"Welcome!\"", "The game selector, a moment after it opens, the first time in a run"],
      ["sounds/loveit.ogg", "\"Love it!\"", "The game selector, a moment after it opens, one time in ten after the first"],
      ["sounds/action.ogg", "\"Action!\"", "The action game is pointed at on the selector"],
      ["sounds/strategy.ogg", "\"Strategy!\"", "The strategy game is pointed at on the selector"],
      ["sounds/puzzle.ogg", "\"Puzzle!\"", "The puzzle game is pointed at on the selector"],
      ["sounds/wakeup.ogg", "Wake up", "Mr. Money has been left alone a long while and wakes"]
    ] },
    { name: "Start and rows", sounds: [
      ["sounds/getready.ogg", "\"Get ready!\"", "A game starts"],
      ["sounds/go.ogg", "\"Go!\"", "The countdown ends and play begins"],
      ["sounds/moveup.ogg", "Row rises", "A new row of coins pushes in from the bottom"],
      ["sounds/piecehit.au", "Piece lands", "A falling piece lands hard"]
    ] },
    { name: "Clearing coins", sounds: [
      ["sounds/clearcoins.ogg", "Clear, small", "A group of 4 coins or fewer is cleared; a pickpocket clears a group"],
      ["sounds/clearcoins2.ogg", "Clear, medium", "A group of 5 to 9 coins is cleared"],
      ["sounds/clearcoins3.ogg", "Clear, big", "A group of 10 coins or more is cleared"],
      ["sounds/tone1.ogg", "Tone 1", "The note after a clear when the meter is nearly empty; the first coin of a row sweep"],
      ["sounds/tone2.ogg", "Tone 2", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone3.ogg", "Tone 3", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone4.ogg", "Tone 4", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone5.ogg", "Tone 5", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone6.ogg", "Tone 6", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone7.ogg", "Tone 7", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone8.ogg", "Tone 8", "The note after a clear as the meter fills; the next coin of a sweep"],
      ["sounds/tone9.ogg", "Tone 9", "The note after a clear when the meter is nearly full; the later coins of a sweep"],
      ["sounds/tonepuzzle.ogg", "Puzzle tone", "The note after every clear in the puzzle game"],
      ["sounds/cointube.ogg", "Coin in meter", "A cleared coin reaches the meter"],
      ["sounds/explode.au", "Coin blast", "A bomb is clicked and blows the coins of its colour, one by one"]
    ] },
    { name: "Bags and bonuses", sounds: [
      ["sounds/bonusdrop.au", "Bag drops", "The meter fills and a moneybag drops in"],
      ["sounds/baghit.ogg", "Bag lands", "A moneybag lands on the pile"],
      ["sounds/gotbag.ogg", "Bag collected", "A moneybag is cleared"],
      ["sounds/bonus.ogg", "Bonus", "A bonus: three or more bags at once, a quick second bag, a tower topple, a four-colour run, a three-column clear"],
      ["sounds/superbonus.ogg", "Super bonus", "A bigger bonus: a six-colour run, a clear of many columns"],
      ["sounds/speedbonus.ogg", "Speed bonus", "A clear made quickly after the last one"],
      ["sounds/megamoney.ogg", "Mega money", "Loaded, never played"]
    ] },
    { name: "Combo voices", sounds: [
      ["sounds/goodjob.ogg", "\"Good job!\"", "A double combo"],
      ["sounds/excellent.ogg", "\"Excellent!\"", "A clear of 10 or more coins"],
      ["sounds/incredible.ogg", "\"Incredible!\"", "Two large clears in a row"],
      ["sounds/awesome.ogg", "\"Awesome!\"", "A very large clear, or three big clears in a row"],
      ["sounds/unbelievable.ogg", "\"Unbelievable!\"", "Three large clears in a row, or two huge ones"],
      ["sounds/bigmoney.ogg", "\"Big money!\"", "A clear of 15 or more coins"]
    ] },
    { name: "Warnings and endings", sounds: [
      ["sounds/three.ogg", "\"Three\"", "Three moves are left in the strategy game"],
      ["sounds/two.ogg", "\"Two\"", "Two moves are left in the strategy game"],
      ["sounds/one.ogg", "\"One\"", "One move is left in the strategy game"],
      ["sounds/warning.ogg", "Warning", "The coins are piled near the top; repeats while they stay so"],
      ["sounds/levelup.ogg", "Level up", "A level is cleared"],
      ["sounds/gameovertone.ogg", "Game over tone", "The game ends"],
      ["sounds/gameover.ogg", "\"Game over\"", "The last coins have fallen away after the game ends"],
      ["sounds/anewhighscore.ogg", "\"A new high score!\"", "The high scores show a new best score"],
      ["sounds/bad.ogg", "Bad", "Loaded, never played"]
    ] },
  ] },
  bookworm: {
    presets: [
      { name: "3-Letter Word", when: "A word is scored: the tiles are eaten and a note rises for each letter", steps: [["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/up1", 200], ["sounds/up2", 300], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/piece_hit", 490], ["sounds/piece_hit", 540], ["sounds/piece_hit", 590], ["sounds/chomp", 660]]},
      { name: "4-Letter Word", when: "A four-letter word is scored", steps: [["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/chomp", 40], ["sounds/chomp", 130], ["sounds/up1", 200], ["sounds/chomp", 200], ["sounds/piece_hit", 240], ["sounds/chomp", 260], ["sounds/piece_hit", 280], ["sounds/up2", 300], ["sounds/piece_hit", 320], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/piece_hit", 490], ["sounds/up4", 500], ["sounds/chomp", 690]]},
      { name: "5-Letter Word", when: "A five-letter word is scored", steps: [["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/appear2", 0], ["sounds/up1", 200], ["sounds/piece_hit", 240], ["sounds/up2", 300], ["sounds/piece_hit", 340], ["sounds/up3", 400], ["sounds/piece_hit", 420], ["sounds/piece_hit", 460], ["sounds/piece_hit", 500], ["sounds/up4", 500], ["sounds/up5", 600], ["sounds/chomp", 660]]},
      { name: "6-Letter Word", when: "A six-letter word is scored, and a special tile appears", steps: [["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/spinsuperbonus", 0], ["sounds/chomp", 170], ["sounds/up1", 200], ["sounds/piece_hit", 240], ["sounds/piece_hit", 300], ["sounds/up2", 300], ["sounds/piece_hit", 350], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/piece_hit", 450], ["sounds/up4", 500], ["sounds/piece_hit", 520], ["sounds/up5", 600], ["sounds/chomp", 660], ["sounds/up6", 700]]},
      { name: "7-Letter Word", when: "A seven-letter word is scored, as fire tiles fall and burn", steps: [["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/flametile", 0], ["sounds/appear2", 0], ["sounds/spinsuperbonus", 0], ["sounds/chomp", 10], ["sounds/chomp", 100], ["sounds/chomp", 170], ["sounds/up1", 200], ["sounds/chomp", 230], ["sounds/piece_hit", 240], ["sounds/up2", 300], ["sounds/piece_hit", 330], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/tileburn", 450], ["sounds/piece_hit", 470], ["sounds/up4", 500], ["sounds/piece_hit", 530], ["sounds/up5", 600], ["sounds/chomp", 690], ["sounds/up6", 700], ["sounds/up7", 800], ["sounds/chomp", 810]]},
      { name: "8-Letter Word", when: "An eight-letter word is scored", steps: [["sounds/chomp", 0], ["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/flametile", 0], ["sounds/spinsuperbonus", 0], ["sounds/chomp", 90], ["sounds/chomp", 160], ["sounds/up1", 200], ["sounds/piece_hit", 240], ["sounds/up2", 300], ["sounds/piece_hit", 330], ["sounds/piece_hit", 390], ["sounds/up3", 400], ["sounds/piece_hit", 430], ["sounds/piece_hit", 470], ["sounds/up4", 500], ["sounds/tileburn", 550], ["sounds/up5", 600], ["sounds/chomp", 690], ["sounds/up6", 700], ["sounds/up7", 800], ["sounds/chomp", 810], ["sounds/up8", 900], ["sounds/chomp", 900]]},
      { name: "Fire Word", when: "A word with a fire tile in it is scored", steps: [["sounds/buttonclick", 0], ["sounds/redtilecleared", 0], ["sounds/appear2", 0], ["sounds/chomp", 30], ["sounds/chomp", 120], ["sounds/chomp", 190], ["sounds/up1", 200], ["sounds/up2", 300], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/piece_hit", 470], ["sounds/up4", 500], ["sounds/piece_hit", 550], ["sounds/up5", 600], ["sounds/chomp", 690]]},
      { name: "Bonus Word", when: "The bonus word is spelled", steps: [["sounds/buttonclick", 0], ["sounds/bonusword", 0], ["sounds/wordcomplete", 0], ["sounds/chomp", 40], ["sounds/chomp", 130], ["sounds/up1", 200], ["sounds/chomp", 200], ["sounds/piece_hit", 240], ["sounds/chomp", 260], ["sounds/piece_hit", 300], ["sounds/up2", 300], ["sounds/chomp", 320], ["sounds/chomp", 370], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/chomp", 410], ["sounds/up4", 500], ["sounds/chomp", 690]]},
      { name: "Level Up", when: "The word that completes a level", steps: [["sounds/buttonclick", 0], ["sounds/wordcomplete", 0], ["sounds/chomp", 40], ["sounds/chomp", 130], ["sounds/up1", 200], ["sounds/chomp", 200], ["sounds/piece_hit", 240], ["sounds/chomp", 270], ["sounds/up2", 300], ["sounds/chomp", 330], ["sounds/piece_hit", 340], ["sounds/up3", 400], ["sounds/piece_hit", 410], ["sounds/piece_hit", 470], ["sounds/chomp", 690], ["sounds/chomp", 810], ["sounds/chomp", 900], ["sounds/lex_levelup", 1060]]},
      { name: "Game Over", when: "A fire tile burns through the bottom row: the screen burns", steps: [["sounds/up1", 0], ["sounds/tileburn", 10], ["sounds/piece_hit", 40], ["sounds/piece_hit", 80], ["sounds/up2", 100], ["sounds/piece_hit", 140], ["sounds/up3", 200], ["sounds/piece_hit", 210], ["sounds/up4", 300], ["sounds/piece_hit", 320], ["sounds/piece_hit", 380], ["sounds/up5", 400], ["sounds/piece_hit", 440], ["sounds/chomp", 460], ["sounds/up6", 500], ["sounds/chomp", 600], ["sounds/chomp", 690], ["sounds/chomp", 760], ["sounds/chomp", 820], ["sounds/chomp", 880], ["sounds/screenburngameover", 890], ["sounds/piece_hit", 950]]}
    ],
    groups: [
    { name: "Lex", sounds: [
      ["sounds/lex_welcome.ogg", "\"Welcome to BookWorm!\"", "The game menu, once a run"],
      ["sounds/lex_go.ogg", "\"Go!\"", "A Classic game starts (half the time)"],
      ["sounds/lex_letsgo.ogg", "\"Let's go!\"", "A Classic game starts (half the time)"],
      ["sounds/lex_getready.ogg", "\"Get ready!\"", "An Action game or level starts"],
      ["sounds/lex_scramble.ogg", "\"Scramble!\"", "Lex is clicked to scramble the letters"],
      ["sounds/lex_careful.ogg", "\"Careful!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_danger.ogg", "\"Danger!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_lookout.ogg", "\"Look out!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_watchout.ogg", "\"Watch out!\"", "A fire tile reaches the bottom row (one of four)"],
      ["sounds/lex_levelup.ogg", "\"Level up!\"", "A level is complete"],
      ["sounds/bonusword.ogg", "\"Bonus!\"", "The bonus word is spelled"],
      ["sounds/lex_burp.ogg", "Burp", "Lex has eaten a word with two or more fire tiles"]
    ] },
    { name: "Lex's praise", sounds: [
      ["sounds/lex_verygood.ogg", "\"Very good!\"", "A gold tile is awarded (one of three)"],
      ["sounds/lex_excellent.ogg", "\"Excellent!\"", "A gold tile is awarded (one of three)"],
      ["sounds/lex_awesome.ogg", "\"Awesome!\"", "A gold tile is awarded (one of three)"],
      ["sounds/lex_fantastic.ogg", "\"Fantastic!\"", "A sapphire tile is awarded (one of two)"],
      ["sounds/lex_astonishing.ogg", "\"Astonishing!\"", "A sapphire tile is awarded (one of two)"],
      ["sounds/lex_incredible.ogg", "\"Incredible!\"", "A diamond tile is awarded (one of two)"],
      ["sounds/lex_unbelievable.ogg", "\"Unbelievable!\"", "A diamond tile is awarded (one of two)"]
    ] },
    { name: "Words", sounds: [
      ["sounds/nextselect.ogg", "Letter picked", "A tile is added to the word"],
      ["sounds/bonuscomplete.ogg", "Special word", "A tile is added that makes a word holding a special tile"],
      ["sounds/buzzer.ogg", "Buzzer", "Not a word, or a scramble refused"],
      ["sounds/wordcomplete.ogg", "Word scored", "A word is scored; an Action level's time bonus"],
      ["sounds/redtilecleared.ogg", "Fire word scored", "A word with a fire tile in it is scored"],
      ["sounds/chomp.ogg", "Chomp", "Lex eats a tile of the word"],
      ["sounds/bad.ogg", "Bad (unused)", "Never played"]
    ] },
    { name: "Word notes", sounds: [
      ["sounds/up1.ogg", "1st letter", "After a word is scored, the note for its 1st letter"],
      ["sounds/up2.ogg", "2nd letter", "After a word is scored, the note for its 2nd letter"],
      ["sounds/up3.ogg", "3rd letter", "After a word is scored, the note for its 3rd letter"],
      ["sounds/up4.ogg", "4th letter", "After a word is scored, the note for its 4th letter"],
      ["sounds/up5.ogg", "5th letter", "After a word is scored, the note for its 5th letter"],
      ["sounds/up6.ogg", "6th letter", "After a word is scored, the note for its 6th letter"],
      ["sounds/up7.ogg", "7th letter", "After a word is scored, the note for its 7th letter"],
      ["sounds/up8.ogg", "8th letter on", "After a word is scored, the note for its 8th letter and each after it"],
      ["sounds/up9.ogg", "9th note (unused)", "Never played: the notes stop at the 8th"]
    ] },
    { name: "Tiles", sounds: [
      ["sounds/piece_hit.ogg", "Tile lands", "A falling tile lands"],
      ["sounds/appear2.ogg", "Green tile", "A green tile appears"],
      ["sounds/spinsuperbonus.ogg", "Special tile", "A gold, sapphire or diamond tile appears"],
      ["sounds/flametile.ogg", "Fire tile", "Fire tiles appear"],
      ["sounds/tileburn.ogg", "Tile burns", "A fire tile burns down into the tile below it"]
    ] },
    { name: "Danger and game over", sounds: [
      ["sounds/warning.ogg", "Warning", "A fire tile on the bottom row (three beeps); the first fire tile's hint; each second of Action's countdown"],
      ["sounds/screenburngameover.ogg", "Screen burns", "Game over: the fire burns up the screen"],
      ["sounds/gameovertone.ogg", "Game over tone (unused)", "Never played"]
    ] },
    { name: "Menus", sounds: [
      ["sounds/buttonclick.ogg", "Click", "A button is pressed"],
      ["sounds/goldtile.ogg", "Volume test", "The Options' sound effects slider is let go"]
    ] }
  ] },
  dynomite: {
    presets: [
      { name: "Blast Chain", when: "A big hit sets off blasts", steps: [["sounds/explosion", 0], ["sounds/fireinthehole", 0], ["sounds/explosion", 400], ["sounds/explosion", 770], ["sounds/blastit", 770], ["sounds/xcombo", 1260], ["sounds/explosion", 1910], ["sounds/blastit", 1910], ["sounds/explosion", 2600], ["sounds/explosion", 2830], ["sounds/explosion", 3460], ["sounds/blastit", 3460], ["sounds/explosion", 3810]]},
      { name: "Combo Ladder", when: "Combos follow one after another", steps: [["sounds/xsupercombo", 0], ["sounds/xcambriancombo", 420], ["sounds/xtriassiccombo", 740], ["sounds/xjurassiccombo", 1060], ["sounds/xcretaceouscombo", 1390], ["sounds/xdynamitecombo", 1840], ["sounds/thunderbolt", 3150]]},
      { name: "Whirleys Coming", when: "The hourglass runs low", steps: [["sounds/ticktock", 0, {"loop": 1, "end": 2000}], ["sounds/whirleysiren", 2000], ["sounds/xwhirleyscoming1", 3000]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed: the intro screen's pages, options, high scores, the side bar, the museum's button, a puzzle in the list"],
      ["sounds/egglands.wav", "Egg lands", "The OK or play button of a page or the title screen is pressed; the tutorial's buttons; also an egg lands on the grid"],
      ["sounds/xwelcometodynomite.ogg", "Welcome voice", "The large Inchy talks on the intro screen when it opens for the first time"],
      ["sounds/xgameover.ogg", "Game over voice", "The large Inchy talks when a game ends (the eggs have run off the board)"],
      ["sounds/inchypop.ogg", "Inchy pops up", "Inchy pops up (his countdown reaches 50); a board is cleared (three times as it counts); every step of the tutorial"],
      ["sounds/whirleyhit.ogg", "Whirley hit", "A whirley is hit by an egg; also Save is pressed when it cannot save, or with no save slot chosen"]
    ] },
    { name: "Shooting and hitting", sounds: [
      ["sounds/rubbershot.ogg", "Rubber shot", "The launcher fires an egg"],
      ["sounds/breakegg.wav", "Egg breaks", "Eggs break: a matching group, eggs past the line, loose eggs dropping, or a flying egg hits"],
      ["sounds/explosion.ogg", "Explosion", "A group of three or more breaks, or a dynamite or blast egg goes off"],
      ["sounds/popbonus.ogg", "Bonus pops out", "A broken egg releases a bonus item that flies to the power-up display"],
      ["sounds/mama1.ogg", "Mama", "A broken egg releases a baby dinosaur (the code loads this file twice and picks either at random)"],
      ["sounds/reward.ogg", "Baby leaves", "A baby dinosaur effect ends as it leaves the screen"],
      ["sounds/scoremultiplier.ogg", "Score multiplier", "A group of eggs breaks while a double, triple or quadruple multiplier is active; a power-up reaches its place"],
      ["sounds/blastit.ogg", "Blast it voice", "The tank flinches at a hit of five or more eggs, or a blast: one of four voices at random"],
      ["sounds/charge.ogg", "Charge voice", "The tank flinches at a hit of five or more eggs, or a blast: one of four voices at random"],
      ["sounds/kaboom.ogg", "Kaboom voice", "The tank flinches at a hit of five or more eggs, or a blast: one of four voices at random"],
      ["sounds/fireinthehole.ogg", "Fire in the hole voice", "The tank flinches at a hit of five or more eggs, or a blast: one of four voices at random"]
    ] },
    { name: "Combos", sounds: [
      ["sounds/xcombo.ogg", "Combo voice", "Inchy calls a chain of 1"],
      ["sounds/xsupercombo.ogg", "Super combo voice", "Inchy calls a chain of 2"],
      ["sounds/xcambriancombo.ogg", "Cambrian combo voice", "Inchy calls a chain of 3"],
      ["sounds/xtriassiccombo.ogg", "Triassic combo voice", "Inchy calls a chain of 4"],
      ["sounds/xjurassiccombo.ogg", "Jurassic combo voice", "Inchy calls a chain of 5"],
      ["sounds/xcretaceouscombo.ogg", "Cretaceous combo voice", "Inchy calls a chain of 6"],
      ["sounds/xdynamitecombo.ogg", "Dynamite combo voice", "Inchy calls a chain of 7; the launcher then gets a stick of dynamite"],
      ["sounds/thunderbolt.ogg", "Thunderbolt", "A dynamite stick is made (also when a saved game that holds one is loaded)"]
    ] },
    { name: "Ceiling and danger", sounds: [
      ["sounds/grind.ogg", "Grind", "Loops while the ceiling or the grid is moving"],
      ["sounds/footfall.ogg", "Footfall", "The ceiling steps down a row (Stomped, and the fossil mode's eggs coming down)"],
      ["sounds/footfallrumble.ogg", "Footfall rumble", "Loops in a puzzle round while one shot is left before the ceiling drops"],
      ["sounds/warning.ogg", "Warning", "In a puzzle round with 1 to 3 shots left before the ceiling drops, repeating every 125 updates"],
      ["sounds/speedup.ogg", "Speed up", "The hourglass runs out in the endless kind: the ceiling speeds up"],
      ["sounds/footlands.ogg", "Foot lands", "The ceiling reaches the bottom and the game ends"],
      ["sounds/aooga.ogg", "Aooga", "The grid sets its alarm flag (eggs close to the bottom)"],
      ["sounds/alert.ogg", "Alert", "Steggy's alert blinks (the code only sets that from a saved game); repeats every 35 updates"]
    ] },
    { name: "Hourglass and whirleys", sounds: [
      ["sounds/ticktock.ogg", "Tick tock", "Loops while the endless mode's hourglass has between 5 and 3 seconds left"],
      ["sounds/whirleysiren.ogg", "Whirley siren", "The hourglass passes 3 seconds left"],
      ["sounds/xwhirleyscoming1.ogg", "Whirleys coming voice", "Kroney calls out near the end of the hourglass (alternates with the second call)"],
      ["sounds/xwhirleyscoming2.ogg", "Whirleys coming voice 2", "Kroney's other call near the end of the hourglass"],
      ["sounds/xtimesrunningout.ogg", "Time running out voice", "Kroney calls out in a hurry, when the call is due and the hurry flag is set"]
    ] },
    { name: "Fossils and the museum", sounds: [
      ["sounds/chime.ogg", "Chime", "A fossil falls into place; a board is cleared; the museum's tyrannosaurus is out; a fossil is placed in the museum"],
      ["sounds/fossil.ogg", "Fossil", "The museum's fossil fragments start to fade in at a round's end"],
      ["sounds/footstep.ogg", "Footstep", "The museum's tyrannosaurus takes a step out of the wall, every 160 updates"],
      ["sounds/roar.ogg", "Roar", "The tyrannosaurus roars after his last step"]
    ] },
    { name: "Unused", sounds: [
      ["sounds/explode.ogg", "Explode (unused)", "Never played"],
      ["sounds/mama2.ogg", "Mama 2 (unused)", "Never played"]
    ] },
  ] },
  mummymaze: {
    presets: [
      { name: "Chamber Opens", when: "A chamber begins", steps: [["sounds/tombslide", 0], ["sounds/mummyhowl", 900]]},
      { name: "Caught", when: "A mummy catches the explorer", steps: [["sounds/mumwalk60a", 0, {"p": 34}], ["sounds/pummel", 680, {"p": 34}], ["sounds/mummyhowl", 1960]]},
      { name: "Escape", when: "The explorer walks out of the exit", steps: [["sounds/mumwalk60b", 0, {"p": 91}], ["sounds/finishedlevel", 680]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed: the menus, the options dialog, the map, the board's buttons and other dialogs"],
      ["sounds/tombslide.ogg", "Tomb slide", "The title screen opens fresh; a level begins (75 updates in, not in the ending modes)"]
    ] },
    { name: "Explorer", sounds: [
      ["sounds/expwalk60a.ogg", "Explorer step 60 A", "The explorer takes a step at the slowest speed setting (one of the A and B pair at random, panned)"],
      ["sounds/expwalk60b.ogg", "Explorer step 60 B", "The explorer takes a step at the slowest speed setting (one of the A and B pair at random, panned)"],
      ["sounds/expwalk30a.ogg", "Explorer step 30 A", "The explorer takes a step at the middle speed setting (random of A and B)"],
      ["sounds/expwalk30b.ogg", "Explorer step 30 B", "The explorer takes a step at the middle speed setting (random of A and B)"],
      ["sounds/expwalk15a.ogg", "Explorer step 15 A", "The explorer takes a step at the fastest speed setting (random of A and B)"],
      ["sounds/expwalk15b.ogg", "Explorer step 15 B", "The explorer takes a step at the fastest speed setting (random of A and B)"]
    ] },
    { name: "Mummies and scorpions", sounds: [
      ["sounds/mumwalk60a.ogg", "Mummy step 60 A", "A mummy steps at the slowest speed setting, once a move (random of A and B, panned)"],
      ["sounds/mumwalk60b.ogg", "Mummy step 60 B", "A mummy steps at the slowest speed setting, once a move (random of A and B, panned)"],
      ["sounds/mumwalk30a.ogg", "Mummy step 30 A", "A mummy steps at the middle speed setting (random of A and B)"],
      ["sounds/mumwalk30b.ogg", "Mummy step 30 B", "A mummy steps at the middle speed setting (random of A and B)"],
      ["sounds/mumwalk15a.ogg", "Mummy step 15 A", "A mummy steps at the fastest speed setting (random of A and B)"],
      ["sounds/mumwalk15b.ogg", "Mummy step 15 B", "A mummy steps at the fastest speed setting (random of A and B)"],
      ["sounds/scorpwalk1.ogg", "Scorpion step 1", "A scorpion starts a move (one of two at random, panned)"],
      ["sounds/scorpwalk2.ogg", "Scorpion step 2", "A scorpion starts a move (one of two at random, panned)"],
      ["sounds/pummel.mp3", "Pummel", "A mummy or scorpion fights something or catches the explorer (panned)"]
    ] },
    { name: "The maze", sounds: [
      ["sounds/gate.ogg", "Gate", "The key is stepped on and the gate opens or closes (panned by the gate)"],
      ["sounds/block.ogg", "Block falls", "The explorer is killed by a trap of the block kind, a moment into its death"],
      ["sounds/pit.ogg", "Pit", "The explorer is killed by a trap of the pit kind, a moment into its death"],
      ["sounds/poison.ogg", "Poison", "A scorpion's last move ends and the explorer is stung"],
      ["sounds/badankh.ogg", "Bad ankh", "The ankh check finds the maze cannot be solved, when the ankh is shown"]
    ] },
    { name: "Level end", sounds: [
      ["sounds/finishedlevel.ogg", "Level finished", "The explorer reaches the exit and walks out (not while a solution is shown, not in the tutorial)"],
      ["sounds/mummyhowl.ogg", "Mummy howl", "The explorer is lost; also about 165 updates after a level begins (not in the ending modes)"],
      ["sounds/opentreasure.ogg", "Open treasure", "The ending of an adventure or classic game begins its fade"]
    ] },
  ] },
  rocketmania: {
    presets: [
      { name: "Rocket Launch", when: "A fuse reaches a rocket", steps: [["sounds/Fuse Burning 01d short", 0], ["sounds/Ba Ding 01b", 460], ["sounds/Rocket Launch Woosh 03b", 460], ["sounds/Rocket Launch Woosh 03b", 460], ["sounds/Rocket Launch Woosh 03b", 460], ["sounds/Fuse Burning 01d short", 1520], ["sounds/Firework Explosion 04", 1950]]},
      { name: "Level Up", when: "The last rocket a level needs", steps: [["sounds/Fuse Burning 01d short", 0], ["sounds/Firework Explosion 06", 480], ["sounds/Crowd Applause 04", 520], ["sounds/Ba Ding 03b", 520], ["sounds/Rocket Launch Woosh 03b", 520], ["sounds/Rocket Launch Woosh 03b", 520], ["sounds/Rocket Launch Woosh 03b", 520], ["sounds/Crowd Applause 01", 520], ["sounds/voice/Level Up 02", 910], ["sounds/Dragon Breathing Fire 05", 1510], ["sounds/Firework Explosion 01", 2010], ["sounds/Crowd Oohs 11", 2510], ["sounds/Firework Explosion 06", 2510], ["sounds/Crowd Applause 01", 3010], ["sounds/Firework Explosion 03", 3010], ["sounds/voice/Well Done 01", 4530, {"end": 4690}], ["sounds/Crowd Applause 02", 4530], ["sounds/voice/Get Ready 02", 4690]]},
      { name: "Time Up", when: "The arcade timer runs out", steps: [["sounds/Timer Danger 06", 0], ["sounds/Timer Danger 06", 1000], ["sounds/Timer Danger 06", 2000], ["sounds/Tubetricks Sting Two v01", 2820], ["sounds/voice/Game Over 02", 2820], ["sounds/Fuse Burning 01d short", 3480], ["sounds/Ba Ding 01b", 3940], ["sounds/Rocket Launch Woosh 03b", 3940], ["sounds/Rocket Launch Woosh 03b", 3940], ["sounds/Rocket Launch Woosh 03b", 3940], ["sounds/Firework Explosion 03", 5430], ["sounds/voice/This Is Your Final Rank 07", 5820, {"end": 5900}], ["sounds/Crowd Applause 02", 5820], ["sounds/voice/These Are The Master Rocketeers 06", 5900]]}
    ],
    groups: [
    { name: "Menus and voices", sounds: [
      ["sounds/ui little beep 05b.ogg", "Button beep", "A button is pressed or let go; a dialog button is pressed; the sound volume slider is moved"],
      ["sounds/voice/welcome to rocket mania 03.ogg", "\"Welcome to Rocket Mania\"", "The main menu, a moment after it opens, the first time in a run"],
      ["sounds/voice/let's launch some fireworks 01.ogg", "\"Let's launch some fireworks\"", "The main menu, a moment after it opens, on later visits"],
      ["sounds/voice/these are the master rocketeers 06.ogg", "\"These are the master rocketeers\"", "The high scores are shown"],
      ["sounds/voice/you have got a high score 04.ogg", "\"You have got a high score\"", "The game ends with a score that makes the table or the day's best"],
      ["sounds/tubetricks sting one v01.ogg", "Record sting", "The dialog for entering a name for a high score opens"],
      ["sounds/hint popup 01.ogg", "Hint pops up", "A hint dialog opens; the talking head's speech bubble finishes growing"]
    ] },
    { name: "Starting a level", sounds: [
      ["sounds/voice/get ready 02.ogg", "\"Get ready\"", "A level starts (the first level says nothing when hints are off)"],
      ["sounds/voice/click on the tile to rotate it___ 01.ogg", "\"Click on the tile to rotate it\"", "The first level, when the player has not moved for a while"],
      ["sounds/lantern 01.ogg", "Lantern", "A lantern lights on the board's top bar"],
      ["sounds/dragon breathing fire 05.ogg", "Dragon fire", "The talking head flies in on a rocket at a level's end"],
      ["sounds/ot - new ice cubes 01b.ogg", "Ice cubes", "A wait before a timed stretch ends and the board resumes"]
    ] },
    { name: "Tiles and fuses", sounds: [
      ["sounds/block rotating 03.ogg", "Tile turns", "A tile is turned"],
      ["sounds/tubebump.ogg", "Tile lands", "A falling tile lands on the stack (not more than once a few ticks)"],
      ["sounds/tube glow 01.ogg", "Fuse lit", "The tiles join from one end to the other and a fuse starts to burn"],
      ["sounds/fuse burning 01d short.ogg", "Fuse burns", "A burn finishes and the fuses are lit row by row"],
      ["sounds/fuse burning 01.ogg", "Special tile flashes", "A special tile, a lit one, starts to flash"],
      ["sounds/magic tinkeling sound with ping 01b.ogg", "Special appears", "A bomb or another special tile is put on a free tile"],
      ["sounds/explosion 01.ogg", "Bomb", "A bomb counts down or goes off; a tile at the top blows up"]
    ] },
    { name: "Launching rockets", sounds: [
      ["sounds/rocket launch woosh 03b.ogg", "Rocket launch", "A rocket is launched, three times over for each group of rockets"],
      ["sounds/ba ding 01b.ogg", "Ding 1", "One group of rockets is launched"],
      ["sounds/ba ding 02b.ogg", "Ding 2", "Two groups of rockets are launched"],
      ["sounds/ba ding 03b.ogg", "Ding 3", "Three groups of rockets are launched"],
      ["sounds/ba ding 04b.ogg", "Ding 4", "Four groups of rockets are launched"],
      ["sounds/ba ding 05b.ogg", "Ding 5", "Five or more groups of rockets are launched"],
      ["sounds/firework explosion 01.ogg", "Firework 1", "A rocket bursts; one of these six at random"],
      ["sounds/firework explosion 03.ogg", "Firework 2", "A rocket bursts; one of these six at random"],
      ["sounds/firework explosion 04.ogg", "Firework 3", "A rocket bursts; one of these six at random"],
      ["sounds/firework explosion 05.ogg", "Firework 4", "A rocket bursts; one of these six at random"],
      ["sounds/firework explosion 06.ogg", "Firework 5", "A rocket bursts; one of these six at random"],
      ["sounds/firework explosion 07.ogg", "Firework 6", "A rocket bursts; one of these six at random"],
      ["sounds/crowd applause 01.ogg", "Applause, small", "Two or three launches go off at once"],
      ["sounds/crowd applause 02.ogg", "Applause, medium", "Four or five launches at once; the level-end dialog opens"],
      ["sounds/crowd applause 04.ogg", "Applause, large", "Six or more launches at once; a launch that wins the full-board bonus"]
    ] },
    { name: "Scoring", sounds: [
      ["sounds/ting 02b.ogg", "Multiplier ting 1 (unused)", "Never played"],
      ["sounds/ting 02b shift 2.ogg", "Multiplier ting 2", "A chain of tiles is counted: the first multiplier"],
      ["sounds/ting 02b shift 4.ogg", "Multiplier ting 3", "A chain of tiles is counted: the next multiplier"],
      ["sounds/ting 02b shift 6.ogg", "Multiplier ting 4", "A chain of tiles is counted: the next multiplier"],
      ["sounds/ting 02b shift 8.ogg", "Multiplier ting 5", "A chain of tiles is counted: the fourth multiplier and up"],
      ["sounds/ching 01.ogg", "Ching, 1", "A small coin reaches the counter"],
      ["sounds/ching 02.ogg", "Ching, 2", "A medium coin reaches the counter; part of the biggest one"],
      ["sounds/ching 05.ogg", "Ching, 5", "A large coin reaches the counter; part of the biggest one"],
      ["sounds/gem picked up.ogg", "Gem 1", "A gem is picked up"],
      ["sounds/gem picked up shift 1.ogg", "Gem 2", "A gem is picked up in a run: the second one"],
      ["sounds/gem picked up shift 2.ogg", "Gem 3", "A gem is picked up in a run: the third one"],
      ["sounds/gem picked up shift 3.ogg", "Gem 4", "A gem is picked up in a run: the fourth one"],
      ["sounds/gem picked up shift 4.ogg", "Gem 5", "A gem is picked up in a run: the fifth one and up"],
      ["sounds/ot - new ice cubes 02b.ogg", "Time bonus", "A time item reaches the clock"],
      ["sounds/crowd oohs 04c.ogg", "Crowd ooh 1 (unused)", "Never played"],
      ["sounds/crowd oohs 05.ogg", "Crowd ooh 2 (unused)", "Never played"],
      ["sounds/crowd oohs 07b.ogg", "Crowd ooh 3 (unused)", "Never played"],
      ["sounds/crowd oohs 08b.ogg", "Crowd ooh 4 (unused)", "Never played"],
      ["sounds/crowd oohs 11.ogg", "Crowd ooh 5 (unused)", "Never played"]
    ] },
    { name: "Time and endings", sounds: [
      ["sounds/timer danger 06.ogg", "Time ticks", "The time is nearly out, or tiles reach the top row, every couple of seconds; the sound volume slider is moved"],
      ["sounds/cockeral crowing 03.ogg", "Rooster", "The time left drops to its last stretch"],
      ["sounds/voice/level up 02.ogg", "\"Level up\"", "A level is cleared, a few seconds into the finish"],
      ["sounds/voice/well done 01.ogg", "\"Well done\"", "The level-end dialog opens after a cleared level"],
      ["sounds/voice/this is your final rank 07.ogg", "\"This is your final rank\"", "The level-end dialog opens after the last game"],
      ["sounds/voice/out of time 02.ogg", "\"Out of time\"", "The game ends because the time ran out"],
      ["sounds/voice/game over 02.ogg", "\"Game over\"", "The game ends because the board filled"],
      ["sounds/tubetricks sting two v01.ogg", "Game over sting", "The game ends"]
    ] },
    { name: "Buying rockets", sounds: [
      ["sounds/coin leaving treasure chest 05.ogg", "Coin spent", "A coin leaves the chest in the shop"],
      ["sounds/rocket being bought 02.ogg", "Rocket bought", "A bought rocket flies to its launcher"],
      ["sounds/magic rocket upgraded 03.ogg", "Rocket upgraded", "A launcher's meter is raised in the shop"],
      ["sounds/magic rocket upgraded 02.ogg", "Rocket upgraded 2 (unused)", "Never played"]
    ] },
  ] },
  sevenseas: {
    presets: [
      { name: "Welcome Aboard", when: "Play is pressed on the intro screen", steps: [["sounds/talkallhands", 0], ["sounds/talkwelcometosevenseas", 580]]},
      { name: "Broadside", when: "A cannon shot hits a ship", steps: [["sounds/boom", 0, {"p": 33}], ["sounds/explode2", 450, {"p": 33}]]},
      { name: "Ship Lost", when: "A life is lost and the ship comes back", steps: [["sounds/explode2", 0], ["sounds/sink", 0], ["sounds/reappear", 1250, {"p": 166}]]}
    ],
    groups: [
    { name: "Menus and voices", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed: the menus, the help screen and the demo screen"],
      ["sounds/talkwelcometosevenseas.ogg", "Welcome voice", "The intro screen, about a second after it opens, the first time the menu shows after the program starts"],
      ["sounds/talkhoistsail.ogg", "Hoist sail voice", "Play or Tutorial is pressed on the intro screen (one of three voices at random)"],
      ["sounds/talkallhands.ogg", "All hands voice", "Play or Tutorial is pressed on the intro screen (one of three voices at random)"],
      ["sounds/talkayeaye.ogg", "Aye aye voice", "Play or Tutorial is pressed on the intro screen (one of three voices at random)"]
    ] },
    { name: "Your ship", sounds: [
      ["sounds/movegood.ogg", "Ship moves", "The player's ship is sent to a new cell (panned by its position)"],
      ["sounds/whirlout.ogg", "Whirlpool out", "The ship sails into an active whirlpool and leaves the board"],
      ["sounds/whirlin.ogg", "Whirlpool in", "The ship comes in from a whirlpool at a level's start"],
      ["sounds/reappear.ogg", "Ship reappears", "The ship is placed again after a life is lost, and again each time its blinking goes off"],
      ["sounds/boom.ogg", "Cannon shot", "Cannons are fired, by the player or by an enemy ship (panned)"]
    ] },
    { name: "Enemies", sounds: [
      ["sounds/moveevil.ogg", "Enemies move", "The enemy ships start their move after any turning is finished"],
      ["sounds/explode.ogg", "Small explosion", "A small explosion is spawned at a cell (panned)"],
      ["sounds/explode2.ogg", "Big explosion", "A big explosion is spawned at a cell (panned)"],
      ["sounds/monster.ogg", "Sea monster", "A sea monster ship is shot, or sails into the player's cell or another ship (panned)"],
      ["sounds/sink.ogg", "Ship sinks", "A life is lost"]
    ] },
    { name: "Scoring and level end", sounds: [
      ["sounds/bonusship.ogg", "Extra ship", "The score passes another thousand and a life is added (also when a level's kill bonus does it)"],
      ["sounds/talkdavyjoneslocker.ogg", "Davy Jones voice", "At a level's end, the pirate has finished appearing and one of four voices is picked at random"],
      ["sounds/talkarrmatey.ogg", "Arr matey voice", "At a level's end, the pirate has finished appearing and one of four voices is picked at random"],
      ["sounds/talkwalktheplank.ogg", "Walk the plank voice", "At a level's end, the pirate has finished appearing and one of four voices is picked at random"],
      ["sounds/talklandlubber.ogg", "Landlubber voice", "At a level's end, the pirate has finished appearing and one of four voices is picked at random"]
    ] },
    { name: "Bonus dig", sounds: [
      ["sounds/bonusdig.ogg", "Dig", "A square is dug on the bonus screen, with nothing special under it (panned by column)"],
      ["sounds/bonusgraves.ogg", "Graves near", "A dug square has graves in the neighbouring squares"],
      ["sounds/bonuslose.ogg", "Ghost", "A square holding a ghost is dug"]
    ] },
  ] }
};
