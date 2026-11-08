#ifndef __BEJEWELEDTWIST_MODFEATURES_H__
#define __BEJEWELEDTWIST_MODFEATURES_H__

// port(mod): Twist's side of the shared mods (platform/ModManager, docs/MOD_MENU.md). A mod turns on a code feature
// in its mod.json ("features": {"fixes": {...}}); the feature's settings are the mod's own settings, read live
// (ModManager::GetLiveValue), so a change in the mod menu applies at once. Without mods every answer is the default,
// the original's behaviour.

namespace Sexy
{

// An enabled mod has feature theName (true, or an object) in its "features"
bool ModFeatureEnabled(const char* theName);

// The live value of setting theSetting of the highest-priority enabled mod with feature theFeature; theDefault
// without that mod or setting
bool ModFeatureBool(const char* theFeature, const char* theSetting, bool theDefault);

}

#endif
