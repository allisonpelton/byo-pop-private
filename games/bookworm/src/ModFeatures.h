#ifndef __BOOKWORM_MODFEATURES_H__
#define __BOOKWORM_MODFEATURES_H__

// port(mod): BookWorm's side of the shared mods (platform/ModManager, docs/MOD_MENU.md). A mod turns on a
// code feature in its mod.json ("features": {"solver": true}); the feature's settings are the mod's own
// settings, read live (ModManager::GetLiveValue), so a change in the mod menu applies at once. Enabled mods
// that change play ("affectsScores", tunables, rules) give a score key: the high scores and the saved games
// are then kept apart from the original ones (ScoreKeySuffix).

#include <string>

namespace Sexy
{

// An enabled mod has feature theName (true, or an object) in its "features"
bool			ModFeatureEnabled(const char* theName);

// The live value of setting theSetting of the highest-priority enabled mod with feature theFeature;
// theDefault without that mod or setting
bool			ModFeatureBool(const char* theFeature, const char* theSetting, bool theDefault);
int				ModFeatureInt(const char* theFeature, const char* theSetting, int theDefault, int theMin, int theMax);

// "" with the original rules, else "-" and the 8-digit score key: appended to the registry key of the high
// scores ("Scores-<key>") and to the saved games' file names ("classic-<key>.dat")
std::string		ScoreKeySuffix();

}

#endif
