#ifndef __MAHJONG_MODFEATURES_H__
#define __MAHJONG_MODFEATURES_H__

// port(mod): the game's side of the shared mods (platform/ModManager, docs/MOD_MENU.md). A mod turns on a code feature
// in its mod.json ("features": {"fixes": true}); the feature's settings are the mod's own settings, read live
// (ModManager::GetLiveValue). The game has one mod, the fixes mod (the game's mods/fixes): with it off, or without it,
// every fix is off and the game is the original.

namespace Sexy
{

// The live value of boolean setting theSetting of the highest-priority enabled mod with feature theFeature;
// theDefault without that mod or setting
bool ModFeatureBool(const char* theFeature, const char* theSetting, bool theDefault);

// A fix of the fixes mod is on: the mod is enabled and the fix's toggle is on
bool FixOn(const char* theFix);

}

#endif
