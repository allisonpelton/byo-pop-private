#ifndef __GAMECONFIG_H__
#define __GAMECONFIG_H__

// Gameplay constants that mods may change ("tunables" in mod.json). Every default is the original
// game's value, so with no mods the game behaves exactly like Bejeweled Deluxe 1.87.
//
// Mod keys (dotted paths in a mod's "tunables" object):
//   scoring.setValues          base points of the 1st..9th+ run of a move   [10,20,30,40,60,80,110,160,210]
//   scoring.lengthBonus        extra points per gem beyond 3 in a run        10
//   scoring.longLengthBonus    additional points per gem beyond 5            10
//   scoring.excellentFactor    EXCELLENT when a move passes factor * mult    120
//   scoring.hintPenalty        hint costs penalty * mult points              50
//   board.gemColors            number of gem colours (3..7)                  7
//   timing.startDrainDelay     timed mode: ticks before the drain starts     300
//   timing.levelDrainDelay     ... after a new level / refilled board        200
//   timing.warningFraction     warning sound when the bar is below this      0.15
//
// Mods that set any tunable automatically get their own high-score table (ModManager score key).
//
// port(variant): the tunables are now part of GameRules (GameRules.h), which also holds the code-mod
// rules ("rules" in mod.json: a superset of "tunables") and the data\diff.txt values. Load() merges the
// enabled mods' tunables and rules into mRules, then LoadFromRules() copies the tunables into the
// members below, which the translated code reads. The rules beyond the tunables do not change play yet
// (docs/CODE_MODS_DESIGN.md 9.2, stage 1). Split: GameConfig.cpp is framework-free (SetValue,
// LoadFromRules); GameConfigMods.cpp reads ModManager and the user dir (Load, ScopedFileName).

#include "GameRules.h"

#include <string>
#include <vector>

namespace Sexy
{

struct GameConfig
{
	std::vector<int> mSetValues = {10, 20, 30, 40, 60, 80, 110, 160, 210};
	int mLengthBonus = 10;
	int mLongLengthBonus = 10;
	double mExcellentFactor = 120.0;
	double mHintPenalty = 50.0;
	int mGemColors = 7;
	int mStartDrainDelay = 300;
	int mLevelDrainDelay = 200;
	double mWarningFraction = 0.15;

	// Any mod is enabled, even one that only changes art or sound. Not a gameplay setting (it does not
	// affect the score key); it switches on the small "modded" signs (ModIndicators.h).
	bool mAnyModEnabled = false;

	// port(variant): the merged rules of the enabled mods (tunables included) and the port features
	GameRules mRules;
	GameFeatures mFeatures;

	// port(variant): test hook -forcevariant (main.cpp): every board runs its generalised paths at default
	// rules (Board::InitVariant); the RNG source, the save format and the tutorial stay original
	bool mForceVariant = false;

	// Base points of the n-th run (1-based) of the current move.
	int SetValue(int theRunNumber) const;

	// Reads the enabled mods' tunables and rules (Platform::ModManager). Called at startup and before each
	// game. Writes <user dir>/mods-status.json when an enabled mod has "rules" or "features".
	void Load();

	// port(variant): the tunables of theRules into the members above
	void LoadFromRules(const GameRules& theRules);

	// port(variant): merges the enabled mods' rules and features and reads the data\diff.txt values, without
	// writing anything (Load, -dumprules)
	static void ResolveRules(GameRules* theRules, GameFeatures* theFeatures, RulesStatus* theStatus);

	// port(variant): -dumprules (main.cpp): the canonical form as text, the RulesId and the merge report
	static std::string DumpRules();

	// File name for a userdata file that must be kept apart for gameplay-changing mod sets,
	// e.g. "userdata\\hiscores.dat" -> "userdata\\hiscores-1a2b3c4d.dat".
	static std::string ScopedFileName(const std::string& theFileName);
};

extern GameConfig gGameConfig;

}

#endif
