#ifndef __MUMMYMAZE_MODFIXES_H__
#define __MUMMYMAZE_MODFIXES_H__

// port(mods): the toggles of the fixes mod (games/mummymaze/mods/fixes, docs/MOD_ROADMAP.md, "The fixes mod"). The mod names the
// feature "fixes"; each fix is a bool setting of it. Without the mod every fix is off and the game plays as the original
// does. The settings are read once at start-up (they are "apply": "restart"), so a game never changes its clock or its search
// in the middle of a chamber.

namespace Sexy
{

struct FixSettings
{
	bool		mGameClock;		// a chamber's time (bonus, adventure time) is counted in updates, not by the machine's clock
	bool		mSolverFight;	// the search for a way out counts the gate flip of a fight between the mummies on the key stone
	bool		mKeyTaps;		// a press of an arrow key or Space that is let go before an update ran still moves the explorer

	FixSettings() : mGameClock(false), mSolverFight(false), mKeyTaps(false) {}
};

FixSettings		LoadFixSettings();

}

#endif
