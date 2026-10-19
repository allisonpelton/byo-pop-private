#ifndef __VERIFIER_H__
#define __VERIFIER_H__

// port(variant): the headless verifier, bejeweled_verify (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part
// of the original game. The game's own Board sources (Board.cpp, BoardRules.cpp, BoardVariant.cpp: unchanged,
// compiled against the framework shims in headless/shim) played without a window, sound, fonts, images, user
// files or mods: what a server has. Its entry points are verify_main.cpp's (the native exe's command line and
// the standalone wasm's exports); the work is here.
//
// Verify: one replay (.bjr), played by the game's replay player (WatchController, the same code as -replay in
// the game) on a headless app (HeadlessApp.h). Every gate, CHECK and END is the player's. Its "REPLAY ..." lines
// go to stdout as in the game, and with theTrace the board's settles as -trace prints them (BoardTrace.h), so
// the game's -replay -trace run and the verifier's can be compared line for line (the drift guard; only the
// "update N" field, the app's update count, differs). Returns the result as one line of JSON:
//   {"result":"ok"|"corrupt"|"unplayable", "message":<the REPLAY end line>, "checks":n, "events":n,
//    "segments":n, "updates":n, "engineAbi":n, "flags":{"hover":b,"ranked":b,"multiSegment":b},
//    "rulesId":<hex RLID>, "end":{"reason":"time up","reasonCode":2,"tick":t,"score":s,"level":l,"gems":g,
//    "draws":d,"logicHash":<16 hex>}, "seeds":[...]}
// ("end" and the rest are the file's; with "ok" the board reached exactly that END).
//
// Difftest: games on seeds theFirst..theLast, each played on two boards in lockstep by two DiffBots with the
// game's seed: a plain board and one with the -forcevariant flags (every generalised code path at the default
// rules). After every update the two boards' LOGIC encodings (StateCodec) must be equal; at every settle also
// the move list (FindMove) and the save a shutdown would write (SaveGameSnapshot). A game runs to its game-over
// screen, New Game, or theTicks updates. theModes: 1 normal, 2 timed, 3 both. Returns the number of games
// that differ; theReport gets a summary per mode and a line per difference. Arguments it does not take (seed 0,
// theLast below theFirst, more than MAX_DIFFTEST_SEEDS seeds, theTicks outside 1..Replay::MAX_TICKS, theModes
// outside 1..3: the wasm export passes them as they come) return -1 with the reason in theReport, and play none.

#include <cstdint>
#include <string>

namespace Sexy
{

namespace Verifier
{

// theBytes are taken (moved in), and parsed once (the player's own Replay::Read)
std::string		VerifyReplay(std::string theBytes, const std::string& theName, bool theTrace, bool forceVariant, bool* isOk);

enum
{
	MODE_NORMAL = 1,
	MODE_TIMED = 2,
	DEFAULT_TICKS = 60000,          // ten minutes of board ticks: a bot's game is over long before in nearly every case
	MAX_DIFFTEST_SEEDS = 100000     // seeds in one call (the oracle plays 500)
};
int				Difftest(uint32_t theFirst, uint32_t theLast, int theTicks, int theModes, bool isVerbose, std::string* theReport);

// The RulesId of a mod.json's "tunables" and "rules" merged over the default rules with the original
// data\diff.txt values (games/bejeweled/docs/CODE_MODS_DESIGN.md 3.7), as JSON:
//   {"rulesId":<64 hex>, "variant":b, "errors":[...], "warnings":[...]}  or  {"error":<why>}
// A text over MAX_MOD_JSON bytes is refused unread (a mod.json is a few KB).
enum
{
	MAX_MOD_JSON = 1024 * 1024
};
std::string		RulesIdJson(const std::string& theModJson);

}

}

#endif
