#include "ModFeatures.h"
#include "platform/ModManager.h"

using namespace Sexy;

bool Sexy::FixEnabled(const char* theSetting)
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has("fixes"))
		{
			Platform::JsonValue aValue = Platform::ModManager::Get().GetLiveValue(aMod->mId, theSetting);
			return aValue.IsBool() ? aValue.AsBool(false) : false;
		}
	}
	return false;
}
