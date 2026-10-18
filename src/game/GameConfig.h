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

	// Base points of the n-th run (1-based) of the current move.
	int SetValue(int theRunNumber) const;

	// Reads the enabled mods' tunables (Platform::ModManager). Called at startup and before each game.
	void Load();

	// File name for a userdata file that must be kept apart for gameplay-changing mod sets,
	// e.g. "userdata\\hiscores.dat" -> "userdata\\hiscores-1a2b3c4d.dat".
	static std::string ScopedFileName(const std::string& theFileName);
};

extern GameConfig gGameConfig;

}

#endif
