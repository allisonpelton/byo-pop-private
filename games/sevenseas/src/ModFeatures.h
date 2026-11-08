#ifndef __SEVENSEAS_MODFEATURES_H__
#define __SEVENSEAS_MODFEATURES_H__

// port(mod): Seven Seas's side of the shared mods (platform/ModManager, docs/MOD_MENU.md). The fixes mod
// (games/sevenseas/mods/fixes, docs/MOD_ROADMAP.md "The fixes mod") has the feature "fixes"; each of its settings
// is a toggle for one fix of a fault of the original. Without the mod, or with a toggle off, the game plays as the
// original does.

namespace Sexy
{

// The fixes mod is enabled and its toggle theSetting is on
bool			FixEnabled(const char* theSetting);

}

#endif
