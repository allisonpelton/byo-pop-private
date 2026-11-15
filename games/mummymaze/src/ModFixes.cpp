// port(mods): the fixes mod's settings (ModFixes.h).

#include "ModFixes.h"
#include "platform/GameOptions.h"

using namespace Sexy;

// The fixes mod's toggles, through Platform::FixOn
FixSettings Sexy::LoadFixSettings()
{
	FixSettings aSettings;
	aSettings.mGameClock = Platform::FixOn("gameClock");
	aSettings.mSolverFight = Platform::FixOn("solverFight");
	aSettings.mKeyTaps = Platform::FixOn("keyTaps");
	return aSettings;
}
