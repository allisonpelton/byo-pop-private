// port(mods): the fixes mod's settings (ModFixes.h).

#include "ModFixes.h"
#include "platform/ModManager.h"

using namespace Sexy;

// The fixes mod's toggles, from the highest-priority enabled mod that has the feature "fixes"
FixSettings Sexy::LoadFixSettings()
{
	FixSettings aSettings;
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (!aMod->mFeaturesJson.IsObject() || !aMod->mFeaturesJson.Has("fixes"))
			continue;
		Platform::JsonValue aClock = Platform::ModManager::Get().GetLiveValue(aMod->mId, "gameClock");
		Platform::JsonValue aFight = Platform::ModManager::Get().GetLiveValue(aMod->mId, "solverFight");
		Platform::JsonValue aTaps = Platform::ModManager::Get().GetLiveValue(aMod->mId, "keyTaps");
		aSettings.mGameClock = aClock.IsBool() && aClock.AsBool(false);
		aSettings.mSolverFight = aFight.IsBool() && aFight.AsBool(false);
		aSettings.mKeyTaps = aTaps.IsBool() && aTaps.AsBool(false);
		break;
	}
	return aSettings;
}
