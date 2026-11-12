#ifndef __COSMICBUGS_CBFIXES_H__
#define __COSMICBUGS_CBFIXES_H__

// port(mods): the toggles of the fixes mod (games/cosmicbugs/mods/fixes; docs/MOD_ROADMAP.md, "The fixes mod"). The mod
// names the feature "fixes" and each fix is a bool setting of it. Every fix is off until CosmicBugsApp::Init has read
// the mod's settings, so a run without the mod (the headless runner, the suite, the oracle, the frozen baseline) plays the
// original. The settings are read once at start-up ("apply": "restart").

namespace cb
{

struct Fixes
{
	bool mWalkerTurnMemory = false; // a walker that turns off a vertical way waits out its 5 seconds before the next turn, as off a horizontal way
	bool mShieldFlash = false;      // an enemy's shield flash starts at the enemy's own y, not its x (which showed for a frame at times)
};

extern Fixes gFixes;

} // namespace cb

#endif
