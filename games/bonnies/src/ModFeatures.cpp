#include "ModFeatures.h"
#include "platform/ModManager.h"

// The highest-priority enabled mod that names feature theName (null without one)
static const Platform::ModInfo* FindFeatureMod(const char* theName)
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has(theName))
			return aMod;
	}
	return nullptr;
}

bool bonnies::FixOn(const char* theFix)
{
	const Platform::ModInfo* aMod = FindFeatureMod("fixes");
	if (aMod == nullptr)
		return false;
	Platform::JsonValue aValue = Platform::ModManager::Get().GetLiveValue(aMod->mId, theFix);
	return aValue.IsBool() ? aValue.AsBool(false) : false;
}
