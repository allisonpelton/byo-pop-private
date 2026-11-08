#ifndef __ATOMICA_FIXES_H__
#define __ATOMICA_FIXES_H__

// The fixes mod (games/atomica/mods/fixes/mod.json; docs/MOD_ROADMAP.md, "The fixes mod"): one mod whose settings are
// toggles, one for each fault of the original that the port keeps. The port without the mod is the original, so every fix
// is off unless the mod is enabled, and a fix is on in an enabled mod until the player turns its toggle off. A toggle
// applies at once (Platform::ModManager::GetLiveValue): each fix asks where it matters. The fixes are listed in
// docs/GAME_NOTES.md, "The fixes mod".

namespace Sexy
{

// true when the fixes mod is enabled and its toggle theFix (a setting id of mod.json) is on
bool FixEnabled(const char* theFix);

}

#endif
