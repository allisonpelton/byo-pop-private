// port(mod): BookWorm's side of the shared mods (ModFeatures.h). Not part of the original game.

#include "ModFeatures.h"
#include "platform/ModManager.h"

using namespace Sexy;

// The highest-priority enabled mod that names feature theName (null without one)
static const Platform::ModInfo* FindFeatureMod(const char* theName)
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has(theName))
			return aMod;
	}
	return NULL;
}

bool Sexy::ModFeatureEnabled(const char* theName)
{
	const Platform::ModInfo* aMod = FindFeatureMod(theName);
	if (aMod == NULL)
		return false;
	const Platform::JsonValue& aValue = aMod->mFeaturesJson[theName];
	return aValue.IsObject() || aValue.AsBool(false);
}

static Platform::JsonValue LiveValue(const char* theFeature, const char* theSetting)
{
	const Platform::ModInfo* aMod = FindFeatureMod(theFeature);
	if (aMod == NULL)
		return Platform::JsonValue();
	return Platform::ModManager::Get().GetLiveValue(aMod->mId, theSetting);
}

bool Sexy::ModFeatureBool(const char* theFeature, const char* theSetting, bool theDefault)
{
	Platform::JsonValue aValue = LiveValue(theFeature, theSetting);
	return aValue.IsBool() ? aValue.AsBool(theDefault) : theDefault;
}

bool Sexy::FixEnabled(const char* theFix)
{
	return ModFeatureEnabled("fixes") && ModFeatureBool("fixes", theFix, true);
}

int Sexy::ModFeatureInt(const char* theFeature, const char* theSetting, int theDefault, int theMin, int theMax)
{
	Platform::JsonValue aValue = LiveValue(theFeature, theSetting);
	if (!aValue.IsNumber())
		return theDefault;
	int anInt = aValue.AsInt(theDefault);
	return (anInt < theMin) ? theMin : ((anInt > theMax) ? theMax : anInt);
}

std::string Sexy::ScoreKeySuffix()
{
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	return aKey.empty() ? std::string() : "-" + aKey;
}
