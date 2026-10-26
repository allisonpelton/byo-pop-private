#ifndef __VERIFIER_H__
#define __VERIFIER_H__

// port(variant): the headless verifier, bejeweled_verify (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part
// of the original game. The game's own Board sources (Board.cpp, BoardRules.cpp, BoardVariant.cpp, BoardSave.cpp: unchanged,
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

struct DiffValues;

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

// Mirrortest (stage 7c, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.16): board.gravity "up" is the vertical mirror of
// "down". Per seed, on a vertically asymmetric mask and its mirror, the upward fill and refill from one RNG state are
// the mirrors of the downward ones (colours, rows, positions, draws); then, on the full 8x8 and on that mask, a down
// board and an up board restored from its mirrored state play mirrored moves (at most theMoves, normal mode), and at
// every settle the grids are mirrored and the score, stats and RNG states equal, until the first level-up or game
// over (whose draws depend on tick alignment). Settles on different ticks are warnings (float rounding at another
// magnitude). Both boards fall with fallAccel 75. Returns the number of seeds that differ (-1 and the reason in
// theReport for arguments it does not take); theReport gets a summary per board and a line per difference.
int				Mirrortest(uint32_t theFirst, uint32_t theLast, int theMoves, bool isVerbose, std::string* theReport);

// The RulesId of a mod.json's "tunables" and "rules" merged over the default rules with the original
// data\diff.txt values (games/bejeweled/docs/CODE_MODS_DESIGN.md 3.7), as JSON:
//   {"rulesId":<64 hex>, "variant":b, "errors":[...], "warnings":[...]}  or  {"error":<why>}
// A text over MAX_MOD_JSON bytes is refused unread (a mod.json is a few KB).
enum
{
	MAX_MOD_JSON = 1024 * 1024
};
std::string		RulesIdJson(const std::string& theModJson);

// The original data\diff.txt values (the RulesId's, and every rule set's with no mod)
DiffValues		OriginalDiffValues();

// Cascadehunt (native only, CascadeHunt.cpp): the original rules from seeds, normal mode, as -gameseed games. At every
// settle every move is tried on a second board restored from the first's SNAPSHOT, to the move's first settle: its
// rounds of matches, runs and points, and whether it beats the level. argv: "cascadehunt", then
//   first --seeds <a>..<b> [--wait <n>] [--top <n>] [-v]: every first move of each opening board; the share of boards
//       with a first move that beats level 1, and the boards whose first move scores most
//   play --seeds <a>..<b> [--policy greedy|random|survive|fair] [--depth <d>] [--samples <k>] [--tolerance <r>]
//       [--wait <n>] [--moves <n>] [--levels <n>] [--progress <n>] [--record <prefix>] [-v]: games to the game over or a
//       cap, the bot playing the policy's move; the moves with the most rounds and points as they are found. survive: the
//       move that scores most of those after which the board has a line of d more moves (default 4) that never runs out
//       of moves, the refills known from the board's RNG. fair: from the board in play alone, the move least likely to
//       end the game, the refills guessed on k boards (default 16); of those within risk r of it (default 0), the one
//       that scores most. --progress: the score every n moves; --record: each game as <prefix>-seed<s>.bjr, verified
//   replay --seed <s> --at <n> --move <c,r,c,r> [--policy ...] [--wait <n>] [--lead <k>] [--pause <n>] [--tail <n>]
//       [--after <m>|all] [--out <prefix>]: that game again to its move n, then the move given (--pause updates after its
//       settle), then m more of the bot's (or all to the game over); <prefix>-full.bjr (the whole game) and
//       <prefix>-short.bjr (a fresh replay holding the save made k moves before, LDSV, to the move given), each verified
//       (VerifyReplay)
// --wait: the updates between a settle and the move (the tick a level-up starts on changes its draws). Exit status 0
// when every move played as it was tried and the replays verify, 2 for a usage error.
int				CascadeHunt(int argc, char** argv);

// Funlab (native only, FunLab.cpp; games/bejeweled/docs/GAME_MODES.md): bot games on mods' rules, measured for what makes
// a game mode fun. argv: "funlab", then [<mod.json>...] [--settings <mod-settings.json>] [--seeds <a>..<b>]
// [--mode normal|timed|both] [--policy skilled|novice|both] [--wait <n>] [--samples <k>] [--moves <n>] [--minutes <m>].
// The mods' rules merge over the defaults with the original diff.txt values, lowest priority first, their settings from
// the settings file (the game's own format) or at their defaults. Each game, from seed a to b, mode by mode and bot by
// bot, prints a line "GAME <json>" of its measures (tools/funlab/report.py sums them up). The skilled bot tries every
// move on k guessed refills (default 2) and takes the safest, then the one that scores most; the novice takes any move.
// Each move comes n updates after the settle (default 100: a second's look). A game stops at its game over or a cap
// (default 3000 moves, 30 minutes). Exit status 0, 1 for rules that do not load, 2 for a usage error.
int				FunLab(int argc, char** argv);

}

}

#endif
