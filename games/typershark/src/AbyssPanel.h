#ifndef __TYPERSHARK_ABYSSPANEL_H__
#define __TYPERSHARK_ABYSSPANEL_H__

// AbyssPanel.cpp: the "Abyss Mode" page of the stats screen: the deepest depth, the sharks shocked, piranhas zapped and
// bosses bested in abyss games, the highest abyss score, and the kill count of each of the four bosses beside the
// boss picture. The StatsScreen sets mUser (SetUser) before the page is shown; Draw reads it.
// Original: class Sexy::AbyssPanel : public TabPanel, vftable 0x53072c, sizeof 0x90 (constructor 0x450d40, Draw
// 0x444730; the deleting destructor is the one it shares with AdventurePanel, 0x450dc0, over TabPanel's destructor body).

#include "TabPanel.h"

namespace Sexy
{

class UserProfile;

class AbyssPanel : public TabPanel
{
public:
	UserProfile*			mUser;			// +0x8c the profile whose numbers are shown (ctor NULL; StatsScreen::SetUser sets it)

public:
	AbyssPanel(WinWSApp* theApp);										// 0x450d40 the title is "Abyss Mode"

	virtual void			Draw(Graphics* g) override;					// 0x444730 slot 16 (only while mShown)
};

}

#endif
