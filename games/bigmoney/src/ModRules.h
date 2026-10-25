#ifndef __BIGMONEY_MODRULES_H__
#define __BIGMONEY_MODRULES_H__

// port(mods): gameplay settings from mods, and the set seed. Not in the original.
//
// Tunables: a mod.json "tunables" object (platform/ModManager.h) changes the numbers the Board's level
// setup and scoring use. Every key is optional: a missing key keeps the original's value, so without
// tunables the game plays exactly as the original. Values out of range are clamped (the range is given
// after each group). A mod with tunables gets its own high-score tables and save (ScopedUserFileName,
// keyed by ModManager::GetScoreKey()).
//
//   "tunables": {
//     "minting": { "speedBase": 0.0025, "speedPerLevel": 0.001,        Action: new rows per tick (>= 0)
//                  "accelBase": 1e-6, "accelPerLevel": 5e-8,             Action: speed-up per mint step (>= 0)
//                  "perClickBase": 0.24501, "perClickPerLevel": 0.011 }, Strategy: rows per click (> 0)
//     "meter":   { "capacityBase": 300, "capacityPerLevel": 20,          points per full meter (>= 1)
//                  "drainBase": 0, "drainPerLevel": 7e-5 },              Action: meter drain (>= 0)
//     "coins":   { "colors": 4, "puzzleColors": 5,                       coin colours, 1..5
//                  "fewerOnFirstLevel": true },                          one colour less on the first level
//     "groups":  { "minSize": 3, "puzzleMinSize": 2 },                   smallest clickable group, 2..15
//     "bags":    { "base": 2.6, "perLevel": 0.26,                        money bags to collect per level
//                  "puzzleBase": 2.6, "puzzlePerLevel": 0.26,            (rounded down)
//                  "min": 1, "max": 8 },                                 1..8 (the bag holder has 8 slots)
//     "scoring": { "multBase": 0.5, "multPerLevel": 0.1,                 score multiplier (>= 0)
//                  "puzzleMult": 3,                                      Puzzle: times the multiplier
//                  "groupPoints": [5, 10, 15, 20, 30, 40, 50, 65, 80, 95, 115, 135, 155, 180],
//                                                                        points for groups of 2, 3, 4...
//                  "groupPointsStep": 25 },                              per coin beyond the list
//     "skill":   { "normalLevels": 4, "hardLevels": 7 }                  Normal and Hard start that many
//   }                                                                    levels ahead (0..50)
//
// Each per-level value is base + L * perLevel, where L is the level from 0 plus the skill's head start, as
// in the original (Board::SetupLevelParams). Combo and bonus points stay as they are, and every score is
// times the multiplier.
//
// Set seed: -gameseed=N (main.cpp) or ?seed=N on the web, N = 1..4294967295. Every new game gets the seed
// (WinBMApp::mPendingSeed) until one is played on it: its first valid click or its game over
// (Board::UsePendingSeed). A seeded board starts its own generator from N. Every random choice that shapes
// the board (new coins, bag columns, puzzles, dollar bombs) comes from that generator. Effects (flung and
// falling coins, shakes) draw from a second one, so they do not shift the board's choices. The first
// level always starts the same; in Strategy and Puzzle the same clicks give the same game. Seeded games
// keep their own high-score tables, and a seeded save ends in an 8-byte trailer ("BMSD", the seed) that
// the original's reader ignores.

#include <string>
#include <vector>

namespace Sexy
{

// The tunables that are not Board members of the original (the base / per-level values are; Board's
// LoadTunables sets those)
struct BoardTunables
{
	int		mColors = 4;
	int		mPuzzleColors = 5;
	bool	mFewerColorsFirstLevel = true;
	int		mMinGroup = 3;
	int		mPuzzleMinGroup = 2;
	int		mBagsMin = 1;
	int		mBagsMax = 8;
	double	mPuzzleMult = 3.0;
	std::vector<int> mGroupPoints;      // for groups of 2, 3, ... (the original's gGroupBaseScores)
	int		mGroupPointsStep = 25;
	int		mNormalLevels = 4;
	int		mHardLevels = 7;

	BoardTunables();

	// Base points of a group of theCount coins: the list, then groupPointsStep per coin beyond it. With
	// the original's list this is exactly its gGroupBaseScores[n - 2], or (n - 2) * 25 - 145 past the end.
	int		GroupPoints(int theCount) const;
};

// The enabled mods' tunable theKey (dotted, e.g. "minting.speedBase"), clamped to theMin..theMax; theDefault
// without one. Logs each value a mod sets.
double	ModTunable(const char* theKey, double theDefault, double theMin, double theMax);
int		ModTunableInt(const char* theKey, int theDefault, int theMin, int theMax);
bool	ModTunableBool(const char* theKey, bool theDefault);

// Port features (mod.json "features"): theName is on when an enabled mod sets it to true or to an object of
// settings. The highest-priority mod that names it decides.
//   "features": { "replays": { "keep": 50 } }     record finished games (Replay.h); keep the newest 50
//   "features": { "livesplit": true }              fill the LiveSplit block (LiveSplitObserver.h; Windows)
//   "features": { "puzzleSolver": { ... } }        the Puzzle solver (PuzzleSolver.h; mods/puzzle-solver).
//                                                  It changes play, so its mod says "affectsScores": true.
bool	ModFeatureEnabled(const char* theName);
// A number in the feature's object ({"keep": 50}), clamped; theDefault when the feature is just true
int		ModFeatureInt(const char* theName, const char* theKey, int theDefault, int theMin, int theMax);
// A true/false or a string in the feature's object; theDefault when it has none
bool	ModFeatureBool(const char* theName, const char* theKey, bool theDefault);
std::string ModFeatureString(const char* theName, const char* theKey, const std::string& theDefault);

// An enabled mod changes something the player sees or plays: anything besides "features" in its mod.json,
// "affectsScores": true, or game files (images, sounds, music, data, properties). The "mods are on" sign
// (ModIndicators.h) shows for these, not for a mod that only turns on replays or LiveSplit.
bool	AnyContentModEnabled();

// "userdata\\savegame.dat" -> "userdata\\savegame-1a2b3c4d.dat" when the enabled mods change gameplay
// (ModManager::GetScoreKey), and "...-seeded.dat" for a game on a seed the player chose (high scores
// only). Unchanged without either.
std::string ScopedUserFileName(const std::string& theFileName, bool theSeeded = false);

}

#endif
