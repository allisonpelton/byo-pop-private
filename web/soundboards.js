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
  ] }
};
