#include "ModFeatures.h"
#include "platform/ModManager.h"

using namespace Sexy;

// The highest-priority enabled mod that names feature theName (null without one)
// port(mods): the highest-priority enabled mod that names a feature
static const Platform::ModInfo* FindFeatureMod(const char* theName)
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods()) {
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has(theName))
			return aMod;
	}
	return nullptr;
}

// port(mods): a mod feature's setting
bool Sexy::ModFeatureBool(const char* theFeature, const char* theSetting, bool theDefault)
{
	const Platform::ModInfo* aMod = FindFeatureMod(theFeature);
	if (aMod == nullptr)
		return theDefault;
	Platform::JsonValue aValue = Platform::ModManager::Get().GetLiveValue(aMod->mId, theSetting);
	return aValue.IsBool() ? aValue.AsBool(theDefault) : theDefault;
}
