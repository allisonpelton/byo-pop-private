#ifndef __BONNIES_MODFEATURES_H__
#define __BONNIES_MODFEATURES_H__

// port(mod): Bonnie's side of the shared mods (platform/ModManager, docs/MOD_MENU.md). A mod turns on a code feature in
// its mod.json ("features": {"fixes": true}); the feature's settings are the mod's own settings. Bonnie's has one mod,
// the fixes mod (games/bonnies/mods/fixes): with it off, or without it, every fix is off and the game is the original.

namespace bonnies
{

// A fix of the fixes mod is on: the mod is enabled and the fix's toggle is on
bool FixOn(const char* theFix);

}

#endif
