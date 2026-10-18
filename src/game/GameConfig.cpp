// GameConfig (GameConfig.h), the framework-free half. port(variant): split; the half that reads the mods is
// GameConfigMods.cpp

#include "GameConfig.h"

#include <algorithm>

using namespace Sexy;

GameConfig Sexy::gGameConfig;

int GameConfig::SetValue(int theRunNumber) const
{
	if (mSetValues.empty())
		return 0;
	int anIndex = std::max(1, theRunNumber) - 1;
	return mSetValues[std::min(anIndex, (int)mSetValues.size() - 1)];
}

void GameConfig::LoadFromRules(const GameRules& theRules)
{
	const GameRules::TunableRules& aTunables = theRules.mTunables;
	mSetValues = aTunables.mSetValues;
	mLengthBonus = aTunables.mLengthBonus;
	mLongLengthBonus = aTunables.mLongLengthBonus;
	mExcellentFactor = aTunables.mExcellentFactor;
	mHintPenalty = aTunables.mHintPenalty;
	// The gem sheet has 7 colours. FillBoard rerolls a gem until it makes no 3-in-a-row; the two gems to
	// its left and the two below can rule out two colours, so it needs at least 3 or it can loop forever.
	// port(variant): the rules allow up to 12 (GameRules::MAX_GEM_COLORS); until the extra gem art exists
	// (docs/CODE_MODS_DESIGN.md 4.2, stage 8) the game keeps the old clamp at 7.
	mGemColors = std::max(3, std::min(7, theRules.mBoard.mGemColors));
	mStartDrainDelay = aTunables.mStartDrainDelay;
	mLevelDrainDelay = aTunables.mLevelDrainDelay;
	mWarningFraction = aTunables.mWarningFraction;
}
