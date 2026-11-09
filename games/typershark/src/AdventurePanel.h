#ifndef __TYPERSHARK_ADVENTUREPANEL_H__
#define __TYPERSHARK_ADVENTUREPANEL_H__

// AdventurePanel.cpp: the "Adventure Mode" page of the stats screen: the sharks shocked, piranhas zapped, bosses bested,
// highest number of treasure words and highest score, the number of expeditions (finished games) with the bronze,
// silver or gold medal, the six gems and the four secret items found (a question mark for a secret item not yet
// found). The StatsScreen sets mUser (SetUser) before the page is shown; Draw reads it.
// Original: class Sexy::AdventurePanel : public TabPanel, vftable 0x530804, sizeof 0x90 (constructor 0x450df0, Draw
// 0x444fa0; the deleting destructor is the one it shares with AbyssPanel, 0x450dc0, over TabPanel's destructor body).

#include "TabPanel.h"

namespace Sexy
{

class UserProfile;

class AdventurePanel : public TabPanel
{
public:
	UserProfile*			mUser;			// +0x8c the profile whose numbers are shown (ctor NULL; StatsScreen::SetUser sets it)

public:
	AdventurePanel(WinWSApp* theApp);									// 0x450df0 the title is "Adventure Mode"

	virtual void			Draw(Graphics* g) override;					// 0x444fa0 slot 16 (only while mShown)
};

}

#endif
