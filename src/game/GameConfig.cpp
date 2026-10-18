#include "GameConfig.h"
#include "../platform/ModManager.h"

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

void GameConfig::Load()
{
	const Platform::ModManager& aMods = Platform::ModManager::Get();
	GameConfig aDefaults;

	std::vector<double> aDefaultSets(aDefaults.mSetValues.begin(), aDefaults.mSetValues.end());
	std::vector<double> aSets = aMods.GetTunableArray("scoring.setValues", aDefaultSets);
	mSetValues.assign(aSets.begin(), aSets.end());
	mLengthBonus = aMods.GetTunableInt("scoring.lengthBonus", aDefaults.mLengthBonus);
	mLongLengthBonus = aMods.GetTunableInt("scoring.longLengthBonus", aDefaults.mLongLengthBonus);
	mExcellentFactor = aMods.GetTunable("scoring.excellentFactor", aDefaults.mExcellentFactor);
	mHintPenalty = aMods.GetTunable("scoring.hintPenalty", aDefaults.mHintPenalty);
	// The gem sheet has 7 colours. FillBoard rerolls a gem until it makes no 3-in-a-row; the two gems to
	// its left and the two below can rule out two colours, so it needs at least 3 or it can loop forever.
	mGemColors = std::max(3, std::min(7, aMods.GetTunableInt("board.gemColors", aDefaults.mGemColors)));
	mStartDrainDelay = aMods.GetTunableInt("timing.startDrainDelay", aDefaults.mStartDrainDelay);
	mLevelDrainDelay = aMods.GetTunableInt("timing.levelDrainDelay", aDefaults.mLevelDrainDelay);
	mWarningFraction = aMods.GetTunable("timing.warningFraction", aDefaults.mWarningFraction);
	mAnyModEnabled = !aMods.GetEnabledMods().empty();
}

std::string GameConfig::ScopedFileName(const std::string& theFileName)
{
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	if (aKey.empty())
		return theFileName;
	size_t aDot = theFileName.find_last_of('.');
	size_t aSlash = theFileName.find_last_of("/\\");
	if (aDot == std::string::npos || (aSlash != std::string::npos && aDot < aSlash))
		return theFileName + "-" + aKey;
	return theFileName.substr(0, aDot) + "-" + aKey + theFileName.substr(aDot);
}
