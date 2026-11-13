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
      ["sounds/click.ogg", "Click", "Any button pressed (the game's updates run 100 a second): title, intro and high score screens, the options dialog's buttons, and the board's side buttons including the sound button"],
      ["sounds/button.ogg", "Button (unused)", "Never played: it is not in the game's sound table, so nothing loads or plays it. Also in Peggle Deluxe, Peggle Extreme, Peggle Nights and Peggle World of Warcraft Edition, as sounds/button.ogg."],
      ["sounds/slideback.ogg", "Slide back (unused)", "Never played: it is not in the game's sound table, so nothing loads or plays it"],
      ["sounds/v4-welcome.ogg", "\"Welcome to Alchemy\"", "On the intro screen, 60 updates (0.6 seconds) after it opens, only while the player has not yet played a game; the wizard speaks with it"]
    ] },
    { name: "Placing tiles", sounds: [
      ["sounds/bad.ogg", "Bad move", "A click on the grid with a tile held when the tile cannot go there, an off-grid click too; a help line may show as well. Also in BookWorm as Bad (unused). Also in Peggle Deluxe, Peggle Extreme, Peggle Nights and Peggle World of Warcraft Edition, as sounds/pegpop.ogg."],
      ["sounds/thunk.ogg", "Tile placed", "Every tile or bomb set on the board by a valid click, together with the forge-down sound where it applies"],
      ["sounds/skull.ogg", "Skull bomb", "A bomb tile is set down: it takes the tile in that cell with it. The tile-placed thunk plays as well"],
      ["sounds/melt.ogg", "Tile melts", "A tile is discarded (forge click or button) and the forge has not overflowed; the next tile comes 25 updates (0.25 seconds) later"]
    ] },
    { name: "The forge", sounds: [
      ["sounds/forgedown.ogg", "Forge down", "A tile is placed while the forge level is above 0, lowering it by 1; never in the timed game"],
      ["sounds/warning.ogg", "Forge warning", "Untimed game only: while the forge level is above 2.3 of 3, once every 150 updates (1.5 seconds) until the game ends"],
      ["sounds/gemchange.ogg", "Gems change", "The next board's tiles have finished fading in, about 67 updates (0.7 seconds) after the statistics button is pressed"]
    ] },
    { name: "Board end", sounds: [
      ["sounds/finalvanish.ogg", "Row clears", "A placed tile completes its row or column; also the click that starts the next board after one is cleared"],
      ["sounds/finalgong.ogg", "Final gong", "The placed tile lights the last unlit square, so the board is won"],
      ["sounds/endofgame.ogg", "End of game", "The game ends: the forge overflows (a discard takes it past 3), or in the timed game it stays full for 150 updates (1.5 seconds)"],
      ["sounds/v4-levelcomplete.ogg", "\"Level complete\"", "After a board is won, once the wizard has fully faded in over the cleared board and its tiles have all gone"],
      ["sounds/v4-gameover.ogg", "\"Game over\"", "The wizard has fully faded in after the game ends (forge overflow or the timed forge staying full)"]
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
      ["sounds/mouseover.ogg", "Menu hover", "The pointer enters a main menu button (Classic, Survival, Hall of Fame, Options, Quit), each at its own pitch; also when the quit box opens and when the options window opens"],
      ["sounds/click2.ogg", "Button click", "Any button goes down: menus, dialogs, pilot choice, story screens, hall of fame, a click on a warp gate panel on the warp screen"],
      ["sounds/warpbutton.ogg", "Warp gate click", "A click on the left or right gate of the checkpoint box, which choose the stage to go on from"],
      ["sounds/v_chooseyourpilot.ogg", "\"Choose your pilot\"", "The pilot choice screen opens; it cuts off any menu voice"],
      ["sounds/timer1.ogg", "Tick", "The sound volume slider is changed without dragging (a click or key); also the weapon box's OK (a fix leaves that out)"]
    ] },
    { name: "Ship", sounds: [
      ["sounds/engineloop.ogg", "Engine loop", "Loops from the game's first update, and again after the warp ends or a story screen closes; off while the field is in the warp or the game is paused"],
      ["sounds/hyperspaceloop1.ogg", "Hyperspace loop", "Loops while the field is in the warp, from the moment the warp begins until it ends; the engine loop is stopped meanwhile"],
      ["sounds/magzap3.ogg", "Pick up", "A click picks up bricks from the field; not repeated while the button is held down"],
      ["sounds/magzap2.ogg", "Fire", "A right click fires the bricks held, unless the level summary is counting"],
      ["sounds/magzap1.ogg", "Can't do that", "A click that fails: six bricks already held, nothing to take, a locked brick, the pointer too low, a power not ready; not repeated while held. Also in Zuma's Revenge, as sounds/magzap1.ogg."],
      ["sounds/laser.ogg", "Laser", "Laser pilot, power active: a click on a brick removes it from the bottom of its column, panned by column"],
      ["sounds/psychic.ogg", "Psychic", "Psychic pilot, power active: a click takes out every lit cell at once, when nothing is moving"],
      ["sounds/activated.ogg", "Tool picked up", "A tool brick is among the bricks picked up from a column, once for each tool brick"]
    ] },
    { name: "Bricks", sounds: [
      ["sounds/gemongem2.ogg", "Brick lands", "A held brick has landed on the ship with none still in flight, panned by the ship's column; also a fired brick lands in the field"],
      ["sounds/blockhit.ogg", "Brick hit", "Fired bricks land and join fewer than 4 of a kind, so no set is made; played at a random fired brick's column"],
      ["sounds/explode1.ogg", "Brick pops", "A brick is taken out because a neighbour is of the kind a kill-all brick kills; the atomic pilot's marked bricks blow up one by one (every 40 updates, then 20)"],
      ["sounds/steelclang1.ogg", "Steel clang", "A whole steel brick takes its first hit (the second hit breaks it silently)"],
      ["sounds/mutator1.ogg", "Mutation", "A brick turns into a mutant brick"],
      ["sounds/mudestroy3.ogg", "Mutant dies", "A mutant brick is destroyed"],
      ["sounds/hyperbrick_sound.ogg", "Booster pulse", "A galaxy brick in the field pulses: every 100 updates (1 s), speeding to every 12 updates over its last 200 of 1000 updates"],
      ["sounds/freezeblock1.ogg", "Freeze", "A freeze brick goes off (not while a level is ending or the game is over)"]
    ] },
    { name: "Sets and combos", sounds: [
      ["sounds/gotset2.ogg", "Set", "Fired bricks make a set of 4 to 6 bricks of one kind, at a random set brick's column; the bubbly sound plays instead if a mark is set for that kind"],
      ["sounds/gotsetbig.ogg", "Big set", "A set of 7 or more bricks is made; the bubbly sound plays instead if a mark is set for that kind"],
      ["sounds/bubbly.ogg", "Set with a mark", "A set is made while a supercharge mark for its brick kind is set; only a mark restored from a saved game's profile record, which a new game never makes"],
      ["sounds/combo_tone.ogg", "Combo ping", "Each set that makes a chain 2 or longer, rising one step a set up to the 11th; at most once every 45 updates (0.45 s)"],
      ["sounds/v_combo1.ogg", "\"Combo!\"", "A chain of 2 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo2.ogg", "\"Super combo!\"", "A chain of 3 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo3.ogg", "\"Mega combo!\"", "A chain of 4 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo4.ogg", "\"Ultra combo!\"", "A chain of 5 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo5.ogg", "\"Giga combo!\"", "A chain of 6 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo6.ogg", "\"Hyper combo!\"", "A chain of 7 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo65.ogg", "\"Galactic combo!\"", "A chain of 8 ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"],
      ["sounds/v_combo7.ogg", "\"Maximum combo!\"", "A chain of 9 or more ends, 100 updates (1 s) after its last set; not in a bonus round or once the stage's brick goal is reached"]
    ] },
    { name: "Power-ups", sounds: [
      ["sounds/killall_explode.ogg", "Kill-all blast", "A kill-all brick goes off (not while a level is ending or the game is over, and not as a mutant)"],
      ["sounds/star_explode.ogg", "Star blast", "A star brick goes off (not while a level is ending or the game is over)"],
      ["sounds/nukeexplosion1.ogg", "Row nuke", "A row nuke brick goes off (not while a level is ending or the game is over)"],
      ["sounds/ckiller4.ogg", "Column nuke", "A column nuke or galaxy brick goes off, or a column nuke is fired from the ship's hand; all of its column goes"],
      ["sounds/galaxy.ogg", "Galaxy", "A galaxy brick goes off, together with the column nuke sound; not while a level is ending or the game is over"],
      ["sounds/star9.ogg", "Fuel cell", "A fuel cell brick is destroyed, unless it is a mutant"],
      ["sounds/v_weaponready.ogg", "\"Weapon ready\"", "The power meter reaches 18000, filling by 1 an update at the base level (3 minutes), faster with upgrades; only while play runs and no power is active"],
      ["sounds/v_poweractivated.ogg", "\"Power activated\"", "The player starts the power from a full meter (laser, atomic, turbot or psychic pilot)"],
      ["sounds/explosion.ogg", "Weapon blast", "The turbot pilot's power starts; the atomic pilot's blast, 150 updates (1.5 s) after its symbol appears"],
      ["sounds/sloooowdown.ogg", "Slow down", "The atomic pilot's power: the final, eighth blast, after seven; also the ships screen opening from the main menu's secret order, 50 updates (0.5 s) later. Also in Bookworm Adventures Volume 2 as Sloooowdown (unused)."],
      ["sounds/powerupwearsoff.ogg", "Power wears off", "The power meter has drained to 0 and the power ends (not the atomic pilot's)"],
      ["sounds/v_weaponupgrade.ogg", "\"Weapon upgraded\"", "The upgrade box has settled in, then 100 updates (1 s) later"]
    ] },
    { name: "Levels and warp", sounds: [
      ["sounds/v_welcome.ogg", "\"Welcome to AstroPop!\"", "The first update of every game board that is not the attract-mode demo"],
      ["sounds/v_getready.ogg", "\"Get ready\"", "A story screen closes, or the warp ends and the next level's bricks load"],
      ["sounds/warp3.ogg", "Warp", "The level-end warp's charge flash reaches its peak (150 of 255); also, every other update, as a bonus round's warp-in white flash passes 59"],
      ["sounds/v_warpengaged.ogg", "\"Warp engaged\"", "The warp starts: the level-up screen's text has faded out and the ship flies off"],
      ["sounds/comptext.ogg", "Computer text", "Each letter of the warp's text typed out, one every 5 updates (0.05 s)"],
      ["sounds/hyperspacethunder1.ogg", "Thunder 1", "In the warp, a lightning flash comes every 100 to 299 updates (1 to 3 s); this or thunder 2, 1 in 2, at a random column's pan"],
      ["sounds/hyperspacethunder2.ogg", "Thunder 2", "In the warp, a lightning flash comes every 100 to 299 updates (1 to 3 s); this or thunder 1, 1 in 2, at a random column's pan"],
      ["sounds/level_up1.ogg", "Hundred left", "100 or fewer bricks are left to pass the stage, once a stage; not in survival"],
      ["sounds/level_up2.ogg", "Fifty left", "50 or fewer bricks are left to pass the stage, once a stage; not in survival"],
      ["sounds/brickmilestone.ogg", "Milestone", "A quarter, half or three quarters of the stage's bricks are cleared; also a galaxy brick starts (it adds 15% of the stage)"],
      ["sounds/v_levelcomplete.ogg", "\"Level complete\"", "The stage's brick goal is reached and the level-up begins, after any chain bonus is paid; not when it starts a bonus round"],
      ["sounds/wordswoosh.ogg", "Word swoosh", "A big word flies across: level complete, quarter, half and three-quarter marks, and survival's minute text every 6000 updates (1 minute)"],
      ["sounds/v_bonusround.ogg", "\"Bonus round\"", "A level ends and the classic game's next screen is a bonus round, which starts at once"],
      ["sounds/v_perfect.ogg", "\"Perfect!\"", "The field is cleared in a stage (the clear bonus is paid), or the level summary's bar fills with no bricks left"],
      ["sounds/count.ogg", "Score count", "The level summary counts its numbers up: every 2nd step of the penalty, every 4th of the clean bonus, every 8th of the pay-out"]
    ] },
    { name: "Danger and game over", sounds: [
      ["sounds/klaxon.ogg", "Klaxon", "Loops while the ship's column has no room for the next brick (the held bricks included); stops when there is room; not in a bonus round or warp"],
      ["sounds/v_caution.ogg", "\"Caution\"", "The tallest column reaches 14 rows high and the siren is idle, not during a bonus round or warp; the siren animation starts with it"],
      ["sounds/v_danger.ogg", "\"Danger\" (unused)", "Never played: it is loaded, but no code plays it; only the caution voice sounds when the stack gets high"],
      ["sounds/v_nomoremoves.ogg", "\"No more moves\"", "Bonus rounds only: no move is left and nothing is moving, the banner shows for 400 updates (4 s) and the field is counted out"],
      ["sounds/v_gameover.ogg", "\"Game over\"", "The game ends, with the mega explosion at the same moment"],
      ["sounds/megaexplosion.ogg", "Mega explosion", "The game ends, with the game over voice at the same moment"],
      ["sounds/v_newhighscore.ogg", "\"A new high score!\"", "200 updates (2 s) after the game ends, if the score takes the top place of the high scores"]
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
      ["sounds/capclick.ogg", "Cap click", "Any menu, dialog or hint box button pressed (the main menu only once its door has slammed); the options dialog's buttons 6, 7, 8 and 10; the bomb button lighting up; the bonus molecule frame sliding in or out"],
      ["sounds/swoop.ogg", "Swoop", "A menu panel slides in or out of the main menu: once when it leaves, once when it lands; and when a game is started from the game-over panel"],
      ["sounds/doorslam.ogg", "Door slam", "The main menu's door shuts as the board closes, once per slam"],
      ["sounds/dooropen.ogg", "Door open", "The main menu's door opens, once its panel has slid fully away"]
    ] },
    { name: "Atoms", sounds: [
      ["sounds/atomappear.ogg", "Atom appears", "A new atom's appearance reaches its last step (24); several in one update give one sound"],
      ["sounds/selectatom.ogg", "Select atom", "An atom is clicked to pick it up; also the options dialog's reset-hints button"],
      ["sounds/moveatom.ogg", "Move atom", "A second click sends the picked atom along a path to a new cell"],
      ["sounds/movebioatom.ogg", "Move biohazard", "A second click sends the picked biohazard atom along a path to a new cell"],
      ["sounds/whoosh.ogg", "Whoosh", "Atoms fly away: a molecule made (once per update), the grid cleared, a swirl taking more than 6 atoms, an element atom, a bonus ring; also a new level's atoms arrive"],
      ["sounds/lowwhoosh.ogg", "Low whoosh", "Each atom a swirl carries off disappears, one sound per atom"],
      ["sounds/magnet.ogg", "Magnet", "After a move, atoms slide toward each other and at least one moves; once per move"]
    ] },
    { name: "Molecules and combos", sounds: [
      ["sounds/createmolecule.ogg", "Molecule created", "A molecule is made: a 3 by 2, 2 by 3 or 2 by 2 block of one atom type; one sound for each block found in a check"],
      ["sounds/electric1.ogg", "Electric 1", "Lightning bolt: one of the three at random for each bolt joining an atom to a same-type neighbour, and each drawn frame of a shaking made molecule; the infection bolt plays all three, then one every 10 updates (0.1 s)"],
      ["sounds/electric2.ogg", "Electric 2", "Lightning bolt: one of the three at random for each bolt joining an atom to a same-type neighbour, and each drawn frame of a shaking made molecule; the infection bolt plays all three, then one every 10 updates (0.1 s)"],
      ["sounds/electric3.ogg", "Electric 3", "Lightning bolt: one of the three at random for each bolt joining an atom to a same-type neighbour, and each drawn frame of a shaking made molecule; the infection bolt plays all three, then one every 10 updates (0.1 s)"],
      ["sounds/geiger1.ogg", "Geiger counter 1", "Loops while the combo is 2 or more, its volume rising 0.1 an update toward (combo - 1) / 12; fades out by 0.01 an update after the combo ends"],
      ["sounds/geiger2.ogg", "Geiger counter 2", "Loops while the combo is above 3, its volume rising 0.1 an update toward (combo - 3) / 8; fades out by 0.01 an update after the combo ends"],
      ["sounds/voices/combo.ogg", "\"Combo!\"", "A molecule takes the combo to 2: the voice comes 30 updates (0.3 s) later, or when he stops talking"],
      ["sounds/voices/supercombo.ogg", "\"Super combo!\"", "A molecule takes the combo to 3: the voice comes 30 updates (0.3 s) later, or when he stops talking"],
      ["sounds/voices/megacombo.ogg", "\"Mega combo!\"", "A molecule takes the combo to 4: the voice comes 30 updates (0.3 s) later, or when he stops talking"],
      ["sounds/voices/ultracombo.ogg", "\"Ultra combo!\"", "A molecule takes the combo to 5: the voice comes 30 updates (0.3 s) later, or when he stops talking"],
      ["sounds/voices/hypercombo.ogg", "\"Hyper combo!\"", "A molecule takes the combo to 6: the voice comes 30 updates (0.3 s) later, or when he stops talking"],
      ["sounds/voices/gigacombo.ogg", "\"Giga combo!\"", "A molecule takes the combo to 7: the voice comes 30 updates (0.3 s) later, or when he stops talking"],
      ["sounds/voices/maxcombo.ogg", "\"Max combo!\"", "A molecule takes the combo to 8: the voice comes 30 updates (0.3 s) later, or when he stops talking; no voice above 8"]
    ] },
    { name: "Bombs and biohazards", sounds: [
      ["sounds/nukeblast.ogg", "Nuke blast", "An atom is destroyed: the atomic bomb bursts it, or an element's expanding ring pops it, or a biohazard atom's molecule shatters it; one sound per atom"],
      ["sounds/heartbeat.ogg", "Heartbeat", "A biohazard atom's glow reaches full brightness, once per pulse of its glow"],
      ["sounds/voices/atomicbombready.ogg", "\"Atomic bomb ready\"", "The atomic bomb button finishes lighting up (its frame reaches 15 of 24); 50 updates (0.5 s) later"],
      ["sounds/voices/atomicbomb.ogg", "\"Atomic bomb!\"", "The bomb button is pressed with atoms on the grid and no level-up showing; the voice follows 1 update later"],
      ["sounds/voices/biohazard.ogg", "\"Biohazard!\"", "The first biohazard atom of a grid finishes appearing; 10 updates (0.1 s) later (one of two random choices, both this voice)"],
      ["sounds/voices/biohazardalert.ogg", "\"Biohazard alert!\" (unused)", "Never played: it is not loaded, and no code asks for it; only the biohazard voice sounds"]
    ] },
    { name: "Elements and bonus molecules", sounds: [
      ["sounds/freeze.ogg", "Freeze", "A freeze takes hold of the board: its progress reaches 128, one step an update, so 1.28 s after it begins"],
      ["sounds/voices/fire.ogg", "Fire voice", "A fire element atom is placed; the voice comes 25 updates (0.25 s) later"],
      ["sounds/voices/ice.ogg", "\"Ice!\"", "A water (ice) element atom is placed; the voice comes 25 updates (0.25 s) later"],
      ["sounds/voices/lightning.ogg", "\"Lightning!\"", "An earth (lightning) element atom is placed; the voice comes 25 updates (0.25 s) later"],
      ["sounds/voices/wind.ogg", "\"Wind!\"", "An air (wind) element atom is placed; the voice comes 25 updates (0.25 s) later"],
      ["sounds/voices/ether.ogg", "\"Ether\"", "An ether element atom is placed; the voice comes 25 updates (0.25 s) later"],
      ["sounds/voices/bonusmolecule.ogg", "\"Bonus molecule!\"", "A bonus molecule is made; the voice comes 5 updates later"],
      ["sounds/voices/bonusmoleculeready.ogg", "\"Bonus molecule ready!\"", "The bonus molecule frame has slid into place with a new pattern; the voice comes 50 updates (0.5 s) later"],
      ["sounds/voices/bonusmoleculeupgraded.ogg", "\"Bonus molecule upgraded!\"", "A bonus molecule of a higher level is made (an upgrade); the voice comes 5 updates later"]
    ] },
    { name: "Level start and end", sounds: [
      ["sounds/voices/getready.ogg", "\"Get ready!\"", "A level begins, 50 updates (0.5 s) after the board starts"],
      ["sounds/voices/go.ogg", "\"Go!\"", "Every atom of the grid has finished appearing, once per level; the voice follows 1 update later"],
      ["sounds/voices/levelup.ogg", "\"Level up!\"", "The meter fills and the level goes up (the action mode: its meter passes its maximum); in the strategy mode it clears other queued voices"],
      ["sounds/voices/gameover.ogg", "\"Game over!\"", "300 updates (3 s) into the game-over count of 600, when the atoms fly away; not while a saved game loads"],
      ["sounds/voices/anewhighscore.ogg", "\"A new high score!\"", "The name entry box opens for a score that earns a place on the table or the best of the day"],
      ["sounds/voices/cool.ogg", "\"Cool!\"", "Okay is pressed on a high score name: one of six voices (this, excellent, great, nifty, terrific, wicked), 1 in 6 each"],
      ["sounds/voices/excellent.ogg", "\"Excellent!\"", "Okay is pressed on a high score name: one of six voices (cool, this, great, nifty, terrific, wicked), 1 in 6 each"],
      ["sounds/voices/great.ogg", "\"Great!\"", "Okay is pressed on a high score name: one of six voices (cool, excellent, this, nifty, terrific, wicked), 1 in 6 each"],
      ["sounds/voices/nifty.ogg", "\"Nifty!\"", "Okay is pressed on a high score name: one of six voices (cool, excellent, great, this, terrific, wicked), 1 in 6 each"],
      ["sounds/voices/terrific.ogg", "\"Terrific!\"", "Okay is pressed on a high score name: one of six voices (cool, excellent, great, nifty, this, wicked), 1 in 6 each"],
      ["sounds/voices/wicked.ogg", "\"Wicked\"", "Okay is pressed on a high score name: one of six voices (cool, excellent, great, nifty, terrific, this), 1 in 6 each"],
      ["sounds/voices/awesome.ogg", "\"Awesome!\" (unused)", "Never played: it is not loaded, and the high score voices choose among six others only"],
      ["sounds/voices/levelcomplete.ogg", "\"Level complete!\" (unused)", "Never played: it is not loaded, and the level-up voice is used when a level ends"],
      ["sounds/voices/beautiful.ogg", "\"Beautiful!\" (unused)", "Never played: it is not loaded, and the high score voices choose among six others only"]
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
    { name: "Menus", sounds: [
      ["sounds/menuclick.ogg", "Click", "Any button pressed in the menus and dialogs, and every button of the options dialog (its Done button clicks twice)"],
      ["sounds/click2.ogg", "Click 2 (unused)", "Never played: it is in the sound table, but no code plays it. Also in Bejeweled 2 as Click 2 (unused)."],
      ["sounds/sidebarclick2.ogg", "Side bar click (unused)", "Never played: it is not in the sound table, so it is never loaded"],
      ["sounds/welcome1.ogg", "\"Welcome to Bejeweled\" (unused)", "Never played: it is not in the sound table, so it is never loaded"],
      ["sounds/goodbye1.ogg", "\"Goodbye!\" (unused)", "Never played: it is not in the sound table, so it is never loaded"]
    ] },
    { name: "Matches", sounds: [
      ["sounds/select.ogg", "Select", "A gem is selected by a click, panned by its column; also the first step of an arrow gem's move in the arrows mod"],
      ["sounds/gotset2.ogg", "Match", "A scoring pass whose longest run is exactly 3 gems, panned by the runs' average column; plays with that pass's cascade sound"],
      ["sounds/gotsetbig2.ogg", "Big match", "A scoring pass whose longest run is 4 or more gems; plays with that pass's cascade sound (the arrows mod uses it for an arrow shot)"],
      ["sounds/multishot.ogg", "Two matches", "A scoring pass finds two or more separate runs at once, on top of the match and cascade sounds"],
      ["sounds/bad2.ogg", "Bad swap", "A swap that makes no match swings back, or two gems that are not neighbours are swapped; also a refused undo. Also in Bejeweled 3, as sounds/badmove.ogg."],
      ["sounds/gemongem2.ogg", "Gems land", "Gems land after falling; at most once every 10 updates (0.1 s; the game runs 100 updates a second), panned by where they land"],
      ["sounds/excellent1.ogg", "\"Excellent!\"", "A move's matches pass 120 points × the level's multiplier, once a move. A match: 10, 20, 30, 40, 60, 80, 110, 160, 210 for the move's 1st to 9th, +10 a gem past 3 and past 5"]
    ] },
    { name: "Cascades", sounds: [
      ["sounds/combo22.ogg", "Cascade 1", "Every scoring pass that is the move's first; plays with the match sound"],
      ["sounds/combo32.ogg", "Cascade 2", "A move's second scoring pass, the gems that fell making another match; plays with the match sound"],
      ["sounds/combo42.ogg", "Cascade 3", "A move's third scoring pass; plays with the match sound"],
      ["sounds/combo52.ogg", "Cascade 4", "A move's fourth scoring pass and every one after it (the sound table repeats this file for 5 and more)"],
      ["sounds/combo62.ogg", "Cascade 5 (unused)", "Never played: the sound table loads cascade 4's file again for fifth and later passes; this file is never loaded"],
      ["sounds/combo72.ogg", "Cascade 6 (unused)", "Never played: the sound table has no entry for it; this file is never loaded"]
    ] },
    { name: "Levels", sounds: [
      ["sounds/go1.ogg", "\"Go!\"", "Update 150 of a new board (1.5 s after it starts), as the Go text bounces in"],
      ["sounds/whirlpool1.ogg", "Whirlpool", "A level is won: loops while the vortex pulls the gems to the middle, its volume rising with the vortex, until the next board or the next vortex"],
      ["sounds/levelup1.ogg", "\"Level up!\"", "In the level-won vortex, the farthest gem comes within 80 pixels of the middle (gems move in 1 pixel an update)"],
      ["sounds/firecrackle.ogg", "Crackle", "The vortex has finished and the gems are gone: the next level's wipe starts; also a lightning gem's blast in a rules mod with special gems. Also in Bejeweled 2 as Fire crackle (unused)."],
      ["sounds/warning1.ogg", "Warning", "Timed mode: the bar is under 15% full; repeats every 240 updates (2.4 s) at 15%, down to every 80 updates (0.8 s) at 5% and below"]
    ] },
    { name: "Game over", sounds: [
      ["sounds/explode2.ogg", "Explosion", "Game over: 200 updates (2 s) after the game ends the shake stops and the gems blow apart; 50 updates after New Game mid-game; a flame gem's blast in a rules mod"],
      ["sounds/gameover1.ogg", "\"Game over\"", "The last gem has fallen off the board after the game ended; not when a New Game ended it"]
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
      ["sounds/mainmenu_mouseover.ogg", "Menu hover", "The pointer enters one of the four main menu game buttons (Classic, Action, Puzzle, Endless) while the game window has focus"],
      ["sounds/mainmenu_mouseoff.ogg", "Menu leave", "The pointer leaves one of the four main menu game buttons, again only while the window has focus"],
      ["sounds/mainmenu_mouseclick.ogg", "Menu click", "One of the four main menu game buttons goes down, once the menu is ready (the secret button has its own sounds)"],
      ["sounds/landinglights.ogg", "Landing lights", "Each time the main menu's three pairs of lights start a new run, while the menu is ready or starting a game"],
      ["sounds/mystery.ogg", "Fairy dust", "The secret button on the main menu is switched on (nothing plays when it is switched off)"],
      ["sounds/mainmenu_gamestart.ogg", "Game start", "A game starts from the main menu, as the board is set up and the portal opens"],
      ["sounds/menuclick.ogg", "Click", "A button goes down on the board (menu, hint), the galaxy map or the high scores (not while it closes), or on the main menu apart from the four game buttons and the secret one"],
      ["sounds/menuclick2.ogg", "Dialog click", "A button goes down in a dialog (Options, name, confirm) or on the puzzle dome; also when an option in Options is switched"],
      ["sounds/click2.ogg", "Click 2 (unused)", "Never played: the file is not in the game's resource list and no code plays it. Also in Bejeweled as Click 2 (unused)."],
      ["sounds/panelslide1.ogg", "Panel slide", "The high scores panel opens or starts closing; a galaxy map or puzzle dome button is released; Options is closed with OK"],
      ["sounds/Welcome.ogg", "\"Welcome to Bejeweled 2\"", "The main menu opens after the title screen with no player name and no saved player; the name dialog follows 1.25 s later"],
      ["sounds/Welcome_Back.ogg", "\"Welcome back\"", "The main menu opens after the title screen when a player name is set or a saved player exists"],
      ["sounds/Goodbye.ogg", "\"Goodbye\"", "Quit is confirmed in the quit dialog; the menu then shuts the game down"]
    ] },
    { name: "Swaps and matches", sounds: [
      ["sounds/select.ogg", "Select", "A gem is clicked and becomes the selected one, panned to its side of the board"],
      ["sounds/bad2.ogg", "Bad swap", "A swap that makes no match is undone, or two gems that are not neighbours are swapped; a locked planet is clicked on the galaxy map. Also in Bejeweled 3, as sounds/badmove.ogg."],
      ["sounds/gotset2.ogg", "Match", "An update finds exactly one set of three or more, with no flame gem made by a gem in two sets; also when the sound volume slider is let go. Also in Chuzzle Deluxe, as sounds/Bonus/GotJewel.ogg."],
      ["sounds/gotsetbig2.ogg", "Big match", "One update finds two or more sets at the same time (no gem shared between sets and no four in a row). Resembles Chuzzle Deluxe's sounds/Bonus/GotJewel.ogg, the same sound, longer."],
      ["sounds/multishot.ogg", "Multi match", "A match makes a flame gem: four in a row, or one gem in two sets (an L, a T or a cross); plays instead of the Match sound"],
      ["sounds/gemongem2.ogg", "Gems land", "Falling gems land, at most once every 10 updates (0.1 s); also when the logo's face gems land on the title screen. Also in Bejeweled 3, as sounds/gem_hit.ogg. Also in Chuzzle Deluxe, as sounds/Bonus/JewelLand.ogg."],
      ["sounds/combo22.ogg", "Cascade 1", "Together with the match sound, when the move's chain is at its first match (a swap resets the chain)"],
      ["sounds/combo32.ogg", "Cascade 2", "Together with the match sound, for the second match of a move: one fall of new gems after the first. Also in Bejeweled 3, as sounds/combo_2.ogg."],
      ["sounds/combo42.ogg", "Cascade 3", "Together with the match sound, for the third match of a move"],
      ["sounds/combo52.ogg", "Cascade 4", "Together with the match sound, for the fourth match of a move"],
      ["sounds/combo62.ogg", "Cascade 5", "Together with the match sound, for the fifth match of a move"],
      ["sounds/combo72.ogg", "Cascade 6", "Together with the match sound, for the sixth match of a move and every one after it"]
    ] },
    { name: "Special gems", sounds: [
      ["sounds/hypergem_creation.ogg", "Hypercube made", "A match of five in a row makes a hypercube (unless a flame gem blast is next to it); also when the title logo's gem finishes forming"],
      ["sounds/hypergem_destroyed.ogg", "Hypercube used", "A hypercube is caught in a flame gem's 3 x 3 blast and bursts into sparks; at most one sound per update"],
      ["sounds/electro_start.ogg", "Lightning start", "A hypercube is swapped with a neighbouring gem and the lightning starts"],
      ["sounds/electro_path.ogg", "Lightning", "The lightning reaches one more gem: each update has a 1 in 25 chance with no gem lit yet, and a better one as more gems are lit"],
      ["sounds/electro_explode.ogg", "Lightning blast", "A gem the lightning lit explodes, 0.67 s (67 updates) after it was lit; one sound for each gem"],
      ["sounds/bombexplode.ogg", "Gem explodes", "A flame gem explodes when matched, once per update however many go off; also when a bomb gem's count reaches zero. Resembles Bejeweled 3's sounds/bomb_explode.ogg, the same sound, mixed differently."]
    ] },
    { name: "Praise", sounds: [
      ["sounds/Good.ogg", "\"Good\"", "A move's points reach 60 × the level's multiplier, at most once in 120 s (12,000 updates); never in Puzzle"],
      ["sounds/excellent1.ogg", "\"Excellent\"", "A move's points reach 125 × the level's multiplier (the word too, but not in Action); never in Puzzle. Also: the logo's gem finishes forming"],
      ["sounds/Incredible.ogg", "\"Incredible\"", "A move's points reach 275 × the level's multiplier (the word too, but not in Action); never in Puzzle"]
    ] },
    { name: "Levels", sounds: [
      ["sounds/Get_ready.ogg", "\"Get ready\"", "A new level's warp finishes (never in Puzzle): in 3D when the board is rebuilt, and 4.5 s (450 updates) into the hyperspace warp"],
      ["sounds/Go.ogg", "\"Go\"", "3 s (300 updates) after a board opens, 0.5 s after Play Again, 0.75 s after Continue on a planet; not while the board slides out"],
      ["sounds/whirlpool1.ogg", "Whirlpool", "A whirlpool warp begins: a level is won and its score has counted up, another planet is chosen, the puzzle dome is clicked, or a secret puzzle run ends"],
      ["sounds/Level_Complete.ogg", "\"Level complete\"", "The level bar is full (not in Puzzle), the board has settled and nothing is still clearing; the whirlpool follows when the score is counted"],
      ["sounds/warning1.ogg", "Warning", "A timed (Action) game's bar is under 15% and has stopped moving: every 0.8 s to 2.4 s, faster as the bar empties; not once the game is over"]
    ] },
    { name: "Puzzles", sounds: [
      ["sounds/planetselect.ogg", "Planet select", "A puzzle is clicked on the galaxy map and its planet is open; a locked planet plays the Bad swap sound instead"],
      ["sounds/galaxymapbutton.ogg", "Galaxy map", "The puzzle dome's button is released, as the galaxy map opens (with the panel slide)"],
      ["sounds/Puzzle_Solved.ogg", "\"Puzzle solved\"", "The board is emptied and the planet's solved count rises to 1, 2 or 3; re-solving a puzzle already solved plays nothing"],
      ["sounds/Warp_Drive_Ready.ogg", "\"Warp drive ready\"", "A puzzle is newly solved and it makes the planet's fourth solved puzzle, which opens the next planet"],
      ["sounds/World_Complete.ogg", "\"World complete\"", "A puzzle is newly solved and it makes the planet's fifth solved puzzle"]
    ] },
    { name: "Game over", sounds: [
      ["sounds/explode2.ogg", "Explosion", "The board blows up 2 s (200 updates) after the game ends, whether by no moves or by time up"],
      ["sounds/No_More_Moves.ogg", "\"No more moves\"", "No move is left on the board after the falls settle, in Classic and Endless (Action ends on time, Puzzle plays nothing)"],
      ["sounds/Time_Up.ogg", "\"Time up\"", "A timed (Action) game's bar runs out while the board is idle; the explosion follows 2 s later"],
      ["sounds/Game_Over.ogg", "\"Game over\" (unused)", "Never played: it is loaded, but the game ends with the No more moves or Time up voice instead"],
      ["sounds/A_New_High_Score.ogg", "\"A new high score\"", "A game ends and its final score earns a place (rank 1 to the list's end) on the high scores"]
    ] },
    { name: "Bejeweled leftovers", sounds: [
      ["sounds/firecrackle.ogg", "Fire crackle (unused)", "Never played: it is loaded, but no code plays it, by id or by number. Also in Bejeweled as Crackle: the vortex has finished and the gems are gone."]
    ] }
  ] },
  // Bejeweled Twist: sounds/combo_1.ogg (Match 1) is left out, never played and nearly silent (its loudest point -49 dB).
  bejeweledtwist: {
    presets: [
      { name: "Start Classic", when: "Classic is chosen", steps: [["sounds/doorlock2", 0, {"v": -650}], ["sounds/voice_classic", 0, {"v": -650}], ["sounds/trans_spacenoise", 0], ["sounds/buttonpress2", 0, {"v": -650}], ["sounds/trans_arrival", 3610, {"v": -650}]]},
      { name: "Level Complete", when: "The level bar fills", steps: [["sounds/voice_levelcomplete", 0, {"v": -650}], ["sounds/trans_spacenoise", 300, {"v": -650, "ramp": [[2600, -563], [2640, -274], [2690, 0]]}], ["sounds/rowblow", 1080, {"v": -1707}], ["sounds/rowblow", 1310, {"v": -1707}], ["sounds/rowblow", 1430, {"v": -1707, "ramp": [[2600, -1628], [2670, -1197]]}], ["sounds/rowblow", 1550, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 1670, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 1790, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 1910, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/rowblow", 2030, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/trans_preignition", 2360, {"v": -1707, "ramp": [[2600, -1628], [2640, -1360], [2690, -1102]]}], ["sounds/geode", 2440, {"v": -1573, "r": 1.0234, "ramp": [[2600, -1494], [2640, -1226], [2690, -968]]}], ["sounds/trans_engineignition", 3600, {"v": -851}], ["sounds/trans_engineloop1", 3600, {"v": -1176, "loop": 1, "end": 8350, "ramp": [[3600, -526], [8260, -627], [8290, -1009], [8310, -1377], [8320, -1628], [8330, -1962], [8340, -2465]]}], ["sounds/trans_arrival", 7210, {"v": -650}], ["sounds/voice_go", 9580, {"v": -650}]]},
      { name: "Flame Gem", when: "A flame gem is made and set off", steps: [["sounds/powergem_created", 0, {"v": -1102, "p": -600}], ["sounds/doubleset", 0, {"v": -796, "p": 1}], ["sounds/preblast", 1090, {"v": -650}], ["sounds/gem_shatters", 1090, {"v": -968, "p": -600}], ["sounds/combo_3", 1090, {"v": -650}], ["sounds/voice_good", 1100, {"v": -650}]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/voice_welcometobejeweledtwist.ogg", "\"Welcome to Bejeweled Twist\"", "The loading screen's countdown reaches 125 updates before its end, so 1.25 s before the main menu comes in"],
      ["sounds/mouseover1.ogg", "Hover 1", "The pointer enters a main menu button, or a dialog button (higher pitch for each button along the row); a silent dialog button plays nothing"],
      ["sounds/mouseover2.ogg", "Hover 2", "The pointer enters a planet on the challenge screen, panned to its place and raised by half a step for each planet number"],
      ["sounds/mouseover3.ogg", "Hover 3", "The pointer enters a mode button on the mode screen, panned to it and raised by two steps for each button along"],
      ["sounds/click.ogg", "Click", "A button goes down on the mode screen, help, profile or high scores panels, the award screens, the end stats, the board's side buttons, or the app's full screen button"],
      ["sounds/click_dialog.ogg", "Dialog click", "A button goes down in a dialog; a button is released in Help; any button or check box is used in Options"],
      ["sounds/menu_button_click.ogg", "Menu click", "A main menu button goes down (not the corner one); Quit or Replay goes down on the high scores; a new player's name is accepted"],
      ["sounds/buttonpress1.ogg", "Replay button", "The replay pod's button is released, taking the last move back"],
      ["sounds/buttonpress2.ogg", "Button press", "A button is released on the mode screen or the challenge screen; also a button beside the level bar, at a higher pitch, while the game is playing and not paused"],
      ["sounds/menu_slide.ogg", "Menu slide", "The high scores panel is left through its Quit bar; also, once per player, the Zen jump tip slides in (Zen after 180,000 updates played, or a first Classic game on a roll below 3 of 5)"],
      ["sounds/doorlock2.ogg", "Door lock", "A main menu door button opens or closes a panel; the profile list opens; mode doors open on the mode screen for modes newly seen"],
      ["sounds/doorlock3.ogg", "Door lock, locked", "The profile button is pressed while the profile list is already open or the high scores are showing, and the menu returns to the main screen"],
      ["sounds/steamlock1.ogg", "Steam lock", "The main menu enters its start state (Play is pressed) and the gates begin to open"],
      ["sounds/lock1.ogg", "Gate lock", "The gates have moved a third (33%) of the way open or shut"],
      ["sounds/innerlock1.ogg", "Inner lock, in", "The mode screen is added to the screen"],
      ["sounds/innerlock2.ogg", "Inner lock, out", "Main Menu is pressed on the mode screen and its panels close"],
      ["sounds/voice_chooseyourgame.ogg", "\"Choose your game\"", "The mode screen opens, once each time it is built"],
      ["sounds/voice_classic.ogg", "\"Classic\"", "The Classic mode button is pressed on the mode screen, and Classic is unlocked"],
      ["sounds/voice_challenge.ogg", "\"Challenge\"", "The Challenge mode button is pressed on the mode screen, and Challenge is unlocked"],
      ["sounds/voice_blitz.ogg", "\"Blitz\"", "The Blitz mode button is pressed on the mode screen, and Blitz is unlocked"],
      ["sounds/voice_zen.ogg", "\"Zen\"", "The Zen mode button is pressed on the mode screen, and Zen is unlocked"],
      ["sounds/zenstart.ogg", "Zen start", "A Zen game starts, new or continued; no other mode plays it"],
      ["sounds/typewriter1.ogg", "Typewriter", "A letter, digit or space is typed into a name box; other keys make no sound"],
      ["sounds/screenprint.ogg", "Screen print", "A high scores category is picked by a click, so its page prints on (it does not play when the page is set at once)"],
      ["sounds/transfer.ogg", "Volume check", "The Sound volume slider in Options is let go (not while it is being dragged). Also in Chuzzle Deluxe, as sounds/Bonus/GotJewel.ogg."],
      ["sounds/bubblepop.ogg", "Dialog pop", "Any dialog pops up"],
      ["sounds/voice_goodbye.ogg", "\"Goodbye\"", "Quit is confirmed in the quit dialog; the game closes once the voice has finished"]
    ] },
    { name: "Starting and the cursor", sounds: [
      ["sounds/voice_go.ogg", "\"Go!\"", "The board has settled and its wait is over: at a game's start (not Blitz, not in the tutorial; a Zen jump tip can come first) and 1 s after each level's start"],
      ["sounds/select.ogg", "Cursor tick", "The cursor moves onto a different gem while it is shown and input is free (not in a solved puzzle); panned to the gem's column"],
      ["sounds/start_rotate.ogg", "Twist start", "A group of four gems starts to turn: normal pitch if the twist will make a match, 5 steps lower if it will not. Also in Bejeweled 3, as sounds/start_rotate.ogg."],
      ["sounds/gem_prelock.ogg", "Gem about to lock", "A gem is marked for a lock, two twists from locking (matching it first saves it); also when a doom gem is placed"],
      ["sounds/badmove.ogg", "Bad move", "A twist is tried before the level starts or locked gems block it; also a twist ends with no match (not in Challenge), 2 steps lower. Also in Bejeweled 3, as sounds/badmove.ogg."],
      ["sounds/tutorial_snap.ogg", "Tutorial snap", "The cursor is moved onto the gem a tutorial step points at"],
      ["sounds/tutorial-match.ogg", "Tutorial match", "The gems of a twist land while the match flash is up: a twist about to make the first flame or electro gem, in Classic or Zen, before its tip has been shown"],
      ["sounds/tutorialcomplete.ogg", "Tutorial done", "The tutorial's last step shows its graduation dialog"],
      ["sounds/reshuffle1.ogg", "Reshuffle", "A challenge is restarted: the score and counts go back to zero and the board is dealt again"],
      ["sounds/purr.ogg", "Purr", "A tooltip for a coal gem opens, only when the coal-as-snack setting is on; other coal tooltips are silent"]
    ] },
    { name: "Matches and chains", sounds: [
      ["sounds/combo_2.ogg", "Match 2", "The 1st match of a twist (step 1 of its chain), when no match in it has six or more gems; also when the wheel's bomb is disarmed. Also in Bejeweled 3, as sounds/combo_2.ogg."],
      ["sounds/combo_3.ogg", "Match 3", "The 2nd match of a twist (chain step 2), when no match in it has six or more gems"],
      ["sounds/combo_4.ogg", "Match 4", "The 3rd match of a twist (chain step 3), when no match in it has six or more gems"],
      ["sounds/combo_5.ogg", "Match 5", "The 4th match of a twist (chain step 4), when no match in it has six or more gems"],
      ["sounds/combo_6.ogg", "Match 6", "The 5th match of a twist (chain step 5), when no match in it has six or more gems"],
      ["sounds/combo_7.ogg", "Match 7", "The 6th match of a twist and every one after it (chain step 6 and up), when no match in it has six or more gems"],
      ["sounds/sixmatch.ogg", "Six-gem match", "A match of six or more gems is found, instead of the Match sound of that step; pitched one step higher for each gem beyond six. Resembles Bejeweled 3's sounds/quest_get.ogg, the same sound, mixed differently."],
      ["sounds/doubleset.ogg", "Double set", "Two or more sets are matched in the same step: normal pitch for two, one step higher for three or more"],
      ["sounds/chainx1.ogg", "Chain note", "A twist's first match joins the chain of matching twists: two notes an octave apart, climbing a step with each link and wrapping every 12"],
      ["sounds/chainparticle1.ogg", "Chain smoke", "A yellow puff of smoke released by the level bar itself reaches its light; the smoke that carries points there is silent"],
      ["sounds/gem_hit.ogg", "Gem lands", "Falling gems land (fruit gems do not count), at most once every 10 updates (0.1 s), panned to where they landed. Also in Bejeweled 3, as sounds/gem_hit.ogg. Also in Chuzzle Deluxe, as sounds/Bonus/JewelLand.ogg."],
      ["sounds/speedbonus.ogg", "Speed bonus", "Every 6th quick twist in a row (each within 1 s of the last) gives a bonus: 500, 1000, then 2000 points; pitch rises with the bonus level"],
      ["sounds/multiplier_up2_1.ogg", "Level up 1 (unused)", "Never played: the multiplier starts at 1, and a rise to level 2 plays Level up 3, so this one is never chosen"],
      ["sounds/multiplier_up2_2.ogg", "Level up 2 (unused)", "Never played: the code picks the file by the new level plus one, starting from level 2, so this one is skipped"],
      ["sounds/multiplier_up2_3.ogg", "Level up 3", "The level bar's multiplier rises to level 2, while the board is not busy"],
      ["sounds/multiplier_up2_4.ogg", "Level up 4", "The multiplier rises to level 3 or higher, while the board is not busy"],
      ["sounds/multiplier_down.ogg", "Multiplier down", "The shown multiplier falls to a lower level, while the board is not busy"],
      ["sounds/voice_good.ogg", "\"Good\"", "When a twist's rating reaches 3. Said once a twist, as its matches fall, for the highest praise its rating has reached; a later match in the same twist can step it up to a higher one. Never in Challenge, in a replay, in Blitz's first 10 seconds or when the twist ends the level. The rating adds up, all from the twist alone: its cascades after the first, to the power 1.5; the gems it cleared, divided by 15, to the power 1.5; 8 for each gem past five in its longest match; the fruit gems it set off, squared; 1 for each lock broken, bomb defused and flame gem made; and 1 for the first and 2 for each more of the electro gems made, flame gems destroyed, electro gems destroyed and coal destroyed, each of the four counted on its own"],
      ["sounds/voice_excellent.ogg", "\"Excellent\"", "When a twist's rating reaches 6. Said once a twist, as its matches fall, for the highest praise its rating has reached; a later match in the same twist can step it up to a higher one. Never in Challenge, in a replay, in Blitz's first 10 seconds or when the twist ends the level. The rating adds up, all from the twist alone: its cascades after the first, to the power 1.5; the gems it cleared, divided by 15, to the power 1.5; 8 for each gem past five in its longest match; the fruit gems it set off, squared; 1 for each lock broken, bomb defused and flame gem made; and 1 for the first and 2 for each more of the electro gems made, flame gems destroyed, electro gems destroyed and coal destroyed, each of the four counted on its own"],
      ["sounds/voice_awesome.ogg", "\"Awesome\"", "When a twist's rating reaches 12. Said once a twist, as its matches fall, for the highest praise its rating has reached; a later match in the same twist can step it up to a higher one. Never in Challenge, in a replay, in Blitz's first 10 seconds or when the twist ends the level. The rating adds up, all from the twist alone: its cascades after the first, to the power 1.5; the gems it cleared, divided by 15, to the power 1.5; 8 for each gem past five in its longest match; the fruit gems it set off, squared; 1 for each lock broken, bomb defused and flame gem made; and 1 for the first and 2 for each more of the electro gems made, flame gems destroyed, electro gems destroyed and coal destroyed, each of the four counted on its own"],
      ["sounds/voice_incredible.ogg", "\"Incredible\"", "When a twist's rating reaches 20. Said once a twist, as its matches fall, for the highest praise its rating has reached; a later match in the same twist can step it up to a higher one. Never in Challenge, in a replay, in Blitz's first 10 seconds or when the twist ends the level. The rating adds up, all from the twist alone: its cascades after the first, to the power 1.5; the gems it cleared, divided by 15, to the power 1.5; 8 for each gem past five in its longest match; the fruit gems it set off, squared; 1 for each lock broken, bomb defused and flame gem made; and 1 for the first and 2 for each more of the electro gems made, flame gems destroyed, electro gems destroyed and coal destroyed, each of the four counted on its own"],
      ["sounds/voice_spectacular.ogg", "\"Spectacular\"", "When a twist's rating reaches 30. Said once a twist, as its matches fall, for the highest praise its rating has reached; a later match in the same twist can step it up to a higher one. Never in Challenge, in a replay, in Blitz's first 10 seconds or when the twist ends the level. The rating adds up, all from the twist alone: its cascades after the first, to the power 1.5; the gems it cleared, divided by 15, to the power 1.5; 8 for each gem past five in its longest match; the fruit gems it set off, squared; 1 for each lock broken, bomb defused and flame gem made; and 1 for the first and 2 for each more of the electro gems made, flame gems destroyed, electro gems destroyed and coal destroyed, each of the four counted on its own"],
      ["sounds/voice_unbelievable.ogg", "\"Unbelievable\"", "When a twist's rating reaches 45. Said once a twist, as its matches fall, for the highest praise its rating has reached; a later match in the same twist can step it up to a higher one. Never in Challenge, in a replay, in Blitz's first 10 seconds or when the twist ends the level. The rating adds up, all from the twist alone: its cascades after the first, to the power 1.5; the gems it cleared, divided by 15, to the power 1.5; 8 for each gem past five in its longest match; the fruit gems it set off, squared; 1 for each lock broken, bomb defused and flame gem made; and 1 for the first and 2 for each more of the electro gems made, flame gems destroyed, electro gems destroyed and coal destroyed, each of the four counted on its own"]
    ] },
    { name: "Special gems", sounds: [
      ["sounds/powergem_created.ogg", "Flame gem made", "A gem becomes a flame gem (a match of four); not while the board fills quietly, as when a game is loaded"],
      ["sounds/preblast.ogg", "Flame gem charging", "A flame gem goes off: it plays at the instant of the blast, together with Flame gem blast, and starts the first-flame tip once per player. Also in Bejeweled 3, as sounds/preblast.ogg."],
      ["sounds/gem_shatters.ogg", "Flame gem blast", "A flame gem goes off, blasting the gems one cell around it, at the same instant as Flame gem charging"],
      ["sounds/electrogem_created.ogg", "Electro gem made", "A gem becomes an electro gem (a match of five); not while the board fills quietly"],
      ["sounds/electro_start1.ogg", "Electro start", "A plain electro gem is matched and goes off; a flame electro gem plays Flame electro instead"],
      ["sounds/electro_explode.ogg", "Electro strike", "The first step (step 0) of each electro gem's lightning strike, when its bolts hit"],
      ["sounds/flamelightning.ogg", "Flame electro", "An electro gem that is also a flame gem goes off, in place of Electro start"],
      ["sounds/supergem_create.ogg", "Super gem made (unused)", "Never played: it is loaded, but neither the port nor the original's code ever plays it, by id or by number"],
      ["sounds/wildcard_destroyed.ogg", "Electro gem goes off", "Every electro gem that goes off, at the same instant as Electro start or Flame electro, as its strike begins"],
      ["sounds/bonuslaser.ogg", "Fruit laser", "A fruit gem's shockwave is created, as the fruit is blasted"],
      ["sounds/fruit_appears.ogg", "Fruit gem appears", "A fruit gem is put on the board while it is not busy; not while the board fills quietly"],
      ["sounds/gem_fruit_destroyed.ogg", "Fruit gem matched", "A fruit gem is matched and its blast starts"],
      ["sounds/fruitbonuscount.ogg", "Fruit count", "A match of the fruit bonus's next colour while the bonus is running; the pitch rises with the number of colours already matched (0 to 3)"],
      ["sounds/rowblow.ogg", "Row blows", "Once for each row that breaks up in the jump after a level (a row every 12 updates), and once for each gem of a fruit blast's sparks"],
      ["sounds/gem_lock_start.ogg", "Lock starts", "A marked gem is still unmatched when its two twists run out, and its lock starts closing"],
      ["sounds/gem_lock_end.ogg", "Lock closes", "A new lock finishes closing over its gem, 50 updates (0.5 s) after Lock starts; a doom lock becomes a doom gem with a count then"],
      ["sounds/gem_lock_break.ogg", "Lock breaks", "A match breaks the lock on a gem, shattering it into shards"],
      ["sounds/coal_blasted.ogg", "Coal blasted", "A coal gem is broken by a match or blast next to it, shedding its pieces. Resembles Bejeweled 3's sounds/diamond_mine_bigstone_cracked.ogg, the same sound, mixed differently."],
      ["sounds/geode.ogg", "Geode", "A geode pops and shows its points, pitched up one step for each geode before it (at most 10); also each rank bonus light that fills, up to 6 steps up. Also in Bejeweled 3, as sounds/rank_countup.ogg."],
      ["sounds/soulify.ogg", "Gem becomes a soul", "A gem is turned into a soul, 80 updates (0.8 s) after the soul effect starts"],
      ["sounds/gemsoul-lands3.ogg", "Soul lands", "A soul lands on the board and gives its gem its special; at most one sound every 2 updates, and one more when the first soul lands"]
    ] },
    { name: "Bombs and counters", sounds: [
      ["sounds/gem_countdown.ogg", "Counter gem falls", "A countdown gem lands for the first time (not in Puzzle)"],
      ["sounds/counter_warning.ogg", "Counter warning", "After a move, the lowest countdown gem shows 5 or fewer: the pitch starts 3 steps below normal at 5 and rises one step for each number it falls"],
      ["sounds/gem_countdown_destroyed.ogg", "Counter gem destroyed", "A countdown gem is matched and defused; also when a new row slides into the high scores list, 3 steps up. Resembles Bejeweled 3's sounds/gem_countdown_destroyed.ogg, the same sound, mixed differently."],
      ["sounds/bomb_explode.ogg", "Bomb explodes", "A countdown gem reaches zero; also when a burst goes off (once per update), a nuke's blast grows past 1.6, and the end glass hits the floor"],
      ["sounds/doomgem_appears.ogg", "Doom gem appears", "A plain gem inside the board's border that no twist would match is turned into a doom gem"],
      ["sounds/doomgem_countdown.ogg", "Doom gem ticks", "A doom gem's count drops by one with each move, and when a doom lock closes into its doom gem with its count. Resembles Bejeweled 3's sounds/bomb_appears.ogg, the same sound, mixed differently."],
      ["sounds/doomgem_destroyed.ogg", "Doom gem destroyed", "A doom gem is matched or blasted"],
      ["sounds/doomgemgameover.ogg", "Doom gem game over", "The game ends by a nuke caused by a doom gem going off"],
      ["sounds/endfinal6b.ogg", "Board blows up", "The game ends by a nuke: a countdown gem is the one that ended it, or the wheel stopped on a bad slot. Resembles Bejeweled 3's sounds/timebombexplode.ogg, the same sound, mixed differently."]
    ] },
    { name: "Time and levels", sounds: [
      ["sounds/voice_thirtyseconds.ogg", "\"Thirty seconds\"", "The Blitz clock's seconds left reach 30"],
      ["sounds/voice_oneminute.ogg", "\"One minute\"", "The Blitz clock's seconds left reach 60"],
      ["sounds/voice_twominutes.ogg", "\"Two minutes\"", "The Blitz clock's seconds left reach 120"],
      ["sounds/voice_threeminutes.ogg", "\"Three minutes\"", "The Blitz clock's seconds left reach 180"],
      ["sounds/voice_fourminutes.ogg", "\"Four minutes\"", "The Blitz clock's seconds left reach 240"],
      ["sounds/voice_nine.ogg", "\"Nine\"", "The Blitz clock's seconds left reach 9"],
      ["sounds/voice_eight.ogg", "\"Eight\"", "The Blitz clock's seconds left reach 8"],
      ["sounds/voice_seven.ogg", "\"Seven\"", "The Blitz clock's seconds left reach 7"],
      ["sounds/voice_six.ogg", "\"Six\"", "The Blitz clock's seconds left reach 6"],
      ["sounds/voice_five.ogg", "\"Five\"", "The Blitz clock's seconds left reach 5"],
      ["sounds/voice_four.ogg", "\"Four\"", "The Blitz clock's seconds left reach 4"],
      ["sounds/voice_three.ogg", "\"Three\"", "The Blitz clock's seconds left reach 3"],
      ["sounds/voice_two.ogg", "\"Two\"", "The Blitz clock's seconds left reach 2"],
      ["sounds/voice_one.ogg", "\"One\"", "The Blitz clock's seconds left reach 1"],
      ["sounds/voice_levelcomplete.ogg", "\"Level complete\"", "The level bar fills and the level-up show begins (in Zen, when each new level starts)"],
      ["sounds/whirlpool1.ogg", "Whirlpool", "The level-up show has run 3.5 s (350 updates), or 2 s (200) when a tip is showing, and the board starts to drain for the jump"],
      ["sounds/trans_spacenoise.ogg", "Space noise", "The jump to the next level begins, as the space scene comes in; it also starts on the other warp scenes"],
      ["sounds/trans_preignition.ogg", "Engine warms", "The jump's tunnel countdown has 125 updates (1.25 s) left, just before the engine ignites"],
      ["sounds/trans_engineignition.ogg", "Engine starts", "The jump's engine ignites, when the tunnel countdown ends and the engine loop starts"],
      ["sounds/trans_engineloop1.ogg", "Engine runs", "The engine loop starts with the ignition and runs through the jump (loops)"],
      ["sounds/trans_arrival.ogg", "Arrival", "The jump has 140 updates (1.4 s) left, as the next level comes into view"],
      ["sounds/zentransition1.ogg", "Zen change 1", "A new Zen level starts and the wheel picture's wheels grow in"],
      ["sounds/zentransition2.ogg", "Zen change 2", "The Zen picture's first change finishes (after about 3 s, 300 updates)"],
      ["sounds/zentransition3.ogg", "Zen change 3", "The Zen picture's second change passes 30% of its run, 1.5 s after the first one finishes"],
      ["sounds/spinsequencer1.ogg", "Spin sequence", "A countdown gem reaches zero and the skull wheel starts to spin up"],
      ["sounds/rewind.ogg", "Rewind", "A move is run backwards in a replay; it loops until the replay moves on"],
      ["sounds/replay_popup.ogg", "Replay pops up", "The replay pod rises, in Classic or Zen only, once the board has settled after the first flame gem or the first geode of a player. Resembles Bejeweled 3's sounds/replay_popup.ogg, the same sound, mixed differently."]
    ] },
    { name: "The wheel", sounds: [
      ["sounds/x_spinnerappears1.ogg", "Wheel appears", "The skull wheel has opened fully and its slots come into view"],
      ["sounds/siren.ogg", "Siren", "The wheel's siren loops while the wheel is up; its volume changes when the wheel is won or lost"],
      ["sounds/temp_timpani.ogg", "Timpani", "A beat as each of the wheel's slots is lit in turn, alternating left (normal pitch) and right (4 steps lower)"],
      ["sounds/x_wheelspin.ogg", "Wheel click 1", "A slot passes the wheel's pointer while the wheel is in its fast-spinning state, at half volume"],
      ["sounds/x_wheelspin2.ogg", "Wheel click 2", "A slot passes the pointer in the wheel's other spinning states, such as slowing down, at half volume"],
      ["sounds/skull-added.ogg", "Skull added", "A skull is added to the wheel, a quarter (25%) of the way through that step"],
      ["sounds/x_win.ogg", "Wheel win", "The wheel stops on a good slot and the bomb's count is set to 20"],
      ["sounds/x_win2.ogg", "Wheel win, sparkle", "After a win, the gap between the gems closes to its minimum, once"],
      ["sounds/x_lose.ogg", "Wheel lose", "The wheel stops on a bad slot, and the two skulls' slots are crossed out"],
      ["sounds/voice_bombdisarmed.ogg", "\"Bomb disarmed\"", "A win is tallied and the bomb is disarmed, 0.01 s after the Match 2 chime, and the wobble ends"],
      ["sounds/spinner_unlock.ogg", "Wheel unlocks", "The wheel unfolds on the board: full volume if it was lost, a fifth of it if it was won"]
    ] },
    { name: "Game end and awards", sounds: [
      ["sounds/voice_gameover.ogg", "\"Game over\"", "Any game ends (no moves, a nuke, Blitz time up or a lost wheel), as the board's end sequence begins"],
      ["sounds/tally_zoomin.ogg", "Score zooms in", "Blitz's FINAL SCORE label comes in; also after a nuke's zoom ends, and when the Blitz clock's time tally reaches its full count"],
      ["sounds/blitz-countup.ogg", "Score counts up", "A tally step: the Blitz time count (pitch up to 5 steps), the end stats bars (every 3rd step, up to 12), a fruit blast, and the wheel's bomb count"],
      ["sounds/voice_anewhighscore.ogg", "\"A new high score\"", "The high scores page opens on a new record, 1 s (100 updates) after the new record's row is set"],
      ["sounds/voice_challengecomplete.ogg", "\"Challenge complete\"", "Every colour of a fruit bonus has been matched in order, in Classic, Blitz or Zen (never in Challenge mode)"],
      ["sounds/challengecompleted.ogg", "Challenge done", "At the same instant as the \"Challenge complete\" voice: a fruit bonus's last colour is matched"],
      ["sounds/challenge_unlock.ogg", "Challenge unlocks", "A planet on the challenge screen moves on one stage: its bar fills and the new number glows"],
      ["sounds/voice_newchallengeunlocked.ogg", "\"New challenge unlocked\"", "The challenge screen's dimming passes 90% while a new challenge is announced"],
      ["sounds/gameunlocked.ogg", "Mode unlocked", "A game mode is unlocked for the profile for the first time, as its banner comes up"],
      ["sounds/lightbar_blink.ogg", "Rank light", "The rank bonus bar fills a slot and lights up"],
      ["sounds/rank_completion.ogg", "Rank complete", "Continue is pressed on an award screen, or the rank bonus screen is clicked once it is filled. Resembles Bejeweled 3's sounds/multiplier_appears.ogg, the same sound, mixed differently."],
      ["sounds/trophy_awarded.ogg", "Trophy", "A trophy or award is shown: the credits open, the fruit award screen starts, and an award screen's text fades in (about 40% in)"],
      ["sounds/eclipseintro1.ogg", "Eclipse intro (unused)", "Never played: it is loaded, but no code plays it, by id or by number, in the port or in the original"],
      ["sounds/badloop.ogg", "Bad loop (trial only)", "Trial only: the unregistered game's trial limit dialog sets up this looping sound when it opens; the port removed the dialog"],
      ["sounds/goodloop.ogg", "Good loop (trial only)", "Trial only: the unregistered game's trial limit dialog sets up this looping sound when it opens, beside Bad loop; the port removed the dialog"],
      ["sounds/trialclick.ogg", "Trial click (trial only)", "Trial only: an unregistered copy's click on a challenge beyond the free ones (past the third planet or stage); the port removed it"],
      ["sounds/triallimit.ogg", "Trial limit (trial only)", "Trial only: the unregistered game's limit dialog opens when its free levels run out (the jump after level 10 of Classic or 15 of Zen); the port removed it"],
      ["sounds/trialunlock.ogg", "Trial unlock (unused)", "Never played: it is loaded, but not even the original's trial code plays it"]
    ] },
  ] },
  bigmoney: {
    presets: [
      // the voices of a game start and of combos, from Board.cpp (the log they first came from had its bot clicking every 13
      // updates, and the game selector's welcome, 100 updates after it opens, landing on the start): Get Ready at the board's
      // first update and Go 139 updates later (mGoTimer 140); a combo's voice is the click's, by its size and the two before,
      // so those presets are clicks a second apart
      { name: "Get Ready", when: "A game starts: Get ready! at once, Go! 1.39 s later", steps: [["sounds/getready", 0], ["sounds/go", 1390]]},
      { name: "Big Clear", when: "One click of 15 to 19 coins, the click before under 10: Big money! (+200)", steps: [["sounds/bigmoney", 0], ["sounds/clearcoins3", 0], ["sounds/tone1", 0]]},
      { name: "Triple Combo", when: "Three clicks of 7 to 9 coins in a row, 1 s apart: Good job! on the 2nd (+75), Awesome! on the 3rd (+100)", steps: [["sounds/clearcoins2", 0], ["sounds/tone1", 0], ["sounds/goodjob", 1000], ["sounds/clearcoins2", 1000], ["sounds/tone2", 1000], ["sounds/awesome", 2000], ["sounds/clearcoins2", 2000], ["sounds/tone3", 2000]]},
      { name: "Triple Mega", when: "Three clicks of 10 to 12 coins in a row, 1 s apart: Excellent! (+25), Incredible! (+250), Unbelievable! (+350)", steps: [["sounds/excellent", 0], ["sounds/clearcoins3", 0], ["sounds/tone1", 0], ["sounds/incredible", 1000], ["sounds/clearcoins3", 1000], ["sounds/tone2", 1000], ["sounds/unbelievable", 2000], ["sounds/clearcoins3", 2000], ["sounds/tone3", 2000]]},
      { name: "Level Up", when: "The last bag of a level is won", steps: [["sounds/clearcoins", 0], ["sounds/tone9", 0], ["sounds/cointube", 20], ["sounds/cointube", 40], ["sounds/levelup", 70], ["sounds/cointube", 80], ["sounds/cointube", 160]]},
      { name: "Game Over", when: "The coins reach the top", steps: [["sounds/gameovertone", 0], ["sounds/gameover", 2800], ["sounds/anewhighscore", 5590]]}
    ],
    groups: [
    { name: "Selector", sounds: [
      ["sounds/buttonclick.ogg", "Button click", "A button goes down on the game selector, the board, the high scores, Help or Options, or in a dialog"],
      ["sounds/welcome.ogg", "\"Welcome!\"", "1 s (100 updates) after the game selector opens, the first time it opens since the game started"],
      ["sounds/loveit.ogg", "\"Love it!\"", "1 s (100 updates) after the game selector opens, with a 1 in 10 chance, on every opening after the first"],
      ["sounds/action.ogg", "\"Action!\"", "The Action game button is clicked to select it (a second click starts the game), unless Mr. Money is still saying a greeting"],
      ["sounds/strategy.ogg", "\"Strategy!\"", "The Strategy game button is clicked to select it (a second click starts the game), unless Mr. Money is still saying a greeting"],
      ["sounds/puzzle.ogg", "\"Puzzle!\"", "The Puzzle game button is clicked to select it (a second click starts the game), unless Mr. Money is still saying a greeting"],
      ["sounds/wakeup.ogg", "\"Wake up!\"", "120 s (12,000 updates) pass with no mouse or key input; it plays once per idle stretch"]
    ] },
    { name: "Start and rows", sounds: [
      ["sounds/getready.ogg", "\"Get ready!\"", "At the first update of every game's board, as it starts"],
      ["sounds/go.ogg", "\"Go!\"", "1.4 s (140 updates) after a game's board starts, and 0.8 s (80 updates) after each later level begins"],
      ["sounds/moveup.ogg", "Row rises", "A minted row of coins pushes up from the bottom of the board, each time the pile has room and the mint delay is over"],
      ["sounds/piecehit.au", "Piece lands", "A coin lands while falling faster than 4, at most once every 8 updates"]
    ] },
    { name: "Clearing coins", sounds: [
      ["sounds/clearcoins.ogg", "Clear, small", "A click clears a group of 2 to 4 coins; also when a pickpocket clears a group"],
      ["sounds/clearcoins2.ogg", "Clear, medium", "A click clears a group of 5 to 9 coins"],
      ["sounds/clearcoins3.ogg", "Clear, big", "A click clears a group of 10 coins or more"],
      ["sounds/tone1.ogg", "Tone 1", "After a Strategy or Action clear, when the meter is less than 1/9 full; in the Puzzle end sweep, the first scoring row. Also in Insaniquarium Deluxe, as sounds/bonuscollect.ogg."],
      ["sounds/tone2.ogg", "Tone 2", "After a Strategy or Action clear, when the meter is 1/9 to under 2/9 full; in the Puzzle end sweep, the second scoring row"],
      ["sounds/tone3.ogg", "Tone 3", "After a Strategy or Action clear, when the meter is 2/9 to under 3/9 full; in the Puzzle end sweep, the third scoring row"],
      ["sounds/tone4.ogg", "Tone 4", "After a Strategy or Action clear, when the meter is 3/9 to under 4/9 full; in the Puzzle end sweep, the fourth scoring row"],
      ["sounds/tone5.ogg", "Tone 5", "After a Strategy or Action clear, when the meter is 4/9 to under 5/9 full; in the Puzzle end sweep, the fifth scoring row"],
      ["sounds/tone6.ogg", "Tone 6", "After a Strategy or Action clear, when the meter is 5/9 to under 6/9 full; in the Puzzle end sweep, the sixth scoring row"],
      ["sounds/tone7.ogg", "Tone 7", "After a Strategy or Action clear, when the meter is 6/9 to under 7/9 full; in the Puzzle end sweep, the seventh scoring row"],
      ["sounds/tone8.ogg", "Tone 8", "After a Strategy or Action clear, when the meter is 7/9 to under 8/9 full; in the Puzzle end sweep, the eighth scoring row"],
      ["sounds/tone9.ogg", "Tone 9", "After a Strategy or Action clear, when the meter is 8/9 full or more; in the Puzzle end sweep, the ninth scoring row and every one after"],
      ["sounds/tonepuzzle.ogg", "Puzzle tone", "After every click's clear in the Puzzle game (it replaces the meter tones there)"],
      ["sounds/cointube.ogg", "Coin in meter", "Each cleared coin that runs down the tube and reaches the meter"],
      ["sounds/explode.au", "Coin blast", "A dollar bomb goes off and the coins of its colour explode one after another, each with this sound. Also in Peggle Deluxe, Peggle Extreme, Peggle Nights and Peggle World of Warcraft Edition, as sounds/explode.ogg. Also in Insaniquarium Deluxe, as sounds/explode.au."]
    ] },
    { name: "Bags and bonuses", sounds: [
      ["sounds/bonusdrop.au", "Bag drops", "The meter becomes full while playing and a moneybag drops in; the meter empties"],
      ["sounds/baghit.ogg", "Bag lands", "A falling moneybag or dollar bomb reaches the pile (or the floor) while playing"],
      ["sounds/gotbag.ogg", "Bag collected", "A moneybag leaves the board because the coins under it were cleared"],
      ["sounds/bonus.ogg", "Bonus", "3 or more bags left on the board at a level's end; a bag collected within 5 s (500 updates) of the last; a tower toppled; and, if the click has no voice, 4 clicks in a row of one colour or 3 to 5 columns emptied"],
      ["sounds/superbonus.ogg", "\"Super bonus!\"", "When the click has no other voice: 6 clicks in a row of one colour, or 6 or more columns emptied by one click"],
      ["sounds/speedbonus.ogg", "\"Speed bonus!\"", "Action only: each click adds 0.16 to a gauge that falls 0.0025 per update; when it reaches 1 (about 7 quick clicks) this plays and 40 points are added"],
      ["sounds/megamoney.ogg", "\"Mega money!\" (unused)", "Never played: it is loaded, but no code plays it, in the port or in the original"]
    ] },
    { name: "Combo voices", sounds: [
      ["sounds/goodjob.ogg", "\"Good job!\"", "This click and the one before: 7+ coins each (+75), when no bigger combo applies"],
      ["sounds/excellent.ogg", "\"Excellent!\"", "A click of 10 to 14 coins, when no combo applies (+25)"],
      ["sounds/incredible.ogg", "\"Incredible!\"", "This click and the one before: 10+ coins each (+250), not both 13+"],
      ["sounds/awesome.ogg", "\"Awesome!\"", "A click of 20+ coins (+400); three clicks in a row of 13+ (+450); or of 7+ (+100)"],
      ["sounds/unbelievable.ogg", "\"Unbelievable!\"", "Three clicks in a row of 10+ coins, or two of 13+ (+350)"],
      ["sounds/bigmoney.ogg", "\"Big money!\"", "A click of 15 to 19 coins, the click before under 10 (+200)"]
    ] },
    { name: "Warnings and endings", sounds: [
      ["sounds/three.ogg", "\"Three\"", "After a Strategy click or move that leaves exactly 3 moves, with no moneybag above the cleared group; in Action, the first second (0 to 0.99 s) of the 3 s overflow countdown at the top"],
      ["sounds/two.ogg", "\"Two\"", "After a Strategy click or move that leaves exactly 2 moves; in Action, 1 s into the 3 s overflow countdown"],
      ["sounds/one.ogg", "\"One\"", "After a Strategy click or move that leaves exactly 1 move; in Action, 2 s into the 3 s overflow countdown, before the game ends at 3 s"],
      ["sounds/warning.ogg", "Warning", "Action: coins within 2 rows of the top, every 1.2 s falling to 0.4 s as they rise; Strategy and Puzzle: coins at the top row, every 1.5 s (150 updates)"],
      ["sounds/levelup.ogg", "\"Level up!\"", "The bags a level needs are banked and the level is complete, as the bills fall and the board falls away"],
      ["sounds/gameovertone.ogg", "Game over tone", "The game ends: coins overflow the top in Strategy or Puzzle, or the 3 s countdown ends in Action. Also in BookWorm as Game over tone (unused)."],
      ["sounds/gameover.ogg", "\"Game over\"", "After the game ends, once the board's last coins have been flung away and no loose pieces are left"],
      ["sounds/anewhighscore.ogg", "\"A new high score!\"", "The high scores open on a final score that is the best of today or earns a rank on the list"],
      ["sounds/bad.ogg", "Bad (trial only)", "Trial only: played when an invalid registration code is typed in; the port removed that path. Also in BookWorm as Bad (unused). Also in Peggle Deluxe, Peggle Extreme, Peggle Nights and Peggle World of Warcraft Edition, as sounds/pegpop.ogg."]
    ] },
  ] },
  bonnies: {
    presets: [
      { name: "Level Up", when: "A level's last word", steps: [["Sound/Level_Up_Quick", 0], ["Sound/Word_06", 0], ["Sound/Level_Up", 1440, {"v": -194}], ["Sound/AddPoint", 2940, {"v": -796, "r": 2.0}], ["Sound/AddPoint", 3000, {"v": -796, "r": 2.07}], ["Sound/AddPoint", 3090, {"v": -796, "r": 2.14}], ["Sound/AddPoint", 3150, {"v": -796, "r": 2.21}], ["Sound/AddPoint", 3210, {"v": -796, "r": 2.28}], ["Sound/AddPoint", 3300, {"v": -796, "r": 2.35}], ["Sound/AddPoint", 3360, {"v": -796, "r": 2.42}], ["Sound/AddPoint", 3420, {"v": -796, "r": 2.49}], ["Sound/AddPoint", 3480, {"v": -796, "r": 2.56}], ["Sound/AddPoint", 3540, {"v": -796, "r": 2.63}], ["Sound/AddPoint", 3600, {"v": -796, "r": 2.7}], ["Sound/AddPoint", 3660, {"v": -796, "r": 2.77}], ["Sound/AddPoint", 3720, {"v": -796, "r": 2.84}], ["Sound/AddPoint", 3780, {"v": -796, "r": 2.91}], ["Sound/AddPoint", 3840, {"v": -796, "r": 2.98}], ["Sound/AddPoint", 3900, {"v": -796, "r": 3.05}], ["Sound/AddPoint", 3960, {"v": -796, "r": 3.12}], ["Sound/AddPoint", 4020, {"v": -796, "r": 3.19}], ["Sound/AddPoint", 4080, {"v": -796, "r": 3.26}], ["Sound/AddPoint", 4110, {"v": -796, "r": 3.33}], ["Sound/AddPoint", 4170, {"v": -796, "r": 3.4}], ["Sound/AddPoint", 4230, {"v": -796, "r": 3.47}], ["Sound/AddPoint", 4290, {"v": -796, "r": 3.54}], ["Sound/AddPoint", 4320, {"v": -796, "r": 3.61}], ["Sound/AddPoint", 4380, {"v": -796, "r": 3.68}], ["Sound/AddPoint", 4440, {"v": -796, "r": 3.75}], ["Sound/AddPoint", 4470, {"v": -796, "r": 3.82}], ["Sound/AddPoint", 4530, {"v": -796, "r": 3.89}], ["Sound/AddPoint", 4560, {"v": -796, "r": 3.96}], ["Sound/AddPoint", 4620, {"v": -796, "r": 4.03}], ["Sound/AddPoint", 4650, {"v": -796, "r": 4.1}], ["Sound/AddPoint", 4710, {"v": -796, "r": 4.17}], ["Sound/AddPoint", 4740, {"v": -796, "r": 4.24}], ["Sound/AddPoint", 4800, {"v": -796, "r": 4.31}], ["Sound/AddPoint", 4830, {"v": -796, "r": 4.38}], ["Sound/AddPoint", 4860, {"v": -796, "r": 4.45}], ["Sound/AddPoint", 4920, {"v": -796, "r": 4.52}], ["Sound/AddPoint", 4950, {"v": -796, "r": 4.59}], ["Sound/AddPoint", 4980, {"v": -796, "r": 4.66}], ["Sound/AddPoint", 5040, {"v": -796, "r": 4.73}], ["Sound/AddPoint", 5070, {"v": -796, "r": 4.8}], ["Sound/AddPoint", 5100, {"v": -796, "r": 4.87}], ["Sound/AddPoint", 5130, {"v": -796, "r": 4.94}], ["Sound/AddPoint", 5160, {"v": -796, "r": 5.01}], ["Sound/AddPoint", 5220, {"v": -796, "r": 5.08}], ["Sound/AddPoint", 5250, {"v": -796, "r": 5.15}], ["Sound/AddPoint", 5280, {"v": -796, "r": 5.22}], ["Sound/AddPoint", 5310, {"v": -796, "r": 5.29}], ["Sound/AddPoint", 5340, {"v": -796, "r": 5.36}], ["Sound/AddPoint", 5370, {"v": -796, "r": 5.43}], ["Sound/AddPoint", 5400, {"v": -796, "r": 5.5}], ["Sound/AddPoint", 5430, {"v": -796, "r": 5.57}], ["Sound/AddPoint", 5490, {"v": -796, "r": 5.64}], ["Sound/AddPoint", 5520, {"v": -796, "r": 5.71}], ["Sound/AddPoint", 5550, {"v": -796, "r": 5.78}], ["Sound/AddPoint", 5580, {"v": -796, "r": 5.85}], ["Sound/AddPoint", 5610, {"v": -796, "r": 5.92}], ["Sound/AddPoint", 5640, {"v": -796, "r": 5.99}], ["Sound/AddPoint", 5670, {"v": -796, "r": 6.06}]]},
      { name: "Typewriter", when: "A story page types itself", steps: [["Sound/Type_01", 0, {"v": -699}], ["Sound/Type_01", 60, {"v": -699}], ["Sound/Type_02", 120, {"v": -699}], ["Sound/type_space_01", 150, {"v": -699}], ["Sound/Type_03", 330, {"v": -699}], ["Sound/Type_02", 390, {"v": -699}], ["Sound/Type_02", 450, {"v": -699}], ["Sound/Type_01", 540, {"v": -699}], ["Sound/Type_01", 600, {"v": -699}], ["Sound/type_space_01", 630, {"v": -699}], ["Sound/Type_02", 780, {"v": -699}], ["Sound/Type_03", 870, {"v": -699}], ["Sound/type_space_01", 900, {"v": -699}], ["Sound/Type_01", 1050, {"v": -699}], ["Sound/Type_02", 1140, {"v": -699}], ["Sound/Type_01", 1200, {"v": -699}], ["Sound/Type_01", 1260, {"v": -699}], ["Sound/Type_01", 1320, {"v": -699}], ["Sound/Type_02", 1410, {"v": -699}], ["Sound/type_space_01", 1440, {"v": -699}], ["Sound/Type_01", 1590, {"v": -699}], ["Sound/Type_02", 1680, {"v": -699}], ["Sound/Type_02", 1740, {"v": -699}], ["Sound/Type_03", 1830, {"v": -699}], ["Sound/Type_02", 1920, {"v": -699}], ["Sound/type_space_01", 1950, {"v": -699}], ["Sound/Type_01", 2070, {"v": -699}], ["Sound/Type_03", 2160, {"v": -699}], ["Sound/Type_01", 2220, {"v": -699}], ["Sound/typewriter_carriagereturn_01", 2250, {"v": -605}]]},
      { name: "Blocker Fallout", when: "A word takes a blocker off the shelf", steps: [["Sound/Word_04", 0], ["Sound/Blocker_Fallout", 810, {"v": -378}]]}
    ],
    groups: [
    { name: "Buttons and menus", sounds: [
      ["Sound/Button_Press.ogg", "Button press", "A button without a sound of its own is pressed. The same recording as Tile placed 2."],
      ["Sound/Button_Release.ogg", "Button release", "A button without a sound of its own is released. The same recording as Tile placed 3."],
      ["Sound/Button2_Press.ogg", "Round button press", "A button is pressed on the options menu, the player list and the new-player name box"],
      ["Sound/Button2_Release.ogg", "Round button release", "A button is released on the options menu, the player list and the new-player name box"],
      ["Sound/PaperButton_Press.ogg", "Paper button press", "A button is pressed on the dialogs, the help arrows, the high scores, the story and the interstitial screens"],
      ["Sound/PaperButton_Release.ogg", "Paper button release", "A button is released on the dialogs, the help arrows, the high scores, the story and the interstitial screens"],
      ["Sound/Checkbox_Press.ogg", "Check box press", "A check box is pressed, or the sound volume slider is let go (full volume)"],
      ["Sound/Logo.ogg", "Logo", "The logo screen's first update, 20 ms later (it plays once as the logo screen starts, before the title)"],
      ["Sound/Bell.ogg", "Shop bell", "The bell area of the main menu's picture is clicked, every click (full volume)"],
      ["Sound/CashRegister.ogg", "Cash register", "The cash register area of the main menu's picture is clicked, every click (full volume)"],
      ["Sound/type_01.ogg", "Typewriter key 1", "The main menu's typewriter is clicked (85% volume), or a letter of the story is typed (45%): a 1 in 3 draw among the three keys"],
      ["Sound/type_02.ogg", "Typewriter key 2", "The main menu's typewriter is clicked (85% volume), or a letter of the story is typed (45%): a 1 in 3 draw among the three keys"],
      ["Sound/type_03.ogg", "Typewriter key 3", "The main menu's typewriter is clicked (85% volume), or a letter of the story is typed (45%): a 1 in 3 draw among the three keys"]
    ] },
    { name: "Building words", sounds: [
      ["Sound/Place_01.ogg", "Tile placed 1", "A tile is clicked or dragged onto the word's path (a different tile from the last one): a 1 in 3 draw among the three place sounds"],
      ["Sound/Place_02.ogg", "Tile placed 2", "A tile is clicked or dragged onto the word's path (a different tile from the last one): a 1 in 3 draw among the three place sounds. The same recording as Button press."],
      ["Sound/Place_03.ogg", "Tile placed 3", "A tile is clicked or dragged onto the word's path (a different tile from the last one): a 1 in 3 draw among the three place sounds. The same recording as Button release."],
      ["Sound/Cancel.ogg", "Path dropped", "A right click in the game while the word's path holds at least one tile (not during the tutorial's first steps or a pause); the path is dropped at 75% volume"],
      ["Sound/Error.ogg", "Error", "A wrong tile is picked during the tutorial (at most once per 700 ms); a fixed or star tile is clicked once both hints have been shown, or while the tutorial's hint shows; a path that is not a word is submitted (85% volume); Alt+Enter is refused because the desktop is too small (full volume)"],
      ["Sound/Scramble.ogg", "Shuffle", "The shuffle button is pressed and the board still has a move to shuffle into (otherwise the game ends instead); plays at 75% volume"]
    ] },
    { name: "Scoring words", sounds: [
      ["Sound/Word_03.ogg", "Word of 3", "A word is submitted and scores: 3 letters or fewer (the sound is chosen by the word's letter count)"],
      ["Sound/Word_04.ogg", "Word of 4", "A word of exactly 4 letters is submitted and scores"],
      ["Sound/Word_05.ogg", "Word of 5", "A word of exactly 5 letters is submitted and scores"],
      ["Sound/Word_06.ogg", "Word of 6 or more", "A word of 6 or more letters is submitted and scores"],
      ["Sound/Blocker_Fallout.ogg", "Blocker falls out", "At a refill, each blocker tile with no cell below it that has bounced fewer than 3 times drops out of the board, at 65% volume"]
    ] },
    { name: "Level end", sounds: [
      ["Sound/Level_Up_Quick.ogg", "Level won", "A scored word leaves no more cells to fill on the board (the level's fill count is met), so the level is won; it plays together with that word's sound"],
      ["Sound/Level_Up.ogg", "Level summary", "The level's summary page starts after a win (80% volume, 10 ms after the page begins)"],
      ["Sound/AddPoint.ogg", "Score counts up", "Each step of the level's bonus counted into the score, every 34 to 100 ms; the pitch starts at 2x and rises 0.07 a step for 80 steps (40% volume)"],
      ["Sound/GameOver.ogg", "Game over", "The game ends (time is up, the goal is lost or no moves are left); starts 150 ms after the game-over state is set"]
    ] },
    { name: "Story typewriter", sounds: [
      ["Sound/type_space_01.ogg", "Typewriter space", "A space of the story is typed on the intro screen (45% volume; the next key waits 1.8 times the usual delay)"],
      ["Sound/typewriter_carriagereturn_01.ogg", "Carriage return", "Each line of the story is finished on the intro screen, before its letters are typed (50% volume)"],
      ["Sound/Screen_Transition_01.ogg", "Transition 1", "The story is all typed and the screen leaves; chosen by the level counter (from 0) modulo 4: this one for 0, 4, 8 and so on (the screen then fades over 0.4 s)"],
      ["Sound/Screen_Transition_02.ogg", "Transition 2", "The story is all typed and the screen leaves; chosen by the level counter (from 0) modulo 4: this one for 1, 5, 9 and so on"],
      ["Sound/Screen_Transition_03.ogg", "Transition 3", "The story is all typed and the screen leaves; chosen by the level counter (from 0) modulo 4: this one for 2, 6, 10 and so on"],
      ["Sound/Screen_Transition_04.ogg", "Transition 4", "The story is all typed and the screen leaves; chosen by the level counter (from 0) modulo 4: this one for 3, 7, 11 and so on"]
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
    { name: "Menus", sounds: [
      ["sounds/buttonclick.ogg", "Click", "Any button is pressed: title, game selector, board, dialogs, options, help, journal, fame, level summary; also Lex is clicked"],
      ["sounds/goldtile.ogg", "Volume test", "The Options' sound effects slider is let go (silent while it is dragged), at the new volume"]
    ] },
    { name: "Lex", sounds: [
      ["sounds/lex_welcome.ogg", "\"Welcome to BookWorm!\"", "The game selector has been open for 100 updates (1 second); only the first time in a run, never again until the game is restarted"],
      ["sounds/lex_go.ogg", "\"Go!\"", "A Classic game starts and Lex is idle: a 50% draw between this and \"Let's go!\""],
      ["sounds/lex_letsgo.ogg", "\"Let's go!\"", "A Classic game starts and Lex is idle: the other 50% of the draw, against \"Go!\""],
      ["sounds/lex_getready.ogg", "\"Get ready!\"", "An Action game starts and Lex is idle (every Action game, never a draw)"],
      ["sounds/lex_scramble.ogg", "\"Scramble!\"", "Lex is clicked and the scramble goes ahead (it needs a normal top tile and 9 or fewer fire tiles); plays with the fire-tile sound"],
      ["sounds/lex_careful.ogg", "\"Careful!\"", "Lex starts to sweat (a fire tile on the bottom row) and is not moving: one of four at random, a 1 in 4 chance each"],
      ["sounds/lex_danger.ogg", "\"Danger!\"", "Lex starts to sweat (a fire tile on the bottom row) and is not moving: one of four at random, a 1 in 4 chance each"],
      ["sounds/lex_lookout.ogg", "\"Look out!\"", "Lex starts to sweat (a fire tile on the bottom row) and is not moving: one of four at random, a 1 in 4 chance each"],
      ["sounds/lex_watchout.ogg", "\"Watch out!\"", "Lex starts to sweat (a fire tile on the bottom row) and is not moving: one of four at random, a 1 in 4 chance each"],
      ["sounds/lex_levelup.ogg", "\"Level up!\"", "The game score reaches the level's threshold; once Lex is idle, no tiles are flying and no dialog is open, before the summary"],
      ["sounds/bonusword.ogg", "\"Bonus!\"", "A word equal to the current bonus word is submitted; the points depend on how many bonus words came before"],
      ["sounds/lex_burp.ogg", "Burp", "A word with two or more fire tiles is scored; 50 updates (0.5 s) after the last fire tile reaches Lex's mouth, if he is idle and the board has not burned down"]
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
      ["sounds/nextselect.ogg", "Letter picked", "A tile is clicked or typed onto the word and the word is not a valid word holding a special tile; also when a hint's or typed word's path is selected"],
      ["sounds/bonuscomplete.ogg", "Special word", "A tile is added that turns the word into a valid word holding a gold, sapphire or diamond tile (it was not valid before)"],
      ["sounds/buzzer.ogg", "Buzzer", "Submit on a non-word; a scramble refused (over 9 fire tiles, no normal top tile, or Classic with fire on the bottom row); Undo with nothing to undo; a hint or typed letter finds no word"],
      ["sounds/wordcomplete.ogg", "Word scored", "A word with no fire tile is scored; also an Action level's time bonus (the next level starts with the countdown under 10 while Lex sweats)"],
      ["sounds/redtilecleared.ogg", "Fire word scored", "A word containing at least one fire tile is scored (it replaces the usual word-scored sound)"],
      ["sounds/chomp.ogg", "Chomp", "A scored word's tile reaches Lex's mouth, at most once every 4 updates (0.04 s), so a long word chomps several times"],
      ["sounds/bad.ogg", "Bad (unused)", "Never played: it is loaded, but no code asks for it (a wrong word plays the buzzer). Also in Alchemy as Bad move: a click on the grid with a tile held when the tile cannot go there, an off-grid click too. Also in Big Money as Bad (trial only): played when an invalid registration code is typed in. Also in Peggle Deluxe, Peggle Extreme, Peggle Nights and Peggle World of Warcraft Edition, as sounds/pegpop.ogg."]
    ] },
    { name: "Word notes", sounds: [
      ["sounds/up1.ogg", "1st letter", "After a word is scored, 20 updates (0.2 s) later; one note per letter follows every 10 updates (0.1 s), rising up to the 8th"],
      ["sounds/up2.ogg", "2nd letter", "0.1 s after the 1st note; every scored word has at least 3 letters, so this always plays"],
      ["sounds/up3.ogg", "3rd letter", "0.1 s after the 2nd note; every scored word has at least 3 letters, so this always plays"],
      ["sounds/up4.ogg", "4th letter", "0.1 s after the 3rd note, only in a word of 4 or more letters"],
      ["sounds/up5.ogg", "5th letter", "0.1 s after the 4th note, only in a word of 5 or more letters"],
      ["sounds/up6.ogg", "6th letter", "0.1 s after the 5th note, only in a word of 6 or more letters"],
      ["sounds/up7.ogg", "7th letter", "0.1 s after the 6th note, only in a word of 7 or more letters"],
      ["sounds/up8.ogg", "8th letter on", "0.1 s after the 7th note, in a word of 8 or more letters; it repeats for the 9th letter and every one after it"],
      ["sounds/up9.ogg", "9th note (unused)", "Never played: the note played is the 1st to 7th letter's, then the 8th for every later one, so the 9th is never asked for"]
    ] },
    { name: "Tiles", sounds: [
      ["sounds/piece_hit.ogg", "Tile lands", "A falling tile reaches its place at a speed over 4, at most once every 4 updates (0.04 s)"],
      ["sounds/appear2.ogg", "Green tile", "Each new normal tile that falls in after a word has a 1 in 20 chance of turning green; also when an undo puts the board back"],
      ["sounds/spinsuperbonus.ogg", "Special tile", "A scored word earns a special tile on a random normal tile: 4 letters 1 in 100 (gold), 5 letters 1 in 5, 6 letters always, longer words better kinds"],
      ["sounds/flametile.ogg", "Fire tile", "The first fire tile made after each word or scramble (refill, Action's burning tile, or a scramble); the rest of that word's fire tiles are silent"],
      ["sounds/tileburn.ogg", "Tile burns", "A burning fire tile's delay ends and it starts burning into the tile below; at most once per update however many tiles burn"]
    ] },
    { name: "Danger and game over", sounds: [
      ["sounds/warning.ogg", "Warning", "Classic: three beeps 64 updates (0.64 s) apart when a fire tile reaches the bottom row; one beep with the first fire-tile hint; Action: every 100 updates (1 s) while Lex sweats"],
      ["sounds/screenburngameover.ogg", "\"The end!\"", "A fire tile on the bottom row ends the game (Classic: not in the selection; Action: countdown below 0); when the fire line's delay runs out, once Lex is idle"],
      ["sounds/gameovertone.ogg", "Game over tone (unused)", "Never played: it is loaded, but no code asks for it (game over plays the screen-burn clip). Also in Big Money as Game over tone: the game ends."]
    ] }
  ] },
  bookwormadv: {
    presets: [
      { name: "Word Attack", when: "Lex's word hits: the bites, the blow and the foe's cry", steps: [["sounds/bite_01", 0, {"v": -146}], ["sounds/bite_01", 140, {"v": -146}], ["sounds/bite_01", 270, {"v": -146}], ["sounds/bite_01", 400, {"v": -146}], ["sounds/bite_01", 500, {"v": -146}], ["sounds/largeimpact_01", 600, {"v": -146}], ["sounds/moan_01", 640, {"v": -146}], ["sounds/humanmalegrunt2", 640, {"v": -146}], ["sounds/SwordSpearImpact_WithWoosh", 640, {"v": -146}]]},
      { name: "Level Up", when: "A fire word finishes a foe, and Lex levels up", steps: [["sounds/largeimpact_01", 0, {"v": -146}], ["sounds/GiantGruntDry1", 0, {"v": -146}], ["sounds/GiantGruntEcho2", 0, {"v": -146}], ["sounds/fire6", 0, {"v": -146}], ["sounds/XPGain4", 3340, {"v": -146, "loop": 1, "end": 4830, "ramp": [[4330, -165], [4490, -509], [4620, -913], [4720, -1392], [4760, -1673], [4790, -1950], [4800, -10000]]}], ["sounds/crowdcheer", 4330], ["sounds/firework1", 5920, {"v": -146}], ["sounds/firework2", 6510, {"v": -146}], ["sounds/lex_levelup", 6560, {"v": -146}]]},
      { name: "Battle Won", when: "The last foe falls and a treasure is won", steps: [["sounds/largeimpact_01", 0, {"v": -146}], ["sounds/GiantGruntDry1", 0, {"v": -146}], ["sounds/GiantGruntEcho2", 0, {"v": -146}], ["sounds/fire6", 0, {"v": -146}], ["sounds/purify2", 4080, {"v": -146}], ["sounds/XPGain4", 5380, {"v": -146, "loop": 1, "end": 7790, "ramp": [[7290, -165], [7450, -509], [7580, -913], [7680, -1392], [7720, -1673], [7750, -1950], [7760, -10000]]}], ["sounds/newtreasure", 8280, {"v": -146}], ["sounds/firework2", 8780, {"v": -146}], ["sounds/crowdcheer", 8780]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/click1.ogg", "Click 1", "A button is pressed in a dialog, the options screen, the stats book or the treasure screen"],
      ["sounds/click2.ogg", "Click 2", "A main menu, Hall of Fame, mini-game menu or options button is pressed, or a button on the board"],
      ["sounds/lex_chomp.ogg", "Lex chomps", "Lex chomps on the title screen while the game loads, and once more as he walks off"],
      ["sounds/textreadblip.wav", "Text blip", "Every other letter is typed out while a conversation line is shown"]
    ] },
    { name: "Main menu voices", sounds: [
      ["sounds/v_adventure.ogg", "\"Adventure\"", "Lex reacts when the pointer moves onto the Adventure item of the main menu"],
      ["sounds/v_arena.ogg", "\"Arena\"", "Lex reacts when the pointer moves onto the Arena item of the main menu"],
      ["sounds/v_clipsandgiggles.ogg", "\"Clips and Giggles\"", "Lex reacts when the pointer moves onto the film reel item of the main menu. Also in Bookworm Adventures Volume 2 as \"Clips and giggles\" (unused)."],
      ["sounds/v_dontleaveme.ogg", "\"Don't leave me\"", "The Quit item is pressed on the main menu, as the really-quit confirmation opens"],
      ["sounds/v_halloffame.ogg", "\"Hall of Fame\"", "Lex reacts when the pointer moves onto the Hall of Fame item of the main menu"],
      ["sounds/v_minigames.ogg", "\"Mini-games\"", "Lex reacts when the pointer moves onto the Mini-games item of the main menu"],
      ["sounds/v_options.ogg", "\"Options\"", "Lex reacts when the pointer moves onto the Options item of the main menu"],
      ["sounds/v_quit.ogg", "\"Quit\"", "Lex reacts when the pointer moves onto the Quit item of the main menu"],
      ["sounds/v_tomeofknowledge.ogg", "\"Tome of Knowledge\"", "Lex reacts when the pointer moves onto the Tome of Knowledge item of the main menu"],
      ["sounds/v_welcometobookwormadventures.ogg", "\"Welcome to Bookworm Adventures\"", "Lex greets the player when the title screen gives way to the main menu"]
    ] },
    { name: "Word play", sounds: [
      ["sounds/buzzer.ogg", "Word refused", "A word is refused in timed mode, submit is pressed with no valid word, or a potion is clicked while it cannot be used"],
      ["sounds/gemspawn.ogg", "Gem appears", "A gem appears on a tile (the tutorial, the letter mini-games and some creatures do this)"],
      ["sounds/sheepbleat.ogg", "Sheep bleat", "Each time the selected tiles spell TYSEN in a fight, in place of the praise voice; also a sheep, ewe, ram or goat creature bleats"],
      ["sounds/sloooowdown.ogg", "Slow down", "The hidden word that sets off the AstroPop flash is spelled in a fight (not in a mini-game started from the menu) while no flash is showing. Also in Bookworm Adventures Volume 2 as Sloooowdown (unused)."],
      ["sounds/tile_select.ogg", "Tile select", "A letter tile is picked on the board, or a potion is picked by its number key"],
      ["sounds/v_cashmoney.ogg", "\"Cash money\"", "Each time the selected tiles spell JEFF or STEVE in a fight, in place of the praise voice"],
      ["sounds/v_iloveyou.ogg", "\"I love you\"", "Each time the selected tiles spell JULIE or JASON in a fight, in place of the praise voice"],
      ["sounds/v_scramble.ogg", "\"Scramble\"", "Lex's scramble starts in a fight, from the Scramble button on a full board or from Yes on the scramble hint; never in a mini-game"],
      ["sounds/word_power.ogg", "Word power", "Each time the selected tiles come to spell a valid word the foe is not immune to, in a fight; the pitch rises with the word's length plus its tiles' bonuses, capped at 14"]
    ] },
    { name: "Lex's praise", sounds: [
      ["sounds/lex_good.ogg", "\"Good!\"", "A spelled word counting 6 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed"],
      ["sounds/lex_wow.ogg", "\"Wow!\"", "A spelled word counting 7 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; also Book 2's Mirage Lex blocking a word in untimed play"],
      ["sounds/lex_nice.ogg", "\"Nice!\"", "A spelled word counting 8 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Word Master: the last prize line solved; Book 2's Mirage Lex's cry when beaten"],
      ["sounds/lex_verygood.ogg", "\"Very good!\"", "A spelled word counting 9 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Word Master: the fourth prize line solved"],
      ["sounds/lex_awesome.ogg", "\"Awesome!\"", "A spelled word counting 10 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed"],
      ["sounds/lex_excellent.ogg", "\"Excellent!\"", "A spelled word counting 11 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Word Master: the third prize line solved"],
      ["sounds/lex_fantastic.ogg", "\"Fantastic!\"", "A spelled word counting 12 or more (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed"],
      ["sounds/lex_astonishing.ogg", "\"Astonishing!\"", "Never for a word in battle (the count stops at 12); Word Master: the top prize line solved"]
    ] },
    { name: "Lex", sounds: [
      ["sounds/lex_burp.ogg", "Lex burps", "Each time the selected tiles spell SUKHBIR in a fight, in place of the praise voice; also Lex's burp animation, on the eighth click in a row on him while he idles on the player's turn"],
      ["sounds/v_giggle1.ogg", "Giggle 1", "Lex's click giggle animation, on the first three and the fifth and sixth clicks in a row on him while he idles on the player's turn in a fight; each giggle picks this voice or Giggle 2 with equal chance"],
      ["sounds/v_giggle2.ogg", "Giggle 2", "Lex's click giggle animation, on the first three and the fifth and sixth clicks in a row on him while he idles on the player's turn in a fight; each giggle picks this voice or Giggle 1 with equal chance; the main menu Lex plays this one in two of his animations"],
      ["sounds/v_sneeze.ogg", "Sneeze", "Lex's click sneeze animation, on the fourth and seventh click in a row on him while he idles on the player's turn in a fight"]
    ] },
    { name: "Lex attacks", sounds: [
      ["sounds/bite_01.ogg", "Bite", "Lex bites in an attack; the Bayou Beast, Bog Serpent, Connystein, Wolfman and Codex fights use it too"],
      ["sounds/bluntimpact_dry.wav", "Blunt impact", "Lex's word attack lands (his default attack and many creatures' fights use it)"],
      ["sounds/bluntimpact_withwoosh.ogg", "Blunt with woosh", "Lex's word attack lands (his default attack and many creatures' fights use it)"],
      ["sounds/fire3.ogg", "Fire attack 3", "Lex's word attack lands (his default attack and many creatures' fights use it)"],
      ["sounds/fire6.ogg", "Fire attack 6", "Lex's word attack lands (his default attack and many creatures' fights use it)"],
      ["sounds/swordspearimpact_withwoosh.ogg", "Sword with woosh", "Lex's word attack lands (his default attack and many creatures' fights use it)"]
    ] },
    { name: "Battle", sounds: [
      ["sounds/low_health.wav", "Low health", "Lex's health is at a quarter of his maximum or less in a fight: one beep per pulse of his health bar, quieter from 500 updates on; in Letter Rip and Link 'n Spell, every 50 updates (0.5 s) of their last 1000 (10 s). Also in Zuma's Revenge, as sounds/challenge_countdown.ogg."],
      ["sounds/v_bossbattle.ogg", "\"Boss battle\"", "When a boss fight's intro flash peaks, together with Firework 2; it follows the boss's own opening sound when the boss has one"],
      ["sounds/warpin.ogg", "Warp in", "Lex's warp-in animation on the map: back from the treasure screen or from a death conversation, as some book 1 conversations end, and with a newly unlocked mini-game; also the Mirage Goat's hit sound"],
      ["sounds/warpout.ogg", "Warp out", "Lex warps out of the map as a chapter or mini-game starts; the main menu Lex plays it in several of his animations"]
    ] },
    { name: "Rewards", sounds: [
      ["sounds/chaching.ogg", "Cash register", "Coins are counted up in a mini-game; the Mirage Trojan fight uses it too"],
      ["sounds/crowdcheer.ogg", "Crowd cheers", "The level-up effect starts its crowd; the checkpoint banner fades in; the altar's white flash peaks (with Firework 2); the book manager's end banner fades in; a mini-game's results screen opens"],
      ["sounds/firework1.ogg", "Firework 1", "The level-up effect's XP timer runs out; the checkpoint banner fades in (with the crowd). Also in Bejeweled 3, as sounds/firework_thump.ogg."],
      ["sounds/firework2.ogg", "Firework 2", "The level-up effect's white flash falls to 20 or less, just before Lex's level-up voice; a boss intro flash peaks; the altar's white flash peaks (with the crowd); the main menu Lex plays it in three animations"],
      ["sounds/lex_levelup.ogg", "\"Level up\"", "When the level-up effect's white flash fades out, after a fight that levels Lex up"],
      ["sounds/newtreasure.ogg", "\"New treasure!\"", "A treasure is won at the end of a fight, or shown on the treasure screen"],
      ["sounds/vo_annihilated.ogg", "\"Annihilated\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_crushed.ogg", "\"Crushed\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_decimated.ogg", "\"Decimated\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_destroyed.ogg", "\"Destroyed\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_obliterated.ogg", "\"Obliterated\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_vanquished.ogg", "\"Vanquished\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_whomped.ogg", "\"Whomped\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/xpgain4.ogg", "Experience counts", "The experience stream flows into Lex at a level-up, a fight end or a gravestone"]
    ] },
    { name: "Potions", sounds: [
      ["sounds/heal2.ogg", "Health potion", "Lex uses a health potion while his health is below its maximum"],
      ["sounds/healpotionused_02.ogg", "Creature heals", "Attack sound of a creature's heal-itself attack (mostly Regenerate): offered only at or below a set share of its health, then by a random roll against a set chance (Circe: 0.75 and 30 percent)"],
      ["sounds/potionpickup.ogg", "Potion pickup", "A potion dropped by a gravestone starts flying to Lex's potion row, once it is at x 165 or less and the gravestone's animation is done"],
      ["sounds/powerdown_01.ogg", "Power down", "A creature weakens the attack of Lex, or a boosted attack ends"],
      ["sounds/powerup3.ogg", "Attack potion", "Lex uses an attack potion; also the attack sound of the power-up attacks of the Codex, Dracula and a few other book 2 and 3 creatures"],
      ["sounds/poweruppotion_02.ogg", "Power up", "Attack sound of a creature's power-up attack, which boosts its next attack (Circe, Cerberus, Nemean Lion and others)"],
      ["sounds/purify2.ogg", "Purify", "Lex uses a purify potion; also Word Master plays it each time a puzzle line is solved, a few creatures (Mirage Nymph, Dracula, Fallen Warrior Hero) play it as they cure an ailment, and the main menu Lex plays it once"],
      ["sounds/weaken3.ogg", "Weaken effect", "A curse that cuts the target's damage (a damage multiplier under 1 on its attacks) is applied; not when a saved fight is restored"]
    ] },
    { name: "Tile effects", sounds: [
      ["sounds/boardfrozen_01.ogg", "Board frozen", "Attack sound of the freezing attacks of five creatures (Hydra, Naiad, Elementalist, Mirage Nymph, Mirage Skeleton)"],
      ["sounds/boardstunned_01.ogg", "Board stunned", "Attack sound of the stun-and-damage attacks of nine creatures (Caledonian Boar, Centaur Warrior, Cernian Hind, Dryad, Satyr, Sea Serpent, Hydra and others)"],
      ["sounds/fire1.ogg", "Fire effect", "The burning ailment is applied to Lex or a foe, and again each time it burns at a turn's end; also the attack sound of the burn attacks of many creatures"],
      ["sounds/ice1.ogg", "Frozen effect", "The frozen status is applied to Lex or a foe; also an attack sound of the Unliving Frostmancer"],
      ["sounds/ice4.ogg", "Petrify effect", "The petrified status is applied to Lex or a foe; also an attack sound of the Elementalist, Disturbed Skeleton, Fallen Wizard Hero and Unliving Frostmancer. Also in Zuma's Revenge, as sounds/freeze.ogg."],
      ["sounds/lightning_attack_01.ogg", "Lightning", "A lightning attack lands (Circe, Hydra, Sea Elemental and wizards)"],
      ["sounds/lockit.ogg", "Tile locked", "Attack sound of the tile-locking attacks of about a dozen creatures (Locks tiles and damages Lex)"],
      ["sounds/petrify_attack_01.ogg", "Tiles petrified", "A creature turns tiles to stone (Basilisk, Medusa and others)"],
      ["sounds/poison1.ogg", "Poison effect", "The poison ailment is applied to Lex or a foe, and again each time it hurts at a turn's end; also an attack sound of the Embalmed Guardian, Swashbuckler, Dracula, Grave Robber and Unliving Archer"],
      ["sounds/poison_attack_01.ogg", "Poison attack", "Attack sound of the poisoning attacks of many creatures (Poisons and damages Lex)"],
      ["sounds/tiles_plagued_01.ogg", "Tiles plagued", "Attack sound of a tile-plague attack (Plagues tiles and damages Lex), as of Deino and Nessus"],
      ["sounds/tiles_smashed_01.ogg", "Tiles smashed", "Attack sound of the tile-smashing attacks of many creatures (Smashes tiles and damages Lex)"]
    ] },
    { name: "Mini-games", sounds: [
      ["sounds/lex_incredible.ogg", "\"Incredible!\"", "Word Master only: the second prize line solved"],
      ["sounds/lex_watchout.ogg", "\"Watch out!\"", "Word Master: a miss brings the prize down to its last line; also Book 2's Mirage Lex's cry as the fight starts, pitched down"],
      ["sounds/count.wav", "Counting tick", "The final score counts up in Letter Rip, Link 'n Spell and Word Master, a tick whenever the count's sound timer reads 0; also the Mirage Skeleton's hit sound"],
      ["sounds/v_go.ogg", "\"Go\"", "Letter Rip and Link 'n Spell: the start countdown reaches 0, 300 updates (3 s) after Three"],
      ["sounds/v_newminigame.ogg", "\"New mini-game\"", "50 updates (0.5 s) after Lex's warp-in sound, as a conversation announces a newly unlocked mini-game"],
      ["sounds/v_one.ogg", "\"One\"", "Letter Rip and Link 'n Spell: the start countdown reaches 1, 200 updates (2 s) after Three"],
      ["sounds/v_three.ogg", "\"Three\"", "Letter Rip and Link 'n Spell: the start countdown begins, one update after its timer starts; also when the tutorial's arrow hints end"],
      ["sounds/v_two.ogg", "\"Two\"", "Letter Rip and Link 'n Spell: the start countdown reaches 2, 100 updates (1 s) after Three"]
    ] },
    { name: "Human voices", sounds: [
      ["sounds/female seduce.ogg", "Female seduce", "Death sound of Mirage Trojan"],
      ["sounds/female_groan_01.ogg", "Female groan 1", "Opening sound of Hateful Housemaid; Death sound of Hateful Housemaid, Connystein, Quag Hag; Attack or hit of Scylla, Siren, Circe and 9 more (Arena too)"],
      ["sounds/female_groan_02.ogg", "Female groan 2", "Death sound of Scylla, Sea Witch, Siren and 9 more; Attack or hit of Mysterious Assassin (Arena too)"],
      ["sounds/female_mmm.ogg", "Female mmm", "Opening sound of Siren, Circe, Dryad; Death sound of Mirage Skeleton; Attack or hit of Vampire Mistress (Arena too)"],
      ["sounds/female_ooh.ogg", "Female ooh", "Opening sound of Helead, Naiad, Vampire Mistress; Attack or hit of Siren"],
      ["sounds/humanmalegrunt2.ogg", "Human male grunt 2", "Death sound of Alexander, Polydamas, Trojan Captain and 11 more; Attack or hit of Snake Charmer, Thieves 2-3, Frankenstein (Arena too)"],
      ["sounds/humanmalegrunt3.ogg", "Human male grunt 3", "Death sound of Snake Charmer, Thieves 2-3, Summoner and 1 more; Attack or hit of Cutthroat, Thief 1, Deck Swabber and 1 more (Arena too)"],
      ["sounds/male_anger.ogg", "Male anger", "Opening sound of Polydamas, Trojan Captain, Bandit and 6 more; Attack or hit of Cannoneer, Angry Mob, Eternal Wanderer and 2 more (Arena too)"],
      ["sounds/male_laugh.ogg", "Male laugh", "Opening sound of Alexander, Centaur Warrior, Satyr and 1 more; Attack or hit of Pharaoh of Old, Dread Pirate Al-Robarts (Arena too)"],
      ["sounds/male_laugh2.ogg", "Male laugh 2", "Opening sound of Trojan Warrior, Centaur Archer, Sword Swallower and 3 more; Attack or hit of Pharaoh of Old, Crazy Murray (Arena too)"],
      ["sounds/male_low_anger.ogg", "Male low anger", "Opening sound of Thieves 4-5; Death sound of Nessus, Embalmed Guardian, Cannoneer and 2 more; Attack or hit of Firebreather, Pharaoh of Old, Twisted Vizier (Arena too)"],
      ["sounds/male_low_pain.ogg", "Male low pain", "Death sound of Firebreather; Attack or hit of Nessus, Thieves 4-5, Shaitan and 1 more (Arena too)"],
      ["sounds/male_low_warcry.ogg", "Male low warcry", "Opening sound of Nessus, Firebreather, Cannoneer and 5 more; Death sound of Thieves 4-5; Attack or hit of Shaitan (Arena too)"],
      ["sounds/male_pain.ogg", "Male pain", "Death sound of Swashbuckler, Warrenstein, Fallen Swashbuckler Hero; Attack or hit of Bandit, Sword Swallower, Thieves 9-11 and 3 more (Arena too)"],
      ["sounds/moan_01.ogg", "Moan 1", "Death sound of Bandit, Sword Swallower, Highwayman; Attack or hit of Alexander, Polydamas, Trojan Captain and 8 more (Arena too). Also in Bookworm Adventures Volume 2 as Moan 1 (unused)."],
      ["sounds/witch_groan_01.ogg", "Witch groan 1", "Opening sound of Deino, Enyo, Pemphredo; Death sound of Haggard Heckler; Attack or hit of Wandering Hag, Connystein, Quag Hag"],
      ["sounds/witchy_laugh_01.ogg", "Witchy laugh 1", "Opening sound of Haggard Heckler, Quag Hag; Attack or hit of Deino, Enyo, Pemphredo and 1 more"],
      ["sounds/witchy_laugh_02.ogg", "Witchy laugh 2", "Opening sound of Scylla, Sea Witch, Harpy and 3 more; Death sound of Deino, Enyo, Pemphredo; Attack or hit of Mirage Skeleton, Haggard Heckler"]
    ] },
    { name: "Animals", sounds: [
      ["sounds/bat_death_01.ogg", "Bat death 1", "Attack or hit of Mama Roc (Arena too). Also in Bookworm Adventures Volume 2 as Bat death 1 (unused)."],
      ["sounds/dog_barking_01.ogg", "Dog barking 1", "Opening sound of Thieves 6-8, Wolf Pack; Attack or hit of Orthrus, Flying Wolf, Carrion Dog"],
      ["sounds/dog_barking_02.ogg", "Dog barking 2", "Opening sound of War Hound, Enchanted Hound, Flying Wolf; Attack or hit of Thieves 6-8, Undead Hound"],
      ["sounds/dog_whimper_01.ogg", "Dog whimper 1", "Death sound of War Hound, Enchanted Hound, Wolf Pack and 1 more; Attack or hit of Thieves 6-8"],
      ["sounds/eagle_cry_01.ogg", "Eagle cry 1", "Opening sound of Enchanted Eagle, Griffon, Bronze Stymphalian and 3 more; Death sound of Bronze Stymphalian, Steel Stymphalian, Roc Hatchling; Attack or hit of Mama Roc (Arena too). Also in Bookworm Adventures Volume 2 as Eagle cry 1 (unused)."],
      ["sounds/feline_whimper_01.ogg", "Feline whimper 1", "Opening sound of Carrion Dog; Death sound of Bilge Dog, Flying Wolf; Attack or hit of Enchanted Lion, Nemean Lion, Wolfman (Arena too)"],
      ["sounds/feline_whimper_02.ogg", "Feline whimper 2", "Opening sound of Orthrus, Undead Hound, Bilge Dog; Death sound of Nemean Lion; Attack or hit of Cerberus, Wolfman, Codex (Arena too)"],
      ["sounds/generic_slosh_02.ogg", "Generic slosh 2", "Opening sound of Sea Elemental, Air Spirit; Attack or hit of Helead, Deck Swabber, Hateful Housemaid and 2 more"],
      ["sounds/pig_squeal_01.ogg", "Pig squeal 1", "Opening sound of Calydonian Boar, Erymanthian Boar; Death sound of Plagued Pork"],
      ["sounds/pig_squeal_02.ogg", "Pig squeal 2", "Opening sound of Flying Pig, Plagued Pork, Were-Pig; Death sound of Calydonian Boar, Erymanthian Boar, Flying Pig; Attack or hit of Enchanted Scorpion"],
      ["sounds/snake_hiss_01.ogg", "Snake hiss 1", "Opening sound of Sea Serpent, Enchanted Scorpion, Enchanted Serpent and 6 more; Death sound of Flying Snake"],
      ["sounds/wolf_howl_01.ogg", "Wolf howl 1", "Death sound of Orthrus, Thieves 6-8, Undead Hound; Attack or hit of Flying Wolf, Wolf Pack"]
    ] },
    { name: "Beasts and giants", sounds: [
      ["sounds/dragon_bite.ogg", "Dragon bite", "Opening sound of Bayou Beast; Attack or hit of Hydra, Basilisk, Greater Basilisk and 1 more"],
      ["sounds/dragon_death.ogg", "Dragon death", "Death sound of Hydra, Basilisk, Greater Basilisk and 2 more; Attack or hit of The Mummy (Arena too)"],
      ["sounds/dragon_pain.ogg", "Dragon pain", "Attack or hit of Kraken, Basilisk, Greater Basilisk and 2 more (Arena too)"],
      ["sounds/dragon_roar.ogg", "Dragon roar", "Opening sound of Kraken, Hydra, Basilisk and 2 more; Attack or hit of Kraken, Bilge Dog. Also in Bookworm Adventures Volume 2 as Dragon roar (unused)."],
      ["sounds/elephant_01.ogg", "Elephant 1", "Opening sound of Elephant, Mirage Skeleton. Also in Bookworm Adventures Volume 2 as Elephant 1 (unused)."],
      ["sounds/elephant_death_01.ogg", "Elephant death 1", "Opening sound of Bull Elephant. Also in Bookworm Adventures Volume 2 as Elephant death 1 (unused)."],
      ["sounds/elephant_death_02.ogg", "Elephant death 2", "Death sound of Elephant. Also in Bookworm Adventures Volume 2 as Elephant death 2 (unused)."],
      ["sounds/elephant_death_03.ogg", "Elephant death 3", "Death sound of Bull Elephant. Also in Bookworm Adventures Volume 2 as Elephant death 3 (unused)."],
      ["sounds/generic_animal_grumble_01.ogg", "Generic animal grumble 1", "Opening sound of Cerberus, Minotaur, Nemean Lion; Death sound of Chimera, Minotaur (Arena too)"],
      ["sounds/giantgruntdry1.ogg", "Giant grunt dry 1", "Opening sound of Cyclops Herder, Cyclops Warrior; Attack or hit of Polyphemus, Manticore, Minotaur and 1 more (Arena too)"],
      ["sounds/giantgruntecho2.ogg", "Giant grunt echo 2", "Death sound of Cyclops Herder, Cyclops Warrior, Polyphemus and 1 more; Attack or hit of The Creature (Arena too)"],
      ["sounds/kubwa_barh.ogg", "Kubwa barh", "Attack or hit of Thieves 12-14 (Arena too). Also in Bookworm Adventures Volume 2 as Kubwa barh (unused)."],
      ["sounds/kubwa_greeting.ogg", "Kubwa greeting", "Used in the Arena fights. Also in Bookworm Adventures Volume 2 as Kubwa greeting (unused)."],
      ["sounds/kubwa_greeting2.ogg", "Kubwa greeting 2", "Opening sound of Thieves 12-14 (Arena too). Also in Bookworm Adventures Volume 2 as Kubwa greeting 2 (unused)."],
      ["sounds/kubwa_roar.ogg", "Kubwa roar", "Death sound of Thieves 12-14 (Arena too)"],
      ["sounds/roar_01.ogg", "Roar 1", "Opening sound of Charybdis, Enchanted Lion, Phantom and 2 more; Attack or hit of Griffon (Arena too). Also in Bookworm Adventures Volume 2 as Roar 1 (unused)."]
    ] },
    { name: "Spirits and magic", sounds: [
      ["sounds/fading_ghostly_wail_01.ogg", "Fading ghostly wail 1", "Opening sound of Banshee, Manes, Tombstone Gargoyle and 1 more; Death sound of Medusa, Sphinx, Flying Armor; Attack or hit of Phantom, Grim (Arena too)"],
      ["sounds/fading_ghostly_wail_02.ogg", "Fading ghostly wail 2", "Opening sound of Medusa, Flying Armor, Necromancer; Death sound of Charybdis, Banshee, Manes and 4 more; Attack or hit of Hypnotist (Arena too)"],
      ["sounds/fading_ghostly_wail_03.ogg", "Fading ghostly wail 3", "Opening sound of Shade, Specter, Embalmed Guardian and 1 more; Death sound of Mummified Servant, Tombstone Gargoyle, Gargoyle; Attack or hit of Manes, Grim (Arena too)"],
      ["sounds/pulsing_attack_01.ogg", "Pulsing attack 1", "Attack or hit of Circe, Dryad, Helead and 3 more (Arena too). Also in Bookworm Adventures Volume 2 as Pulsing attack 1 (unused)."],
      ["sounds/skeleton_warcry_01.ogg", "Skeleton warcry 1", "Opening sound of Limping Leper, Disturbed Skeleton, Ghoul Minion"],
      ["sounds/sonic_attack_02.ogg", "Sonic attack 2", "Opening sound of Hypnotist; Death sound of Mirage Nymph; Attack or hit of Shade, Specter, Snake Charmer and 5 more (Arena too)"],
      ["sounds/wail_03.ogg", "Wail 3", "Opening sound of Parkerstein; Death sound of Necromancer; Attack or hit of Phantom, Shade, Specter"],
      ["sounds/wail_05.ogg", "Wail 5", "Death sound of Cerberus; Attack or hit of Codex (Arena too)"]
    ] },
    { name: "Attacks and impacts", sounds: [
      ["sounds/arrow_shot_01.ogg", "Arrow shot 1", "Attack or hit of Alexander, Polydamas, Centaur Archer and 4 more (Arena too)"],
      ["sounds/bowfiring.ogg", "Bow firing", "Opening sound of Unliving Archer; Attack or hit of Fallen Huntress Hero. Also in Bookworm Adventures Volume 2 as Bow firing (unused)."],
      ["sounds/largeimpact_01.ogg", "Large impact 1", "Death sound of Cernian Hind, Enchanted Eagle, Enchanted Lion and 14 more; Attack or hit of Thieves 12-14, Calydonian Boar, Centaur Warrior and 25 more (Arena too)"],
      ["sounds/slicing_attack_01.ogg", "Slicing attack 1", "Opening sound of Mysterious Assassin, Fallen Warrior Hero, Vampire Cultist; Attack or hit of Trojan Captain, Trojan Warrior, Sea Witch and 29 more (Arena too)"],
      ["sounds/slicing_attack_02.ogg", "Slicing attack 2", "Opening sound of Unliving Brute; Attack or hit of War Hound, Scylla, Enchanted Eagle and 25 more (Arena too)"],
      ["sounds/slump_small_01.ogg", "Slump small 1", "Death sound of Mirage Goat, Limping Leper, Unliving Archer and 1 more; Attack or hit of Firebreather, Mummified Servant, Mirage Skeleton and 13 more (Arena too)"],
      ["sounds/smallimpact_01.ogg", "Small impact 1", "Death sound of Enormous Serpent, Angry Ram, Enchanted Ram and 4 more; Attack or hit of Bilge Dog, Dread Pirate Al-Robarts, Bull Elephant and 21 more (Arena too)"],
      ["sounds/swordspearimpact.ogg", "Sword spear impact", "Opening sound of Fearful Farmer, Grave Robber; Attack or hit of Polydamas, Trojan Spearman, Flying Armor (Arena too)"],
      ["sounds/swordspearimpact_dry.ogg", "Sword spear impact dry", "Death sound of Mr. Blobs; Attack or hit of Crazy Murray, Flying Pig, Flying Snake and 1 more (Arena too)"]
    ] },
    { name: "Codex and Nemesis", sounds: [
      ["sounds/codex_flinch.ogg", "Codex flinch", "Attack or hit of Codex. Also in Bookworm Adventures Volume 2 as Codex flinch (unused)."],
      ["sounds/codex_laugh.ogg", "Codex laugh", "A Codex animation of the last chapter laughs. Also in Bookworm Adventures Volume 2 as Codex laugh (unused)."],
      ["sounds/codex_nyah.ogg", "Codex nyah", "A Codex animation of the last chapter taunts. Also in Bookworm Adventures Volume 2 as Codex nyah (unused)."],
      ["sounds/lexbeam.ogg", "Lex beam", "Lex's beam animation in the fight with the Codex. Also in Bookworm Adventures Volume 2 as Lex beam (unused)."],
      ["sounds/nemesis_laugh.ogg", "Nemesis laugh", "The animation of Cassandra and the Nemesis laughs. Also in Bookworm Adventures Volume 2 as Nemesis laugh (unused)."],
      ["sounds/v_autological.ogg", "\"Autological\"", "Lex says it in the animation of the Codex fight. The game also has the same recording as sounds/lex_autological.ogg, never played. Also in Bookworm Adventures Volume 2 as \"Autological\" (two copies, neither played)."]
    ] },
    { name: "Unused", sounds: [
      ["sounds/lex_autological.ogg", "\"Autological\" (unused)", "Never played: loaded as LEX_AUTOLOGICAL, but no code or script asks for it. The same recording as \"Autological\" (sounds/v_autological.ogg), which Lex says in the Codex fight; only this copy is never played. Also in Bookworm Adventures Volume 2 as \"Autological\" (two copies, neither played)."],
      ["sounds/lex_unbelievable.ogg", "\"Unbelievable\" (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures Volume 2 as \"Unbelievable\" (unused)."],
      ["sounds/v_buynow.ogg", "\"Buy now\" (trial only)", "Trial only: the upsell trailer the trial showed said it once; the port, made from the full game, has no trial and leaves the trailer's resources out. Also in Bookworm Adventures Volume 2 as \"Buy now\" (trial only)."],
      ["sounds/vo_routed.ogg", "\"Routed\" (unused)", "Never played: loaded as OVERKILL_ROUTED, but the scripts' overkill table names only the seven other overkill voices. Also in Bookworm Adventures Volume 2 as \"Routed\" (unused)."]
    ] }
  ] },
  bookwormadv2: {
    presets: [
      { name: "Battle Start", when: "A battle opens: the fireworks, the warm-up call and the cymbal", steps: [["sounds/firework2", 0], ["sounds/v_warmup", 0], ["sounds/cymbalcrash", 4310]]},
      { name: "Fire Hit", when: "A fire word hits the foe: the blow, the flames and the cymbal", steps: [["sounds/largeimpact_01", 0], ["sounds/smallimpact_01", 0], ["sounds/fire6", 0], ["sounds/cymbalcrash", 2110]]},
      { name: "Low Health", when: "A foe's blow leaves Lex low, and the alarm starts", steps: [["sounds/largeimpact_01", 0], ["sounds/low_health", 210, {"end": 650}], ["sounds/low_health", 650, {"end": 1090}], ["sounds/low_health", 1090]]},
      { name: "Killing Blow", when: "The word that defeats a foe: the blows, the voice's call, a gem and the foe's own death cry (here a fiddler's)", steps: [["sounds/largeimpact_01", 0, {"v": -146}], ["sounds/smallimpact_01", 0, {"v": -146}], ["sounds/vo_crushed", 0, {"v": -146}], ["sounds/gemspawn", 0, {"v": -146}], ["sounds/stringdeath", 0, {"v": -146}], ["sounds/swordspearimpact_withwoosh", 0, {"v": -146}]]},
      { name: "New Treasure", when: "A treasure is won after a battle: its chime, then the fireworks and the crowd", steps: [["sounds/newtreasure", 0, {"v": -146}], ["sounds/firework2", 500, {"v": -146}], ["sounds/crowdcheer", 500]]},
      { name: "Boss Battle", when: "A boss battle opens: the fireworks and the call", steps: [["sounds/firework2", 0, {"v": -146}], ["sounds/v_bossbattle", 0, {"v": -146}]]},
      { name: "Level Up", when: "Lex goes up a level after a battle: the fireworks and his cheer", steps: [["sounds/firework1", 0, {"v": -146}], ["sounds/firework2", 590, {"v": -146}], ["sounds/lex_levelup", 640, {"v": -146}]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/click1.ogg", "Click 1", "A button is pressed in a dialog, the options screen, the stats book or the treasure screen"],
      ["sounds/click2.ogg", "Click 2", "A main menu, Hall of Fame, mini-game menu or options button is pressed, or a button on the board"],
      ["sounds/lex_chomp.ogg", "Lex chomps", "Lex chomps on the title screen while the game loads, and once more as he walks off"],
      ["sounds/textreadtype01.ogg", "Text blip", "Every other letter is typed out while a conversation line is shown"]
    ] },
    { name: "Main menu voices", sounds: [
      ["sounds/v_adventure.ogg", "\"Adventure\"", "Lex reacts when the pointer moves onto the Adventure item of the main menu"],
      ["sounds/v_arena.ogg", "\"Arena\"", "Lex reacts when the pointer moves onto the Arena item of the main menu"],
      ["sounds/v_dontleaveme.ogg", "\"Don't leave me\"", "The Quit item is pressed on the main menu, as the really-quit confirmation opens"],
      ["sounds/v_halloffame.ogg", "\"Hall of Fame\"", "Lex reacts when the pointer moves onto the Hall of Fame item of the main menu"],
      ["sounds/v_minigames.ogg", "\"Mini-games\"", "Lex reacts when the pointer moves onto the Mini-games item of the main menu"],
      ["sounds/v_options.ogg", "\"Options\"", "Lex reacts when the pointer moves onto the Options item of the main menu"],
      ["sounds/v_quit.ogg", "\"Quit\"", "Lex reacts when the pointer moves onto the Quit item of the main menu"],
      ["sounds/v_tomeofknowledge.ogg", "\"Tome of Knowledge\"", "Lex reacts when the pointer moves onto the Tome of Knowledge item of the main menu"],
      ["sounds/v_welcometobookwormadventures.ogg", "\"Welcome to Bookworm Adventures\"", "Lex greets the player when the title screen gives way to the main menu"]
    ] },
    { name: "Word play", sounds: [
      ["sounds/buzzer.ogg", "Word refused", "A word is refused in timed mode, submit is pressed with no valid word, a potion is clicked while it cannot be used, or in the Muta Words mini-game"],
      ["sounds/gemspawn.ogg", "Gem appears", "A gem appears on a tile (the tutorial, the letter mini-games and some creatures do this)"],
      ["sounds/rainbowspawn.ogg", "Rainbow tile appears", "A Rainbow bonus tile is added to the board in play, not while a saved or scripted grid is set up"],
      ["sounds/scroll.ogg", "Word scroll", "When Lex wins a fight, the word scroll with his best and longest word begins to unroll; rolling it up again is silent"],
      ["sounds/tile_select.ogg", "Tile select", "A letter tile is picked on the board, or a potion is picked by its number key"],
      ["sounds/tile_undo.ogg", "Tile taken back", "A tile is taken back off the word (a tutorial step does it too)"],
      ["sounds/v_scramble.ogg", "\"Scramble\"", "Lex's scramble starts in a fight, from the Scramble button on a full board or from Yes on the scramble hint; never in a mini-game"],
      ["sounds/warptileshimmer.ogg", "Warp tile shimmer", "At the end of each round, once for each warp tile on the board as its warped animation plays; not while the foe is dying"],
      ["sounds/word_power.ogg", "Word power", "Each time the selected tiles come to spell a valid word the foe is not immune to, in a fight; the pitch rises with the word's length plus its tiles' bonuses, capped at 14"]
    ] },
    { name: "Lex's praise", sounds: [
      ["sounds/lex_good.ogg", "\"Good!\"", "A spelled word counting 6 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 6 tiles"],
      ["sounds/lex_wow.ogg", "\"Wow!\"", "A spelled word counting 7 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 7 tiles; also Book 6's Mirage Lex's cry when beaten"],
      ["sounds/lex_nice.ogg", "\"Nice!\"", "A spelled word counting 8 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 8 tiles; Word Master: the last prize line solved"],
      ["sounds/lex_verygood.ogg", "\"Very good!\"", "A spelled word counting 9 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 9 tiles; Word Master: the fourth prize line solved"],
      ["sounds/lex_awesome.ogg", "\"Awesome!\"", "A spelled word counting 10 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 10 tiles; also Book 6's Mirage Lex's cry as the fight starts"],
      ["sounds/lex_excellent.ogg", "\"Excellent!\"", "A spelled word counting 11 (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 11 tiles; Word Master: the third prize line solved"],
      ["sounds/lex_fantastic.ogg", "\"Fantastic!\"", "A spelled word counting 12 or more (each tile 1, more with treasures and gem tiles; the count stops at 12); its bubble shows unless the game is timed; Golden Coins and Word Up: a word of 12 tiles"],
      ["sounds/lex_astonishing.ogg", "\"Astonishing!\"", "Never for a word in battle (the count stops at 12); Golden Coins and Word Up: a word of 13 tiles; Word Master: the top prize line solved"]
    ] },
    { name: "Hidden words", sounds: [
      ["sounds/lex_burp.ogg", "Lex burps", "Each time the selected tiles spell SUKHBIR in a fight, in place of the praise voice"],
      ["sounds/sheepbleat.ogg", "Sheep bleat", "Each time the selected tiles spell BILL in a fight, in place of the praise voice; also a sheep, ram or sheep-like creature bleats"],
      ["sounds/v_bythepowerofgreyskull.ogg", "\"By the power of Greyskull\"", "Each time the selected tiles spell ANTHONY in a fight, in place of the praise voice"],
      ["sounds/v_cashmoney.ogg", "\"Cash money\"", "Each time the selected tiles spell JEFF or STEVE in a fight, in place of the praise voice"],
      ["sounds/v_crazydelicious.ogg", "\"Crazy delicious!\"", "Each time the selected tiles spell TYSEN in a fight, in place of the praise voice"],
      ["sounds/v_iloveyou.ogg", "\"I love you\"", "Each time the selected tiles spell JULIE, JASON or TIFFANY in a fight, in place of the praise voice"],
      ["sounds/v_wormofbookcraft.ogg", "\"Worm of Bookcraft!\"", "Each time the selected tiles spell CARL in a fight, in place of the praise voice"]
    ] },
    { name: "Lex", sounds: [
      ["sounds/lexdeath.ogg", "Lex is defeated", "Lex dies in a fight, as the death music starts"],
      ["sounds/lexwalkloop.ogg", "Lex walks", "Lex walks, a looping footstep sound, in fights and at the level-up effect"],
      ["sounds/v_giggle1.ogg", "Giggle 1 (unused)", "Never played: only the giggle animation command plays it, and none of the game's animations has that command"],
      ["sounds/v_giggle2.ogg", "Giggle 2", "The main menu Lex plays it in two of his animations"],
      ["sounds/v_heehee.ogg", "\"Hee hee\"", "Lex's click giggle animation, on the fourth and seventh click in a row on him while he idles on the player's turn in a fight"],
      ["sounds/v_hiccup.ogg", "Hiccup", "Lex's click hiccup animation, on the first three and the fifth and sixth clicks in a row on him while he idles on the player's turn in a fight"],
      ["sounds/v_howlongcanittaketogotothebathroom.ogg", "\"How long can it take to go to the bathroom?\"", "Lex says it when the player has been idle a while in a fight"],
      ["sounds/v_ifyoudontstartplayingagainsoon.ogg", "\"If you don't start playing again soon...\"", "Lex says it when the player has been idle a while in a fight"],
      ["sounds/v_sneeze.ogg", "Sneeze", "Lex's click sneeze animation, on the eighth click in a row on him while he idles on the player's turn in a fight; the count then starts over"],
      ["sounds/v_wakeupsleepyhead.ogg", "\"Wake up, sleepyhead!\"", "Lex says it when the player has been idle a while in a fight"]
    ] },
    { name: "Lex attacks", sounds: [
      ["sounds/bite_01.ogg", "Bite", "Lex bites in his own animation, and in the Caterpillar, Dormouse, Mrs. Sprat, Walrus, dance battle and Mech Hound fights"],
      ["sounds/bluntimpact_dry.ogg", "Blunt impact", "Lex's word attack lands; effect animations and the Hen fight use it too"],
      ["sounds/bluntimpact_withwoosh.ogg", "Blunt with woosh", "The attack4 animation of Lex's word attack triggers it; some creatures use it too"],
      ["sounds/bowlstrike.ogg", "Bowl strike", "Lex's own animation strikes with a bowl"],
      ["sounds/fire3.ogg", "Fire attack 3", "Lex's word attack lands; many creatures' fire attacks use it too"],
      ["sounds/fire6.ogg", "Fire attack 6", "Lex's word attack lands; many creatures' fire attacks use it too"],
      ["sounds/swordspearimpact_withwoosh.ogg", "Sword with woosh", "The attack6 animation of Lex's word attack triggers it; many creatures use it too"]
    ] },
    { name: "Battle", sounds: [
      ["sounds/low_health.ogg", "Low health", "Lex's health is at a quarter of his maximum or less in a fight: one beep per pulse of his health bar, quieter from 500 updates on; in Letter Rip, Link 'n Spell and Muta Words, every 50 updates (0.5 s) of their last 1000 (10 s). Also in Zuma's Revenge, as sounds/challenge_countdown.ogg."],
      ["sounds/v_bossbattle.ogg", "\"Boss battle\"", "When a boss intro's flash peaks, together with Firework 2, except in the book 6 chapter 7 dance battle, the book 0 warm-up and the Hydra's fight"],
      ["sounds/v_survivalbattle.ogg", "\"Survival battle\"", "When the intro flash peaks in the fight against the Hydra, together with Firework 2"],
      ["sounds/v_warmup.ogg", "\"Warm up\"", "When the intro flash peaks in a book 0 fight, together with Firework 2"],
      ["sounds/warpin.ogg", "Warp in", "Lex's warp-in animation on the map: back from the treasure screen or from a death conversation, and as some conversations of books 4 to 6 end"],
      ["sounds/warpout.ogg", "Warp out", "Lex warps out of the map as a chapter or mini-game starts; the main menu Lex plays it in several of his animations"]
    ] },
    { name: "Dance battle", sounds: [
      ["sounds/camera2.ogg", "Camera flash", "Eight commands of Lex's dance animation in the book 6 dance battle play it"],
      ["sounds/crowdcheer1.ogg", "Crowd cheer", "Two commands of Lex's dance animation in the book 6 dance battle play it"],
      ["sounds/dancedeath.ogg", "Dance ends", "The death sound of the dance battle's foe (book 6 chapter 7)"],
      ["sounds/danceengage.ogg", "Dance opens", "The opening sound of the dance battle's foe (book 6 chapter 7)"],
      ["sounds/robot-e.ogg", "Robot E", "One command of Lex's dance animation in the book 6 dance battle plays it, after the L and before the X"],
      ["sounds/robot-l.ogg", "Robot L", "One command of Lex's dance animation in the book 6 dance battle plays it, the first of the four"],
      ["sounds/robot-lex.ogg", "Robot Lex", "One command of Lex's dance animation in the book 6 dance battle plays it, the last of the four"],
      ["sounds/robot-x.ogg", "\"X!\"", "One command of Lex's dance animation in the book 6 dance battle plays it, after the E"],
      ["sounds/v_dancebattle.ogg", "\"Dance battle\"", "When the intro flash peaks in the book 6 chapter 7 fight, together with Firework 2"]
    ] },
    { name: "Rewards", sounds: [
      ["sounds/chaching.ogg", "Cash register", "Coins are counted up in a mini-game; the Mirage Trojan fight uses it too"],
      ["sounds/crowdcheer.ogg", "Crowd cheers", "The level-up effect starts its crowd; the checkpoint banner fades in; the altar's white flash peaks (with Firework 2); the book manager's end banner fades in; a mini-game's results screen opens"],
      ["sounds/firework1.ogg", "Firework 1", "The level-up effect's XP timer runs out; the checkpoint banner fades in (with the crowd). Also in Bejeweled 3, as sounds/firework_thump.ogg."],
      ["sounds/firework2.ogg", "Firework 2", "The level-up effect's white flash falls to 20 or less, just before Lex's level-up voice; a boss intro flash peaks; the altar's white flash peaks (with the crowd); the main menu Lex plays it in three animations"],
      ["sounds/lex_levelup.ogg", "\"Level up\"", "When the level-up effect's white flash fades out, after a fight that levels Lex up"],
      ["sounds/newtreasure.ogg", "\"New treasure!\"", "A treasure is won at the end of a fight, or shown on the treasure screen"],
      ["sounds/v_newbadge.ogg", "\"New badge\"", "As the new-badge effect's fade-in reaches its top alpha of 191 and the badge animation starts"],
      ["sounds/v_newcompanion.ogg", "\"New companion\"", "On the treasure screen, once the altar's white flash is at 400 or less, when the treasure won is a companion and the fight is not a replay"],
      ["sounds/vo_annihilated.ogg", "\"Annihilated\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_crushed.ogg", "\"Crushed\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_decimated.ogg", "\"Decimated\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_destroyed.ogg", "\"Destroyed\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_obliterated.ogg", "\"Obliterated\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_vanquished.ogg", "\"Vanquished\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/vo_whomped.ogg", "\"Whomped\"", "A fight is won with overkill damage, announced with this word"],
      ["sounds/xpgain4.ogg", "Experience counts", "The experience stream flows into Lex at a level-up, a fight end or a gravestone"]
    ] },
    { name: "Potions", sounds: [
      ["sounds/heal2.ogg", "Health potion", "Lex uses a health potion while his health is below its maximum"],
      ["sounds/healpotionused_02.ogg", "Creature heals", "Attack sound of a creature's heal-itself attack: Regenerate (Stick Pig, Straw Pig, the Imperial Foot Soldier and Sergeant and others) or a heal effect (Caterpillar, Jabberwocky, Monkey King and others)"],
      ["sounds/potionpickup.ogg", "Potion pickup", "A potion dropped by a gravestone starts flying to Lex's potion row, once it is at x 165 or less and the gravestone's animation is done"],
      ["sounds/powerdown_01.ogg", "Power down", "A creature weakens the attack of Lex, or a boosted attack ends"],
      ["sounds/powerup3.ogg", "Attack potion", "Lex uses an attack potion"],
      ["sounds/poweruppotion_02.ogg", "Power up", "Attack sound of the strengthening attacks of the Caterpillar, Harmony, War and the War Pig, which boost the enemy's next attack"],
      ["sounds/purify2.ogg", "Purify", "Lex uses a purify potion; also Word Master plays it each time a puzzle line is solved, the Cheshire Cat treasure plays it, and the main menu Lex plays it once"],
      ["sounds/weaken3.ogg", "Weaken effect", "A curse that cuts the target's damage (a damage multiplier under 1 on its attacks) is applied; not when a saved fight is restored"]
    ] },
    { name: "Tile effects", sounds: [
      ["sounds/boardfrozen_01.ogg", "Board frozen", "Attack sound of the South King's and the Butterfly's freezing attacks"],
      ["sounds/boardstunned_01.ogg", "Board stunned", "Attack sound of attacks of many creatures, mostly stun-and-damage attacks (Caterpillar, Dormouse, Mad Hatter, Queen of Hearts, Door, Mutant Ape and others)"],
      ["sounds/fire1.ogg", "Fire effect", "The burning ailment is applied to Lex or a foe, and again each time it burns at a turn's end; also the attack sound of the Candlestick, Gingerbread House and Jabberwocky"],
      ["sounds/ice1.ogg", "Frozen effect", "The frozen status is applied to Lex or a foe; also the attack sound of the Cold Porridge and the Teacups"],
      ["sounds/ice4.ogg", "Petrify effect", "The petrified status is applied to Lex or a foe; also the attack sound of the Troglock Cook. Also in Zuma's Revenge, as sounds/freeze.ogg."],
      ["sounds/lightning_attack_01.ogg", "Lightning", "A lightning attack lands (Circe, Hydra, Sea Elemental and wizards)"],
      ["sounds/lockit.ogg", "Tile locked", "Attack sound of the tile-locking attacks of Old MacDonald, the Pig, the Sheep, the Year of the Dog and Rooster, the Door and others"],
      ["sounds/petrify_attack_01.ogg", "Tiles petrified", "A creature turns tiles to stone (Basilisk, Medusa and others)"],
      ["sounds/poison1.ogg", "Poison effect", "The poison ailment is applied to Lex or a foe, and again each time it hurts at a turn's end; also the curse-tile and reflect ailments apply it, and the Tweedledee has it as an attack sound"],
      ["sounds/poison_attack_01.ogg", "Poison attack", "Attack sound of the Dragon King's and the Shrimp Soldier's poisoning attacks"],
      ["sounds/tiles_plagued_01.ogg", "Tiles plagued", "Attack sound of the West King's tile-poisoning attack"],
      ["sounds/tiles_smashed_01.ogg", "Tiles smashed", "Attack sound of tile-smashing attacks (Caterpillar, Monkey King, Tree Demon, War Bear, North King and others)"]
    ] },
    { name: "Companions and tiles", sounds: [
      ["sounds/eggtimer.ogg", "Egg timer", "Mother Goose sets her egg timer"],
      ["sounds/herbertshield.ogg", "Herbert shield", "Herbert puts up his shield"],
      ["sounds/lmhiya.ogg", "\"Hiya\"", "Master Wong attacks"],
      ["sounds/madhat_reverb.ogg", "Mad Hat effect", "The Mad Hat effect plays"],
      ["sounds/mkxform.ogg", "Monkey King transforms", "The Monkey King companion transforms"],
      ["sounds/skullupgrade.ogg", "Skull upgrade", "The skeleton skull companion is upgraded"],
      ["sounds/timehelmet_reverb.ogg", "Mad hat scramble", "The mad hat companion's sound when the letters are scrambled (its file is named for the Time Helmet); no words"]
    ] },
    { name: "Cutscenes", sounds: [
      ["sounds/cheshirecat.ogg", "Cheshire cat", "The book 4 chapter 7 cutscene of the Cheshire Cat plays, after the Hydra is beaten"],
      ["sounds/cymbalcrash.ogg", "Cymbal crash", "Each of Stuffy's two animation files plays it once"],
      ["sounds/emperorandking2_light_v2.ogg", "Emperor light", "In book 5 chapter 7, when the cutscene's CROWNED animation starts"],
      ["sounds/emperorandking2_stinger_v2.ogg", "Emperor stinger", "In book 5 chapter 7, as the Crowned cutscene begins"],
      ["sounds/mothergoosestewpot.ogg", "Mother Goose stewpot", "The book 4 chapter 3 cutscene of Mother Goose's stew plays"],
      ["sounds/shackles_break.ogg", "Shackles break", "The book 5 chapter 5 cutscene and the book 6 chapter 3 cutscene each play it as the chains break"],
      ["sounds/sifu_beatdown.ogg", "Sifu beat down", "The book 5 chapter 3 cutscene of the Sifu gang's beat-down plays"],
      ["sounds/skeleskullrevives.ogg", "Skull revives", "The book 6 chapter 7 cutscene of the skeleton skull plays"]
    ] },
    { name: "Mini-games", sounds: [
      ["sounds/lex_incredible.ogg", "\"Incredible!\"", "Word Master only: the second prize line solved"],
      ["sounds/lex_watchout.ogg", "\"Watch out!\"", "Word Master: a miss brings the prize down to its last line"],
      ["sounds/count.ogg", "Counting tick", "The final score counts up in all six mini-games, a tick whenever the count's sound timer reads 0"],
      ["sounds/newminigame.ogg", "New mini-game sting", "50 updates (0.5 s) after Lex's warp-in sound, as a book 4 conversation announces a newly unlocked mini-game"],
      ["sounds/v_go.ogg", "\"Go\"", "Letter Rip, Link 'n Spell, Muta Words and Word Up: the start countdown reaches 0, 300 updates (3 s) after Three"],
      ["sounds/v_newminigame.ogg", "\"New mini-game\"", "50 updates (0.5 s) after Lex's warp-in sound, as a book 4 conversation announces a newly unlocked mini-game"],
      ["sounds/v_one.ogg", "\"One\"", "Letter Rip, Link 'n Spell, Muta Words and Word Up: the start countdown reaches 1, 200 updates (2 s) after Three"],
      ["sounds/v_three.ogg", "\"Three\"", "Letter Rip, Link 'n Spell, Muta Words and Word Up: the start countdown begins, one update after its timer starts; also when the tutorial's arrow hints end"],
      ["sounds/v_two.ogg", "\"Two\"", "Letter Rip, Link 'n Spell, Muta Words and Word Up: the start countdown reaches 2, 100 updates (1 s) after Three"]
    ] },
    { name: "Human voices", sounds: [
      ["sounds/dopeyattack1a.ogg", "Dopey attack 1a", "Opening sound of Tweedledee, Tweedledum, The Deuce and 5 more; Attack or hit of Angry Loyim"],
      ["sounds/dopeyimpact2a.ogg", "Dopey impact 2a", "Death sound of Tweedledee, Tweedledum, Walrus and 7 more"],
      ["sounds/female seduce.ogg", "Female seduce", "Opening sound of Stepfield Wife, Summerspeak (Arena too)"],
      ["sounds/female_groan_01.ogg", "Female groan 1", "Opening sound of Anna Karenina; Attack or hit of Queen of Hearts, Stepfield Wife (Arena too)"],
      ["sounds/female_groan_02.ogg", "Female groan 2", "Opening sound of Mrs. Sprat, Gretel; Death sound of Gretel, Stepfield Wife, Anna Karenina; Attack or hit of Queen of Hearts, Summerspeak (Arena too)"],
      ["sounds/female_mmm.ogg", "Female mmm", "Opening sound of Witch"],
      ["sounds/female_ooh.ogg", "Female ooh", "Opening sound of Strangling Ghost; Attack or hit of Summerspeak"],
      ["sounds/humanmalegrunt2.ogg", "Human male grunt 2", "Death sound of Imperial Sergeant, Rogue Hacker; Attack or hit of Swindler, Gluttonous Monk, Greedy Monk and 2 more"],
      ["sounds/humanmalegrunt3.ogg", "Human male grunt 3", "Death sound of Swindler, Imperial Captain; Attack or hit of Prideful Monk, Slothful Monk, Imperial Sergeant and 1 more (Arena too)"],
      ["sounds/male_anger.ogg", "Male anger", "Opening sound of Swindler, Prideful Monk, Wrathful Monk and 4 more; Death sound of Odysseus; Attack or hit of Greedy Monk, Futurcom Tour Guide (Arena too)"],
      ["sounds/male_laugh.ogg", "Male laugh", "Opening sound of Old MacDonald, Hansel, King of Hearts and 7 more; Death sound of Mad Hatter; Attack or hit of Swindler, Caterpillar, March Hare and 4 more (Arena too)"],
      ["sounds/male_laugh2.ogg", "Male laugh 2", "Opening sound of Gingerbread Man, Mad Hatter, March Hare and 5 more; Attack or hit of Swindler, Hansel, Caterpillar and 2 more (Arena too)"],
      ["sounds/male_low_anger.ogg", "Male low anger", "Opening sound of Revenant, Thought Cop; Attack or hit of South King, West King, Mutant (Arena too)"],
      ["sounds/male_low_pain.ogg", "Male low pain", "Death sound of Old MacDonald, Wrathful Monk, Broken Brother and 3 more; Attack or hit of Gluttonous Monk, Stricken Ghost, East King and 4 more (Arena too)"],
      ["sounds/male_low_warcry.ogg", "Male low warcry", "Opening sound of Broken Brother, Book Burner; Attack or hit of Imperial Captain, Sifu Sekkeen (Arena too)"],
      ["sounds/male_pain.ogg", "Male pain", "Death sound of Gingerbread Man, Hansel, Carpenter and 12 more; Attack or hit of Old MacDonald (Arena too)"],
      ["sounds/witch_groan_01.ogg", "Witch groan 1", "Death sound of Witch; Attack or hit of Grandma (Arena too)"],
      ["sounds/witchy_laugh_01.ogg", "Witchy laugh 1", "Attack or hit of Grandma, Queen of Hearts (Arena too)"],
      ["sounds/witchy_laugh_02.ogg", "Witchy laugh 2", "Opening sound of Grandma; Attack or hit of Witch, Queen of Hearts (Arena too)"]
    ] },
    { name: "Farm animals", sounds: [
      ["sounds/bovineattack1.ogg", "Bovine attack 1", "Opening sound of Space Cow, Cow; Attack or hit of Marsling"],
      ["sounds/bovineattack2.ogg", "Bovine attack 2", "Attack or hit of Space Cow, Cow, Old MacDonald (Arena too)"],
      ["sounds/bovineimpact1.ogg", "Bovine impact 1", "Death sound of Space Cow, Cow"],
      ["sounds/chickenattack1.ogg", "Chicken attack 1", "Opening sound of Chicken, The Hen, Year of the Rooster"],
      ["sounds/chickenattack2.ogg", "Chicken attack 2", "Attack or hit of Chicken"],
      ["sounds/chickenimpact4.ogg", "Chicken impact 4", "Death sound of Chicken; Attack or hit of The Hen"],
      ["sounds/donkeybray.ogg", "Donkey bray", "Opening sound of Donkey of Guizhou"],
      ["sounds/donkeyimpact2.ogg", "Donkey impact 2", "Death sound of Donkey of Guizhou"],
      ["sounds/generic_animal_grumble_01.ogg", "Generic animal grumble 1", "Opening sound of Year of the Horse"],
      ["sounds/pig_squeal_01.ogg", "Pig squeal 1", "Opening sound of Stick Pig; Attack or hit of Brick Pig"],
      ["sounds/pig_squeal_02.ogg", "Pig squeal 2", "Opening sound of Pig Monster; Death sound of Pig, Pig Monster, Warpig; Attack or hit of Straw Pig"],
      ["sounds/pigattack2.ogg", "Pig attack 2", "Opening sound of Pig, Straw Pig, Warpig; Attack or hit of Brick Pig, Stick Pig, Pig Monster"],
      ["sounds/pigattack3.ogg", "Pig attack 3", "Opening sound of Brick Pig; Attack or hit of Pig, Stick Pig, Straw Pig"]
    ] },
    { name: "Wild animals", sounds: [
      ["sounds/bearattack1.ogg", "Bear attack 1", "Attack or hit of Baby Bear, Mama Bear, Papa Bear and 1 more (Arena too)"],
      ["sounds/bearimpact.ogg", "Bear impact", "Death sound of Baby Bear, Mama Bear; Attack or hit of Papa Bear, War Bear (Arena too)"],
      ["sounds/bearroar.ogg", "Bear roar", "Opening sound of Baby Bear, Mama Bear, War Bear and 2 more; Attack or hit of Papa Bear (Arena too)"],
      ["sounds/dog_barking_01.ogg", "Dog barking 1", "Death sound of Year of the Dog; Attack or hit of Big Bad Wolf, Erlang Shen (Arena too)"],
      ["sounds/dog_barking_02.ogg", "Dog barking 2", "Opening sound of Year of the Dog, Mech Hound; Attack or hit of Big Bad Wolf (Arena too)"],
      ["sounds/dog_whimper_01.ogg", "Dog whimper 1", "Death sound of Big Bad Wolf (Arena too)"],
      ["sounds/feline_whimper_01.ogg", "Feline whimper 1", "Opening sound of Cat, Earless Tiger, Elder Tiger Demon and 2 more; Death sound of Earless Tiger, Elder Tiger Demon, Tailless Tiger (Arena too)"],
      ["sounds/feline_whimper_02.ogg", "Feline whimper 2", "Death sound of Cat, Kxitty; Attack or hit of Earless Tiger, Elder Tiger Demon, Tailless Tiger (Arena too)"],
      ["sounds/generic_slosh_02.ogg", "Generic slosh 2", "Death sound of Mrs. Sprat"],
      ["sounds/monkeyattack2.ogg", "Monkey attack 2", "Attack or hit of Monkey King, Monolisk, Mutant Ape (Arena too)"],
      ["sounds/monkeyattack4.ogg", "Monkey attack 4", "Attack or hit of Monkey King, Ape"],
      ["sounds/monkeychallenge.ogg", "Monkey challenge", "Opening sound of Ape, Mutant Ape; Attack or hit of Monkey King, Monkey King (Arena)"],
      ["sounds/snake_hiss_01.ogg", "Snake hiss 1", "Opening sound of Year of the Snake; Death sound of Year of the Snake"],
      ["sounds/wolf_howl_01.ogg", "Wolf howl 1", "Opening sound of Big Bad Wolf; Death sound of Grandma; Attack or hit of Year of the Dog (Arena too)"]
    ] },
    { name: "Beasts and giants", sounds: [
      ["sounds/dragon_bite.ogg", "Dragon bite", "Attack or hit of Jabberwocky, Dragon Guard, Dragon King and 2 more (Arena too)"],
      ["sounds/dragon_death.ogg", "Dragon death", "Death sound of Jabberwocky, Dragon Guard, Dragon King and 2 more (Arena too)"],
      ["sounds/dragon_pain.ogg", "Dragon pain", "Opening sound of Jabberwocky, Dragon Guard, Dragon King and 2 more; Attack or hit of Jabberwocky, Dragon Guard, Dragon King and 3 more (Arena too)"],
      ["sounds/giantgruntecho2.ogg", "Giant grunt echo 2", "Opening sound of Giant; Death sound of Giant (Arena too)"],
      ["sounds/kubwa_roar.ogg", "Kubwa roar", "Attack or hit of Giant (Arena too)"],
      ["sounds/monsterattack2.ogg", "Monster attack 2", "Attack or hit of Bull Demon King (Arena too)"],
      ["sounds/monsterchallenge1.ogg", "Monster challenge 1", "Opening sound of Bull Demon King (Arena too)"],
      ["sounds/monsterimpact2.ogg", "Monster impact 2", "Death sound of Bull Demon King (Arena too)"],
      ["sounds/ogreattack1.ogg", "Ogre attack 1", "Attack or hit of Radioactive Mutant, Troglock King (Arena too)"],
      ["sounds/ogreattack2.ogg", "Ogre attack 2", "Attack or hit of Radioactive Mutant Ape, Troglock King (Arena too)"],
      ["sounds/ogrechallenge.ogg", "Ogre challenge", "Opening sound of Radioactive Mutant Ape; Attack or hit of Tweedledee, Tweedledum, Troglock King (Arena too)"],
      ["sounds/ogrechallengelong.ogg", "Ogre challenge long", "Opening sound of Radioactive Mutant; Attack or hit of Troglock King (Arena too)"],
      ["sounds/ogreimpact1.ogg", "Ogre impact 1", "Attack or hit of Tweedledee, Tweedledum, Radioactive Mutant"],
      ["sounds/ogreimpact1alt.ogg", "Ogre impact 1 alt", "Death sound of Radioactive Mutant; Attack or hit of Radioactive Mutant Ape (Arena too)"],
      ["sounds/ogreimpact2.ogg", "Ogre impact 2", "Death sound of Radioactive Mutant Ape, Troglock King (Arena too)"]
    ] },
    { name: "Spirits and magic", sounds: [
      ["sounds/fading_ghostly_wail_01.ogg", "Fading ghostly wail 1", "Opening sound of Tree Demon, Qin Demon, Screaming Sutra; Death sound of Screaming Sutra"],
      ["sounds/fading_ghostly_wail_02.ogg", "Fading ghostly wail 2", "Death sound of Stricken Ghost, Tree Demon; Attack or hit of Revenant, Strangling Ghost"],
      ["sounds/fading_ghostly_wail_03.ogg", "Fading ghostly wail 3", "Death sound of Strangling Ghost; Attack or hit of Stricken Ghost, Tree Demon"],
      ["sounds/skeleton_warcry_01.ogg", "Skeleton warcry 1", "Opening sound of White Bone Spirit; Death sound of White Bone Spirit"],
      ["sounds/smokeyattack1.ogg", "Smokey attack 1", "Opening sound of Caterpillar, Caterpillar (Arena), Marsling; Attack or hit of Stuffy, Stricken Ghost, Monkey King and 4 more (Arena too)"],
      ["sounds/smokeyattack2.ogg", "Smokey attack 2", "Death sound of Caterpillar (Arena); Attack or hit of Gingerbread Man, Gretel, Witch and 11 more (Arena too)"],
      ["sounds/smokeyimpact.ogg", "Smokey impact", "Death sound of Caterpillar, Marsling, Summerspeak; Attack or hit of Strangling Ghost, Data Node (Arena too)"],
      ["sounds/sonic_attack_02.ogg", "Sonic attack 2", "Opening sound of East King, Erlang Shen, North King and 2 more; Death sound of East King, Erlang Shen, North King and 2 more; Attack or hit of Gluttonous Monk, Greedy Monk, Prideful Monk and 12 more (Arena too)"],
      ["sounds/sparkleattack3.ogg", "Sparkle attack 3", "Opening sound of Moon, Door, Gold and 5 more; Death sound of Moon, Door, Gold and 4 more; Attack or hit of Monkey King, Data Node, Ice Hammer and 2 more (Arena too)"],
      ["sounds/wail_03.ogg", "Wail 3", "Attack or hit of Broken Brother"],
      ["sounds/wail_05.ogg", "Wail 5", "Opening sound of Stricken Ghost"]
    ] },
    { name: "Robots and sci-fi", sounds: [
      ["sounds/roboticattack1.ogg", "Robotic attack 1", "Attack or hit of Duplicant, R. Giskeel, Robert and 3 more (Arena too)"],
      ["sounds/roboticchallenge2.ogg", "Robotic challenge 2", "Opening sound of Overliege, Duplicant, R. Giskeel and 2 more; Attack or hit of Bigger Brother (Arena too)"],
      ["sounds/roboticimpact2.ogg", "Robotic impact 2", "Death sound of Overliege; Attack or hit of Berzerkoid, Duplicant, R. Giskeel and 3 more (Arena too)"],
      ["sounds/scifilaser1.ogg", "Sci-filaser 1", "Attack or hit of Berzerkoid, Kxitty, Marsling and 9 more (Arena too)"],
      ["sounds/scifimech1.ogg", "Sci-fimech 1", "Opening sound of Berzerkoid, Computer Virus; Death sound of Computer Virus; Attack or hit of Overliege, Bigger Brother, The Machine (Arena too)"],
      ["sounds/scifimech2.ogg", "Sci-fimech 2", "Opening sound of Data Node; Death sound of Berzerkoid, Duplicant, R. Giskeel and 3 more; Attack or hit of Overliege, Skeletrox (Arena too)"],
      ["sounds/scifiteleport2.ogg", "Sci-fiteleport 2", "Death sound of Monolisk; Attack or hit of Berzerkoid, Computer Virus, Data Node and 3 more (Arena too)"]
    ] },
    { name: "Plates, strings and squeaks", sounds: [
      ["sounds/plateattack2.ogg", "Plate attack 2", "Opening sound of Dish and Spoon; Attack or hit of Mad Hatter, Stepfield Wife (Arena too)"],
      ["sounds/plateattack3.ogg", "Plate attack 3", "Death sound of Dish and Spoon (Arena too)"],
      ["sounds/plateattack4.ogg", "Plate attack 4", "Opening sound of Cold Porridge, Hot Porridge, Teacups and 1 more (Arena too)"],
      ["sounds/plateimpact4.ogg", "Plate impact 4", "Death sound of Teapot; Attack or hit of Dish and Spoon (Arena too)"],
      ["sounds/plateimpact5.ogg", "Plate impact 5", "Death sound of Cold Porridge, Hot Porridge, Teacups; Attack or hit of Dish and Spoon (Arena too)"],
      ["sounds/plateimpact6.ogg", "Plate impact 6", "Attack or hit of Dish and Spoon (Arena too)"],
      ["sounds/squeakymamattack1.ogg", "Squeaky mam attack 1", "Opening sound of Low Demon; Attack or hit of Dormouse, March Hare (Arena too)"],
      ["sounds/squeakymamattack2.ogg", "Squeaky mam attack 2", "Opening sound of Dormouse, Flamingo Hedgehog"],
      ["sounds/squeakymamdie1.ogg", "Squeaky mamdie 1", "Death sound of Dormouse, March Hare, Flamingo Hedgehog and 1 more; Attack or hit of Mad Hatter (Arena too)"],
      ["sounds/stringattackfiddlebow.ogg", "String attack fiddle bow", "Opening sound of Fiddle; Attack or hit of Qin Demon, East King (Arena too)"],
      ["sounds/stringattackzither.ogg", "String attackzither", "Attack or hit of Qin Demon, East King"],
      ["sounds/stringdeath.ogg", "String death", "Death sound of Fiddle, Qin Demon"],
      ["sounds/stringimpact1alt.ogg", "String impact 1 alt", "Attack or hit of Fiddle"]
    ] },
    { name: "Attacks and impacts", sounds: [
      ["sounds/arrow_shot_01.ogg", "Arrow shot 1", "Attack or hit of Erlang Shen, Odysseus (Arena too)"],
      ["sounds/explosion1.ogg", "Explosion 1", "Attack or hit of Chicken, The Hen, Gingerbread House and 5 more"],
      ["sounds/explosion3.ogg", "Explosion 3", "Attack or hit of The Hen, Gingerbread House, Radioactive Mutant Ape and 1 more (Arena too)"],
      ["sounds/largeimpact_01.ogg", "Large impact 1", "Death sound of Stick Pig, Straw Pig, War Bear and 3 more; Attack or hit of Stuffy, Carpenter, Cat and 49 more"],
      ["sounds/miscimpact1.ogg", "Misc impact 1", "Death sound of Ape, Mutant Ape; Attack or hit of Grandma, Sifu Sekkeen, Year of the Dog and 4 more (Arena too)"],
      ["sounds/miscimpact2.ogg", "Misc impact 2", "Death sound of Brick Pig; Attack or hit of Giant, Mrs. Sprat, Sifu Sekkeen and 4 more (Arena too)"],
      ["sounds/multiexplosion1.ogg", "Multiexplosion 1", "Death sound of The Machine"],
      ["sounds/slicing_attack_01.ogg", "Slicing attack 1", "Attack or hit of Carpenter, Walrus, King of Hearts and 5 more (Arena too)"],
      ["sounds/slicing_attack_02.ogg", "Slicing attack 2", "Attack or hit of Old MacDonald, Carpenter, Queen of Hearts and 4 more (Arena too)"],
      ["sounds/slump_small_01.ogg", "Slump small 1", "Opening sound of Gingerbread House, Seahorse Captain, Shrimp Soldier; Death sound of Gingerbread House"],
      ["sounds/smallimpact_01.ogg", "Small impact 1", "Death sound of Candlestick; Attack or hit of Stuffy, Baby Bear, Big Bad Wolf and 71 more"],
      ["sounds/swordspearimpact.ogg", "Sword spear impact", "Attack or hit of Sword, South King"],
      ["sounds/swordspearimpact_dry.ogg", "Sword spear impact dry", "Attack or hit of Broken Brother, Revenant, War"],
      ["sounds/wetimpact1.ogg", "Wet impact 1", "Death sound of Seahorse Captain, Shark Sergeant, Shrimp Soldier; Attack or hit of Cold Porridge, Pig Monster, Year of the Snake and 1 more"],
      ["sounds/wetimpact4.ogg", "Wet impact 4", "Attack or hit of Mrs. Sprat, Shark Sergeant, Shrimp Soldier and 2 more"]
    ] },
    { name: "Unused", sounds: [
      ["sounds/curtainsclose.ogg", "Curtainsclose (unused)", "Never played: loaded as CURTAIN_CLOSE, but the curtain transition plays no sound and nothing else asks for it"],
      ["sounds/curtainscloseclap.ogg", "Curtainscloseclap (unused)", "Never played: loaded as CURTAIN_CLOSE_CLAP, but the curtain transition plays no sound and nothing else asks for it"],
      ["sounds/curtainsopen.ogg", "Curtainsopen (unused)", "Never played: loaded as CURTAIN_OPEN, but the curtain transition plays no sound and nothing else asks for it"],
      ["sounds/curtainsopenclap.ogg", "Curtainsopenclap (unused)", "Never played: loaded as CURTAIN_OPEN_CLAP, but the curtain transition plays no sound and nothing else asks for it"],
      ["sounds/giantgruntdry1.ogg", "Giant grunt dry 1 (unused)", "Never played: the game never plays it"],
      ["sounds/lex_unbelievable.ogg", "\"Unbelievable\" (unused)", "Never played: the game never plays it. Also in Bookworm Adventures as \"Unbelievable\" (unused)."],
      ["sounds/lextravelloop.ogg", "Lextravel loop (unused)", "Never played: the game never plays it"],
      ["sounds/plateimpact3.ogg", "Plate impact 3 (unused)", "Never played: listed only in book4.xml, and no script asks for it (the scripts play plate impacts 4 to 6)"],
      ["sounds/textreadblip.ogg", "Textreadblip (unused)", "Never played: the game never plays it"],
      ["sounds/textreadtype02.ogg", "Textreadtype 02 (unused)", "Never played: loaded as TEXT_BLIP1, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype03.ogg", "Textreadtype 03 (unused)", "Never played: loaded as TEXT_BLIP2, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype04.ogg", "Textreadtype 04 (unused)", "Never played: loaded as TEXT_BLIP3, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype05.ogg", "Textreadtype 05 (unused)", "Never played: loaded as TEXT_BLIP4, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype06.ogg", "Textreadtype 06 (unused)", "Never played: loaded as TEXT_BLIP5, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype07.ogg", "Textreadtype 07 (unused)", "Never played: loaded as TEXT_BLIP6, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype08.ogg", "Textreadtype 08 (unused)", "Never played: loaded as TEXT_BLIP7, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype09.ogg", "Textreadtype 09 (unused)", "Never played: loaded as TEXT_BLIP8, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/textreadtype10.ogg", "Textreadtype 10 (unused)", "Never played: loaded as TEXT_BLIP9, but a conversation's typing plays only TEXT_BLIP (Textreadtype 01)"],
      ["sounds/v_astonishing.ogg", "\"Astonishing\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_awesome.ogg", "\"Awesome\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_buynow.ogg", "\"Buy now\" (trial only)", "Trial only: the upsell trailer the trial showed said it once; the port, made from the full game, has no trial and leaves the trailer's resources out. Also in Bookworm Adventures as \"Buy now\" (trial only)."],
      ["sounds/v_excellent.ogg", "\"Excellent\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_fantastic.ogg", "\"Fantastic\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_good.ogg", "\"Good\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_illbeback.ogg", "\"I'll be back\" (unused)", "Never played: loaded with the main menu's voices as MM_ILLBEBACK, but no menu item's reaction uses it"],
      ["sounds/v_incredible.ogg", "\"Incredible\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_letsgo.ogg", "\"Let's go\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_levelup.ogg", "\"Levelup\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_newtreasure.ogg", "\"New treasure\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_nice.ogg", "\"Nice\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_timeforbed.ogg", "\"Time for bed\" (unused)", "Never played: loaded with the main menu's voices as MM_BEDTIME, but no menu item's reaction uses it"],
      ["sounds/v_unbelievable.ogg", "\"Unbelievable\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_verygood.ogg", "\"Very good\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_watchout.ogg", "\"Watch out\" (unused)", "Never played: the game never plays it"],
      ["sounds/v_wow.ogg", "\"Wow\" (unused)", "Never played: the game never plays it"],
      ["sounds/vo_routed.ogg", "\"Routed\" (unused)", "Never played: loaded as OVERKILL_ROUTED, but the scripts' overkill table names only the seven other overkill voices. Also in Bookworm Adventures as \"Routed\" (unused)."]
    ] },
    { name: "Bookworm Adventures leftovers", sounds: [
      ["sounds/bat_death_01.ogg", "Bat death 1 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Bat death 1: attack or hit of Mama Roc (Arena too)."],
      ["sounds/bowfiring.ogg", "Bow firing (unused)", "Never played: listed as SOUND7 in book6.xml, but no script asks for it. Also in Bookworm Adventures as Bow firing: opening sound of Unliving Archer."],
      ["sounds/codex_flinch.ogg", "Codex flinch (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Codex flinch: attack or hit of Codex."],
      ["sounds/codex_laugh.ogg", "Codex laugh (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Codex laugh: a Codex animation of the last chapter laughs."],
      ["sounds/codex_nyah.ogg", "Codex nyah (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Codex nyah: a Codex animation of the last chapter taunts."],
      ["sounds/dragon_roar.ogg", "Dragon roar (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Dragon roar: opening sound of Kraken, Hydra, Basilisk and 2 more."],
      ["sounds/eagle_cry_01.ogg", "Eagle cry 1 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Eagle cry 1: opening sound of Enchanted Eagle, Griffon, Bronze Stymphalian and 3 more."],
      ["sounds/elephant_01.ogg", "Elephant 1 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Elephant 1: opening sound of Elephant, Mirage Skeleton."],
      ["sounds/elephant_death_01.ogg", "Elephant death 1 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Elephant death 1: opening sound of Bull Elephant."],
      ["sounds/elephant_death_02.ogg", "Elephant death 2 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Elephant death 2: death sound of Elephant."],
      ["sounds/elephant_death_03.ogg", "Elephant death 3 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Elephant death 3: death sound of Bull Elephant."],
      ["sounds/kubwa_barh.ogg", "Kubwa barh (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Kubwa barh: attack or hit of Thieves 12-14 (Arena too)."],
      ["sounds/kubwa_greeting.ogg", "Kubwa greeting (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Kubwa greeting: used in the Arena fights."],
      ["sounds/kubwa_greeting2.ogg", "Kubwa greeting 2 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Kubwa greeting 2: opening sound of Thieves 12-14 (Arena too)."],
      ["sounds/lex_autological.ogg", "\"Autological\" (unused)", "Never played: loaded as LEX_AUTOLOGICAL, but no code or script asks for it. The same recording as sounds/v_autological.ogg here, also never played. Also in Bookworm Adventures as \"Autological\": Lex says it in the animation of the Codex fight (and an unused copy, sounds/lex_autological.ogg)."],
      ["sounds/lexbeam.ogg", "Lex beam (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Lex beam: Lex's beam animation in the fight with the Codex."],
      ["sounds/moan_01.ogg", "Moan 1 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Moan 1: death sound of Bandit, Sword Swallower, Highwayman."],
      ["sounds/nemesis_laugh.ogg", "Nemesis laugh (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Nemesis laugh: the animation of Cassandra and the Nemesis laughs."],
      ["sounds/pulsing_attack_01.ogg", "Pulsing attack 1 (unused)", "Never played: listed as PULSING_ATTACK in book5.xml, but no script asks for it. Also in Bookworm Adventures as Pulsing attack 1: attack or hit of Circe, Dryad, Helead and 3 more (Arena too)."],
      ["sounds/roar_01.ogg", "Roar 1 (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. Also in Bookworm Adventures as Roar 1: opening sound of Charybdis, Enchanted Lion, Phantom and 2 more."],
      ["sounds/sloooowdown.ogg", "Sloooowdown (unused)", "Never played: loaded as SLOWDOWN, but no code or script asks for it. Also in AstroPop as Slow down: the atomic pilot's power. Also in Bookworm Adventures as Slow down: a hidden word is spelled outside the mini-games."],
      ["sounds/v_autological.ogg", "\"Autological\" (unused)", "Never played: the file is not in the game's resource list, so nothing loads it. The same recording as sounds/lex_autological.ogg here, also never played. Also in Bookworm Adventures as \"Autological\": Lex says it in the animation of the Codex fight (and an unused copy, sounds/lex_autological.ogg)."],
      ["sounds/v_clipsandgiggles.ogg", "\"Clips and giggles\" (unused)", "Never played: loaded with the main menu's voices as MM_CNG, but no menu item's reaction uses it. Also in Bookworm Adventures as \"Clips and Giggles\": Lex reacts when the pointer moves onto the film reel item of the main menu."]
    ] }
  ] },
  cosmicbugs: {
    presets: [
      { name: "Kill Chain", when: "Bugs shot one after another", steps: [["audio/explosion1", 0], ["audio/exp300", 0], ["audio/explosion1", 510], ["audio/exp400", 510], ["audio/explosion1", 790], ["audio/exp500", 790], ["audio/explosion1", 1250], ["audio/exp600", 1250], ["audio/powerup", 1800]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["audio/click.wav", "Click", "A menu item that is unlocked is clicked, or auto-clicks; also a click on the board that fires no beam because an enemy is near the ship while the quota is not yet met (that code asks for a sound the game never loads)"]
    ] },
    { name: "The ship and its beams", sounds: [
      ["audio/drill_shoot.wav", "Beam shot", "A click fires the beams (needs the capture quota not met and no enemy near the ship); plays when fewer than 3 beams are out, and starts the 66 ms drill timing"],
      ["audio/drill.wav", "Beam drilling", "While the beams run, every 66 updates (66 ms) after the shot, once for each beam, so the pair plays together"],
      ["audio/clank.wav", "Beam clank", "A beam's head meets a cell that is not open and stops"],
      ["audio/fill.wav", "Fill", "A captured side starts to fill when the beam pair ends; repeats every 120 updates (0.12 s) while fills run on an ordinary board"],
      ["audio/fill_end.wav", "Fill ends", "A fill finishes and exactly one other fill is still running, on an ordinary board (so a pair of fills ends on this sound)"],
      ["audio/explosion2.wav", "Ship explodes", "The ship explodes"]
    ] },
    { name: "Enemies", sounds: [
      ["audio/explosion1.wav", "Enemy explodes", "Any enemy whose definition names this sound dies (snakes and cannon balls included), and a mine, missile or time bomb blows up"],
      ["audio/bump.wav", "Line bump", "A bump roller bounces off a beam's line that has open cells on its side, two filled cells behind it all along and four cells of one kind at both ends; the line is pushed one cell on"],
      ["audio/canon.wav", "Cannon", "A snake shooter fires a snake 50 pixels out once its own delay has passed and the cell there is open; a cannon walker fires a ball 4000 updates (4 s) after its last shot when 30 or more cells ahead are filled"],
      ["audio/laser.wav", "Laser", "A laser walker fires a shot: more than 4000 updates (4 s) since its last, and one of the ship's beams lines up with it"],
      ["audio/beam_hit.wav", "Laser hits beams", "A laser shot touches a beam; the beams burst into sparks"],
      ["audio/electricity.wav", "Electricity", "An enemy not yet riding a beam touches it while the ship is firing, and more than 1000 updates (1 s) have passed since that enemy's last zap"],
      ["audio/ice.wav", "Freezing", "An enemy is frozen in an ice capsule by an ice shot; plays again as the capsule is set up if it is fully opaque. Resembles The Wizard's Pen's audio/ice.ogg, the same sound, mixed differently."]
    ] },
    { name: "Kill scores", sounds: [
      ["audio/exp100.wav", "Kill 100", "A walker or roller dies as the 1st kill of a chain (100 points); the sound is chosen by the chain times 100, and the chain restarts at 1 once the fills end"],
      ["audio/exp200.wav", "Kill 200", "A walker or roller dies as the 2nd kill of a chain (200 points)"],
      ["audio/exp300.wav", "Kill 300", "A walker or roller dies as the 3rd kill of a chain (300 points)"],
      ["audio/exp400.wav", "Kill 400", "A walker or roller dies as the 4th kill of a chain (400 points)"],
      ["audio/exp500.wav", "Kill 500", "A walker or roller dies as the 5th kill of a chain, or in the blast of a mine, missile or time bomb, which always scores 500"],
      ["audio/exp600.wav", "Kill 600", "A walker or roller dies as the 6th kill of a chain (600 points)"],
      ["audio/exp700.wav", "Kill 700", "A walker or roller dies as the 7th kill of a chain (700 points)"],
      ["audio/exp800.wav", "Kill 800", "A walker or roller dies as the 8th kill of a chain (800 points)"],
      ["audio/exp900.wav", "Kill 900", "A walker or roller dies as the 9th kill of a chain (900 points); the tenth and later kills of one chain play the click sound instead, still 900 points (the code asks for sounds the game never loads)"]
    ] },
    { name: "Power-ups and weapons", sounds: [
      ["audio/powerup.wav", "Power-up", "A power-up is caught, except slow shot, hyper motion and flip flopper (every other kind plays this)"],
      ["audio/powerdn.wav", "Power-down", "A slow shot, hyper motion or flip flopper power-up is caught"],
      ["audio/mine.wav", "Mine launched", "A click launches a mine or a time bomb that is not too close to an enemy; a time bomb's lifetime is then set to 2000 updates (2 s)"],
      ["audio/ice_launch.wav", "Ice shot launched", "An ice shot is launched by a click"],
      ["audio/missile.wav", "Missile launched", "A missile is launched by a click"],
      ["audio/fuse.wav", "Time bomb fuse", "A time bomb burns its fuse; plays whenever its age is a multiple of 230 updates (0.23 s)"]
    ] },
    { name: "Unused", sounds: [
      ["audio/spawn.wav", "Spawn (unused)", "Never played: it is loaded (the first level's sound list names it), but no code asks for it"],
      ["audio/clank2.wav", "Clank 2 (unused)", "Never played: no sound list or code names this file, so it is never even loaded"],
      ["audio/purple.wav", "Purple (unused)", "Never played: no sound list or code names this file, so it is never even loaded"],
      ["audio/rail.wav", "Rail (unused)", "Never played: no sound list or code names this file, so it is never even loaded"]
    ] },
  ] },
  dynomite: {
    presets: [
      { name: "Welcome", when: "The main menu opens: the pop-up and the welcome", steps: [["sounds/inchypop", 0], ["sounds/xwelcometodynomite", 500]]},
      { name: "Shot", when: "The slingshot fires and the egg lands", steps: [["sounds/rubbershot", 0], ["sounds/egglands", 350]]},
      { name: "Big Match", when: "A shot breaks five eggs or more: the breaks, the blast and the tank's shout", steps: [["sounds/rubbershot", 0], ["sounds/egglands", 290], ["sounds/breakegg", 290], ["sounds/explosion", 290], ["sounds/charge", 290]]},
      { name: "Combo Chain", when: "A chain of matches climbs to a dynamite, one call a match, and the thunderbolt makes it", steps: [["sounds/xcombo", 0], ["sounds/xsupercombo", 550], ["sounds/xcambriancombo", 1080], ["sounds/xtriassiccombo", 1610], ["sounds/xjurassiccombo", 2230], ["sounds/xcretaceouscombo", 2830], ["sounds/xdynamitecombo", 3360], ["sounds/thunderbolt", 4840]]},
      { name: "Dynamite Blast", when: "A dynamite goes off: the shout and a blast for each cell", steps: [["sounds/explosion", 0], ["sounds/kaboom", 0], ["sounds/explosion", 0], ["sounds/explosion", 0], ["sounds/explosion", 0], ["sounds/explosion", 0], ["sounds/explosion", 0], ["sounds/explosion", 0], ["sounds/explosion", 0], ["sounds/explosion", 0]]},
      { name: "Baby Dinosaur", when: "A baby egg is broken free, then the baby leaves with its points", steps: [["sounds/mama1", 0], ["sounds/reward", 1380]]},
      { name: "Bonus Egg", when: "A shot breaks a bonus egg", steps: [["sounds/egglands", 0], ["sounds/breakegg", 0], ["sounds/explosion", 0], ["sounds/popbonus", 0]]},
      { name: "Score Multiplier", when: "A match while the score multiplier is on", steps: [["sounds/egglands", 0], ["sounds/breakegg", 0], ["sounds/explosion", 0], ["sounds/scoremultiplier", 0], ["sounds/fireinthehole", 0]]},
      { name: "Whirleys Coming", when: "Endless: the timer ticks down, the siren, and the call that the whirleys are on their way", steps: [["sounds/ticktock", 0, {"loop": 1, "end": 2000}], ["sounds/whirleysiren", 2000], ["sounds/xwhirleyscoming1", 3000]]},
      { name: "Whirley Hit", when: "An egg hits a whirley, and the ceiling speeds up", steps: [["sounds/breakegg", 0], ["sounds/whirleyhit", 0]]},
      { name: "Endless Game Over", when: "Endless: the eggs run off the board, then the game-over talk", steps: [["sounds/breakegg", 0], ["sounds/breakegg", 10], ["sounds/breakegg", 20], ["sounds/breakegg", 30], ["sounds/breakegg", 40], ["sounds/breakegg", 50], ["sounds/breakegg", 60], ["sounds/breakegg", 70], ["sounds/breakegg", 80], ["sounds/breakegg", 90], ["sounds/breakegg", 100], ["sounds/breakegg", 110], ["sounds/breakegg", 120], ["sounds/footlands", 160], ["sounds/whirleysiren", 1120], ["sounds/inchypop", 1410], ["sounds/xgameover", 1910]]},
      { name: "The Foot", when: "Stomped: the warnings, the foot on its way, and the stomp", steps: [["sounds/warning", 0], ["sounds/footfallrumble", 1040, {"loop": 1, "end": 1610}], ["sounds/warning", 1250], ["sounds/footfall", 1610]]},
      { name: "Round Cleared", when: "Stomped: a round is cleared, the bonus lines pop up, and the walls move", steps: [["sounds/egglands", 0], ["sounds/breakegg", 0], ["sounds/explosion", 0], ["sounds/chime", 10], ["sounds/inchypop", 1010], ["sounds/inchypop", 1410], ["sounds/inchypop", 1810], ["sounds/inchypop", 2510], ["sounds/grind", 4520, {"loop": 1, "end": 4540}]]},
      { name: "Stomped Game Over", when: "Stomped: the eggs run off the board, then the game-over talk", steps: [["sounds/breakegg", 0], ["sounds/breakegg", 10], ["sounds/breakegg", 20], ["sounds/breakegg", 40], ["sounds/breakegg", 50], ["sounds/breakegg", 60], ["sounds/breakegg", 70], ["sounds/breakegg", 80], ["sounds/breakegg", 90], ["sounds/breakegg", 100], ["sounds/footlands", 130], ["sounds/inchypop", 1380], ["sounds/xgameover", 1880]]},
      { name: "Fragment Found", when: "Fossil Challenge: a fragment's fossil falls, the bonus lines, the new fragment in the museum, and the walls move", steps: [["sounds/chime", 0], ["sounds/inchypop", 1000], ["sounds/inchypop", 1400], ["sounds/inchypop", 1800], ["sounds/inchypop", 2500], ["sounds/fossil", 5590], ["sounds/grind", 9220, {"loop": 1, "end": 10820}]]},
      { name: "Fossil Whole", when: "Fossil Challenge: a fossil's last fragment, and the fossil is whole in the museum", steps: [["sounds/chime", 0], ["sounds/inchypop", 1000], ["sounds/inchypop", 1400], ["sounds/inchypop", 1800], ["sounds/inchypop", 2500], ["sounds/fossil", 5590], ["sounds/chime", 6400], ["sounds/grind", 9180, {"loop": 1, "end": 9500}]]},
      { name: "Rex Attack", when: "Fossil Challenge: the last fossil, the Rex's steps and roar, and the Rex beaten", steps: [["sounds/fossil", 0], ["sounds/chime", 810], ["sounds/footstep", 4550], ["sounds/footstep", 6150], ["sounds/footstep", 7750], ["sounds/footstep", 9350], ["sounds/roar", 9350], ["sounds/chime", 12520]]},
      { name: "Sand Out Strategic", when: "Fossil Challenge, strategic: the timer ticks down, the siren and the warning call, and the eggs come down a row", steps: [["sounds/ticktock", 0, {"loop": 1, "end": 2000}], ["sounds/whirleysiren", 2000], ["sounds/xtimesrunningout", 3000], ["sounds/footfall", 5470]]},
      { name: "Sand Out Panic", when: "Fossil Challenge, panic: the timer ticks down, the siren and the warning call, and the ceiling speeds up", steps: [["sounds/ticktock", 0, {"loop": 1, "end": 2000}], ["sounds/whirleysiren", 2000], ["sounds/xtimesrunningout", 3000], ["sounds/speedup", 5470]]},
      { name: "Fossil Game Over", when: "Fossil Challenge: the eggs run off the board, then the game-over talk", steps: [["sounds/breakegg", 0], ["sounds/breakegg", 10], ["sounds/breakegg", 20], ["sounds/breakegg", 30], ["sounds/breakegg", 40], ["sounds/breakegg", 50], ["sounds/breakegg", 60], ["sounds/breakegg", 70], ["sounds/breakegg", 80], ["sounds/breakegg", 90], ["sounds/breakegg", 110], ["sounds/breakegg", 120], ["sounds/footlands", 150], ["sounds/inchypop", 1400], ["sounds/xgameover", 1900]]},
      { name: "Time Trial Cleared", when: "Time Trial: the last match clears the board, then the talk", steps: [["sounds/egglands", 0], ["sounds/breakegg", 0], ["sounds/explosion", 0], ["sounds/inchypop", 1260], ["sounds/xgameover", 1760]]},
      { name: "Tutorial Step", when: "The tutorial moves on to its next page", steps: [["sounds/inchypop", 0], ["sounds/inchypop", 480]]},
      { name: "Tutorial End", when: "The tutorial ends", steps: [["sounds/inchypop", 0], ["sounds/chime", 0]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed: the intro screen's pages, options, high scores, the side bar, the museum's button, a puzzle in the list"],
      ["sounds/egglands.wav", "Egg lands", "The OK or play button of a page or the title screen is pressed; the tutorial's buttons; also an egg lands on the grid"],
      ["sounds/xwelcometodynomite.ogg", "\"Welcome to Dynomite\"", "The intro screen opens for the first time in a run: the large Inchy pops up 1.25 s in and talks 0.5 s after that (later openings only pop him up, silent)"],
      ["sounds/xgameover.ogg", "\"Game over\"", "A game ends: the eggs have run off the board (right after the foot-lands sound), or Time Trial's last egg is gone; Inchy pops up 1.25 s later and talks 0.5 s after that"],
      ["sounds/inchypop.ogg", "Inchy pops up", "Inchy pops up when his talk countdown reaches 50 (0.5 s before he speaks); a cleared round at 50, 90 and 130 updates (0.5, 0.9, 1.3 s) and again at 200 (2 s, with the bonus points); every tutorial step"],
      ["sounds/whirleyhit.ogg", "Whirley hit", "A whirley is hit by an egg; also Save is pressed when it cannot save, or with no save slot chosen"]
    ] },
    { name: "Shooting and hitting", sounds: [
      ["sounds/rubbershot.ogg", "Rubber shot", "The launcher fires an egg"],
      ["sounds/breakegg.wav", "Egg breaks", "Eggs break: a matching group, eggs past the line, loose eggs dropping, or a flying egg hits"],
      ["sounds/explosion.ogg", "Explosion", "A group of three or more breaks, or a dynamite or blast egg goes off"],
      ["sounds/popbonus.ogg", "Bonus pops out", "A broken egg releases a bonus item that flies to the power-up display"],
      ["sounds/mama1.ogg", "Mama", "A broken egg releases a baby dinosaur (a 1 in 2 draw between the game's two loads of this same file, so it always sounds the same)"],
      ["sounds/reward.ogg", "Baby leaves", "A baby dinosaur effect ends as it leaves the screen"],
      ["sounds/scoremultiplier.ogg", "Score multiplier", "A group of eggs breaks while a double, triple or quadruple multiplier is active; a power-up reaches its place"],
      ["sounds/blastit.ogg", "\"Blast it!\"", "A shot breaks five or more eggs (unless the eggs are past the line and losing): a 1 in 4 draw among the tank's four voices; the 40-update flinch happens for any hit"],
      ["sounds/charge.ogg", "Charge voice", "A shot breaks five or more eggs (unless the eggs are past the line and losing): a 1 in 4 draw among the tank's four voices"],
      ["sounds/kaboom.ogg", "Kaboom voice", "Always when a blast egg or dynamite goes off (unless the eggs are past the line and losing); also a 1 in 4 draw when a shot breaks five or more eggs"],
      ["sounds/fireinthehole.ogg", "\"Fire in the hole!\"", "A shot breaks five or more eggs (unless the eggs are past the line and losing): a 1 in 4 draw among the tank's four voices"]
    ] },
    { name: "Combos", sounds: [
      ["sounds/xcombo.ogg", "\"Combo!\"", "Inchy calls a chain of 1"],
      ["sounds/xsupercombo.ogg", "\"Super combo!\"", "Inchy calls a chain of 2"],
      ["sounds/xcambriancombo.ogg", "\"Cambrian combo!\"", "Inchy calls a chain of 3"],
      ["sounds/xtriassiccombo.ogg", "\"Triassic combo\"", "Inchy calls a chain of 4"],
      ["sounds/xjurassiccombo.ogg", "\"Jurassic combo\"", "Inchy calls a chain of 5"],
      ["sounds/xcretaceouscombo.ogg", "\"Cretaceous combo\"", "Inchy calls a chain of 6"],
      ["sounds/xdynamitecombo.ogg", "\"Dynamite combo\"", "Inchy calls a chain of 7; the launcher then gets a stick of dynamite"],
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
      ["sounds/xwhirleyscoming1.ogg", "\"Duhh... Whirley's coming\"", "Endless: 2 seconds before the hourglass runs out Kroney calls; this and the second call alternate, one per hourglass"],
      ["sounds/xwhirleyscoming2.ogg", "\"Doy... Whirley's coming\"", "Endless: 2 seconds before the hourglass runs out, on every second hourglass (the other calls alternate with the first)"],
      ["sounds/xtimesrunningout.ogg", "\"Time's running out!\"", "Fossil Challenge (strategic and panic): 2 seconds before the hourglass runs out, once per hourglass"]
    ] },
    { name: "Fossils and the museum", sounds: [
      ["sounds/chime.ogg", "Chime", "A fossil falls into place; a board is cleared; the museum's tyrannosaurus is out; a fossil is placed in the museum"],
      ["sounds/fossil.ogg", "Fossil", "The museum's fossil fragments start to fade in at a round's end"],
      ["sounds/footstep.ogg", "Footstep", "The museum's tyrannosaurus takes a step out of the wall, every 160 updates (1.6 s), each time his steps count down"],
      ["sounds/roar.ogg", "Roar", "The tyrannosaurus roars after his last step"]
    ] },
    { name: "Unused", sounds: [
      ["sounds/explode.ogg", "Explode (unused)", "Never played: the game never loads this file, and the explosion uses the other explosion sound. Resembles Seven Seas' Small explosion (sounds/explode.ogg), the same sound, mixed differently."],
      ["sounds/mama2.ogg", "Mama 2 (unused)", "Never played: the game loads the first mama file twice instead, so this one is never loaded"]
    ] },
  ] },
  mahjongchina: {
    // presets from the sound logs of the Mahjong session's driven runs (5 ms an update; mahjong-soundlogs/INDEX.md)
    presets: [
      { name: "Pair", when: "A free tile is selected, then its partner", steps: [["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 210]]},
      { name: "Not Free", when: "A pair is taken, then a covered tile is clicked", steps: [["Resource.dll|pe:WAVE:176", 0], ["Resource.dll|pe:WAVE:174", 205]]},
      { name: "Bonus", when: "A pair that matches the tray's tile", steps: [["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:352", 205], ["Resource.dll|pe:WAVE:176", 205]]},
      { name: "Gold Match", when: "A gold pair is matched", steps: [["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:377", 205]]},
      { name: "Level Clear", when: "The last tiles have flown off; the map comes back", steps: [["Resource.dll|pe:WAVE:373", 0], ["Resource.dll|pe:WAVE:340", 585], ["Resource.dll|pe:WAVE:381", 1575]]},
      { name: "Key", when: "A key tile is selected, matched and joins its lock", steps: [["Resource.dll|pe:WAVE:378", 0, {"loop": 1, "end": 205}], ["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 205], ["Resource.dll|pe:WAVE:379", 615]]},
      { name: "Magnet", when: "A magnet tile is selected and matched", steps: [["Resource.dll|pe:WAVE:348", 0, {"loop": 1, "end": 205}], ["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 205]]},
      { name: "Joker", when: "A joker tile is selected and matched", steps: [["Resource.dll|pe:WAVE:347", 0, {"loop": 1, "end": 205}], ["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 205]]},
      { name: "Undo", when: "The Undo button", steps: [["Resource.dll|pe:WAVE:105", 0], ["Resource.dll|pe:WAVE:351", 0]]},
      { name: "Shuffle", when: "The Shuffle button", steps: [["Resource.dll|pe:WAVE:105", 0], ["Resource.dll|pe:WAVE:349", 0]]},
      { name: "Ending", when: "The last stage's text goes, then the ending's fanfare", steps: [["Resource.dll|pe:WAVE:382", 0], ["Resource.dll|pe:WAVE:373", 2300]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["Resource.dll|pe:WAVE:105", "Click", "A button, slider or list row is pressed; also a level clicked on the map, a layout slot clicked, and a double-click on the background picture or the style choice"]
    ] },
    { name: "Tiles", sounds: [
      ["Resource.dll|pe:WAVE:175", "Tile selected", "A click on a free tile that can pair makes it the selected one, or deselects it when it was already selected"],
      ["Resource.dll|pe:WAVE:174", "Tile blocked", "A click that hits no tile that can be selected and paired (a covered tile or empty space), while no tiles are flying in or out"],
      ["Resource.dll|pe:WAVE:176", "Pair matched", "Two matching tiles are taken, including a key with its lock and a joker or magnet with its partner; not for a gold pair"],
      ["Resource.dll|pe:WAVE:349", "Shuffle", "A shuffle starts, from the Shuffle button or the shuffle choice of the no-moves dialog"],
      ["Resource.dll|pe:WAVE:350", "Deal", "A game's board is dealt, when a level or layout game starts or is restarted; all other sounds are stopped first"],
      ["Resource.dll|pe:WAVE:351", "Undo", "A move is taken back, from the Undo button or the undo choice of the no-moves dialog; nothing plays when no move has been made"]
    ] },
    { name: "Special tiles", sounds: [
      ["Resource.dll|pe:WAVE:347", "Joker loop", "A joker tile is selected; it loops until the tile is paired, deselected or the selection is cleared"],
      ["Resource.dll|pe:WAVE:348", "Magnet loop", "A magnet tile is selected; it loops until the tile is paired, deselected or the selection is cleared"],
      ["Resource.dll|pe:WAVE:378", "Key loop", "A key tile is selected; it loops until the tile is paired, deselected or the selection is cleared"],
      ["Resource.dll|pe:WAVE:379", "Key joins lock", "A key's flight onto its lock ends and the two join"],
      ["Resource.dll|pe:WAVE:377", "Gold match", "The two gold tiles are paired, which ends the level in the map game"],
      ["Resource.dll|pe:WAVE:376", "Fly-off loop", "The gold pair's flight ends with more than 2 tiles left on the board; it loops as the rest fly off, until the level-complete dialog opens"],
      ["Resource.dll|pe:WAVE:352", "Bonus", "A normal pair is taken that matches the tile shown in the tray and scores points, adding the 500-point bonus"]
    ] },
    { name: "Map", sounds: [
      ["Resource.dll|pe:WAVE:340", "Map curtain", "The map screen comes back after a level or layout game, with its curtain closing; also when a level is started from the map, with the curtain opening"],
      ["Resource.dll|pe:WAVE:381", "Map move", "The map screen comes back after a level that unlocked the next one and the map marker moves on, unless that level ended its stage"],
      ["Resource.dll|pe:WAVE:380", "New stage", "The map screen comes back after the level that ended a stage, and the stage-completed text fades in"],
      ["Resource.dll|pe:WAVE:382", "Stage text gone", "The stage-completed text, held for 3000 milliseconds (3 seconds) once faded in, has faded out"]
    ] },
    { name: "Level end", sounds: [
      ["Resource.dll|pe:WAVE:375", "Layout result", "The result dialog opens at the end of a layout game, once the last tiles are gone"],
      ["Resource.dll|pe:WAVE:373", "Fanfare", "The level-complete dialog opens after a map level (its gold tiles in the tray); also the ending screen opens, 1 second after the map's final move or from the map's ending button"]
    ] },
    { name: "Unused", sounds: [
      ["Resource.dll|pe:WAVE:106", "MP3 resource 106 (unused)", "Never played: its code never refers to the resource"]
    ] },
  ] },
  mahjongjapan: {
    // presets from the sound logs of the Mahjong session's driven runs (5 ms an update; mahjong-soundlogs/INDEX.md)
    presets: [
      { name: "Pair", when: "A free tile is selected, then its partner", steps: [["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 210]]},
      { name: "Not Free", when: "A pair is taken, then a covered tile is clicked", steps: [["Resource.dll|pe:WAVE:176", 0], ["Resource.dll|pe:WAVE:174", 205]]},
      { name: "Bonus", when: "A pair that matches the tray's tile", steps: [["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:352", 205], ["Resource.dll|pe:WAVE:176", 205]]},
      { name: "Gold Match", when: "A gold pair is matched", steps: [["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:377", 205]]},
      { name: "Level Clear", when: "The last tiles have flown off; the map comes back", steps: [["Resource.dll|pe:WAVE:373", 0], ["Resource.dll|pe:WAVE:340", 485], ["Resource.dll|pe:WAVE:381", 1475]]},
      { name: "Key", when: "A key tile is selected, matched and joins its lock", steps: [["Resource.dll|pe:WAVE:378", 0, {"loop": 1, "end": 205}], ["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 205], ["Resource.dll|pe:WAVE:379", 610]]},
      { name: "Magnet", when: "A magnet tile is selected and matched", steps: [["Resource.dll|pe:WAVE:348", 0, {"loop": 1, "end": 205}], ["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 205]]},
      { name: "Joker", when: "A joker tile is selected and matched", steps: [["Resource.dll|pe:WAVE:347", 0, {"loop": 1, "end": 205}], ["Resource.dll|pe:WAVE:175", 0], ["Resource.dll|pe:WAVE:176", 205]]},
      { name: "Undo", when: "The Undo button", steps: [["Resource.dll|pe:WAVE:105", 0], ["Resource.dll|pe:WAVE:351", 0]]},
      { name: "Shuffle", when: "The Shuffle button", steps: [["Resource.dll|pe:WAVE:105", 0], ["Resource.dll|pe:WAVE:349", 0]]},
      { name: "Ending", when: "The last stage's text goes, then the ending's fanfare", steps: [["Resource.dll|pe:WAVE:382", 0], ["Resource.dll|pe:WAVE:373", 2300]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["Resource.dll|pe:WAVE:105", "Click", "A button, slider or list row is pressed; also a level clicked on the map, a layout slot clicked, and a double-click on the background picture or the style choice"]
    ] },
    { name: "Tiles", sounds: [
      ["Resource.dll|pe:WAVE:175", "Tile selected", "A click on a free tile that can pair makes it the selected one, or deselects it when it was already selected"],
      ["Resource.dll|pe:WAVE:174", "Tile blocked", "A click that hits no tile that can be selected and paired (a covered tile or empty space), while no tiles are flying in or out"],
      ["Resource.dll|pe:WAVE:176", "Pair matched", "Two matching tiles are taken, including a key with its lock and a joker or magnet with its partner; not for a gold pair"],
      ["Resource.dll|pe:WAVE:349", "Shuffle", "A shuffle starts, from the Shuffle button or the shuffle choice of the no-moves dialog"],
      ["Resource.dll|pe:WAVE:350", "Deal", "A game's board is dealt, when a level or layout game starts or is restarted; all other sounds are stopped first"],
      ["Resource.dll|pe:WAVE:351", "Undo", "A move is taken back, from the Undo button or the undo choice of the no-moves dialog; nothing plays when no move has been made"]
    ] },
    { name: "Special tiles", sounds: [
      ["Resource.dll|pe:WAVE:347", "Joker loop", "A joker tile is selected; it loops until the tile is paired, deselected or the selection is cleared"],
      ["Resource.dll|pe:WAVE:348", "Magnet loop", "A magnet tile is selected; it loops until the tile is paired, deselected or the selection is cleared"],
      ["Resource.dll|pe:WAVE:378", "Key loop", "A key tile is selected; it loops until the tile is paired, deselected or the selection is cleared"],
      ["Resource.dll|pe:WAVE:379", "Key joins lock", "A key's flight onto its lock ends and the two join"],
      ["Resource.dll|pe:WAVE:377", "Gold match", "The two gold tiles are paired, which ends the level in the map game"],
      ["Resource.dll|pe:WAVE:376", "Fly-off loop", "The gold pair's flight ends with more than 2 tiles left on the board; it loops as the rest fly off, until the level-complete dialog opens"],
      ["Resource.dll|pe:WAVE:352", "Bonus", "A normal pair is taken that matches the tile shown in the tray and scores points, adding the 500-point bonus"]
    ] },
    { name: "Map", sounds: [
      ["Resource.dll|pe:WAVE:340", "Map curtain", "The map screen comes back after a level or layout game, with its curtain closing; also when a level is started from the map, with the curtain opening"],
      ["Resource.dll|pe:WAVE:381", "Map move", "The map screen comes back after a level that unlocked the next one and the map marker moves on, unless that level ended its stage"],
      ["Resource.dll|pe:WAVE:380", "New stage", "The map screen comes back after the level that ended a stage, and the stage-completed text fades in"],
      ["Resource.dll|pe:WAVE:382", "Stage text gone", "The stage-completed text, held for 3000 milliseconds (3 seconds) once faded in, has faded out"]
    ] },
    { name: "Level end", sounds: [
      ["Resource.dll|pe:WAVE:375", "Layout result", "The result dialog opens at the end of a layout game, once the last tiles are gone"],
      ["Resource.dll|pe:WAVE:373", "Fanfare", "The level-complete dialog opens after a map level (its gold tiles in the tray); also the ending screen opens, 1 second after the map's final move or from the map's ending button"]
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
      ["sounds/click.ogg", "Click", "Any button of the menus, options dialog, map, board or other dialogs is pressed (the board's buttons all share this sound)"],
      ["sounds/tombslide.ogg", "Tomb slide", "The title screen opens fresh; also 75 updates (0.75 seconds) after a level begins, except in the two ending modes"]
    ] },
    { name: "Explorer", sounds: [
      ["sounds/expwalk60a.ogg", "Explorer step 60 A", "The explorer starts a step (also when walking into a level) at the slowest speed setting; one of the A and B pair at random, panned to the column"],
      ["sounds/expwalk60b.ogg", "Explorer step 60 B", "The explorer starts a step (also when walking into a level) at the slowest speed setting; one of the A and B pair at random, panned to the column"],
      ["sounds/expwalk30a.ogg", "Explorer step 30 A", "The explorer starts a step at the middle speed setting; one of the A and B pair at random, panned"],
      ["sounds/expwalk30b.ogg", "Explorer step 30 B", "The explorer starts a step at the middle speed setting; one of the A and B pair at random, panned"],
      ["sounds/expwalk15a.ogg", "Explorer step 15 A", "The explorer starts a step at the fastest speed setting; one of the A and B pair at random, panned"],
      ["sounds/expwalk15b.ogg", "Explorer step 15 B", "The explorer starts a step at the fastest speed setting; one of the A and B pair at random, panned"]
    ] },
    { name: "Mummies and scorpions", sounds: [
      ["sounds/mumwalk60a.ogg", "Mummy step 60 A", "A mummy starts its move at the slowest speed setting, once per turn however many steps it takes; one of the A and B pair at random, panned"],
      ["sounds/mumwalk60b.ogg", "Mummy step 60 B", "A mummy starts its move at the slowest speed setting, once per turn however many steps it takes; one of the A and B pair at random, panned"],
      ["sounds/mumwalk30a.ogg", "Mummy step 30 A", "A mummy starts its move at the middle speed setting, once per turn; one of the A and B pair at random, panned"],
      ["sounds/mumwalk30b.ogg", "Mummy step 30 B", "A mummy starts its move at the middle speed setting, once per turn; one of the A and B pair at random, panned"],
      ["sounds/mumwalk15a.ogg", "Mummy step 15 A", "A mummy starts its move at the fastest speed setting, once per turn; one of the A and B pair at random, panned"],
      ["sounds/mumwalk15b.ogg", "Mummy step 15 B", "A mummy starts its move at the fastest speed setting, once per turn; one of the A and B pair at random, panned"],
      ["sounds/scorpwalk1.ogg", "Scorpion step 1", "The scorpion starts a move to another cell; one of the two scorpion steps at random, panned"],
      ["sounds/scorpwalk2.ogg", "Scorpion step 2", "The scorpion starts a move to another cell; one of the two scorpion steps at random, panned"],
      ["sounds/pummel.mp3", "Pummel", "A mummy catches the explorer or fights a scorpion, or a scorpion catches the explorer or walks onto a living mummy; panned to the cell"]
    ] },
    { name: "The maze", sounds: [
      ["sounds/gate.ogg", "Gate", "The explorer or a mummy steps onto the key cell and the gate flips open or closed; panned to the gate"],
      ["sounds/block.ogg", "Block falls", "35 updates (0.35 seconds) after the explorer's death by a trap of the block kind begins; panned to the explorer"],
      ["sounds/pit.ogg", "Pit", "3 updates (0.03 seconds) after the explorer's death by a trap of the pit kind begins; panned to the explorer"],
      ["sounds/poison.ogg", "Poison", "The scorpion's last move of a turn ends without catching the explorer or a mummy; panned to the explorer"],
      ["sounds/badankh.ogg", "Bad ankh", "After a turn in a maze smaller than 7, a solvable position turns unsolvable and the ankh option is on"]
    ] },
    { name: "Level end", sounds: [
      ["sounds/finishedlevel.ogg", "Level finished", "The explorer stands on the exit and starts to walk out; not while a solution is being shown and not in the tutorial or later modes"],
      ["sounds/mummyhowl.ogg", "Mummy howl", "The explorer is lost; also 165 updates (1.65 seconds) after a level begins, except in the two ending modes"],
      ["sounds/opentreasure.ogg", "Open treasure", "The ending of an adventure or classic game opens and its fade begins, with the board ready"]
    ] },
  ] },
  ningpo: {
    presets: [
      { name: "You Win", when: "The last pair is taken", steps: [["ningpo.pyz|zip:nm_youwin.ogg", 0]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["ningpo.pyz|zip:nm_splash.ogg", "Splash", "The game starts, as its screen opens and before its files load"],
      ["ningpo.pyz|zip:nm_guiclick.wav", "Screen click", "A button of a screen or dialog is pressed (exit, pause, options, new game, rules, the title's start arrow and the end screen's arrow, the dialogs' own buttons); also any click while the game is paused"]
    ] },
    { name: "Tiles", sounds: [
      ["ningpo.pyz|zip:nm_click.wav", "Tile selected", "A click on a free tile when none is selected, in a game in progress; also an arrow of the title screen's settings is clicked"],
      ["ningpo.pyz|zip:nm_unclick.wav", "Tile deselected", "The selected tile is clicked again and deselected"],
      ["ningpo.pyz|zip:nm_blocked.ogg", "Blocked tile", "A click on a tile that is not free, or on a free tile of another kind than the selected one (which also drops the selection)"],
      ["ningpo.pyz|zip:nm_match.ogg", "Pair matched", "A pair is taken while tiles are left and a move still exists; the last pair and a pair that leaves no move play their ending sounds instead"],
      ["ningpo.pyz|zip:nm_youwin.ogg", "You win", "The last pair is taken and the game is won"],
      ["ningpo.pyz|zip:nm_gameover.ogg", "Game over", "A pair is taken that leaves tiles but no move, and the game is lost"]
    ] },
    { name: "Buttons", sounds: [
      ["ningpo.pyz|zip:nm_hint.ogg", "Hint, shuffle, undo", "The hint (button, or the F or H key), the shuffle (button, S key or the no-moves dialog) or the Undo button is used while a game is on and not paused; the U key undoes without a sound"]
    ] },
  ] },
  noahsark: {
    presets: [
      { name: "Pair Saved", when: "A pair of animals boards the ark", steps: [["sounds/pop", 0], ["sounds/combo1", 0], ["sounds/winpig", 70]]},
      { name: "Third Combo", when: "A third pair of one kind in a row", steps: [["sounds/combo3", 0], ["sounds/pop", 0], ["sounds/winpig", 70]]},
      { name: "Flood", when: "The water rises to the top", steps: [["sounds/warning", 0], ["sounds/thunder4", 360], ["sounds/warning", 1200], ["sounds/warning", 2400], ["sounds/warning", 3000], ["sounds/warning", 3600], ["sounds/warning", 4200], ["sounds/warning", 4800], ["sounds/thunder4", 4920], ["sounds/warning", 5400], ["sounds/thunder3", 5640], ["sounds/gameover", 10060]]}
    ],
    groups: [
    { name: "Menus and dialogs", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed anywhere in the game; also the tutorial dialog's two demonstration clicks"],
      ["sounds/levelupvoice.ogg", "\"Level up!\"", "The next-level screen opens"]
    ] },
    { name: "Picking animals", sounds: [
      ["sounds/select.ogg", "Select", "An animal is picked, an animal is joined to the picked one by a click or by the mouse dragging on, or a Puzzle animal is picked"],
      ["sounds/deselect.ogg", "Deselect", "The picked animal is put back (a right click or other click that cannot pick or join, outside Stampede), a Puzzle swap is undone, or the undo mod is used"],
      ["sounds/fallingclick.ogg", "Landing click", "Animals land after falling or refilling: a falling animal reaches the floor or one below it, or a refilling one lands 1 time in 1 to 6 at random; at most once per update"]
    ] },
    { name: "Pairs and combos", sounds: [
      ["sounds/pop.ogg", "Pop", "A pair is joined, a Stampede group is taken, a run of three is cleared after a Puzzle swap or when the board settles; plays together with a combo sound or the animal's voice"],
      ["sounds/combo1.ogg", "Combo 1", "A pair is joined whose animal kind differs from the last pair's, which restarts the combo count (plays with the pop)"],
      ["sounds/combo2.ogg", "Combo 2", "A pair is joined: the 2nd pair in a row of the same animal kind (plays with the pop)"],
      ["sounds/combo3.ogg", "Combo 3", "A pair is joined: the 3rd pair in a row of the same animal kind (plays with the pop)"],
      ["sounds/combo4.ogg", "Combo 4", "A pair is joined: the 4th pair in a row of the same animal kind (plays with the pop)"],
      ["sounds/combo5.ogg", "Combo 5", "A pair is joined: the 5th pair in a row of the same animal kind (plays with the pop)"],
      ["sounds/combo6.ogg", "Combo 6", "A pair is joined: the 6th pair in a row of the same animal kind (plays with the pop)"],
      ["sounds/combo7.ogg", "Combo 7", "A pair is joined: the 7th pair in a row of the same animal kind (plays with the pop)"],
      ["sounds/combo8.ogg", "Combo 8", "A pair is joined: the 8th or any later pair in a row of the same animal kind (plays with the pop)"]
    ] },
    { name: "Animal voices", sounds: [
      ["sounds/winpig.ogg", "Pig voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winlion.ogg", "Lion voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winfrog.ogg", "Frog voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winelephant.ogg", "Elephant voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in; also the tutorial's last step"],
      ["sounds/wincow.ogg", "Cow voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winhippo.ogg", "Hippo sound", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winzebra.ogg", "Zebra voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winmonkey.ogg", "Monkey voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winmouse.ogg", "Mouse voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/winbeaver.ogg", "Beaver voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/wingator.ogg", "Gator voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/windog.ogg", "Dog voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"],
      ["sounds/wincat.ogg", "Cat voice", "This animal is saved: a pair meets (not while the leftover animals are being abandoned), a Stampede group is taken, or a Puzzle run of three is found; also 1 time in 1750 each update for each animal of this kind waiting above the board to drop in"]
    ] },
    { name: "Water and the ark", sounds: [
      ["sounds/getready.ogg", "\"Get ready\"", "A level starts (the board is set up)"],
      ["sounds/thunder3.ogg", "Thunder 3", "Lightning starts: checked every 12 updates (0.12 seconds) while playing in any mode but Strategy, 1 to 20 times in 250 as the water nears the ark (none while it is low); one of the two thunders, at random"],
      ["sounds/thunder4.ogg", "Thunder 4", "Lightning starts: checked every 12 updates (0.12 seconds) while playing in any mode but Strategy, 1 to 20 times in 250 as the water nears the ark (none while it is low); one of the two thunders, at random"],
      ["sounds/warning.ogg", "Warning", "While playing, checked every 12 updates: every 120 updates (1.2 seconds) while the water is within 20 of the ark's level, every 60 (0.6 seconds) within 10; also for each animal abandoned in Strategy mode"],
      ["sounds/abandon.ogg", "Abandon", "In Strategy or Action mode, when no move is left and animals remain: every 78 updates (0.78 seconds) a pair is abandoned and points are taken off"]
    ] },
    { name: "Round end", sounds: [
      ["sounds/levelup.au", "Level up", "A new row is pushed in while the top row is not empty, and the round is lost; or, outside Strategy and Puzzle, the board is cleared before the water reaches the ark"],
      ["sounds/gameover.ogg", "\"Game over!\"", "The round ends with animals left behind (or in Strategy mode) and the ark has sailed off, outside a bonus round; the high score box opens"]
    ] },
  ] },
  pixelus: {
    presets: [
      { name: "Island Shore", when: "The island map is open", steps: [["sounds/Island - Lightning 03d", 0], ["sounds/Island - Ocean Surf 04b", 2100, {"v": -549}], ["sounds/Seagull 04b", 3300, {"v": -549}], ["sounds/Seagull 05", 3570, {"v": -549}], ["sounds/Island - Ocean Surf 04b", 10100, {"v": -549}], ["sounds/Seagull 05", 15730, {"v": -549}], ["sounds/Island - Lightning 03d", 16830, {"v": -304}], ["sounds/Island - Lightning 03d", 17730, {"v": -361}], ["sounds/Seagull 04b", 17840, {"v": -549}], ["sounds/Island - Ocean Surf 04b", 18100, {"v": -549}]]},
      { name: "Temple Entry", when: "A temple is chosen on the island", steps: [["sounds/UI Island Click 01", 0, {"v": -549}], ["sounds/Pixelus Inside Temple v01", 0, {"v": -549}], ["sounds/Temple Mouseover 01", 0, {"v": -549}]]},
      { name: "Puzzle Start", when: "A puzzle is chosen in a temple", steps: [["sounds/Temple Mouseover 01", 0, {"v": -549}], ["sounds/Pixelus Start of Puzzle v02", 10, {"v": -549}]]},
      { name: "Puzzle Solved", when: "The last tile reaches its target", steps: [["sounds/Tile Moving 02d", 0, {"v": -549}], ["sounds/Pixelus Puzzle Completed Sting v01", 10, {"v": -549}], ["sounds/Tile Target 02c", 120, {"v": -549}]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["sounds/ui menu mouseover 02.ogg", "Menu hover", "The mouse comes onto a main menu button"],
      ["sounds/ui menu click 01c.ogg", "Menu click", "A main menu button is pressed (on the mouse going down)"],
      ["sounds/ui little beep 05b.ogg", "Little beep", "A button of the game's own style without a sound of its own is pressed (Continue, OK and the like), or a dialog's Yes or No. Also in Hammer Heads Deluxe, as sounds/UI - Check.ogg."],
      ["sounds/ui general click 02.ogg", "General click (unused)", "Never played: it is in the sound table, but no code asks for it"]
    ] },
    { name: "Island map", sounds: [
      ["sounds/ui island mouseover 02.ogg", "Island hover", "The mouse has rested on the same temple on the island for 50 updates (0.5 seconds); played once, with its pitch set by which temple it is"],
      ["sounds/ui island click 01.ogg", "Island click", "The mouse comes onto a temple's button on the island"],
      ["sounds/pixelus temple screen v01.ogg", "Temple opens", "The first time one of the eight temples shows on the island map for the player, as its door opens. Also in Hammer Heads Deluxe, as sounds/Sting - Gnome King.ogg."],
      ["sounds/seagull 05.ogg", "Seagull 1", "While the island map is open: 350 to 449 updates (3.5 to 4.5 seconds) after it opens, then every 600 to 5599 updates (6 to 56 seconds), at random"],
      ["sounds/seagull 04b.ogg", "Seagull 2", "While the island map is open: 400 to 499 updates (4 to 5 seconds) after it opens, then every 600 to 3599 updates (6 to 36 seconds), at random"],
      ["sounds/island - ocean surf 04b.ogg", "Ocean surf", "While the island map is open, every 800 updates (8 seconds): on every second wave cycle of the sea"],
      ["sounds/island - lightning 02c.ogg", "Lightning 1", "A lightning flash on the island map: the first 0 to 5 seconds after it opens, then 10 updates plus 0 to 35 seconds after the last; one of the two lightning sounds at random, at a random volume"],
      ["sounds/island - lightning 03d.ogg", "Lightning 2", "A lightning flash on the island map: the first 0 to 5 seconds after it opens, then 10 updates plus 0 to 35 seconds after the last; one of the two lightning sounds at random, at a random volume. Also in Hammer Heads Deluxe, as sounds/Gnome King - Flashing.ogg."]
    ] },
    { name: "Temples", sounds: [
      ["sounds/pixelus inside temple v01.ogg", "Inside the temple", "A temple's puzzle screen opens"],
      ["sounds/temple mouseover 01.ogg", "Puzzle hover", "The mouse comes onto a different puzzle's picture in a temple"],
      ["sounds/chain breaking 01.ogg", "Chain breaks", "A temple's puzzle screen opens with every puzzle but the last solved, the first time only; the chain on the last puzzle breaks"],
      ["sounds/pixelus hint screen v01.ogg", "Hint screen (unused)", "Never played: it is in the sound table, but no code asks for it"],
      ["sounds/hint popup 01.ogg", "Hint popup (unused)", "Never played: it is in the sound table, but no code asks for it. Also in Hammer Heads Deluxe, as sounds/Hint Popup 01.ogg and sounds/UI - Target Over Gnome.ogg."]
    ] },
    { name: "Puzzles", sounds: [
      ["sounds/pixelus start of puzzle v02.ogg", "Puzzle starts", "The first update of a puzzle once the help screen is gone (it comes with the music of the puzzles, 150 updates later). Also in Hammer Heads Deluxe, as sounds/UI - Time Bonus.ogg."],
      ["sounds/tile moving 02d.ogg", "Tile moves", "A tile is thrown in or pulled back, an undo or redo of either, and six times in each 950 updates of the help screen's demonstration (at 50, 150, 250, 500, 600 and 700)"],
      ["sounds/tile target 02c.ogg", "Tile on target", "A tile comes to rest on a wanted cell (a start spot adds this sound to the other one); also 11 updates after each of the help demonstration's three throws"],
      ["sounds/tile non-target 02b.ogg", "Tile off target", "A tile comes to rest on a cell that is not wanted"],
      ["sounds/illegal blocked 03.ogg", "Blocked", "A click on an empty cell of the grid from which no tile can be thrown"],
      ["sounds/illegal fixed 01c.ogg", "Fixed tile", "A click on a fixed tile, which cannot be pulled"],
      ["sounds/illegal nothing 01.ogg", "Tile stuck", "A click on a loose tile that cannot be pulled back"],
      ["sounds/illegal none left 03b.ogg", "No tiles left", "A throw is tried when none can be made: no tile left in the supply, or the way is blocked with no slide out"]
    ] },
    { name: "Completion", sounds: [
      ["sounds/pixelus puzzle completed sting v01.ogg", "Puzzle done sting", "Every wanted cell holds a tile, which completes the puzzle (once per puzzle); the music fades out"],
      ["sounds/pixelus temple completed sting v01.ogg", "Temple done sting", "The completion screen opens after a temple's last puzzle (any completion mode but 2 and 4)"],
      ["sounds/pixelus game completed sting v01.ogg", "Game done sting", "The completion screen opens in completion mode 2 or 4"]
    ] },
  ] },
  rocketmania: {
    presets: [
      { name: "Rocket Launch", when: "A fuse reaches a rocket", steps: [["sounds/Fuse Burning 01d short", 0], ["sounds/Ba Ding 01b", 460], ["sounds/Rocket Launch Woosh 03b", 460], ["sounds/Rocket Launch Woosh 03b", 460], ["sounds/Rocket Launch Woosh 03b", 460], ["sounds/Fuse Burning 01d short", 1520], ["sounds/Firework Explosion 04", 1950]]},
      { name: "Level Up", when: "The last rocket a level needs", steps: [["sounds/Fuse Burning 01d short", 0], ["sounds/Firework Explosion 06", 480], ["sounds/Crowd Applause 04", 520], ["sounds/Ba Ding 03b", 520], ["sounds/Rocket Launch Woosh 03b", 520], ["sounds/Rocket Launch Woosh 03b", 520], ["sounds/Rocket Launch Woosh 03b", 520], ["sounds/Crowd Applause 01", 520], ["sounds/voice/Level Up 02", 910], ["sounds/Dragon Breathing Fire 05", 1510], ["sounds/Firework Explosion 01", 2010], ["sounds/Crowd Oohs 11", 2510], ["sounds/Firework Explosion 06", 2510], ["sounds/Crowd Applause 01", 3010], ["sounds/Firework Explosion 03", 3010], ["sounds/voice/Well Done 01", 4530, {"end": 4690}], ["sounds/Crowd Applause 02", 4530], ["sounds/voice/Get Ready 02", 4690]]},
      { name: "Buy Rockets", when: "Between levels the coins collected fly out one by one, each buying a rocket, one upgrading a launcher; then the next level's Get Ready", steps: [["sounds/Coin Leaving Treasure Chest 05", 0], ["sounds/Coin Leaving Treasure Chest 05", 500], ["sounds/Rocket Being Bought 02", 550], ["sounds/Coin Leaving Treasure Chest 05", 1000], ["sounds/Rocket Being Bought 02", 1050], ["sounds/Coin Leaving Treasure Chest 05", 1500], ["sounds/Rocket Being Bought 02", 1550], ["sounds/Coin Leaving Treasure Chest 05", 2000], ["sounds/Rocket Being Bought 02", 2050], ["sounds/Rocket Being Bought 02", 2550], ["sounds/Magic Rocket Upgraded 03", 2550], ["sounds/voice/Get Ready 02", 2840]]},
      { name: "Time Up", when: "Classic mode's night timer runs out", steps: [["sounds/Timer Danger 06", 0], ["sounds/Timer Danger 06", 1000], ["sounds/Timer Danger 06", 2000], ["sounds/Tubetricks Sting Two v01", 2820], ["sounds/voice/Game Over 02", 2820], ["sounds/Fuse Burning 01d short", 3480], ["sounds/Ba Ding 01b", 3940], ["sounds/Rocket Launch Woosh 03b", 3940], ["sounds/Rocket Launch Woosh 03b", 3940], ["sounds/Rocket Launch Woosh 03b", 3940], ["sounds/Firework Explosion 03", 5430], ["sounds/voice/This Is Your Final Rank 07", 5820, {"end": 5900}], ["sounds/Crowd Applause 02", 5820], ["sounds/voice/These Are The Master Rocketeers 06", 5900]]}
    ],
    groups: [
    { name: "Menus and voices", sounds: [
      ["sounds/ui little beep 05b.ogg", "Button beep", "A button is pressed on a main screen, on the high score dialog or on a hint dialog; a button beside the board is let go. Also in Hammer Heads Deluxe, as sounds/UI - Check.ogg."],
      ["sounds/voice/welcome to rocket mania 03.ogg", "\"Welcome to Rocket Mania\"", "The main menu, 100 updates (1 second) after it opens, the first time in a run; the talking head speaks with it"],
      ["sounds/voice/let's launch some fireworks 01.ogg", "\"Let's launch some fireworks\"", "The main menu, 100 updates (1 second) after it opens, on every visit after the first in a run; the talking head speaks with it"],
      ["sounds/voice/these are the master rocketeers 06.ogg", "\"These are the master rocketeers\"", "The high score screen opens, whether from the menu or after a game"],
      ["sounds/voice/you have got a high score 04.ogg", "\"You have got a high score\"", "A game ends with a score that makes the all-time table, the day's best or both"],
      ["sounds/tubetricks sting one v01.ogg", "Record sting", "The dialog for entering a name for a high score opens"],
      ["sounds/hint popup 01.ogg", "Hint pops up", "A hint dialog opens, unless it is a game mode's hint; the talking head's speech bubble finishes growing"]
    ] },
    { name: "Starting a level", sounds: [
      ["sounds/voice/get ready 02.ogg", "\"Get ready\"", "A level starts; the first level says it only when hints are off"],
      ["sounds/voice/click on the tile to rotate it___ 01.ogg", "\"Click on the tile to rotate it\"", "150 updates (1.5 seconds) into the first new game of a run, with hints on; not when a saved game is continued"],
      ["sounds/lantern 01.ogg", "Lantern", "One of the meters on the board's top bar fills up"],
      ["sounds/dragon breathing fire 05.ogg", "Dragon fire", "A level is cleared: 100 updates (1 second) after the clear, as the talking head flies in on a rocket. Also in Hammer Heads Deluxe, as sounds/Gnome King - Droning.ogg."],
      ["sounds/ot - new ice cubes 01b.ogg", "Ice cubes", "Classic mode: the pause of the night timer that a clock item started, 1000 updates (10 seconds) for each clock picked up, runs out and the timer goes on"]
    ] },
    { name: "Tiles and fuses", sounds: [
      ["sounds/block rotating 03.ogg", "Tile turns", "A tile is turned"],
      ["sounds/tubebump.ogg", "Tile lands", "A falling tile comes to rest on the stack (an arcade-mode top tile too); at most once every 15 updates (0.15 seconds)"],
      ["sounds/tube glow 01.ogg", "Fuse lit", "The tiles join from one end to the other and a fuse starts to burn"],
      ["sounds/fuse burning 01d short.ogg", "Fuse burns", "A burn finishes and the fuses are lit row by row"],
      ["sounds/fuse burning 01.ogg", "Special tile flashes", "A special tile, a lit one, starts to flash"],
      ["sounds/magic tinkeling sound with ping 01b.ogg", "Special appears", "A bomb or another special tile is put on a free tile"],
      ["sounds/explosion 01.ogg", "Bomb", "A bomb counts down or goes off; a tile at the top blows up. Also in Hammer Heads Deluxe, as sounds/Bomb Face - Explosion.ogg."]
    ] },
    { name: "Launching rockets", sounds: [
      ["sounds/rocket launch woosh 03b.ogg", "Rocket launch", "Rockets are launched: three times at once for one to three rockets, once per rocket for four or more"],
      ["sounds/ba ding 01b.ogg", "Ding 1", "One rocket is launched"],
      ["sounds/ba ding 02b.ogg", "Ding 2", "Two rockets are launched at once"],
      ["sounds/ba ding 03b.ogg", "Ding 3", "Three rockets are launched at once"],
      ["sounds/ba ding 04b.ogg", "Ding 4", "Four rockets are launched at once"],
      ["sounds/ba ding 05b.ogg", "Ding 5", "Five or more rockets are launched at once"],
      ["sounds/firework explosion 01.ogg", "Firework 1", "A launched rocket's burst goes off, 150 updates (1.5 seconds) after the launch plus 50 updates (0.5 seconds) for each rocket before it; one of the six at random"],
      ["sounds/firework explosion 03.ogg", "Firework 2", "A launched rocket's burst goes off, 150 updates (1.5 seconds) after the launch plus 50 updates (0.5 seconds) for each rocket before it; one of the six at random. Also in Bejeweled 3, as sounds/firework_thump.ogg."],
      ["sounds/firework explosion 04.ogg", "Firework 3", "A launched rocket's burst goes off, 150 updates (1.5 seconds) after the launch plus 50 updates (0.5 seconds) for each rocket before it; one of the six at random"],
      ["sounds/firework explosion 05.ogg", "Firework 4", "A launched rocket's burst goes off, 150 updates (1.5 seconds) after the launch plus 50 updates (0.5 seconds) for each rocket before it; one of the six at random"],
      ["sounds/firework explosion 06.ogg", "Firework 5", "A launched rocket's burst goes off, 150 updates (1.5 seconds) after the launch plus 50 updates (0.5 seconds) for each rocket before it; one of the six at random"],
      ["sounds/firework explosion 07.ogg", "Firework 6", "A launched rocket's burst goes off, 150 updates (1.5 seconds) after the launch plus 50 updates (0.5 seconds) for each rocket before it; one of the six at random. Resembles Bejeweled 3's sounds/firework_crackle.ogg, the same sound, shorter."],
      ["sounds/crowd applause 01.ogg", "Applause, small", "Two or three rockets are launched at once, as they go; again when the last of three bursts. Also in Hammer Heads Deluxe, as sounds/Crowd Applause 01.ogg."],
      ["sounds/crowd applause 02.ogg", "Applause, medium", "Four or five rockets are launched at once, as they go and again when the last one bursts; the level-end dialog opens. Also in Hammer Heads Deluxe, as sounds/Crowd Applause 02.ogg."],
      ["sounds/crowd applause 04.ogg", "Applause, large", "Six or more rockets are launched at once, as they go and again when the last one bursts; a launch wins the full-board bonus. Also in Hammer Heads Deluxe, as sounds/Crowd Applause 04.ogg. Also in Chuzzle Deluxe, as sounds/TrophyJoy.ogg."]
    ] },
    { name: "Scoring", sounds: [
      ["sounds/ting 02b.ogg", "Multiplier ting 1 (unused)", "Never played"],
      ["sounds/ting 02b shift 2.ogg", "Multiplier ting 2", "A multiplier special is picked up as a burn reaches it, the first one of that burn"],
      ["sounds/ting 02b shift 4.ogg", "Multiplier ting 3", "A multiplier special is picked up as a burn reaches it, the second one of that burn"],
      ["sounds/ting 02b shift 6.ogg", "Multiplier ting 4", "A multiplier special is picked up as a burn reaches it, the third one of that burn"],
      ["sounds/ting 02b shift 8.ogg", "Multiplier ting 5", "A multiplier special is picked up as a burn reaches it, the fourth one of that burn or a later one"],
      ["sounds/ching 01.ogg", "Ching, 1", "The smallest coin reaches the coin counter"],
      ["sounds/ching 02.ogg", "Ching, 2", "A medium coin reaches the coin counter; with Ching 5, the biggest coin"],
      ["sounds/ching 05.ogg", "Ching, 5", "A large coin reaches the coin counter; with Ching 2, the biggest coin"],
      ["sounds/gem picked up.ogg", "Gem 1", "A matches special or a clock is picked up before any gem of the same burn. Also in Hammer Heads Deluxe, as sounds/Flag Mole - Picked and Moving.ogg."],
      ["sounds/gem picked up shift 1.ogg", "Gem 2", "A gem is picked up as the first gem of a burn; a matches special or a clock after one gem"],
      ["sounds/gem picked up shift 2.ogg", "Gem 3", "A gem is picked up as the second gem of a burn; a matches special or a clock after two gems"],
      ["sounds/gem picked up shift 3.ogg", "Gem 4", "A gem is picked up as the third gem of a burn; a matches special or a clock after three gems"],
      ["sounds/gem picked up shift 4.ogg", "Gem 5", "A gem is picked up as the fourth gem of a burn or a later one; a matches special or a clock after four gems or more"],
      ["sounds/ot - new ice cubes 02b.ogg", "Time bonus", "A clock item reaches the clock; the night timer then stops for 1000 updates (10 seconds)"],
      ["sounds/crowd oohs 04c.ogg", "Crowd ooh 1", "The middle rocket of a launch of two or more bursts, 1 time in 4 (one of four sounds at random)"],
      ["sounds/crowd oohs 05.ogg", "Crowd ooh 2", "The middle rocket of a launch of two or three bursts, 1 time in 4"],
      ["sounds/crowd oohs 07b.ogg", "Crowd ooh 3", "The middle rocket of a launch of four or more bursts, 2 times in 4"],
      ["sounds/crowd oohs 08b.ogg", "Crowd ooh 4", "The middle rocket of a launch of two or more bursts, 1 time in 4"],
      ["sounds/crowd oohs 11.ogg", "Crowd ooh 5", "The middle rocket of a launch of two or three bursts, 1 time in 4"]
    ] },
    { name: "Time and endings", sounds: [
      ["sounds/timer danger 06.ogg", "Time ticks", "Arcade mode: every 100 updates (1 second) while a tile rests in the top row of the stack; classic mode: every 100 updates once 90 percent or more of the night's time has gone; the sound volume slider is moved, at most once every 50 updates (0.5 seconds)"],
      ["sounds/cockeral crowing 03.ogg", "Rooster", "Classic mode: once a level, when the night timer's value drops to 10 or below"],
      ["sounds/voice/level up 02.ogg", "\"Level up\"", "A level is cleared, 40 updates (0.4 seconds) after the clear"],
      ["sounds/voice/well done 01.ogg", "\"Well done\"", "The level-end dialog opens after a cleared level"],
      ["sounds/voice/this is your final rank 07.ogg", "\"This is your final rank\"", "The level-end dialog opens at the end of a game"],
      ["sounds/voice/out of time 02.ogg", "\"Out of time\"", "Classic mode: the night timer runs out and the game ends"],
      ["sounds/voice/game over 02.ogg", "\"Game over\"", "The game ends because the top line is full, or in strategy mode because it ran out of moves"],
      ["sounds/tubetricks sting two v01.ogg", "Game over sting", "The game ends, whatever the cause"]
    ] },
    { name: "Buying rockets", sounds: [
      ["sounds/coin leaving treasure chest 05.ogg", "Coin spent", "A coin leaves the chest on the buy-rockets screen"],
      ["sounds/rocket being bought 02.ogg", "Rocket bought", "55 updates (0.55 seconds) after a coin leaves the chest, as it reaches its item"],
      ["sounds/magic rocket upgraded 03.ogg", "Rocket upgraded", "55 updates (0.55 seconds) after a coin leaves the chest, when that coin raises a launcher's meter; it plays with Rocket bought"],
      ["sounds/magic rocket upgraded 02.ogg", "Rocket upgraded 2 (unused)", "Never played"]
    ] },
  ] },
  sevenseas: {
    presets: [
      { name: "Welcome Aboard", when: "The intro screen opens, the first time after the game starts", steps: [["sounds/talkwelcometosevenseas", 0]]},
      { name: "Broadside", when: "A cannon shot hits a ship", steps: [["sounds/boom", 0, {"p": 33}], ["sounds/explode2", 450, {"p": 33}]]},
      { name: "Ship Lost", when: "A life is lost and the ship comes back", steps: [["sounds/explode2", 0], ["sounds/sink", 0], ["sounds/reappear", 1250, {"p": 166}]]}
    ],
    groups: [
    { name: "Menus and voices", sounds: [
      ["sounds/click.ogg", "Click", "A button is pressed: the menus, the options and other dialogs, the help screen and the demo screen (also the demo's own clicks)"],
      ["sounds/talkwelcometosevenseas.ogg", "\"Welcome to Seven Seas\"", "On the intro screen, 60 updates (0.6 seconds) after it opens, until the first game of the run has begun; the pirate speaks with it"],
      ["sounds/talkhoistsail.ogg", "\"Hoist the mainsail\"", "Play or Tutorial is pressed on the intro screen: one of the three voices, at random"],
      ["sounds/talkallhands.ogg", "\"All hands on deck\"", "Play or Tutorial is pressed on the intro screen: one of the three voices, at random"],
      ["sounds/talkayeaye.ogg", "\"Aye aye, Captain\"", "Play or Tutorial is pressed on the intro screen: one of the three voices, at random"]
    ] },
    { name: "Your ship", sounds: [
      ["sounds/movegood.ogg", "Ship moves", "The player's ship is ordered to a new cell, by click or key (panned to the ship); also each move of the ship in the help demo"],
      ["sounds/whirlout.ogg", "Whirlpool out", "The player's ship sails onto an active whirlpool and spins out of the board; also in the help demo when the ship reaches the top row"],
      ["sounds/whirlin.ogg", "Whirlpool in", "The ship has finished spinning out (30 animation steps) and is put on a random free cell, spiralling back in; also in the help demo"],
      ["sounds/reappear.ogg", "Ship reappears", "After a life is lost, when the flash has run for 18 updates and the ship is placed again, and twice more as its blinking goes off, every 30 updates (0.3 seconds)"],
      ["sounds/boom.ogg", "Cannon shot", "The player's ship or an enemy ship fires cannons, if at least one shot is made (panned to the ship); also in the help demo"]
    ] },
    { name: "Enemies", sounds: [
      ["sounds/moveevil.ogg", "Enemies move", "The enemy ships start their move, once every ship has finished turning; also in the help demo"],
      ["sounds/explode.ogg", "Small explosion", "A small explosion: an enemy ship that is not a sea monster runs into another ship, or into land or a wreck (panned to the cell). Resembles Dynomite's Explode (unused) (sounds/explode.ogg), the same sound, mixed differently."],
      ["sounds/explode2.ogg", "Big explosion", "A big explosion: a cannon shot hits a ship or the player's ship, or an enemy ship sails into the player's cell (panned to the cell); also in the help demo"],
      ["sounds/monster.ogg", "Sea monster", "A sea monster is hit by a shot, sails into the player's cell (and a life is lost) or into another ship, or sails onto a wreck (panned to the cell)"],
      ["sounds/sink.ogg", "Ship sinks", "The player's ship is hit or caught and a life is lost"]
    ] },
    { name: "Scoring and level end", sounds: [
      ["sounds/bonusship.ogg", "Extra ship", "The score passes another thousand and a life is added, whether by a ship or wreck's points or by a level's kill bonus"],
      ["sounds/talkdavyjoneslocker.ogg", "\"Down to Davy Jones' locker\"", "At a level's end, once the pirate has finished appearing: one of the four voices, at random"],
      ["sounds/talkarrmatey.ogg", "\"Arr, matey!\"", "At a level's end, once the pirate has finished appearing: one of the four voices, at random"],
      ["sounds/talkwalktheplank.ogg", "\"Walk the plank!\"", "At a level's end, once the pirate has finished appearing: one of the four voices, at random"],
      ["sounds/talklandlubber.ogg", "\"Avast, ye landlubber\"", "At a level's end, once the pirate has finished appearing: one of the four voices, at random"]
    ] },
    { name: "Bonus dig", sounds: [
      ["sounds/bonusdig.ogg", "Dig", "A square is dug on the bonus screen and no ghost is under it (panned by column)"],
      ["sounds/bonusgraves.ogg", "Graves near", "A square is dug with no ghost under it, and a ghost or grave lies in the 3-by-3 block of squares ending at it (up and to its left); plays with the dig sound"],
      ["sounds/bonuslose.ogg", "Ghost", "A square with a ghost under it is dug on the bonus screen; the ghost appears"]
    ] },
  ] },
  tiptop: {
    // presets from the Tip Top session's sound logs (-fast, 10 ms updates; tiptop-soundlogs/INDEX.md), the bot's own input
    // (MouseOver, MouseClick, LaunchBlockMoves, BlockLaunch) left out
    presets: [
      { name: "Level Won", when: "A level's quota is met: the win, the empty rows counted one by one, the blocks drop", steps: [["TipTop.dat|tiptop:sounds/LevelWin.ogg", 0], ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", 270, {"end": 670}], ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", 670, {"end": 1070}], ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", 1070, {"end": 1470}], ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", 1470, {"end": 1870}], ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", 1870, {"end": 2320}], ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", 2320], ["TipTop.dat|tiptop:sounds/BlocksFall.ogg", 2780]]},
      { name: "Rocket Cascade", when: "A rocket lands and its blast starts a match of 5, then cascades 2 to 5", steps: [["TipTop.dat|tiptop:sounds/RocketLaunch.ogg", 0, {"end": 500}], ["TipTop.dat|tiptop:sounds/RocketCollision.ogg", 500], ["TipTop.dat|tiptop:sounds/ColorMatchGroup5.ogg", 690, {"end": 690}], ["TipTop.dat|tiptop:sounds/ColorMatchCascade1.ogg", 690, {"end": 1220}], ["TipTop.dat|tiptop:sounds/ColorMatchCascade2.ogg", 1220, {"end": 1220}], ["TipTop.dat|tiptop:sounds/ColorMatchCascade3.ogg", 1220, {"end": 1660}], ["TipTop.dat|tiptop:sounds/FloaterLaunched.ogg", 1500], ["TipTop.dat|tiptop:sounds/ColorMatchCascade4.ogg", 1660, {"end": 1660}], ["TipTop.dat|tiptop:sounds/ColorMatchCascade4.ogg", 1660, {"end": 2070}]]},
      { name: "Hammer", when: "A hammer is launched, lands and smashes, and a floater rises", steps: [["TipTop.dat|tiptop:sounds/HammerLaunch.ogg", 0, {"end": 250}], ["TipTop.dat|tiptop:sounds/HammerCollision.ogg", 250], ["TipTop.dat|tiptop:sounds/FloaterLaunched.ogg", 380]]},
      { name: "Rainbow", when: "A rainbow piece is launched and lands, matching its colour", steps: [["TipTop.dat|tiptop:sounds/RainbowLaunch.ogg", 0, {"end": 250}], ["TipTop.dat|tiptop:sounds/RainbowCollision.ogg", 250], ["TipTop.dat|tiptop:sounds/ColorMatch.ogg", 260]]},
      { name: "Column Nuke", when: "A column nuke is launched and clears its column", steps: [["TipTop.dat|tiptop:sounds/ColumnNukeLaunch.ogg", 0, {"end": 190}], ["TipTop.dat|tiptop:sounds/ColumnNukeCollision.ogg", 190]]},
      { name: "Color Bomb", when: "A color bomb is launched and goes off, the stack shaking", steps: [["TipTop.dat|tiptop:sounds/shake1.ogg", 0], ["TipTop.dat|tiptop:sounds/ColorbombLaunch.ogg", 70, {"end": 560}], ["TipTop.dat|tiptop:sounds/shake1.ogg", 450, {"end": 560}], ["TipTop.dat|tiptop:sounds/FloaterLaunched.ogg", 550], ["TipTop.dat|tiptop:sounds/ColorBombCollision.ogg", 560]]},
      { name: "Game Lost", when: "The stack reaches the top: the shake, then the loss", steps: [["TipTop.dat|tiptop:sounds/shake0.ogg", 0, {"end": 250}], ["TipTop.dat|tiptop:sounds/FloaterLaunched.ogg", 220, {"end": 230}], ["TipTop.dat|tiptop:sounds/GameLost.ogg", 260]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["TipTop.dat|tiptop:sounds/splash.ogg", "\"Welcome to Tip Top\"", "The splash screen opens, as its loading bar starts"],
      ["TipTop.dat|tiptop:sounds/MouseOver.ogg", "Mouse over", "The pointer comes onto a button or check box; also onto the level summary's button"],
      ["TipTop.dat|tiptop:sounds/MouseClick.ogg", "Mouse click", "A button is pressed, or Enter or Esc on a button that answers to that key; also a click or Enter on the level summary's button. The same recording as Volume test."],
      ["TipTop.dat|tiptop:sounds/VolumeAdjust.ogg", "Volume test", "The sound volume slider is moved or its track is clicked, unless the sound is still playing; it plays at the new volume. The same recording as Mouse click."]
    ] },
    { name: "Level start and end", sounds: [
      ["TipTop.dat|tiptop:sounds/GameStart.ogg", "Game start", "A game's first level begins (the level is the configured start level)"],
      ["TipTop.dat|tiptop:sounds/LevelStart.ogg", "Level start", "A later level begins; also when a bonus round begins"],
      ["TipTop.dat|tiptop:sounds/LevelWin.ogg", "Level won", "A level is completed, as its completed picture and sparkles appear; also when a bonus round is completed"],
      ["TipTop.dat|tiptop:sounds/GameLost.ogg", "Game lost", "The game is over, in the normal game or in a bonus round"],
      ["TipTop.dat|tiptop:sounds/EmptyRowCount.ogg", "Empty row", "A cleared row scores the empty-row bonus"]
    ] },
    { name: "Matches", sounds: [
      ["TipTop.dat|tiptop:sounds/ColorMatch.ogg", "Match of 3", "A match of exactly three blocks that is the first of its chain"],
      ["TipTop.dat|tiptop:sounds/ColorMatchGroup4.ogg", "Match of 4", "A match of four blocks that is the first of its chain"],
      ["TipTop.dat|tiptop:sounds/ColorMatchGroup5.ogg", "\"Mega\"", "A match of five blocks that is the first of its chain"],
      ["TipTop.dat|tiptop:sounds/ColorMatchGroup6.ogg", "\"Super\"", "A match of six or more blocks that is the first of its chain"],
      ["TipTop.dat|tiptop:sounds/ColorMatchCascade1.ogg", "\"Double!\"", "The second match of a chain, whatever its size"],
      ["TipTop.dat|tiptop:sounds/ColorMatchCascade2.ogg", "\"Triple!\"", "The third match of a chain, whatever its size"],
      ["TipTop.dat|tiptop:sounds/ColorMatchCascade3.ogg", "\"Quad!\"", "The fourth match of a chain, whatever its size"],
      ["TipTop.dat|tiptop:sounds/ColorMatchCascade4.ogg", "\"Super combo!\"", "The fifth match of a chain or any later one, whatever its size"]
    ] },
    { name: "Blocks", sounds: [
      ["TipTop.dat|tiptop:sounds/BlockLaunch.ogg", "Block launched", "The player launches a plain block from the bottom of its column"],
      ["TipTop.dat|tiptop:sounds/FloaterLaunched.ogg", "Floater launched", "A plain block enters from the top of the board (a block of the queue that is not the player's)"],
      ["TipTop.dat|tiptop:sounds/LaunchBlockMoves.ogg", "Launcher moves", "The launcher's column changes while the preview block is drawn, but not when its first column is set"],
      ["TipTop.dat|tiptop:sounds/BlocksFall.ogg", "Blocks fall", "At the end of a level, when blocks are left on the board and start to fly off the screen, row after row"]
    ] },
    { name: "Power pieces", sounds: [
      ["TipTop.dat|tiptop:sounds/RocketLaunch.ogg", "Rocket launch", "A rocket piece starts to move: launched by the player, or entering from the top"],
      ["TipTop.dat|tiptop:sounds/RocketCollision.ogg", "Rocket hits", "A rocket reaches the first block in its way (the one row it taps) and goes off"],
      ["TipTop.dat|tiptop:sounds/HammerLaunch.ogg", "Hammer launch", "A hammer piece starts to move: launched by the player, or entering from the top"],
      ["TipTop.dat|tiptop:sounds/HammerCollision.ogg", "Hammer hits", "A hammer reaches the first block in its way (the one row it taps) and goes off"],
      ["TipTop.dat|tiptop:sounds/MagnetLaunch.ogg", "Magnet launch", "A magnet piece starts to move: launched by the player, or entering from the top"],
      ["TipTop.dat|tiptop:sounds/RainbowLaunch.ogg", "Rainbow launch", "A rainbow piece starts to move: launched by the player, or entering from the top"],
      ["TipTop.dat|tiptop:sounds/RainbowCollision.ogg", "Rainbow hits", "A rainbow piece comes to rest and goes off"],
      ["TipTop.dat|tiptop:sounds/ColorbombLaunch.ogg", "Color bomb launch", "A color bomb piece starts to move: launched by the player, or entering from the top"],
      ["TipTop.dat|tiptop:sounds/ColorBombCollision.ogg", "Color bomb hits", "A color bomb piece comes to rest and goes off"],
      ["TipTop.dat|tiptop:sounds/ColumnNukeLaunch.ogg", "Column nuke launch", "A column nuke piece starts to move: launched by the player, or entering from the top"],
      ["TipTop.dat|tiptop:sounds/ColumnNukeCollision.ogg", "Column nuke hits", "A column nuke piece comes to rest and goes off"]
    ] },
    { name: "Warnings", sounds: [
      ["TipTop.dat|tiptop:sounds/timewarning.ogg", "Time warning", "Loops while the level's meter is at or below a sixth of full, in Timed mode or a bonus round on the game screen; it stops otherwise"],
      ["TipTop.dat|tiptop:sounds/shake0.ogg", "Shake 0", "Loops, restarted every update, while play is on (not game over, level complete, a bonus round or a busy screen) and the stack's nearest distance to the edge is 1 row or less; every mode's table sets it"],
      ["TipTop.dat|tiptop:sounds/shake1.ogg", "Shake 1", "Loops, restarted every update, while play is on (not game over, level complete, a bonus round or a busy screen) and the stack's nearest distance to the edge is 2 rows or less; only the Timed and Speed tables set it"],
      ["TipTop.dat|tiptop:sounds/shake2.ogg", "Shake 2 (unused)", "Never played: no mode's shake table (Regular and Strategy 5,0,5,5,5; Timed and Speed 5,0,1,5,5) selects it"]
    ] },
  ] },
  typershark: {
    presets: [
      { name: "Treasure Streak", when: "The diver reaches the treasure and the streak begins", steps: [["sounds/treasure", 0], ["sounds/tone1", 60], ["sounds/diverenters", 60], ["sounds/tone1", 120], ["sounds/diverenters", 120], ["sounds/tone2", 180], ["sounds/diverenters", 180], ["sounds/tone2", 240], ["sounds/diverenters", 240], ["sounds/tone3", 290], ["sounds/diverenters", 290], ["sounds/tone3", 340], ["sounds/diverenters", 340], ["sounds/tone4", 390], ["sounds/diverenters", 390], ["sounds/tone4", 450], ["sounds/diverenters", 450], ["sounds/tone7", 800], ["sounds/diverenters", 800], ["sounds/tone8", 900], ["sounds/diverenters", 900], ["sounds/tone9", 960], ["sounds/diverenters", 960], ["sounds/tone10", 1140], ["sounds/tone11", 1190], ["sounds/tone12", 1300], ["sounds/diverenters", 1300], ["sounds/tone12", 1360], ["sounds/diverenters", 1360]]},
      { name: "Set Sail", when: "A site is done and the ship sails on", steps: [["sounds/sonarping1", 0], ["sounds/shipmove", 90]]}
    ],
    groups: [
    { name: "Menus and map", sounds: [
      ["sounds/buttonclick.ogg", "Button click", "A button is pressed anywhere in the game: the menus, dialogs, the options dialog, the hint dialog and the typing tutor"],
      ["sounds/shipmove.ogg", "Ship moves", "The ship moves one step along its route on the level map (every half second)"],
      ["sounds/divebell1.ogg", "Dive bell", "The ship arrives at the end of its route on the map (up to three times)"],
      ["sounds/wave_bonus.ogg", "Wave bonus 1", "A bronze medal ends a wave (under 80 percent); also when the options dialog's sound effects slider is released"]
    ] },
    { name: "Typing", sounds: [
      ["sounds/type.ogg", "Type 1", "A correct letter is typed in play, in the streak minigame or in the typing tutor (one of the three type sounds, 1 time in 3); always this one when a clam is opened in the clam bonus level"],
      ["sounds/type2.ogg", "Type 2", "A correct letter is typed in play, in the streak minigame or in the typing tutor (one of the three type sounds, 1 time in 3)"],
      ["sounds/type3.ogg", "Type 3", "A correct letter is typed in play, in the streak minigame or in the typing tutor (one of the three type sounds, 1 time in 3)"],
      ["sounds/wrong.ogg", "Wrong key", "A wrong letter is typed in play (with or without a creature locked on), in the streak minigame or in the typing tutor; in the clam bonus level, a key that matches no closed clam or one pressed while a clam without treasure is open"],
      ["sounds/emp.ogg", "EMP", "Enter is pressed with the EMP's charge full and the EMP not already running, while the diver is swimming"]
    ] },
    { name: "Enemies and weapons", sounds: [
      ["sounds/sharkhurt.ogg", "Shark hurt", "The black shark is hit and still has words left; not when the EMP does it"],
      ["sounds/sharkdies.ogg", "Shark dies", "A shark is killed, or the black shark's last word is finished; not when the EMP kills it"],
      ["sounds/piranhadies1.ogg", "Piranha dies", "A piranha is killed; not when the EMP kills it"],
      ["sounds/jellyfish.ogg", "Jellyfish", "A bonus creature (jellyfish) is shocked; not when the EMP does it"],
      ["sounds/playerdies.ogg", "Diver bitten", "A shark or a piranha reaches the diver, once for each creature"],
      ["sounds/torpedolaunch.ogg", "Torpedo launch", "The torpedo boss fires a volley of torpedoes: once for the first torpedo of the volley"],
      ["sounds/explosion.ogg", "Explosion", "A cannon ball or a torpedo explodes; every 33 updates (0.33 seconds) while the torpedo boss blows up"],
      ["sounds/sonarping1.ogg", "Sonar ping", "The sonar sweep starts when a wave ends (the diver in its sonar state); the torpedo boss, while it is alive, every 2 seconds"]
    ] },
    { name: "Waves and rewards", sounds: [
      ["sounds/wave_bonus2.ogg", "Wave bonus 2", "A silver medal ends a wave (80 to 94 percent)"],
      ["sounds/wave_bonus3.ogg", "Wave bonus 3", "A gold medal ends a wave (95 percent or more); the first pair of clams is found in the clam bonus level"],
      ["sounds/wingem.ogg", "Gem", "A level is won (the diver is back at the bottom with the treasure); the treasure is shown in the clam bonus level or the streak minigame ends; a second or later pair of clams is found"],
      ["sounds/treasure.ogg", "Treasure", "The diver's air gauge fills at the treasure and the streak minigame starts; a clam with treasure is opened with no other clam open. Resembles Insaniquarium Deluxe's sounds/diamond.au, the same sound, mixed differently."],
      ["sounds/bad.ogg", "Bad clam", "A clam with no treasure finishes opening in the clam bonus level"],
      ["sounds/tone1.ogg", "Tone 1", "The streak minigame's 1st or 2nd word (the tone rises every two words)"],
      ["sounds/tone2.ogg", "Tone 2", "The streak minigame's 3rd or 4th word; the 1st bonus creature of a wave shocked"],
      ["sounds/tone3.ogg", "Tone 3", "The streak minigame's 5th or 6th word; the 2nd bonus creature of a wave shocked"],
      ["sounds/tone4.ogg", "Tone 4", "The streak minigame's 7th or 8th word; the 3rd bonus creature of a wave shocked"],
      ["sounds/tone5.ogg", "Tone 5", "The streak minigame's 9th or 10th word; the 4th bonus creature of a wave shocked"],
      ["sounds/tone6.ogg", "Tone 6", "The streak minigame's 11th or 12th word; the 5th bonus creature of a wave shocked"],
      ["sounds/tone7.ogg", "Tone 7", "The streak minigame's 13th or 14th word; the 6th bonus creature of a wave shocked"],
      ["sounds/tone8.ogg", "Tone 8", "The streak minigame's 15th or 16th word; the 7th bonus creature of a wave shocked"],
      ["sounds/tone9.ogg", "Tone 9", "The streak minigame's 17th or 18th word; the 8th bonus creature of a wave shocked"],
      ["sounds/tone10.ogg", "Tone 10", "The streak minigame's 19th or 20th word; the 9th bonus creature of a wave shocked"],
      ["sounds/tone11.ogg", "Tone 11", "The streak minigame's 21st or 22nd word; the 10th bonus creature of a wave shocked"],
      ["sounds/tone12.ogg", "Tone 12", "The streak minigame's 23rd or 24th word; the 11th bonus creature of a wave shocked"],
      ["sounds/tone13.ogg", "Tone 13", "The streak minigame's 25th or later word; the 12th or later bonus creature of a wave shocked"]
    ] },
    { name: "Diver and sea", sounds: [
      ["sounds/diverenters.ogg", "Diver enters", "A level's board first updates; the diver enters the water; a stealth shark or stealth piranha surfaces (30 bubbles); a streak word is finished (50 bubbles)"],
      ["sounds/underwaterloop.ogg", "Underwater loop", "Loops while the game board is shown; its volume goes to zero while the game is paused"],
      ["sounds/eerie1.au", "Eerie 1 (unused)", "Never played: loaded and held as a sound instance, but no code plays it"],
      ["sounds/eerie3.au", "Eerie 3 (unused)", "Never played: loaded and held as a sound instance, but no code plays it"],
      ["sounds/extralife.au", "Extra life (unused)", "Never played: loaded, but no code asks for it"]
    ] },
  ] },
  waterbugs: {
    presets: [
      { name: "Beam Shot", when: "A click fires the beams: they drill across the board until each meets a closed cell", steps: [["audio/drill_shoot", 0], ["audio/drill", 60], ["audio/drill", 60], ["audio/drill", 130], ["audio/drill", 130], ["audio/drill", 190], ["audio/drill", 190], ["audio/drill", 260], ["audio/drill", 260], ["audio/drill", 330], ["audio/drill", 330], ["audio/drill", 390], ["audio/drill", 390], ["audio/drill", 460], ["audio/drill", 460], ["audio/drill", 520], ["audio/drill", 520], ["audio/drill", 590], ["audio/drill", 590], ["audio/drill", 660], ["audio/drill", 660], ["audio/drill", 720], ["audio/drill", 720], ["audio/drill", 790], ["audio/drill", 790], ["audio/drill", 850], ["audio/drill", 850], ["audio/drill", 920], ["audio/drill", 920], ["audio/drill", 990], ["audio/drill", 990], ["audio/drill", 1050], ["audio/drill", 1050], ["audio/drill", 1120], ["audio/drill", 1120], ["audio/drill", 1180], ["audio/drill", 1180], ["audio/clank", 1200]]},
      { name: "Fill", when: "The beams close off a side, which fills in steps until it is done", steps: [["audio/fill", 0], ["audio/fill", 180], ["audio/fill", 360], ["audio/fill", 540], ["audio/fill", 720], ["audio/fill", 900], ["audio/fill", 1080], ["audio/fill", 1260], ["audio/fill", 1440], ["audio/fill_end", 1540]]},
      { name: "Ship Zapped", when: "An enemy touches a beam while the ship is firing, and the ship explodes", steps: [["audio/electricity", 0], ["audio/explosion2", 90]]},
      { name: "Board Summary", when: "A board's figures appear", steps: [["audio/pop", 0], ["audio/pop", 300], ["audio/pop", 600]]}
    ],
    groups: [
    { name: "Menus and summary", sounds: [
      ["audio/click.wav", "Click", "A menu item is clicked while the pointer is on it (a locked level's does not click), or an item that clicks itself is reached"],
      ["audio/pop.wav", "Summary pop", "Each of the five figures of the end-of-board summary (fill percent, best fill, shots, fewest shots, bonus) starts to appear"]
    ] },
    { name: "The ship and its beams", sounds: [
      ["audio/drill_shoot.wav", "Beam shot", "A click fires the ship's beams; the sound plays only if fewer than three beams are out"],
      ["audio/drill.wav", "Beam drilling", "While the beams run, once for each beam every 66 steps (0.066 seconds; the game takes 1000 steps a second) of the time since the shot"],
      ["audio/clank.wav", "Beam clank", "A beam's head meets a cell that is not open and stops"],
      ["audio/buzzer.wav", "Buzzer", "A click with an enemy near the ship while the fill quota is not yet passed; no beams fire. Also in Banana Bugs, as sounds/buzzer.ogg."],
      ["audio/fill.wav", "Fill", "A fill runs on a board: every 180 steps (0.18 seconds) of the board's age while fills are going, and once when the beams have both stopped and the chosen side starts to fill"],
      ["audio/fill_end.wav", "Fill ends", "A fill finishes with one other fill still going on the board"],
      ["audio/explosion2.wav", "Ship explodes", "The ship explodes"],
      ["audio/invincibubble.wav", "Bubble protects", "An enemy touches a beam that would hit the ship while the ship is in its invincibubble"]
    ] },
    { name: "Enemies", sounds: [
      ["audio/explosion1.wav", "Enemy explodes", "A defined enemy, shot or time bomb explodes"],
      ["audio/bump.wav", "Line bump", "A bump roller pushes a line of filled cells one cell on"],
      ["audio/frog_jump.wav", "Frog jumps", "A frog has been sitting for more than 1000 steps (1 second) and hops"],
      ["audio/frog_land.wav", "Frog lands", "A frog lands, turns round and sits"],
      ["audio/crab.wav", "Crab nips", "A crab walker is at frame 12 of its animation with a line or filled cell two cells ahead of it; it blasts that cell"],
      ["audio/canon.wav", "Cannon", "A snake shooter fires a snake, its shot delay after its last shot; a cannon walker fires a cannon ball, more than 4000 steps (4 seconds) after its last shot, when more than 29 cells of the 9-by-9 block ahead of it are filled or lines"],
      ["audio/laser.wav", "Laser", "A laser walker fires a laser shot, more than 4000 steps (4 seconds) after its last shot, while a beam of the ship runs past it"],
      ["audio/beam_hit.wav", "Laser hits beams", "A laser shot touches a beam; the beams burst into sparks"],
      ["audio/electricity.wav", "Electricity", "An enemy touches a beam while the ship is firing (at most once a second per enemy)"]
    ] },
    { name: "Kill scores", sounds: [
      ["audio/exp100.wav", "Kill 100", "An enemy dies as the first of its chain (100 points); the sound is chosen by the points: 100 times the chain, or the fixed points of the kill (500 for a time bomb's blast)"],
      ["audio/exp200.wav", "Kill 200", "An enemy dies as the second of its chain (200 points)"],
      ["audio/exp300.wav", "Kill 300", "An enemy dies as the third of its chain (300 points)"],
      ["audio/exp400.wav", "Kill 400", "An enemy dies as the fourth of its chain (400 points)"],
      ["audio/exp500.wav", "Kill 500", "An enemy dies as the fifth of its chain, or in a time bomb's blast (500 points)"],
      ["audio/exp600.wav", "Kill 600", "An enemy dies as the sixth of its chain (600 points)"],
      ["audio/exp700.wav", "Kill 700", "An enemy dies as the seventh of its chain (700 points)"],
      ["audio/exp800.wav", "Kill 800", "An enemy dies as the eighth of its chain (800 points)"],
      ["audio/exp900.wav", "Kill 900", "An enemy dies as the ninth of its chain (900 points)"]
    ] },
    { name: "Power-ups and weapons", sounds: [
      ["audio/powerup.wav", "Power-up", "Any other power-up is caught (an extra life included)"],
      ["audio/powerdn.wav", "Power-down", "A flip flopper or hyper motion power-up is caught"],
      ["audio/mine.wav", "Mine launched", "A magnet or a time bomb is launched by a click"],
      ["audio/anchor.wav", "Anchor launched", "An anchor is launched by a click"],
      ["audio/missile.wav", "Missile launched", "A missile is launched by a click"],
      ["audio/fuse.wav", "Time bomb fuse", "A time bomb burns its fuse: every 230 steps (0.23 seconds) of its age while it has not yet gone off"]
    ] },
    { name: "Unused", sounds: [
      ["audio/extralife.wav", "Extra life (unused)", "Never played: the level files load it, but no code asks for it"],
      ["audio/rail.wav", "Rail (unused)", "Never played: no sound list or code names this file, so it is never even loaded"]
    ] }
  ] },
  wordharmony: {
    presets: [
      { name: "Level Clear", when: "The last word of a level is found and its tallies count up", steps: [["res/sounds/pop_wind", 0], ["res/music/intense-levelend", 30], ["res/sounds/ball_pop", 30], ["res/sounds/ball_pop", 130], ["res/sounds/ball_pop", 230], ["res/sounds/ball_pop", 330], ["res/sounds/message_add", 1600], ["res/sounds/message_add", 2100], ["res/sounds/message_add", 2600], ["res/sounds/stats_countup", 3600], ["res/sounds/stats_countup", 3960], ["res/sounds/stats_countup", 4260], ["res/sounds/stats_countup", 4500], ["res/sounds/stats_countup", 4730], ["res/sounds/stats_countup", 4900], ["res/sounds/stats_countup", 5060], ["res/sounds/stats_countup", 5200], ["res/sounds/stats_countup", 5330], ["res/sounds/stats_countup", 5400], ["res/sounds/stats_countup", 5500], ["res/sounds/stats_countup", 5560]]},
      { name: "Level Start", when: "A level opens and its first balls drop in", steps: [["res/sounds/level_intro", 0], ["res/sounds/level_begin", 1040], ["res/sounds/ball_rustle2", 1240], ["res/sounds/ball_rustle2", 1370]]},
      { name: "Word Found", when: "A word is accepted and its balls pop", steps: [["res/sounds/pop_wind", 0], ["res/sounds/ball_pop", 30], ["res/sounds/ball_pop", 130], ["res/sounds/ball_pop", 230], ["res/sounds/ball_pop", 330]]}
    ],
    groups: [
    { name: "Menus", sounds: [
      ["res/sounds/menu_mouseover.wav", "Mouse over", "The pointer enters a button or check box of a screen"],
      ["res/sounds/menu_clicked.wav", "Menu click", "A button or check box of a screen is pressed"]
    ] },
    { name: "Level start", sounds: [
      ["res/sounds/level_intro.wav", "Level intro", "A level's loading screen opens, as the level's music starts"],
      ["res/sounds/level_begin.wav", "Level begins", "The level's play starts, or a saved game is loaded back in"]
    ] },
    { name: "Balls", sounds: [
      ["res/sounds/ball_rustle.wav", "Next ball shown", "Freematch only: the HUD shows the next ball's letters and the new-ball blossom, each time the next piece is shown. The same recording as Ball pops."],
      ["res/sounds/ball_rustle2.WAV", "Balls arrive", "A ball, a word's balls or a recycled word's balls are dropped in (not when set quiet)"],
      ["res/sounds/ball_pop.wav", "Ball pops", "Each picked ball of a found word pops, one after the other. The same recording as Next ball shown."],
      ["res/sounds/pop_wind.WAV", "Word found", "A word is accepted and scored"]
    ] },
    { name: "Bonuses", sounds: [
      ["res/sounds/speedy_match.wav", "Speedy match", "A word is made less than 2 seconds after the previous word"],
      ["res/sounds/special_longword.wav", "Long word", "A word of more than 5 letters is made"],
      ["res/sounds/special_bonus.wav", "Bonus word", "The word made is the level's bonus word"],
      ["res/sounds/special_multiplier.wav", "Multiplier ball", "A popping ball carries a 2x, 3x or 4x multiplier"]
    ] },
    { name: "Level end and results", sounds: [
      ["res/sounds/level_complete.wav", "Level complete", "A puzzle or solitaire level ends"],
      ["res/music/intense-levelend.wav", "Level up", "A freematch level ends (this file is in res/music, loaded as a sound)"],
      ["res/music/intenseEnd.wav", "Game over", "A game ends without quitting (this file is in res/music, loaded as a sound)"],
      ["res/sounds/message_add.wav", "Result line", "Solitaire and freematch results screen, once the balls have settled: a line of the level's results shows, the first at once, the second 0.5 seconds later and the third 1 second after the first (the results timer starts at 3 seconds); not in puzzle mode"],
      ["res/sounds/stats_countup.wav", "Bonus counts up", "Solitaire and freematch results screen, once the results timer is under 1 second and a bonus is left: each 100 points counted into the score, the steps coming faster as the count goes on"]
    ] },
    { name: "Unused", sounds: [
      ["res/sounds/level_up.wav", "Level up chime (unused)", "Never played: not in the game's sound list (its LEVEL_UP is res/music/intense-levelend.wav), so nothing loads it"]
    ] }
  ] }
};
