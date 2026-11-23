// webstarts.js: each game's way to a first board in its web build, for the tools that drive a game there over the DevTools
// protocol (tools/webtest/webmem.js, webdet.js): the commands sent in turn once the game is up, after "seed 1". Its page
// check's start where it has one (games/<game>/tools/web/pagecheck.js), otherwise the mode its notes and tools use. Each
// run starts from a wiped profile, so a game that asks a new player's name is given one.
//   "@name X"          types X into the new player's dialog (or Bejeweled Twist's new player screen) that a fresh profile
//                      opens, when one comes, and presses Enter (Bookworm Adventures' web driver,
//                      games/bookwormadv/tools/web/moveloop.js; Twist's onegame.js); after it, the conversations and dialogs
//                      that come are clicked away by the bot's moves before each command
//   "@name X OKX OKY"  the same with the name box's OK clicked there instead of Enter (Hammer Heads' page check)
//   "@click X Y"       clicks there until the game's state changes (Hammer Heads' loading screen leaves only by its play
//                      button, and its level's introduction by its Continue button, as games/hammerheads/tools/web/onegame.js
//                      clicks them)
// Both Bookworm Adventures put a new player straight into the first battle, with no menu to go to, so their "state" only
// waits for that. The Mahjong Escapes count their layouts only once the list has been opened.
"use strict";
const START = {
	alchemy: ["goto strategic"],
	astropop: ["goto classic 1"],
	atomica: ["goto menu", "goto strategy"],
	bejeweled: ["goto timed"],
	bejeweled2: ["goto classic"],
	bejeweled3: ["@name Test", "goto classic"],
	bejeweledtwist: ["@name Test", "goto classic"],
	bigmoney: ["goto selector"],
	bonnies: ["goto classic"],
	bookworm: ["goto classic"],
	bookwormadv: ["@name Test", "state"],
	bookwormadv2: ["@name Test", "state"],
	cosmicbugs: ["goto game"],
	dynomite: ["goto endless"],
	hammerheads: ["@click 397 532", "@name Test 310 392", "goto classic", "@click 400 438"],
	heavyweapon: ["level 1"],
	mahjongchina: ["set player Test", "goto classic", "goto classic 1"],
	mahjongjapan: ["set player Test", "goto classic", "goto classic 1"],
	mummymaze: ["goto classic"],
	ningpo: ["goto game"],
	noahsark: ["goto action"],
	pixelus: ["goto normal"],
	pizzafrenzy: ["level 1 1"],
	rocketmania: ["goto arcade 2"],
	sevenseas: ["goto easy"],
	tiptop: ["goto timed"],
	typershark: ["goto adventure 0"],
	venice: ["player Test", "unlock all", "level 5"],
	waterbugs: ["level 1"],
	wordharmony: ["goto think", "level 1 think"],
};
module.exports = START;
