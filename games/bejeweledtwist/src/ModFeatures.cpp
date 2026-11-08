#include "ModFeatures.h"
#include "platform/ModManager.h"

using namespace Sexy;

// The highest-priority enabled mod that names feature theName (null without one)
static const Platform::ModInfo* FindFeatureMod(const char* theName)
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods()) {
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has(theName))
			return aMod;
	}
	return nullptr;
}

bool Sexy::ModFeatureEnabled(const char* theName)
{
	const Platform::ModInfo* aMod = FindFeatureMod(theName);
	if (aMod == nullptr)
		return false;
	const Platform::JsonValue& aValue = aMod->mFeaturesJson[theName];
	return aValue.IsObject() || aValue.AsBool(false);
}

bool Sexy::ModFeatureBool(const char* theFeature, const char* theSetting, bool theDefault)
{
	const Platform::ModInfo* aMod = FindFeatureMod(theFeature);
	if (aMod == nullptr)
		return theDefault;
	Platform::JsonValue aValue = Platform::ModManager::Get().GetLiveValue(aMod->mId, theSetting);
	return aValue.IsBool() ? aValue.AsBool(theDefault) : theDefault;
}
